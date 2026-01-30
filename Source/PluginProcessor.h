#pragma once

#include "Core/DelayMatrix.h"
#include "Core/RoutingGraph.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>


/**
 * @brief UDS - Universal Delay System
 *
 * 8-band configurable delay matrix inspired by the Yamaha UD Stomp.
 */
class UDSAudioProcessor : public juce::AudioProcessor {
public:
  UDSAudioProcessor()
      : AudioProcessor(
            BusesProperties()
                .withInput("Input", juce::AudioChannelSet::stereo(), true)
                .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
        parameters_(*this, nullptr, juce::Identifier("UDSParameters"),
                    createParameterLayout()) {
    // Initialize with default parallel routing
    routingGraph_.setDefaultParallelRouting();
  }

  ~UDSAudioProcessor() override = default;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override {
    delayMatrix_.prepare(sampleRate, static_cast<size_t>(samplesPerBlock));
  }

  void releaseResources() override { delayMatrix_.reset(); }

  bool isBusesLayoutSupported(const BusesLayout& layouts) const override {
    const auto& mainOutput = layouts.getMainOutputChannelSet();
    const auto& mainInput = layouts.getMainInputChannelSet();

    // 1. Output must be Stereo or Mono
    if (mainOutput != juce::AudioChannelSet::stereo() &&
        mainOutput != juce::AudioChannelSet::mono())
      return false;

    // 2. Input must be Stereo or Mono
    if (mainInput != juce::AudioChannelSet::stereo() &&
        mainInput != juce::AudioChannelSet::mono())
      return false;

    // 3. We allow Mono->Stereo, Stereo->Stereo, Mono->Mono.
    // We do NOT allow Stereo Input -> Mono Output (downmixing not supported
    // yet)
    if (mainInput.size() > mainOutput.size())
      return false;

    return true;
  }

  void processBlock(juce::AudioBuffer<float>& buffer,
                    juce::MidiBuffer& midiMessages) override {
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    auto numSamples = buffer.getNumSamples();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
      buffer.clear(i, 0, numSamples);

    // --- MIDI Expression Pedal Handling ---
    // CC#11 (Expression) and CC#4 (Foot Controller) update expressionValue_
    for (const auto metadata : midiMessages) {
      auto m = metadata.getMessage();
      if (m.isController()) {
        int cc = m.getControllerNumber();
        if (cc == 11 || cc == 4) { // Expression or Foot Pedal
          expressionValue_.store(m.getControllerValue() / 127.0f);
        }
      }
    }
    // --- Apply Input Gain (Pre-Delay) ---
    // Expression pedal modulates from -60dB (0) to parameter value (1)
    float inputGainDb = parameters_.getRawParameterValue("inputGain")->load();
    float exprValue = expressionValue_.load(); // 0-1 from MIDI CC
    // When expression = 0, effective gain = -60dB (silent)
    // When expression = 1, effective gain = inputGainDb (parameter value)
    float effectiveInputGainDb = -60.0f + (inputGainDb + 60.0f) * exprValue;
    if (effectiveInputGainDb > -59.9f) {
      buffer.applyGain(juce::Decibels::decibelsToGain(effectiveInputGainDb));
    } else {
      buffer.applyGain(0.0f); // Effectively silent
    }

    // Get I/O mode: 0=Auto, 1=Mono, 2=Mono→Stereo, 3=Stereo
    int ioMode =
        static_cast<int>(parameters_.getRawParameterValue("ioMode")->load());

    // Apply I/O mode processing
    if (ioMode == 1) {
      // Mono: Mix input to mono, process, output mono on both channels
      if (totalNumInputChannels >= 2) {
        // Mix L+R to mono
        buffer.addFrom(0, 0, buffer, 1, 0, numSamples, 0.5f);
        buffer.applyGain(0, 0, numSamples, 0.5f);
      }
      // After processing, we'll copy ch0 to ch1 at the end
    } else if (ioMode == 2 || (ioMode == 0 && totalNumInputChannels == 1 &&
                               totalNumOutputChannels == 2)) {
      // Mono→Stereo: Expand mono input to stereo buffer
      if (totalNumInputChannels == 1 && totalNumOutputChannels >= 2) {
        buffer.copyFrom(1, 0, buffer, 0, 0, numSamples);
      } else if (totalNumInputChannels >= 2) {
        // Even with stereo input, treat as mono then expand
        buffer.addFrom(0, 0, buffer, 1, 0, numSamples, 0.5f);
        buffer.applyGain(0, 0, numSamples, 0.5f);
        buffer.copyFrom(1, 0, buffer, 0, 0, numSamples);
      }
    }
    // ioMode == 3 (Stereo) or ioMode == 0 (Auto) with stereo: no preprocessing
    // needed

    // Get BPM from host, or use internal metronome BPM for standalone
    double bpm = internalBpm_.load(); // Default to internal metronome BPM
    if (auto* pHead = getPlayHead()) {
      if (auto pos = pHead->getPosition()) {
        if (pos->getBpm().hasValue()) {
          bpm = *pos->getBpm();
        }
      }
    }

    // Get global mix parameter
    float mix = parameters_.getRawParameterValue("mix")->load() / 100.0f;
    float dryLevel =
        parameters_.getRawParameterValue("dryLevel")->load() / 100.0f;
    float dryPan = parameters_.getRawParameterValue("dryPan")->load();

    // Get master LFO parameters
    float masterLfoRate =
        parameters_.getRawParameterValue("masterLfoRate")->load();
    float masterLfoDepth =
        parameters_.getRawParameterValue("masterLfoDepth")->load() / 100.0f;
    int masterLfoWaveform = static_cast<int>(
        parameters_.getRawParameterValue("masterLfoWaveform")->load());

    // Master LFO waveform: 0=None, 1=Sine, 2=Triangle, 3=Saw, 4=Square,
    // 5=Brownian, 6=Lorenz
    if (masterLfoWaveform == 0) {
      // None: disable master LFO by setting depth to 0
      masterLfoDepth = 0.0f;
    }
    // Waveform index for setMasterLfo: 0=Sine, 1=Triangle, etc. (shift by -1
    // when not None)
    int adjustedWaveform = (masterLfoWaveform > 0) ? masterLfoWaveform - 1 : 0;
    delayMatrix_.setMasterLfo(masterLfoRate, masterLfoDepth, adjustedWaveform);

    // Note division multipliers for tempo sync
    static const float noteDivisionMultipliers[] = {
        4.0f,   // 1/1 (whole)
        2.0f,   // 1/2
        1.0f,   // 1/4
        0.5f,   // 1/8
        0.25f,  // 1/16
        0.125f, // 1/32
        1.5f,   // 1/4 dotted
        0.75f,  // 1/8 dotted
        0.667f, // 1/4 triplet (2/3)
        0.333f  // 1/8 triplet (1/3)
    };

    // Update delay matrix parameters from APVTS
    // First pass: check if any band is soloed
    bool anySoloed = false;
    for (int band = 0; band < 8; ++band) {
      juce::String prefix = "band" + juce::String(band) + "_";
      if (parameters_.getRawParameterValue(prefix + "solo")->load() > 0.5f) {
        anySoloed = true;
        break;
      }
    }

    for (int band = 0; band < 8; ++band) {
      juce::String prefix = "band" + juce::String(band) + "_";

      uds::DelayBandParams params;

      // Check if tempo sync is enabled
      bool tempoSync =
          parameters_.getRawParameterValue(prefix + "tempoSync")->load() > 0.5f;

      if (tempoSync) {
        // Calculate delay time from BPM and note division
        int divisionIndex = static_cast<int>(
            parameters_.getRawParameterValue(prefix + "noteDivision")->load());
        divisionIndex = juce::jlimit(0, 9, divisionIndex);
        float multiplier = noteDivisionMultipliers[divisionIndex];
        float quarterNoteMs = static_cast<float>(60000.0 / bpm);
        params.delayTimeMs = quarterNoteMs * multiplier;
        // Clamp to max delay time (2000ms)
        params.delayTimeMs = juce::jmin(params.delayTimeMs, 2000.0f);
      } else {
        params.delayTimeMs =
            parameters_.getRawParameterValue(prefix + "time")->load();
      }

      params.feedback =
          parameters_.getRawParameterValue(prefix + "feedback")->load() /
          100.0f;
      params.level = juce::Decibels::decibelsToGain(
          parameters_.getRawParameterValue(prefix + "level")->load());
      params.pan = parameters_.getRawParameterValue(prefix + "pan")->load();
      params.hiCutHz =
          parameters_.getRawParameterValue(prefix + "hiCut")->load();
      params.loCutHz =
          parameters_.getRawParameterValue(prefix + "loCut")->load();
      params.lfoRateHz =
          parameters_.getRawParameterValue(prefix + "lfoRate")->load();
      params.lfoDepth =
          parameters_.getRawParameterValue(prefix + "lfoDepth")->load() /
          100.0f;
      params.attackTimeMs =
          parameters_.getRawParameterValue(prefix + "attack")->load();

      // LFO waveform (0=None, 1=Sine, 2=Triangle, 3=Saw, 4=Square, 5=Brownian,
      // 6=Lorenz)
      int lfoWaveformIndex = static_cast<int>(
          parameters_.getRawParameterValue(prefix + "lfoWaveform")->load());

      if (lfoWaveformIndex == 0) {
        // None: disable modulation effectively
        params.lfoDepth = 0.0f;
      } else {
        // Map 1-based index to 0-based enum
        params.modulationType =
            static_cast<uds::ModulationType>(lfoWaveformIndex - 1);
      }

      params.phaseInvert =
          parameters_.getRawParameterValue(prefix + "phaseInvert")->load() >
          0.5f;
      params.pingPong =
          parameters_.getRawParameterValue(prefix + "pingPong")->load() > 0.5f;
      params.enabled =
          parameters_.getRawParameterValue(prefix + "enabled")->load() > 0.5f;

      // Algorithm selection (0=Digital, 1=Analog, 2=Tape, 3=LoFi)
      int algoIndex = static_cast<int>(
          parameters_.getRawParameterValue(prefix + "algorithm")->load());
      params.algorithm = static_cast<uds::DelayAlgorithmType>(algoIndex);

      // Solo/mute logic
      bool isMuted =
          parameters_.getRawParameterValue(prefix + "mute")->load() > 0.5f;
      bool isSoloed =
          parameters_.getRawParameterValue(prefix + "solo")->load() > 0.5f;

      // Apply mute: if muted, or if another band is soloed and this one isn't
      if (isMuted || (anySoloed && !isSoloed)) {
        params.level = 0.0f;
      }

      // Master LFO modulation is now handled by ModulationEngine inside
      // DelayMatrix

      delayMatrix_.setBandParams(band, params);
    }

    // Process through delay matrix with current routing
    delayMatrix_.processWithRouting(buffer, mix, routingGraph_, dryLevel,
                                    dryPan);

    // Copy band levels for UI activity indicators
    for (int band = 0; band < 8; ++band) {
      bandLevels_[band].store(delayMatrix_.getBandLevel(band));
    }

    // Apply mono output mode post-processing
    if (ioMode == 1 && totalNumOutputChannels >= 2) {
      // Mono: Copy processed channel 0 to channel 1
      buffer.copyFrom(1, 0, buffer, 0, 0, numSamples);
    }

    // --- Apply Master Output (Post-Delay) ---
    float masterOutputDb =
        parameters_.getRawParameterValue("masterOutput")->load();
    if (masterOutputDb > -59.9f) {
      buffer.applyGain(juce::Decibels::decibelsToGain(masterOutputDb));
    } else {
      buffer.applyGain(0.0f);
    }
  }

  juce::AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override { return true; }

  const juce::String getName() const override { return JucePlugin_Name; }
  bool acceptsMidi() const override { return true; }
  bool producesMidi() const override { return false; }
  bool isMidiEffect() const override { return false; }
  double getTailLengthSeconds() const override { return 6.0; }

  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram(int) override {}
  const juce::String getProgramName(int) override { return {}; }
  void changeProgramName(int, const juce::String&) override {}

  void getStateInformation(juce::MemoryBlock& destData) override {
    // Create root XML element
    auto xml = std::make_unique<juce::XmlElement>("UDSState");

    // Add APVTS parameters
    auto state = parameters_.copyState();
    xml->addChildElement(state.createXml().release());

    // Add routing graph
    xml->addChildElement(routingGraph_.toXml().release());

    copyXmlToBinary(*xml, destData);
  }

  void setStateInformation(const void* data, int sizeInBytes) override {
    std::unique_ptr<juce::XmlElement> xmlState(
        getXmlFromBinary(data, sizeInBytes));

    if (xmlState == nullptr)
      return;

    // Handle both old format (just APVTS) and new format (UDSState wrapper)
    if (xmlState->hasTagName("UDSState")) {
      // New format: extract APVTS and routing separately
      if (auto* apvtsXml =
              xmlState->getChildByName(parameters_.state.getType())) {
        parameters_.replaceState(juce::ValueTree::fromXml(*apvtsXml));
      }
      if (auto* routingXml = xmlState->getChildByName("Routing")) {
        routingGraph_.fromXml(routingXml);
      }
    } else if (xmlState->hasTagName(parameters_.state.getType())) {
      // Old format: just APVTS state (backwards compatibility)
      parameters_.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
  }

  juce::AudioProcessorValueTreeState& getParameters() { return parameters_; }

  // Access to routing graph for editor
  uds::RoutingGraph& getRoutingGraph() { return routingGraph_; }
  const uds::RoutingGraph& getRoutingGraph() const { return routingGraph_; }

  // Internal metronome BPM for standalone mode
  void setInternalBpm(double bpm) { internalBpm_.store(bpm); }
  double getInternalBpm() const { return internalBpm_.load(); }

  // Per-band output levels for activity indicators
  float getBandLevel(int band) const {
    if (band >= 0 && band < 8)
      return bandLevels_[band].load();
    return 0.0f;
  }

  // Safety mute status for UI
  bool isSafetyMuted() const { return delayMatrix_.isSafetyMuted(); }
  int getSafetyMuteReason() const {
    return static_cast<int>(delayMatrix_.getSafetyMuteReason());
  }
  void unlockSafetyMute() { delayMatrix_.unlockSafetyMute(); }

  // Accessors for preset management
  juce::AudioProcessorValueTreeState& getAPVTS() { return parameters_; }

private:
  juce::AudioProcessorValueTreeState parameters_;
  uds::DelayMatrix delayMatrix_;
  uds::RoutingGraph routingGraph_;
  std::atomic<double> internalBpm_{120.0};
  std::array<std::atomic<float>, 8> bandLevels_{
      {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}};

  // Expression pedal value (0-1 normalized, updated from MIDI CC)
  std::atomic<float> expressionValue_{1.0f};

  static juce::AudioProcessorValueTreeState::ParameterLayout
  createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Input Gain (Pre-Delay) - Essential for Volume Swells
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"inputGain", 1}, "Input Gain",
        juce::NormalisableRange<float>(-60.0f, 6.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    // Global mix (wet level)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"mix", 2}, "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // Master Output (Post-Delay)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"masterOutput", 1}, "Master Output",
        juce::NormalisableRange<float>(-60.0f, 6.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    // Expression Pedal Target (-1 = Input Gain default)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"expressionTarget", 1}, "Expression Target",
        juce::NormalisableRange<float>(-1.0f, 200.0f, 1.0f), -1.0f));

    // I/O Configuration mode
    // Auto: Use host-negotiated bus layout
    // Mono: Force mono in/out
    // MonoToStereo: Force mono input, stereo output (stereo expander)
    // Stereo: Force stereo in/out
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"ioMode", 1}, "I/O Mode",
        juce::StringArray{"Auto", "Mono", "Mono→Stereo", "Stereo"}, 0));

    // Dry level (for MagicStomp presets that attenuate dry signal)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"dryLevel", 1}, "Dry Level",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // Dry pan (for MagicStomp presets with stereo-separated dry signal)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"dryPan", 1}, "Dry Pan",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f));

    // Master LFO (modulates all bands together for chorus-like effects)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"masterLfoRate", 1}, "Master LFO Rate",
        juce::NormalisableRange<float>(0.01f, 10.0f, 0.01f, 0.5f), 0.5f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"masterLfoDepth", 1}, "Master LFO Depth",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"masterLfoWaveform", 1}, "Master LFO Waveform",
        juce::StringArray{"None", "Sine", "Triangle", "Saw", "Square",
                          "Brownian", "Lorenz"},
        0));

    // Per-band parameters (8 bands)
    for (int band = 0; band < 8; ++band) {
      juce::String prefix = "band" + juce::String(band) + "_";
      juce::String bandName = "Band " + juce::String(band + 1) + " ";

      params.push_back(std::make_unique<juce::AudioParameterFloat>(
          juce::ParameterID{prefix + "time", 2}, bandName + "Time",
          juce::NormalisableRange<float>(1.0f, 700.0f, 0.1f, 0.5f), 250.0f,
          juce::AudioParameterFloatAttributes().withLabel("ms")));

      params.push_back(std::make_unique<juce::AudioParameterFloat>(
          juce::ParameterID{prefix + "feedback", 2}, bandName + "Feedback",
          juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 30.0f,
          juce::AudioParameterFloatAttributes().withLabel("%")));

      params.push_back(std::make_unique<juce::AudioParameterFloat>(
          juce::ParameterID{prefix + "level", 2}, bandName + "Level",
          juce::NormalisableRange<float>(-60.0f, 6.0f, 0.1f), 0.0f,
          juce::AudioParameterFloatAttributes().withLabel("dB")));

      params.push_back(std::make_unique<juce::AudioParameterFloat>(
          juce::ParameterID{prefix + "pan", 2}, bandName + "Pan",
          juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f));

      params.push_back(std::make_unique<juce::AudioParameterFloat>(
          juce::ParameterID{prefix + "hiCut", 2}, bandName + "Hi-Cut",
          juce::NormalisableRange<float>(1000.0f, 20000.0f, 1.0f, 0.3f),
          12000.0f, juce::AudioParameterFloatAttributes().withLabel("Hz")));

      params.push_back(std::make_unique<juce::AudioParameterFloat>(
          juce::ParameterID{prefix + "loCut", 2}, bandName + "Lo-Cut",
          juce::NormalisableRange<float>(20.0f, 2000.0f, 1.0f, 0.3f), 80.0f,
          juce::AudioParameterFloatAttributes().withLabel("Hz")));

      params.push_back(std::make_unique<juce::AudioParameterFloat>(
          juce::ParameterID{prefix + "lfoRate", 2}, bandName + "LFO Rate",
          juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.5f), 1.0f,
          juce::AudioParameterFloatAttributes().withLabel("Hz")));

      params.push_back(std::make_unique<juce::AudioParameterFloat>(
          juce::ParameterID{prefix + "lfoDepth", 2}, bandName + "LFO Depth",
          juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f,
          juce::AudioParameterFloatAttributes().withLabel("%")));

      // Attack envelope for volume swell effects (0 = instant, >0 = swell)
      params.push_back(std::make_unique<juce::AudioParameterFloat>(
          juce::ParameterID{prefix + "attack", 1}, bandName + "Attack",
          juce::NormalisableRange<float>(0.0f, 2000.0f, 1.0f, 0.4f), 0.0f,
          juce::AudioParameterFloatAttributes().withLabel("ms")));

      params.push_back(std::make_unique<juce::AudioParameterChoice>(
          juce::ParameterID{prefix + "lfoWaveform", 2},
          bandName + "LFO Waveform",
          juce::StringArray{"None", "Sine", "Triangle", "Saw", "Square",
                            "Brownian", "Lorenz"},
          0));

      params.push_back(std::make_unique<juce::AudioParameterBool>(
          juce::ParameterID{prefix + "phaseInvert", 2},
          bandName + "Phase Invert", false));

      params.push_back(std::make_unique<juce::AudioParameterBool>(
          juce::ParameterID{prefix + "pingPong", 2}, bandName + "Ping Pong",
          false));

      params.push_back(std::make_unique<juce::AudioParameterBool>(
          juce::ParameterID{prefix + "enabled", 2}, bandName + "Enabled",
          true));

      // Algorithm selection
      params.push_back(std::make_unique<juce::AudioParameterChoice>(
          juce::ParameterID{prefix + "algorithm", 2}, bandName + "Algorithm",
          juce::StringArray{"Digital", "Analog", "Tape", "Lo-Fi"}, 0));

      // Tempo sync
      params.push_back(std::make_unique<juce::AudioParameterBool>(
          juce::ParameterID{prefix + "tempoSync", 2}, bandName + "Tempo Sync",
          false));

      params.push_back(std::make_unique<juce::AudioParameterChoice>(
          juce::ParameterID{prefix + "noteDivision", 2},
          bandName + "Note Division",
          juce::StringArray{"1/1", "1/2", "1/4", "1/8", "1/16", "1/32",
                            "1/4 Dotted", "1/8 Dotted", "1/4 Triplet",
                            "1/8 Triplet"},
          3)); // Default to 1/8

      // Solo/Mute
      params.push_back(std::make_unique<juce::AudioParameterBool>(
          juce::ParameterID{prefix + "solo", 2}, bandName + "Solo", false));

      params.push_back(std::make_unique<juce::AudioParameterBool>(
          juce::ParameterID{prefix + "mute", 2}, bandName + "Mute", false));
    }

    return {params.begin(), params.end()};
  }

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UDSAudioProcessor)
};
