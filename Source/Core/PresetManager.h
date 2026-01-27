#pragma once

#include "../PluginProcessor.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>


namespace uds {

/**
 * @brief Preset metadata structure
 */
struct PresetInfo {
  juce::String name;
  juce::String author;
  juce::String category;
  juce::File file;
  bool isFactory = false;
};

/**
 * @brief Manages preset save/load operations
 *
 * Features:
 * - Scans factory and user preset folders
 * - Saves/loads complete plugin state (parameters + routing)
 * - Maintains current preset list for browser
 */
class PresetManager {
public:
  PresetManager(UDSAudioProcessor& processor) : processor_(processor) {
    // Initialize preset directories
    userPresetDirectory_ =
        juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("UDS")
            .getChildFile("Presets");
    userPresetDirectory_.createDirectory();

    // Create factory presets on first run
    createFactoryPresets();

    scanPresets();

    // Select first preset on startup if available
    for (size_t i = 0; i < presets_.size(); ++i) {
      if (presets_[i].name.startsWith("01")) {
        currentPresetIndex_ = static_cast<int>(i);
        break;
      }
    }
  }

  /** @brief Get list of all available presets */
  const std::vector<PresetInfo>& getPresets() const { return presets_; }

  /** @brief Get current preset index (-1 if modified/unsaved) */
  int getCurrentPresetIndex() const { return currentPresetIndex_; }

  /** @brief Get current preset name */
  juce::String getCurrentPresetName() const {
    if (currentPresetIndex_ >= 0 &&
        currentPresetIndex_ < static_cast<int>(presets_.size())) {
      return presets_[static_cast<size_t>(currentPresetIndex_)].name;
    }
    return isModified_ ? "Modified" : "Init";
  }

  /** @brief Load a preset by index */
  bool loadPreset(int index) {
    if (index < 0 || index >= static_cast<int>(presets_.size()))
      return false;

    const auto& preset = presets_[static_cast<size_t>(index)];
    if (!preset.file.existsAsFile())
      return false;

    // Load XML from file
    auto xml = juce::XmlDocument::parse(preset.file);
    if (xml == nullptr)
      return false;

    // Convert to binary and restore state
    juce::MemoryBlock data;
    juce::AudioProcessor::copyXmlToBinary(*xml, data);
    processor_.setStateInformation(data.getData(),
                                   static_cast<int>(data.getSize()));

    currentPresetIndex_ = index;
    isModified_ = false;

    if (onPresetChanged)
      onPresetChanged();

    return true;
  }

  /** @brief Load next/previous preset */
  void loadNextPreset() {
    if (presets_.empty())
      return;
    int next = (currentPresetIndex_ + 1) % static_cast<int>(presets_.size());
    loadPreset(next);
  }

  void loadPreviousPreset() {
    if (presets_.empty())
      return;
    int prev = currentPresetIndex_ - 1;
    if (prev < 0)
      prev = static_cast<int>(presets_.size()) - 1;
    loadPreset(prev);
  }

  /** @brief Save current state as user preset */
  bool savePreset(const juce::String& name) {
    auto file = userPresetDirectory_.getChildFile(name + ".udspreset");

    // Get current state as XML
    juce::MemoryBlock data;
    processor_.getStateInformation(data);

    auto xml = juce::AudioProcessor::getXmlFromBinary(
        data.getData(), static_cast<int>(data.getSize()));
    if (xml == nullptr)
      return false;

    // Add metadata
    xml->setAttribute("presetName", name);
    xml->setAttribute("presetAuthor", "User");
    xml->setAttribute("presetCategory", "User");

    // Save to file
    if (!xml->writeTo(file))
      return false;

    // Refresh preset list
    scanPresets();

    // Find and set as current
    for (size_t i = 0; i < presets_.size(); ++i) {
      if (presets_[i].file == file) {
        currentPresetIndex_ = static_cast<int>(i);
        break;
      }
    }

    isModified_ = false;
    if (onPresetChanged)
      onPresetChanged();

    return true;
  }

  /** @brief Mark state as modified */
  void markModified() {
    isModified_ = true;
    if (onPresetChanged)
      onPresetChanged();
  }

  /** @brief Check if current state is modified */
  bool isModified() const { return isModified_; }

  /** @brief Re-scan preset folders */
  void scanPresets() {
    presets_.clear();

    // Scan user presets
    for (const auto& file : userPresetDirectory_.findChildFiles(
             juce::File::findFiles, false, "*.udspreset")) {
      PresetInfo info;
      info.file = file;
      info.name = file.getFileNameWithoutExtension();
      info.isFactory = false;

      // Try to read metadata from file
      if (auto xml = juce::XmlDocument::parse(file)) {
        info.name = xml->getStringAttribute("presetName", info.name);
        info.author = xml->getStringAttribute("presetAuthor", "User");
        info.category = xml->getStringAttribute("presetCategory", "User");
      }

      presets_.push_back(info);
    }

    // Sort alphabetically
    std::sort(presets_.begin(), presets_.end(),
              [](const PresetInfo& a, const PresetInfo& b) {
                return a.name.compareIgnoreCase(b.name) < 0;
              });
  }

  /** @brief Open user preset folder in system file browser */
  void showPresetFolder() { userPresetDirectory_.revealToUser(); }

  // Callback when preset changes
  std::function<void()> onPresetChanged;

  /**
   * @brief Creates factory presets if they don't exist
   *
   * 12 presets inspired by Allan Holdsworth's UD Stomp programming style:
   * - Stereo Enhanced Lead (4): Short delays for stereo widening
   * - Rhythmic Echoes (4): Medium delays for rhythmic patterns
   * - Vintage Textures (4): Long delays with character algorithms
   */
  void createFactoryPresets() {
    // === STEREO ENHANCED LEAD ===

    // 1. Stereo Widener - subtle L/R spread
    createPreset("01 - Stereo Widener", "Factory", "Stereo Lead",
                 {{true, 15.0f, 20.0f, -1.0f, -3.0f, 0}, // Band 1: 15ms hard L
                  {true, 25.0f, 15.0f, 1.0f, -3.0f, 0}}, // Band 2: 25ms hard R
                 {{0, 1}, {0, 2}, {1, 9}, {2, 9}});      // Parallel to output

    // 2. Chorus Lead - multi-tap with modulation
    createPreset(
        "02 - Chorus Lead", "Factory", "Stereo Lead",
        {{true, 12.0f, 10.0f, -0.7f, -4.0f, 0, 12000, 80, 0.5f, 15.0f, 1},
         {true, 18.0f, 10.0f, 0.3f, -4.0f, 0, 12000, 80, 0.6f, 12.0f, 1},
         {true, 24.0f, 10.0f, -0.3f, -5.0f, 0, 12000, 80, 0.4f, 18.0f, 1},
         {true, 30.0f, 10.0f, 0.7f, -5.0f, 0, 12000, 80, 0.5f, 10.0f, 1}},
        {{0, 1}, {0, 2}, {0, 3}, {0, 4}, {1, 9}, {2, 9}, {3, 9}, {4, 9}});

    // 3. Wide Shimmer - sparkly modulated
    createPreset(
        "03 - Wide Shimmer", "Factory", "Stereo Lead",
        {{true, 20.0f, 25.0f, -1.0f, -2.0f, 0, 15000, 60, 0.8f, 20.0f, 2},
         {true, 35.0f, 20.0f, 1.0f, -2.0f, 0, 15000, 60, 0.7f, 25.0f, 2}},
        {{0, 1}, {0, 2}, {1, 9}, {2, 9}});

    // 4. Synth Lead - saw mod for synth texture
    createPreset(
        "04 - Synth Lead", "Factory", "Stereo Lead",
        {{true, 10.0f, 15.0f, -0.8f, -3.0f, 0, 10000, 100, 1.0f, 30.0f, 3},
         {true, 20.0f, 12.0f, 0.0f, -4.0f, 0, 10000, 100, 1.2f, 25.0f, 3},
         {true, 40.0f, 10.0f, 0.8f, -5.0f, 0, 10000, 100, 0.8f, 35.0f, 3}},
        {{0, 1}, {0, 2}, {0, 3}, {1, 9}, {2, 9}, {3, 9}});

    // === RHYTHMIC ECHOES ===

    // 5. Dotted Eighth - classic Edge/U2 delay
    createPreset("05 - Dotted Eighth", "Factory", "Rhythmic",
                 {{true, 375.0f, 40.0f, 0.0f, -2.0f, 0, 8000, 100}},
                 {{0, 1}, {1, 9}});

    // 6. Ping Pong 8th - bouncing stereo
    createPreset("06 - Ping Pong 8th", "Factory", "Rhythmic",
                 {{true, 250.0f, 50.0f, -1.0f, -3.0f, 0, 10000, 80, 0, 0, 0,
                   false, true},
                  {true, 250.0f, 50.0f, 1.0f, -3.0f, 0, 10000, 80, 0, 0, 0,
                   false, true}},
                 {{0, 1}, {1, 2}, {2, 1}, {1, 9}, {2, 9}}); // Cross-feedback

    // 7. Polyrhythm - complex pattern
    createPreset("07 - Polyrhythm", "Factory", "Rhythmic",
                 {{true, 200.0f, 30.0f, -0.5f, -4.0f, 0},
                  {true, 300.0f, 30.0f, 0.0f, -4.0f, 0},
                  {true, 400.0f, 30.0f, 0.5f, -4.0f, 0}},
                 {{0, 1}, {0, 2}, {0, 3}, {1, 9}, {2, 9}, {3, 9}});

    // 8. Analog Echo - warm filtered
    createPreset(
        "08 - Analog Echo", "Factory", "Rhythmic",
        {{true, 350.0f, 55.0f, 0.0f, -1.0f, 1, 6000, 150}}, // Analog algo
        {{0, 1}, {1, 9}});

    // === VINTAGE TEXTURES ===

    // 9. Tape Echo - wow/flutter warmth
    createPreset(
        "09 - Tape Echo", "Factory", "Vintage",
        {{true, 400.0f, 45.0f, -0.3f, -2.0f, 2, 5000, 120, 0.3f, 8.0f, 1}},
        {{0, 1}, {1, 9}});

    // 10. Space Echo - dub-style series
    createPreset("10 - Space Echo", "Factory", "Vintage",
                 {{true, 250.0f, 35.0f, -0.5f, -3.0f, 1, 4000, 200},
                  {true, 500.0f, 40.0f, 0.5f, -4.0f, 1, 3500, 250}},
                 {{0, 1}, {1, 2}, {2, 9}}); // Series routing

    // 11. Ambient Wash - clean long tails
    createPreset("11 - Ambient Wash", "Factory", "Vintage",
                 {{true, 600.0f, 60.0f, -0.7f, -4.0f, 0, 12000, 40},
                  {true, 700.0f, 55.0f, 0.7f, -4.0f, 0, 12000, 40}},
                 {{0, 1}, {0, 2}, {1, 9}, {2, 9}});

    // 12. Lo-Fi Dreams - crushed and noisy
    createPreset(
        "12 - Lo-Fi Dreams", "Factory", "Vintage",
        {{true, 300.0f, 50.0f, 0.0f, -2.0f, 3, 4000, 300, 0.2f, 5.0f, 4}},
        {{0, 1}, {1, 9}});
  }

  /**
   * @brief Helper struct for band configuration
   */
  struct BandConfig {
    bool enabled = false;
    float timeMs = 250.0f;
    float feedbackPct = 30.0f;
    float pan = 0.0f;
    float levelDb = 0.0f;
    int algorithm = 0; // 0=Digital, 1=Analog, 2=Tape, 3=LoFi
    float hiCut = 12000.0f;
    float loCut = 80.0f;
    float lfoRate = 0.0f;
    float lfoDepth = 0.0f;
    int lfoWaveform = 0; // 0=None, 1=Sine, 2=Tri, 3=Saw, 4=Square
    bool phaseInvert = false;
    bool pingPong = false;
  };

  /**
   * @brief Create a preset file with given configuration
   */
  void createPreset(const juce::String& name, const juce::String& author,
                    const juce::String& category,
                    std::initializer_list<BandConfig> bands,
                    std::initializer_list<std::pair<int, int>> routing) {
    auto file = userPresetDirectory_.getChildFile(name + ".udspreset");
    if (file.existsAsFile())
      return; // Don't overwrite existing presets

    auto xml = std::make_unique<juce::XmlElement>("UDSState");
    xml->setAttribute("presetName", name);
    xml->setAttribute("presetAuthor", author);
    xml->setAttribute("presetCategory", category);

    // Create APVTS subtree
    auto* apvtsXml = xml->createNewChildElement("UDSParameters");
    apvtsXml->setAttribute("mix", 50.0f);

    // Configure all 8 bands
    int bandIdx = 0;
    for (const auto& band : bands) {
      juce::String prefix = "band" + juce::String(bandIdx) + "_";
      apvtsXml->setAttribute(prefix + "enabled", band.enabled);
      apvtsXml->setAttribute(prefix + "time", band.timeMs);
      apvtsXml->setAttribute(prefix + "feedback", band.feedbackPct);
      apvtsXml->setAttribute(prefix + "pan", band.pan);
      apvtsXml->setAttribute(prefix + "level", band.levelDb);
      apvtsXml->setAttribute(prefix + "algorithm", band.algorithm);
      apvtsXml->setAttribute(prefix + "hiCut", band.hiCut);
      apvtsXml->setAttribute(prefix + "loCut", band.loCut);
      apvtsXml->setAttribute(prefix + "lfoRate", band.lfoRate);
      apvtsXml->setAttribute(prefix + "lfoDepth", band.lfoDepth);
      apvtsXml->setAttribute(prefix + "lfoWaveform", band.lfoWaveform);
      apvtsXml->setAttribute(prefix + "phaseInvert", band.phaseInvert);
      apvtsXml->setAttribute(prefix + "pingPong", band.pingPong);
      ++bandIdx;
    }

    // Disable remaining bands
    for (int i = bandIdx; i < 8; ++i) {
      juce::String prefix = "band" + juce::String(i) + "_";
      apvtsXml->setAttribute(prefix + "enabled", false);
    }

    // Create routing
    auto* routingXml = xml->createNewChildElement("Routing");
    for (const auto& conn : routing) {
      auto* connXml = routingXml->createNewChildElement("Connection");
      connXml->setAttribute("source", conn.first);
      connXml->setAttribute("dest", conn.second);
    }

    xml->writeTo(file);
  }

  UDSAudioProcessor& processor_;
  juce::File userPresetDirectory_;
  std::vector<PresetInfo> presets_;
  int currentPresetIndex_ = -1;
  bool isModified_ = false;
};

} // namespace uds
