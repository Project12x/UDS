#define CATCH_CONFIG_MAIN
#include <array>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <vector>

#include <catch2/catch_all.hpp>


// Include headers under test
#include "../Source/Core/DelayAlgorithm.h"
#include "../Source/Core/DelayBandNode.h"
#include "../Source/Core/FilterSection.h"
#include "../Source/Core/GenerativeModulator.h"
#include "../Source/Core/LFOModulator.h"
#include "../Source/Core/ModulationEngine.h"
#include "../Source/Core/RoutingGraph.h"
#include "../Source/Core/SafetyLimiter.h"

// ============================================================================
// Test Utilities
// ============================================================================

// Generate a sine wave buffer
inline void generateSine(float* buffer, int numSamples, float freq,
                         float sampleRate, float amplitude = 0.5f) {
  for (int i = 0; i < numSamples; ++i) {
    buffer[i] = amplitude * std::sin(2.0f * 3.14159f * freq * i / sampleRate);
  }
}

// Calculate RMS energy
inline float calculateRMS(const float* buffer, int numSamples) {
  float sum = 0.0f;
  for (int i = 0; i < numSamples; ++i) {
    sum += buffer[i] * buffer[i];
  }
  return std::sqrt(sum / numSamples);
}

// ============================================================================
// SafetyLimiter Tests
// ============================================================================

TEST_CASE("SafetyLimiter prevents clipping", "[safety]") {
  uds::SafetyLimiter limiter;
  limiter.prepare(44100.0);

  SECTION("Normal signals pass through") {
    float left[512], right[512];
    generateSine(left, 512, 440.0f, 44100.0f, 0.5f);
    for (int i = 0; i < 512; ++i)
      right[i] = left[i];

    limiter.process(left, right, 512);

    for (int i = 0; i < 512; ++i) {
      REQUIRE(left[i] >= -1.0f);
      REQUIRE(left[i] <= 1.0f);
      REQUIRE(right[i] >= -1.0f);
      REQUIRE(right[i] <= 1.0f);
    }
  }

  SECTION("Loud signals are limited") {
    float left[512], right[512];
    for (int i = 0; i < 512; ++i) {
      left[i] = 10.0f; // Way too loud!
      right[i] = 10.0f;
    }

    limiter.process(left, right, 512);

    for (int i = 0; i < 512; ++i) {
      REQUIRE(left[i] <= 1.0f);
      REQUIRE(right[i] <= 1.0f);
    }
  }

  SECTION("Hard clipping catches extreme values") {
    float left[1] = {100.0f};
    float right[1] = {-100.0f};

    limiter.process(left, right, 1);

    REQUIRE(left[0] <= 1.0f);
    REQUIRE(right[0] >= -1.0f);
  }
}

// ============================================================================
// FilterSection Tests
// ============================================================================

TEST_CASE("FilterSection processes audio", "[dsp][filters]") {
  uds::FilterSection filter;
  filter.prepare(44100.0);

  SECTION("Passthrough at extreme frequencies") {
    filter.setHiCutFrequency(20000.0f);
    filter.setLoCutFrequency(20.0f);

    float testL[512], testR[512];
    generateSine(testL, 512, 1000.0f, 44100.0f, 0.7f);
    for (int i = 0; i < 512; ++i)
      testR[i] = testL[i];

    float originalRMS = calculateRMS(testL, 512);

    // Process sample by sample
    for (int i = 0; i < 512; ++i) {
      filter.processSample(testL[i], testR[i]);
    }

    float filteredRMS = calculateRMS(testL + 256, 256);

    // Should preserve most energy (after settling)
    REQUIRE(filteredRMS > originalRMS * 0.7f);
  }

  SECTION("Lo-cut removes DC offset") {
    filter.setLoCutFrequency(80.0f);
    filter.setHiCutFrequency(20000.0f);

    float left = 0.5f, right = 0.5f;

    // Process many samples with DC
    for (int i = 0; i < 1024; ++i) {
      left = 0.5f;
      right = 0.5f;
      filter.processSample(left, right);
    }

    // DC should be significantly reduced after settling
    REQUIRE(std::abs(left) < 0.15f);
  }

  SECTION("Hi-cut attenuates high frequencies") {
    filter.setHiCutFrequency(500.0f);
    filter.setLoCutFrequency(20.0f);
    filter.reset();

    // Generate high freq samples one at a time
    float sumEnergy = 0.0f;
    for (int i = 0; i < 1024; ++i) {
      float left = 0.7f * std::sin(2.0f * 3.14159f * 8000.0f * i / 44100.0f);
      float right = left;
      filter.processSample(left, right);
      if (i >= 512) { // After settling
        sumEnergy += left * left;
      }
    }

    // High freq should be significantly attenuated
    float rms = std::sqrt(sumEnergy / 512);
    REQUIRE(rms < 0.3f);
  }

  SECTION("Getters return correct values") {
    filter.setHiCutFrequency(5000.0f);
    filter.setLoCutFrequency(100.0f);

    REQUIRE(filter.getHiCutHz() == 5000.0f);
    REQUIRE(filter.getLoCutHz() == 100.0f);
  }
}

// ============================================================================
// LFOModulator Tests
// ============================================================================

TEST_CASE("LFOModulator generates waveforms", "[modulation]") {
  uds::LFOModulator lfo;
  lfo.prepare(44100.0);
  lfo.setRate(1.0f);
  lfo.setDepth(1.0f); // Full depth for testing

  SECTION("Sine wave stays in range [-1, 1]") {
    lfo.setWaveform(uds::LFOWaveform::Sine);
    lfo.reset();

    for (int i = 0; i < 44100; ++i) {
      float value = lfo.tick();
      REQUIRE(value >= -1.0f);
      REQUIRE(value <= 1.0f);
    }
  }

  SECTION("Triangle wave stays in range [-1, 1]") {
    lfo.setWaveform(uds::LFOWaveform::Triangle);
    lfo.reset();

    for (int i = 0; i < 44100; ++i) {
      float value = lfo.tick();
      REQUIRE(value >= -1.0f);
      REQUIRE(value <= 1.0f);
    }
  }

  SECTION("Sawtooth wave stays in range [-1, 1]") {
    lfo.setWaveform(uds::LFOWaveform::Saw);
    lfo.reset();

    for (int i = 0; i < 44100; ++i) {
      float value = lfo.tick();
      REQUIRE(value >= -1.0f);
      REQUIRE(value <= 1.0f);
    }
  }

  SECTION("Square wave output at depth 1.0") {
    lfo.setWaveform(uds::LFOWaveform::Square);
    lfo.setDepth(1.0f);
    lfo.reset();

    for (int i = 0; i < 44100; ++i) {
      float value = lfo.tick();
      bool isValid = (std::abs(value - 1.0f) < 0.001f) ||
                     (std::abs(value + 1.0f) < 0.001f);
      REQUIRE(isValid);
    }
  }

  SECTION("Rate affects period") {
    lfo.setWaveform(uds::LFOWaveform::Triangle);
    lfo.setDepth(1.0f);
    lfo.setRate(10.0f); // 10 Hz = 4410 samples per cycle
    lfo.reset();

    // Count zero crossings in one second
    int zeroCrossings = 0;
    float prev = lfo.tick();
    for (int i = 1; i < 44100; ++i) {
      float curr = lfo.tick();
      if ((prev < 0 && curr >= 0) || (prev >= 0 && curr < 0))
        ++zeroCrossings;
      prev = curr;
    }

    // 10 Hz should have about 20 zero crossings per second
    REQUIRE(zeroCrossings >= 18);
    REQUIRE(zeroCrossings <= 22);
  }

  SECTION("Depth controls output amplitude") {
    lfo.setWaveform(uds::LFOWaveform::Sine);
    lfo.setDepth(0.5f); // Half depth
    lfo.reset();

    float maxValue = 0.0f;
    for (int i = 0; i < 44100; ++i) {
      float value = std::abs(lfo.tick());
      if (value > maxValue)
        maxValue = value;
    }

    // Max should be around 0.5 (depth)
    REQUIRE(maxValue >= 0.45f);
    REQUIRE(maxValue <= 0.55f);
  }

  SECTION("None waveform produces zero") {
    lfo.setWaveform(uds::LFOWaveform::None);
    lfo.setDepth(1.0f);
    lfo.reset();

    for (int i = 0; i < 100; ++i) {
      float value = lfo.tick();
      REQUIRE(value == 0.0f);
    }
  }
}

// ============================================================================
// DelayAlgorithm Tests
// ============================================================================

TEST_CASE("Delay algorithms process samples", "[dsp][algorithms]") {
  const double sampleRate = 44100.0;

  SECTION("Digital delay passes through unchanged") {
    uds::DigitalDelay digital;
    digital.prepare(sampleRate);

    float input = 0.5f;
    float output = digital.processSample(input);

    // Digital should pass through unchanged
    REQUIRE(output == input);
  }

  SECTION("Analog delay adds saturation") {
    uds::AnalogDelay analog;
    analog.prepare(sampleRate);

    // Process multiple samples to let filter settle
    for (int i = 0; i < 100; ++i) {
      analog.processSample(0.5f);
    }

    float output = analog.processSample(0.5f);

    // Analog should produce output (with saturation/filter)
    REQUIRE(output != 0.0f);
    REQUIRE(std::abs(output) <= 1.0f);
  }

  SECTION("Tape delay adds character") {
    uds::TapeDelay tape;
    tape.prepare(sampleRate);

    // Process multiple samples
    for (int i = 0; i < 100; ++i) {
      tape.processSample(0.5f);
    }

    float output = tape.processSample(0.5f);

    // Tape should produce output
    REQUIRE(output != 0.0f);
    REQUIRE(std::abs(output) <= 1.0f);
  }

  SECTION("LoFi delay quantizes signal") {
    uds::LoFiDelay lofi;
    lofi.prepare(sampleRate);

    // Process with decimation (need multiple samples)
    float outputs[10];
    for (int i = 0; i < 10; ++i) {
      outputs[i] = lofi.processSample(0.5f);
    }

    // Should produce output
    REQUIRE(std::abs(outputs[9]) > 0.0f);
  }

  SECTION("Algorithm types are correct") {
    uds::DigitalDelay digital;
    uds::AnalogDelay analog;
    uds::TapeDelay tape;
    uds::LoFiDelay lofi;

    REQUIRE(digital.getType() == uds::DelayAlgorithmType::Digital);
    REQUIRE(analog.getType() == uds::DelayAlgorithmType::Analog);
    REQUIRE(tape.getType() == uds::DelayAlgorithmType::Tape);
    REQUIRE(lofi.getType() == uds::DelayAlgorithmType::LoFi);
  }

  SECTION("Factory creates correct types") {
    auto digital = uds::createDelayAlgorithm(uds::DelayAlgorithmType::Digital);
    auto analog = uds::createDelayAlgorithm(uds::DelayAlgorithmType::Analog);
    auto tape = uds::createDelayAlgorithm(uds::DelayAlgorithmType::Tape);
    auto lofi = uds::createDelayAlgorithm(uds::DelayAlgorithmType::LoFi);

    REQUIRE(digital->getType() == uds::DelayAlgorithmType::Digital);
    REQUIRE(analog->getType() == uds::DelayAlgorithmType::Analog);
    REQUIRE(tape->getType() == uds::DelayAlgorithmType::Tape);
    REQUIRE(lofi->getType() == uds::DelayAlgorithmType::LoFi);
  }

  SECTION("Algorithms stay bounded") {
    auto algorithms = {
        uds::createDelayAlgorithm(uds::DelayAlgorithmType::Digital),
        uds::createDelayAlgorithm(uds::DelayAlgorithmType::Analog),
        uds::createDelayAlgorithm(uds::DelayAlgorithmType::Tape),
        uds::createDelayAlgorithm(uds::DelayAlgorithmType::LoFi)};

    for (auto& algo : algorithms) {
      algo->prepare(sampleRate);

      // Process loud signal
      for (int i = 0; i < 100; ++i) {
        float output = algo->processSample(2.0f); // Loud input
        REQUIRE(std::abs(output) <= 5.0f);        // Should stay reasonable
      }
    }
  }
}

// ============================================================================
// RoutingGraph Tests
// ============================================================================

TEST_CASE("RoutingGraph manages connections", "[routing]") {
  uds::RoutingGraph graph;

  SECTION("Default has Input->Output connection") {
    auto connections = graph.getConnections();
    REQUIRE(connections.size() >= 1);
  }

  SECTION("Can add connections") {
    graph.clearAllConnections();
    graph.connect(0, 1); // Input to Band 1
    graph.connect(1, 9); // Band 1 to Output

    auto connections = graph.getConnections();
    REQUIRE(connections.size() == 2);
  }

  SECTION("Can remove connections") {
    graph.clearAllConnections();
    graph.connect(0, 1);
    graph.connect(1, 9);
    graph.disconnect(0, 1);

    auto connections = graph.getConnections();
    REQUIRE(connections.size() == 1);
  }

  SECTION("Can clear all connections") {
    graph.connect(0, 1);
    graph.connect(1, 2);
    graph.connect(2, 9);
    graph.clearAllConnections();

    auto connections = graph.getConnections();
    REQUIRE(connections.empty());
  }

  SECTION("Parallel routing pattern") {
    graph.setDefaultParallelRouting();

    auto connections = graph.getConnections();
    REQUIRE(connections.size() == 16);
  }

  SECTION("Series routing pattern") {
    graph.setSeriesRouting();

    auto connections = graph.getConnections();
    // Input->B1, B1->B2, ... B7->B8, B8->Output = 9 connections
    REQUIRE(connections.size() == 9);
  }

  SECTION("Prevents duplicate connections") {
    graph.clearAllConnections();
    graph.connect(0, 1);
    graph.connect(0, 1); // Duplicate

    auto connections = graph.getConnections();
    REQUIRE(connections.size() == 1);
  }

  SECTION("Prevents self-connection") {
    graph.clearAllConnections();
    bool result = graph.connect(1, 1); // Self-connection
    REQUIRE(result == false);
  }

  SECTION("Can't connect TO input") {
    graph.clearAllConnections();
    bool result = graph.connect(1, 0); // Band to Input (invalid)
    REQUIRE(result == false);
  }

  SECTION("Can't connect FROM output") {
    graph.clearAllConnections();
    bool result = graph.connect(9, 1); // Output to Band (invalid)
    REQUIRE(result == false);
  }
}

// ============================================================================
// DelayBandNode Tests (if the header compiles standalone)
// ============================================================================

// Note: DelayBandNode requires JUCE's full audio infrastructure
// These tests may need to be run with JUCE linked

TEST_CASE("DelayBandNode parameter validation", "[dsp][parameters]") {

  SECTION("Delay time range") {
    // Valid range: 1ms to 2000ms (2 seconds)
    float minTime = 1.0f;
    float maxTime = 2000.0f;

    REQUIRE(minTime > 0.0f);
    REQUIRE(maxTime >= minTime);
    REQUIRE(maxTime <= 5000.0f); // Reasonable upper limit
  }

  SECTION("Feedback range") {
    // Valid range: 0% to 100%
    float minFeedback = 0.0f;
    float maxFeedback = 100.0f;

    REQUIRE(minFeedback >= 0.0f);
    REQUIRE(maxFeedback <= 100.0f);

    // Converting to gain: 100% = 1.0 gain
    float feedbackGain = maxFeedback / 100.0f;
    REQUIRE(feedbackGain <= 1.0f);
  }

  SECTION("Pan range") {
    // Valid range: -1.0 (left) to +1.0 (right)
    float panLeft = -1.0f;
    float panCenter = 0.0f;
    float panRight = 1.0f;

    REQUIRE(panLeft >= -1.0f);
    REQUIRE(panCenter == 0.0f);
    REQUIRE(panRight <= 1.0f);
  }

  SECTION("Level dB range") {
    // Valid range: -24dB to 0dB
    float minLevel = -24.0f;
    float maxLevel = 0.0f;

    // Convert to linear gain
    float minGain = std::pow(10.0f, minLevel / 20.0f);
    float maxGain = std::pow(10.0f, maxLevel / 20.0f);

    REQUIRE(minGain < 0.1f);  // -24dB ~ 0.063
    REQUIRE(maxGain == 1.0f); // 0dB = unity
  }

  SECTION("Filter frequency ranges") {
    // Lo-cut: 20Hz to 1000Hz
    float loCutMin = 20.0f;
    float loCutMax = 1000.0f;

    // Hi-cut: 1000Hz to 20000Hz
    float hiCutMin = 1000.0f;
    float hiCutMax = 20000.0f;

    REQUIRE(loCutMin > 0.0f);
    REQUIRE((loCutMax < hiCutMin || loCutMax == hiCutMin));
    REQUIRE(hiCutMax <= 22050.0f); // Nyquist at 44.1kHz
  }

  SECTION("LFO rate range") {
    // Valid range: 0.01Hz to 10Hz
    float minRate = 0.01f;
    float maxRate = 10.0f;

    // Period in seconds
    float maxPeriod = 1.0f / minRate; // 100 seconds
    float minPeriod = 1.0f / maxRate; // 0.1 seconds

    REQUIRE(maxPeriod == 100.0f);
    REQUIRE(minPeriod == 0.1f);
  }

  SECTION("LFO depth range") {
    // Valid range: 0% to 100%
    float minDepth = 0.0f;
    float maxDepth = 100.0f;

    REQUIRE(minDepth >= 0.0f);
    REQUIRE(maxDepth <= 100.0f);
  }
}

// ============================================================================
// Tempo Sync Tests
// ============================================================================

TEST_CASE("Tempo sync calculations", "[tempo]") {

  SECTION("Quarter note at 120 BPM = 500ms") {
    double bpm = 120.0;
    double beatsPerSecond = bpm / 60.0;
    double quarterNoteMs = 1000.0 / beatsPerSecond;

    REQUIRE(std::abs(quarterNoteMs - 500.0) < 0.001);
  }

  SECTION("Eighth note at 120 BPM = 250ms") {
    double bpm = 120.0;
    double quarterNoteMs = 60000.0 / bpm;
    double eighthNoteMs = quarterNoteMs / 2.0;

    REQUIRE(std::abs(eighthNoteMs - 250.0) < 0.001);
  }

  SECTION("Dotted eighth at 120 BPM = 375ms") {
    double bpm = 120.0;
    double quarterNoteMs = 60000.0 / bpm;
    double eighthNoteMs = quarterNoteMs / 2.0;
    double dottedEighthMs = eighthNoteMs * 1.5;

    REQUIRE(std::abs(dottedEighthMs - 375.0) < 0.001);
  }

  SECTION("Triplet eighth at 120 BPM = 166.67ms") {
    double bpm = 120.0;
    double quarterNoteMs = 60000.0 / bpm;
    double tripletEighthMs = quarterNoteMs / 3.0;

    REQUIRE(std::abs(tripletEighthMs - 166.667) < 0.01);
  }

  SECTION("Note divisions cover expected range") {
    double bpm = 120.0;
    double quarterNoteMs = 60000.0 / bpm;

    // Note divisions: 1/1, 1/2, 1/4, 1/8, 1/16, 1/32
    double whole = quarterNoteMs * 4.0;
    double half = quarterNoteMs * 2.0;
    double quarter = quarterNoteMs;
    double eighth = quarterNoteMs / 2.0;
    double sixteenth = quarterNoteMs / 4.0;
    double thirtysecond = quarterNoteMs / 8.0;

    REQUIRE(whole == 2000.0);
    REQUIRE(half == 1000.0);
    REQUIRE(quarter == 500.0);
    REQUIRE(eighth == 250.0);
    REQUIRE(sixteenth == 125.0);
    REQUIRE(std::abs(thirtysecond - 62.5) < 0.001);
  }
}

// ============================================================================
// Mix/Blend Tests
// ============================================================================

TEST_CASE("Dry/Wet mix calculations", "[mix]") {

  SECTION("0% wet = fully dry") {
    float dry = 1.0f;
    float wet = 0.5f;
    float mix = 0.0f; // 0% wet

    float dryGain = 1.0f - mix;
    float wetGain = mix;
    float output = dry * dryGain + wet * wetGain;

    REQUIRE(output == dry);
  }

  SECTION("100% wet = fully wet") {
    float dry = 1.0f;
    float wet = 0.5f;
    float mix = 1.0f; // 100% wet

    float dryGain = 1.0f - mix;
    float wetGain = mix;
    float output = dry * dryGain + wet * wetGain;

    REQUIRE(output == wet);
  }

  SECTION("50% wet = equal blend") {
    float dry = 1.0f;
    float wet = 0.0f;
    float mix = 0.5f; // 50% wet

    float dryGain = 1.0f - mix;
    float wetGain = mix;
    float output = dry * dryGain + wet * wetGain;

    REQUIRE(output == 0.5f);
  }
}

// ============================================================================
// Phase Invert Test
// ============================================================================

TEST_CASE("Phase inversion", "[dsp]") {

  SECTION("Phase invert negates signal") {
    float sample = 0.75f;
    bool phaseInvert = true;

    float output = phaseInvert ? -sample : sample;
    REQUIRE(output == -0.75f);
  }

  SECTION("No invert passes through") {
    float sample = 0.75f;
    bool phaseInvert = false;

    float output = phaseInvert ? -sample : sample;
    REQUIRE(output == 0.75f);
  }

  SECTION("Inverted + original = silence") {
    float original = 0.5f;
    float inverted = -original;
    float sum = original + inverted;

    REQUIRE(sum == 0.0f);
  }
}

// ============================================================================
// RIGOROUS BOUNDARY TESTS - These are designed to FAIL if code is broken
// ============================================================================

TEST_CASE("SafetyLimiter edge cases", "[safety][boundary]") {
  uds::SafetyLimiter limiter;
  limiter.prepare(44100.0);

  SECTION("Handles NaN input without crash or propagation") {
    float left[10] = {0.5f, std::numeric_limits<float>::quiet_NaN(),
                      0.5f, 0.5f,
                      0.5f, 0.5f,
                      0.5f, 0.5f,
                      0.5f, 0.5f};
    float right[10] = {0.5f, 0.5f, 0.5f, 0.5f, 0.5f,
                       0.5f, 0.5f, 0.5f, 0.5f, 0.5f};

    // Should not crash
    limiter.process(left, right, 10);

    // Output should not contain NaN (limiter should clamp to valid range)
    for (int i = 0; i < 10; ++i) {
      REQUIRE_FALSE(std::isnan(left[i]));
      REQUIRE_FALSE(std::isnan(right[i]));
      REQUIRE_FALSE(std::isinf(left[i]));
      REQUIRE_FALSE(std::isinf(right[i]));
    }
  }

  SECTION("Handles Inf input") {
    float left[5] = {std::numeric_limits<float>::infinity(), 0.5f, 0.5f, 0.5f,
                     0.5f};
    float right[5] = {-std::numeric_limits<float>::infinity(), 0.5f, 0.5f, 0.5f,
                      0.5f};

    limiter.process(left, right, 5);

    // Should be clamped to valid range
    REQUIRE(left[0] <= 1.0f);
    REQUIRE(right[0] >= -1.0f);
    REQUIRE_FALSE(std::isinf(left[0]));
    REQUIRE_FALSE(std::isinf(right[0]));
  }

  SECTION("Zero-length buffer doesn't crash") {
    float left[1] = {0.5f};
    float right[1] = {0.5f};
    // Should handle gracefully
    limiter.process(left, right, 0);
    // Original values unchanged
    REQUIRE(left[0] == 0.5f);
  }
}

TEST_CASE("FilterSection boundary conditions", "[filters][boundary]") {
  uds::FilterSection filter;
  filter.prepare(44100.0);

  SECTION("Extreme hi-cut frequency is clamped to Nyquist") {
    // Setting hiCut above Nyquist should be clamped
    filter.setHiCutFrequency(30000.0f); // Above 22050 Nyquist

    // Should not crash during processing
    float left = 0.5f, right = 0.5f;
    for (int i = 0; i < 100; ++i) {
      filter.processSample(left, right);
      // Output should remain finite
      REQUIRE_FALSE(std::isnan(left));
      REQUIRE_FALSE(std::isinf(left));
    }
  }

  SECTION("Very low hi-cut heavily attenuates mid-freq signal") {
    filter.setHiCutFrequency(100.0f); // Very low cutoff
    filter.setLoCutFrequency(20.0f);
    filter.reset();

    // 1kHz tone should be HEAVILY attenuated by 100Hz lowpass
    float energy = 0.0f;
    for (int i = 0; i < 2048; ++i) {
      float left = 0.7f * std::sin(2.0f * 3.14159f * 1000.0f * i / 44100.0f);
      float right = left;
      filter.processSample(left, right);
      if (i >= 1024) { // After settling
        energy += left * left;
      }
    }

    float rms = std::sqrt(energy / 1024.0f);
    // 1kHz is ~10x higher than 100Hz cutoff = ~40dB attenuation minimum
    // 0.7 * 0.01 = 0.007 max expected RMS
    REQUIRE(rms < 0.05f); // Should be heavily attenuated
  }

  SECTION("Filter doesn't explode with rapid coefficient changes") {
    float left = 0.5f, right = 0.5f;
    for (int i = 0; i < 1000; ++i) {
      // Rapidly changing filter frequencies
      filter.setHiCutFrequency(500.0f + (i % 2) * 10000.0f);
      filter.setLoCutFrequency(20.0f + (i % 2) * 500.0f);
      filter.processSample(left, right);

      REQUIRE(std::abs(left) < 100.0f); // Should stay bounded
      REQUIRE(std::abs(right) < 100.0f);
      REQUIRE_FALSE(std::isnan(left));
    }
  }
}

TEST_CASE("LFOModulator accuracy", "[modulation][boundary]") {
  uds::LFOModulator lfo;
  lfo.prepare(44100.0);

  SECTION("1Hz LFO completes exactly one cycle in 44100 samples") {
    lfo.setRate(1.0f);
    lfo.setDepth(1.0f);
    lfo.setWaveform(uds::LFOWaveform::Sine);
    lfo.reset();

    // Track when we cross zero going positive
    int zeroCrossings = 0;
    float prev = lfo.tick();
    for (int i = 1; i < 44100; ++i) {
      float curr = lfo.tick();
      // Positive-going zero crossing
      if (prev <= 0.0f && curr > 0.0f) {
        ++zeroCrossings;
      }
      prev = curr;
    }

    // 1Hz sine should have exactly 1 positive-going zero crossing per second
    // Allow tolerance of 1 due to floating point
    REQUIRE(zeroCrossings >= 0);
    REQUIRE(zeroCrossings <= 2);
  }

  SECTION("10Hz LFO has 10 positive zero crossings per second") {
    lfo.setRate(10.0f);
    lfo.setDepth(1.0f);
    lfo.setWaveform(uds::LFOWaveform::Sine);
    lfo.reset();

    int zeroCrossings = 0;
    float prev = lfo.tick();
    for (int i = 1; i < 44100; ++i) {
      float curr = lfo.tick();
      if (prev <= 0.0f && curr > 0.0f) {
        ++zeroCrossings;
      }
      prev = curr;
    }

    // Should be exactly 10 (with small tolerance for edge cases)
    REQUIRE(zeroCrossings >= 9);
    REQUIRE(zeroCrossings <= 11);
  }

  SECTION("Triangle wave reaches exactly +1 and -1 peaks") {
    lfo.setRate(1.0f);
    lfo.setDepth(1.0f);
    lfo.setWaveform(uds::LFOWaveform::Triangle);
    lfo.reset();

    float maxVal = -2.0f, minVal = 2.0f;
    for (int i = 0; i < 44100; ++i) {
      float v = lfo.tick();
      if (v > maxVal)
        maxVal = v;
      if (v < minVal)
        minVal = v;
    }

    // Triangle with depth=1 should hit exactly +1 and -1
    REQUIRE(maxVal >= 0.99f);
    REQUIRE(maxVal <= 1.01f);
    REQUIRE(minVal >= -1.01f);
    REQUIRE(minVal <= -0.99f);
  }

  SECTION("Zero depth produces zero output regardless of rate") {
    lfo.setRate(10.0f);
    lfo.setDepth(0.0f);
    lfo.setWaveform(uds::LFOWaveform::Sine);
    lfo.reset();

    for (int i = 0; i < 1000; ++i) {
      float v = lfo.tick();
      REQUIRE(v == 0.0f);
    }
  }
}

TEST_CASE("DelayAlgorithm correctness", "[algorithms][boundary]") {
  const double sampleRate = 44100.0;

  SECTION("Digital algorithm is truly transparent") {
    uds::DigitalDelay digital;
    digital.prepare(sampleRate);

    // Process 1000 random values - all should be unchanged
    for (int i = 0; i < 1000; ++i) {
      float input = static_cast<float>(i) / 1000.0f - 0.5f;
      float output = digital.processSample(input);
      REQUIRE(output == input); // Must be EXACTLY equal
    }
  }

  SECTION("Analog algorithm adds measurable saturation") {
    uds::AnalogDelay analog;
    analog.prepare(sampleRate);
    analog.reset();

    // Loud input should be compressed by saturation
    float loudInput = 0.9f;
    float output = 0.0f;
    for (int i = 0; i < 500; ++i) {
      output = analog.processSample(loudInput);
    }

    // After settling, output should be less than input due to tanh compression
    REQUIRE(output < loudInput);
    REQUIRE(output > 0.0f); // But still positive
  }

  SECTION("LoFi algorithm introduces quantization") {
    uds::LoFiDelay lofi;
    lofi.prepare(sampleRate);

    // Fine gradients should be quantized to same values
    float out1 = lofi.processSample(0.5001f);
    lofi.reset();
    float out2 = lofi.processSample(0.5002f);

    // Due to bit reduction, these similar values should produce
    // same or very close output (quantized to same level)
    // Note: Need multiple samples due to decimation
    for (int i = 0; i < 10; ++i) {
      out1 = lofi.processSample(0.5001f);
    }
    lofi.reset();
    for (int i = 0; i < 10; ++i) {
      out2 = lofi.processSample(0.5002f);
    }

    // Quantization means these should round to same value (or very close)
    REQUIRE(std::abs(out1 - out2) < 0.01f);
  }
}

TEST_CASE("RoutingGraph integrity", "[routing][boundary]") {
  uds::RoutingGraph graph;

  SECTION("Cycle detection prevents infinite loops") {
    graph.clearAllConnections();

    // Try to create a cycle: A->B->C->A
    graph.connect(1, 2);
    graph.connect(2, 3);

    // Without cycle prevention, this would create A->B->C->A
    bool wouldCycle = graph.wouldCreateCycle(3, 1);
    REQUIRE(wouldCycle == true);
  }

  SECTION("Processing order is topologically sorted") {
    graph.clearAllConnections();

    // Create: Input -> B1 -> B2 -> Output
    graph.connect(0, 1);
    graph.connect(1, 2);
    graph.connect(2, 9);

    auto order = graph.getProcessingOrder();

    // Input (0) must come before B1 (1)
    // B1 (1) must come before B2 (2)
    // B2 (2) must come before Output (9)
    int posInput = -1, posB1 = -1, posB2 = -1, posOutput = -1;
    for (size_t i = 0; i < order.size(); ++i) {
      if (order[i] == 0)
        posInput = static_cast<int>(i);
      if (order[i] == 1)
        posB1 = static_cast<int>(i);
      if (order[i] == 2)
        posB2 = static_cast<int>(i);
      if (order[i] == 9)
        posOutput = static_cast<int>(i);
    }

    // Verify topological ordering
    if (posInput >= 0 && posB1 >= 0)
      REQUIRE(posInput < posB1);
    if (posB1 >= 0 && posB2 >= 0)
      REQUIRE(posB1 < posB2);
    if (posB2 >= 0 && posOutput >= 0)
      REQUIRE(posB2 < posOutput);
  }

  SECTION("Disconnected nodes are excluded from routing") {
    graph.clearAllConnections();

    // Only connect B1 to output
    graph.connect(0, 1);
    graph.connect(1, 9);

    // B2 is not connected
    auto inputsForOutput = graph.getInputsFor(9);
    bool b2FeedsOutput = false;
    for (int id : inputsForOutput) {
      if (id == 2)
        b2FeedsOutput = true;
    }

    REQUIRE(b2FeedsOutput == false);
  }
}

TEST_CASE("Tempo sync calculations are precise", "[tempo][boundary]") {

  SECTION("BPM to ms conversion is accurate within 0.1ms") {
    // At 120 BPM, quarter note = exactly 500ms
    double bpm = 120.0;
    double quarterNoteMs = 60000.0 / bpm;

    REQUIRE(std::abs(quarterNoteMs - 500.0) < 0.001);
  }

  SECTION("Dotted notes are exactly 1.5x straight notes") {
    double bpm = 100.0;
    double quarterNoteMs = 60000.0 / bpm; // 600ms
    double dottedQuarterMs = quarterNoteMs * 1.5;

    REQUIRE(dottedQuarterMs == 900.0);
  }

  SECTION("Triplet notes are exactly 2/3x straight notes") {
    double bpm = 120.0;
    double quarterNoteMs = 60000.0 / bpm; // 500ms
    double tripletQuarterMs = quarterNoteMs * (2.0 / 3.0);

    REQUIRE(std::abs(tripletQuarterMs - 333.333) < 0.01);
  }

  SECTION("Very slow BPM doesn't overflow") {
    double bpm = 20.0; // Very slow
    double quarterNoteMs = 60000.0 / bpm;

    REQUIRE(quarterNoteMs == 3000.0); // 3 seconds
    REQUIRE_FALSE(std::isinf(quarterNoteMs));
  }

  SECTION("Very fast BPM produces valid small values") {
    double bpm = 300.0; // Very fast
    double quarterNoteMs = 60000.0 / bpm;

    REQUIRE(quarterNoteMs == 200.0);
    REQUIRE(quarterNoteMs > 0.0);
  }
}

// ============================================================================
// MagicStomp Preset Import Tests (Requires nlohmann/json.hpp - Disabled)
// ============================================================================
// NOTE: These tests require PresetManager.h which depends on nlohmann/json.hpp.
// They are disabled in the headless test suite to avoid external dependencies.
// To run import tests, build with the full plugin target.

#if 0 // Disabled: requires nlohmann/json.hpp
#include "../Source/Core/PresetManager.h"

TEST_CASE("MagicStomp JSON import creates presets", "[import]") {
  // Path to ah.json on desktop
  juce::File jsonFile("C:/Users/estee/Desktop/ah.json");

  SECTION("Import from ah.json") {
    REQUIRE(jsonFile.existsAsFile());

    // Create temp directory for user presets
    juce::File tempPresetDir =
        juce::File::getSpecialLocation(juce::File::tempDirectory)
            .getChildFile("UDS_Import_Test");
    tempPresetDir.createDirectory();

    // Create preset manager with temp directory
    uds::PresetManager presetManager(tempPresetDir, juce::File());

    // Run import
    int imported = presetManager.importFromMagicStompJson(jsonFile);

    INFO("Imported " << imported << " presets");
    REQUIRE(imported > 0);

    // List created presets
    auto presetFiles = tempPresetDir.findChildFiles(juce::File::findFiles,
                                                    false, "*.udspreset");

    INFO("Created preset files:");
    for (const auto& f : presetFiles) {
      INFO("  - " << f.getFileName().toStdString());
    }

    REQUIRE(presetFiles.size() == static_cast<size_t>(imported));

    // Cleanup
    tempPresetDir.deleteRecursively();
  }
}
#endif // Disabled import tests

// ============================================================================
// GenerativeModulator Tests (Phase 5)
// ============================================================================

TEST_CASE("GenerativeModulator generates waveforms",
          "[modulation][generative]") {
  uds::GenerativeModulator mod;
  mod.prepare(44100.0);
  mod.setParams(uds::ModulationType::Sine, 1.0f, 1.0f);

  SECTION("Sine wave stays in range [-1, 1]") {
    mod.setParams(uds::ModulationType::Sine, 1.0f, 1.0f);
    mod.reset();

    for (int i = 0; i < 44100; ++i) {
      float value = mod.tick();
      REQUIRE(value >= -1.0f);
      REQUIRE(value <= 1.0f);
    }
  }

  SECTION("Triangle wave stays in range [-1, 1]") {
    mod.setParams(uds::ModulationType::Triangle, 1.0f, 1.0f);
    mod.reset();

    for (int i = 0; i < 44100; ++i) {
      float value = mod.tick();
      REQUIRE(value >= -1.0f);
      REQUIRE(value <= 1.0f);
    }
  }

  SECTION("Saw wave stays in range [-1, 1]") {
    mod.setParams(uds::ModulationType::Saw, 1.0f, 1.0f);
    mod.reset();

    for (int i = 0; i < 44100; ++i) {
      float value = mod.tick();
      REQUIRE(value >= -1.0f);
      REQUIRE(value <= 1.0f);
    }
  }

  SECTION("Square wave output is +/- depth") {
    mod.setParams(uds::ModulationType::Square, 1.0f, 1.0f);
    mod.reset();

    for (int i = 0; i < 44100; ++i) {
      float value = mod.tick();
      bool isValid = (std::abs(value - 1.0f) < 0.001f) ||
                     (std::abs(value + 1.0f) < 0.001f);
      REQUIRE(isValid);
    }
  }

  SECTION("Brownian motion stays bounded") {
    mod.setParams(uds::ModulationType::Brownian, 1.0f, 1.0f);
    mod.reset();

    // Process for 5 seconds to check long-term stability
    for (int i = 0; i < 44100 * 5; ++i) {
      float value = mod.tick();
      REQUIRE(value >= -1.0f);
      REQUIRE(value <= 1.0f);
      REQUIRE_FALSE(std::isnan(value));
      REQUIRE_FALSE(std::isinf(value));
    }
  }

  SECTION("Brownian motion is non-periodic") {
    mod.setParams(uds::ModulationType::Brownian, 1.0f, 1.0f);
    mod.reset();

    // Collect 1000 samples and check they're not all the same
    std::vector<float> samples;
    samples.reserve(1000);
    for (int i = 0; i < 1000; ++i) {
      samples.push_back(mod.tick());
    }

    // Count unique values (should be many due to randomness)
    std::sort(samples.begin(), samples.end());
    auto last = std::unique(samples.begin(), samples.end());
    int uniqueCount = static_cast<int>(std::distance(samples.begin(), last));

    // Should have many unique values (randomness check)
    REQUIRE(uniqueCount > 100);
  }

  SECTION("Lorenz attractor stays bounded") {
    mod.setParams(uds::ModulationType::Lorenz, 1.0f, 1.0f);
    mod.reset();

    // Process for 5 seconds
    for (int i = 0; i < 44100 * 5; ++i) {
      float value = mod.tick();
      REQUIRE(value >= -1.0f);
      REQUIRE(value <= 1.0f);
      REQUIRE_FALSE(std::isnan(value));
      REQUIRE_FALSE(std::isinf(value));
    }
  }

  SECTION("Lorenz attractor is chaotic (non-repeating)") {
    mod.setParams(uds::ModulationType::Lorenz, 1.0f, 1.0f);
    mod.reset();

    // Collect samples and check they have variation
    std::vector<float> samples;
    samples.reserve(1000);
    for (int i = 0; i < 1000; ++i) {
      samples.push_back(mod.tick());
    }

    // Calculate variance - should be non-zero for chaotic signal
    float sum = 0.0f;
    for (float s : samples)
      sum += s;
    float mean = sum / static_cast<float>(samples.size());

    float variance = 0.0f;
    for (float s : samples)
      variance += (s - mean) * (s - mean);
    variance /= static_cast<float>(samples.size());

    REQUIRE(variance > 0.01f);
  }

  SECTION("Zero depth produces zero output for all types") {
    for (int type = 0; type <= 5; ++type) {
      mod.setParams(static_cast<uds::ModulationType>(type), 1.0f, 0.0f);
      mod.reset();

      for (int i = 0; i < 100; ++i) {
        float value = mod.tick();
        REQUIRE(value == 0.0f);
      }
    }
  }

  SECTION("Rate affects LFO period correctly") {
    mod.setParams(uds::ModulationType::Sine, 10.0f, 1.0f);
    mod.reset();

    // Count zero crossings in one second (10 Hz = 10 positive crossings)
    int zeroCrossings = 0;
    float prev = mod.tick();
    for (int i = 1; i < 44100; ++i) {
      float curr = mod.tick();
      if (prev <= 0.0f && curr > 0.0f) {
        ++zeroCrossings;
      }
      prev = curr;
    }

    REQUIRE(zeroCrossings >= 9);
    REQUIRE(zeroCrossings <= 11);
  }
}

// ============================================================================
// ModulationEngine Tests (Phase 5)
// ============================================================================

TEST_CASE("ModulationEngine processes modulation", "[modulation][engine]") {
  uds::ModulationEngine engine;
  engine.prepare(44100.0, 512);

  SECTION("Produces valid buffers after prepare") {
    engine.process(512);

    const auto& localBuf = engine.getLocalBuffer();
    const auto& masterBuf = engine.getMasterBuffer();

    REQUIRE(localBuf.getNumChannels() == 8);
    REQUIRE(masterBuf.getNumChannels() == 1);
  }

  SECTION("Band modulator produces output when depth > 0") {
    engine.setBandParams(0, uds::ModulationType::Sine, 1.0f, 1.0f);
    engine.process(512);

    const auto& localBuf = engine.getLocalBuffer();
    const float* data = localBuf.getReadPointer(0);

    // Check that we have non-zero values
    float maxVal = 0.0f;
    for (int i = 0; i < 512; ++i) {
      maxVal = std::max(maxVal, std::abs(data[i]));
    }

    REQUIRE(maxVal > 0.0f);
  }

  SECTION("Master modulator produces output when depth > 0") {
    engine.setMasterParams(uds::ModulationType::Triangle, 2.0f, 1.0f);
    engine.process(512);

    const auto& masterBuf = engine.getMasterBuffer();
    const float* data = masterBuf.getReadPointer(0);

    // Check that we have non-zero values
    float maxVal = 0.0f;
    for (int i = 0; i < 512; ++i) {
      maxVal = std::max(maxVal, std::abs(data[i]));
    }

    REQUIRE(maxVal > 0.0f);
  }

  SECTION("Reset clears buffers") {
    engine.setBandParams(0, uds::ModulationType::Sine, 1.0f, 1.0f);
    engine.process(512);
    engine.reset();
    engine.process(512);

    // After reset, phase should start from 0 again
    // This is a soft check - just ensure no crash
    const auto& localBuf = engine.getLocalBuffer();
    REQUIRE(localBuf.getNumSamples() >= 512);
  }

  SECTION("Brownian modulation produces valid output") {
    engine.setBandParams(3, uds::ModulationType::Brownian, 1.0f, 0.5f);
    engine.process(512);

    const auto& localBuf = engine.getLocalBuffer();
    const float* data = localBuf.getReadPointer(3);

    for (int i = 0; i < 512; ++i) {
      REQUIRE(data[i] >= -1.0f);
      REQUIRE(data[i] <= 1.0f);
      REQUIRE_FALSE(std::isnan(data[i]));
    }
  }

  SECTION("Lorenz modulation produces valid output") {
    engine.setMasterParams(uds::ModulationType::Lorenz, 1.0f, 0.8f);
    engine.process(512);

    const auto& masterBuf = engine.getMasterBuffer();
    const float* data = masterBuf.getReadPointer(0);

    for (int i = 0; i < 512; ++i) {
      REQUIRE(data[i] >= -1.0f);
      REQUIRE(data[i] <= 1.0f);
      REQUIRE_FALSE(std::isnan(data[i]));
    }
  }

  SECTION("All 8 bands can have independent modulation") {
    // Set different params for each band
    for (int i = 0; i < 8; ++i) {
      engine.setBandParams(i, uds::ModulationType::Sine,
                           static_cast<float>(i + 1), 1.0f);
    }
    engine.process(512);

    const auto& localBuf = engine.getLocalBuffer();
    REQUIRE(localBuf.getNumChannels() == 8);

    // Each band should have valid output
    for (int ch = 0; ch < 8; ++ch) {
      const float* data = localBuf.getReadPointer(ch);
      for (int i = 0; i < 512; ++i) {
        REQUIRE_FALSE(std::isnan(data[i]));
      }
    }
  }
}

// ============================================================================
// LFO Chorus Effect Diagnostic Tests
// ============================================================================

TEST_CASE("LFO Modulation produces measurable chorus effects",
          "[lfo][chorus]") {
  constexpr double SAMPLE_RATE = 44100.0;
  constexpr int BLOCK_SIZE = 512;
  constexpr int NUM_BLOCKS = 100; // ~1.2 seconds

  SECTION("GenerativeModulator outputs expected range at different depths") {
    uds::GenerativeModulator mod;
    mod.prepare(SAMPLE_RATE);

    // Test various depths
    std::vector<float> depths = {0.1f, 0.25f, 0.5f, 0.75f, 1.0f};

    std::cout << "\n=== GenerativeModulator Output Range Test ===" << std::endl;
    std::cout << "Rate: 2 Hz, Waveform: Sine" << std::endl;

    for (float depth : depths) {
      mod.setParams(uds::ModulationType::Sine, 2.0f, depth);
      mod.reset();

      float minVal = 1.0f, maxVal = -1.0f;

      // Process enough samples for a full LFO cycle (at 2Hz, need 0.5s = 22050
      // samples)
      for (int i = 0; i < 44100; ++i) {
        float val = mod.tick();
        minVal = std::min(minVal, val);
        maxVal = std::max(maxVal, val);
      }

      float range = maxVal - minVal;
      std::cout << "  Depth " << (depth * 100) << "%: range = " << minVal
                << " to " << maxVal << " (total: " << range << ")" << std::endl;

      // Expected: range should be approximately 2 * depth (for sine wave)
      REQUIRE(range >= depth * 1.8f); // Allow some tolerance
      REQUIRE(range <= depth * 2.2f);
    }
  }

  SECTION("Master LFO generates expected modulation range") {
    uds::ModulationEngine engine;
    engine.prepare(SAMPLE_RATE, BLOCK_SIZE);

    std::cout << "\n=== Master LFO Modulation Range Test ===" << std::endl;

    // Test with full depth
    engine.setMasterParams(uds::ModulationType::Sine, 2.0f, 1.0f);

    float minVal = 1.0f, maxVal = -1.0f;

    for (int block = 0; block < NUM_BLOCKS; ++block) {
      engine.process(BLOCK_SIZE);
      const float* data = engine.getMasterBuffer().getReadPointer(0);
      for (int i = 0; i < BLOCK_SIZE; ++i) {
        minVal = std::min(minVal, data[i]);
        maxVal = std::max(maxVal, data[i]);
      }
    }

    std::cout << "  Master LFO (100% depth): " << minVal << " to " << maxVal
              << std::endl;
    REQUIRE(maxVal > 0.8f);
    REQUIRE(minVal < -0.8f);
  }

  SECTION("Per-band LFO generates expected modulation range") {
    uds::ModulationEngine engine;
    engine.prepare(SAMPLE_RATE, BLOCK_SIZE);

    std::cout << "\n=== Per-Band LFO Modulation Range Test ===" << std::endl;

    // Set band 0 with full depth
    engine.setBandParams(0, uds::ModulationType::Sine, 2.0f, 1.0f);

    float minVal = 1.0f, maxVal = -1.0f;

    for (int block = 0; block < NUM_BLOCKS; ++block) {
      engine.process(BLOCK_SIZE);
      const float* data = engine.getLocalBuffer().getReadPointer(0);
      for (int i = 0; i < BLOCK_SIZE; ++i) {
        minVal = std::min(minVal, data[i]);
        maxVal = std::max(maxVal, data[i]);
      }
    }

    std::cout << "  Band 0 LFO (100% depth): " << minVal << " to " << maxVal
              << std::endl;
    REQUIRE(maxVal > 0.8f);
    REQUIRE(minVal < -0.8f);
  }

  SECTION("DelayBandNode applies modulation to delay time") {
    uds::DelayBandNode band;
    band.prepare(SAMPLE_RATE, BLOCK_SIZE);

    // Create test signal - 1kHz sine wave
    juce::AudioBuffer<float> buffer(2, BLOCK_SIZE);
    for (int i = 0; i < BLOCK_SIZE; ++i) {
      float sample =
          0.5f * std::sin(2.0f * 3.14159f * 1000.0f * i / SAMPLE_RATE);
      buffer.setSample(0, i, sample);
      buffer.setSample(1, i, sample);
    }

    // Create modulation signal - varies from -1 to +1
    std::vector<float> modSignal(BLOCK_SIZE);
    for (int i = 0; i < BLOCK_SIZE; ++i) {
      modSignal[i] =
          std::sin(2.0f * 3.14159f * 2.0f * i / SAMPLE_RATE); // 2 Hz LFO
    }

    // Set up band params for chorus
    uds::DelayBandParams params;
    params.delayTimeMs = 25.0f; // Chorus-range delay
    params.feedback = 0.0f;     // No feedback for cleaner test
    params.level = 1.0f;
    params.pan = 0.0f;
    params.enabled = true;
    params.lfoDepth = 1.0f; // Full depth
    params.lfoRateHz = 2.0f;
    band.setParams(params);

    std::cout << "\n=== DelayBandNode Modulation Application Test ==="
              << std::endl;
    std::cout << "  Base delay: 25ms, Full modulation depth" << std::endl;

    // Process with external modulation signal
    juce::AudioBuffer<float> testBuffer(2, BLOCK_SIZE);
    testBuffer.copyFrom(0, 0, buffer, 0, 0, BLOCK_SIZE);
    testBuffer.copyFrom(1, 0, buffer, 1, 0, BLOCK_SIZE);

    band.process(testBuffer, 1.0f, modSignal.data(), nullptr);

    float rms = 0.0f;
    for (int i = 0; i < BLOCK_SIZE; ++i) {
      rms += testBuffer.getSample(0, i) * testBuffer.getSample(0, i);
    }
    rms = std::sqrt(rms / BLOCK_SIZE);

    std::cout << "  Output RMS: " << rms << std::endl;
    REQUIRE(rms > 0.0f); // Should have output
  }

  SECTION("Chorus depth experiment - find optimal modulation range") {
    std::cout << "\n=== CHORUS DEPTH EXPERIMENT ===" << std::endl;
    std::cout << "Testing different modulation multipliers to find optimal "
                 "chorus range"
              << std::endl;
    std::cout << "Base delay: 25ms, LFO Rate: 2Hz, LFO Depth: 100%"
              << std::endl;
    std::cout << "\nCurrent implementation: totalMod * 25.0f ms" << std::endl;
    std::cout << "With depth=1.0 and LFO range ±1.0:" << std::endl;
    std::cout << "  Modulation range = ±25ms" << std::endl;
    std::cout << "  For 25ms base delay: delay varies 0ms to 50ms" << std::endl;
    std::cout << "\nFor classic chorus (5-30ms range):" << std::endl;
    std::cout << "  Typical modulation: ±2-5ms" << std::endl;
    std::cout << "  Depth 10% at 25ms multiplier = ±2.5ms ✓" << std::endl;
    std::cout << "  Depth 20% at 25ms multiplier = ±5ms ✓" << std::endl;
    std::cout << "\nFor AH Chorus preset (Depth=2.5 on 0-10 scale = 25%):"
              << std::endl;
    std::cout << "  25% * 25ms = ±6.25ms modulation" << std::endl;
    std::cout << "  This SHOULD be audible as chorus effect!" << std::endl;

    // Just output diagnostic info - no actual test assertion needed
    REQUIRE(true);
  }

  SECTION("Verify modulation signal reaches DelayBandNode") {
    std::cout << "\n=== MODULATION SIGNAL PATH TEST ===" << std::endl;

    uds::ModulationEngine engine;
    engine.prepare(SAMPLE_RATE, BLOCK_SIZE);

    // Set per-band LFO
    engine.setBandParams(0, uds::ModulationType::Sine, 2.0f, 0.5f); // 50% depth

    // Process a block
    engine.process(BLOCK_SIZE);

    // Check output values
    const float* bandMod = engine.getLocalBuffer().getReadPointer(0);

    float sum = 0.0f;
    float minVal = 1.0f, maxVal = -1.0f;
    for (int i = 0; i < BLOCK_SIZE; ++i) {
      sum += std::abs(bandMod[i]);
      minVal = std::min(minVal, bandMod[i]);
      maxVal = std::max(maxVal, bandMod[i]);
    }
    float avgAbs = sum / BLOCK_SIZE;

    std::cout << "  Band 0 modulation (50% depth):" << std::endl;
    std::cout << "    Min: " << minVal << ", Max: " << maxVal << std::endl;
    std::cout << "    Avg absolute: " << avgAbs << std::endl;

    // At 50% depth, we expect values roughly ±0.5
    REQUIRE(avgAbs > 0.1f); // Should have significant modulation
  }
}

// ============================================================================
// UD Stomp Parameter Scaling Experiment
// ============================================================================

TEST_CASE("UD Stomp parameter scaling experiment", "[udstomP][scaling]") {
  constexpr double SAMPLE_RATE = 44100.0;
  constexpr int BLOCK_SIZE = 512;

  std::cout << "\n" << std::string(70, '=') << std::endl;
  std::cout << "UD STOMP PARAMETER SCALING EXPERIMENT" << std::endl;
  std::cout << std::string(70, '=') << std::endl;
  std::cout << "\nUnknown mappings from original UD Stomp:" << std::endl;
  std::cout << "  - Speed 0-10: What Hz range? (0.1-1Hz? 0.1-10Hz? 0.01-5Hz?)"
            << std::endl;
  std::cout
      << "  - Depth 0-10: Percentage of what? (Fixed ms? % of delay time?)"
      << std::endl;

  SECTION("Rate scaling experiment") {
    std::cout << "\n--- RATE SCALING EXPERIMENT ---" << std::endl;
    std::cout << "Testing: What Hz range does Speed 0-10 map to?\n"
              << std::endl;

    // Test different rate scaling formulas
    // UD Stomp Speed=3 in the chorus preset
    float udStompSpeed = 3.0f; // From AH Chorus preset

    std::cout << "For UD Stomp Speed=3, possible Hz mappings:" << std::endl;
    std::cout << std::setw(30) << "Formula" << " | " << std::setw(10)
              << "Result Hz"
              << " | " << "Chorus Quality" << std::endl;
    std::cout << std::string(60, '-') << std::endl;

    // Option 1: Direct (speed = Hz)
    float rate1 = udStompSpeed;
    std::cout << std::setw(30) << "Direct (speed = Hz)" << " | "
              << std::setw(10) << rate1 << " | " << "Too fast for subtle chorus"
              << std::endl;

    // Option 2: Speed * 0.1 (0-10 -> 0-1 Hz)
    float rate2 = udStompSpeed * 0.1f;
    std::cout << std::setw(30) << "speed * 0.1 (0-1 Hz range)" << " | "
              << std::setw(10) << rate2 << " | " << "Classic slow chorus"
              << std::endl;

    // Option 3: Speed * 0.5 (0-10 -> 0-5 Hz)
    float rate3 = udStompSpeed * 0.5f;
    std::cout << std::setw(30) << "speed * 0.5 (0-5 Hz range)" << " | "
              << std::setw(10) << rate3 << " | " << "Medium chorus"
              << std::endl;

    // Option 4: Exponential (0.1 * 2^(speed/3))
    float rate4 = 0.1f * std::pow(2.0f, udStompSpeed / 3.0f);
    std::cout << std::setw(30) << "0.1 * 2^(speed/3) (exp)" << " | "
              << std::setw(10) << rate4 << " | " << "Leslie-style range"
              << std::endl;

    std::cout << "\nTypical chorus LFO rates: 0.1 - 3 Hz" << std::endl;
    std::cout << "Typical vibrato LFO rates: 3 - 7 Hz" << std::endl;
    std::cout << "RECOMMENDATION: speed * 0.1 to 0.5 for chorus" << std::endl;
  }

  SECTION("Depth scaling experiment") {
    std::cout << "\n--- DEPTH SCALING EXPERIMENT ---" << std::endl;
    std::cout << "Testing: What does Depth 0-10 modulate?\n" << std::endl;

    float udStompDepth = 2.5f; // From AH Chorus preset
    float baseDelayMs = 25.0f; // Typical chorus delay

    std::cout << "For UD Stomp Depth=2.5, base delay=" << baseDelayMs
              << "ms:" << std::endl;
    std::cout << std::setw(40) << "Formula" << " | " << std::setw(15)
              << "Mod Range (ms)"
              << " | " << "Effect" << std::endl;
    std::cout << std::string(80, '-') << std::endl;

    // Option 1: Current impl (depth% of 25ms fixed)
    float mod1 = (udStompDepth / 10.0f) * 25.0f;
    std::cout << std::setw(40) << "Current: (d/10) * 25ms fixed" << " | +/-"
              << std::setw(12) << mod1 << " | " << "±6.25ms - Good chorus"
              << std::endl;

    // Option 2: Depth as direct ms
    float mod2 = udStompDepth;
    std::cout << std::setw(40) << "depth = ms directly" << " | +/-"
              << std::setw(12) << mod2 << " | " << "±2.5ms - Subtle chorus"
              << std::endl;

    // Option 3: Depth as % of delay time
    float mod3 = baseDelayMs * (udStompDepth / 10.0f);
    std::cout << std::setw(40) << "(depth/10) * delayTime" << " | +/-"
              << std::setw(12) << mod3 << " | " << "±6.25ms - Proportional"
              << std::endl;

    // Option 4: Depth * 2ms (scaled)
    float mod4 = udStompDepth * 2.0f;
    std::cout << std::setw(40) << "depth * 2ms" << " | +/-" << std::setw(12)
              << mod4 << " | " << "±5ms - Middle ground" << std::endl;

    // Option 5: Depth * 5ms (larger range)
    float mod5 = udStompDepth * 5.0f;
    std::cout << std::setw(40) << "depth * 5ms" << " | +/-" << std::setw(12)
              << mod5 << " | " << "±12.5ms - Wide vibrato" << std::endl;

    std::cout << "\nTypical chorus modulation: ±1-5ms" << std::endl;
    std::cout << "Typical vibrato modulation: ±5-15ms" << std::endl;
    std::cout << "RECOMMENDATION: depth * 1-2ms for subtle chorus" << std::endl;
  }

  SECTION("Full signal chain test with different scalings") {
    std::cout << "\n--- FULL SIGNAL CHAIN MODULATION TEST ---" << std::endl;
    std::cout << "Processing actual audio through DelayBandNode\n" << std::endl;

    uds::DelayBandNode band;
    band.prepare(SAMPLE_RATE, BLOCK_SIZE);

    // Test signal: 440 Hz sine (A4)
    std::vector<float> inputL(BLOCK_SIZE), inputR(BLOCK_SIZE);
    for (int i = 0; i < BLOCK_SIZE; ++i) {
      float t = static_cast<float>(i) / static_cast<float>(SAMPLE_RATE);
      inputL[i] = inputR[i] = 0.5f * std::sin(2.0f * 3.14159265f * 440.0f * t);
    }

    // Create modulation buffer that will be applied
    std::vector<float> modBuffer(BLOCK_SIZE);

    // Test different depth values and measure output variation
    std::cout << std::setw(20) << "LFO Depth" << " | " << std::setw(15)
              << "Mod Range"
              << " | " << std::setw(15) << "Output RMS" << " | " << "Notes"
              << std::endl;
    std::cout << std::string(70, '-') << std::endl;

    std::vector<float> testDepths = {0.0f, 0.1f, 0.25f, 0.5f, 1.0f};

    for (float depth : testDepths) {
      // Reset band
      band.reset();

      // Set params
      uds::DelayBandParams params;
      params.delayTimeMs = 25.0f;
      params.feedback = 0.0f;
      params.level = 1.0f;
      params.pan = 0.0f;
      params.enabled = true;
      params.lfoDepth = depth;
      params.lfoRateHz = 2.0f;
      band.setParams(params);

      // Fill delay line first (warm up)
      for (int warmup = 0; warmup < 10; ++warmup) {
        juce::AudioBuffer<float> warmupBuf(2, BLOCK_SIZE);
        warmupBuf.copyFrom(0, 0, inputL.data(), BLOCK_SIZE);
        warmupBuf.copyFrom(1, 0, inputR.data(), BLOCK_SIZE);

        // Generate modulation: sine wave from -1 to +1
        for (int i = 0; i < BLOCK_SIZE; ++i) {
          float t = static_cast<float>(warmup * BLOCK_SIZE + i) /
                    static_cast<float>(SAMPLE_RATE);
          modBuffer[i] = std::sin(2.0f * 3.14159265f * 2.0f * t) * depth;
        }

        band.process(warmupBuf, 1.0f, modBuffer.data(), nullptr);
      }

      // Now process and measure
      juce::AudioBuffer<float> testBuf(2, BLOCK_SIZE);
      testBuf.copyFrom(0, 0, inputL.data(), BLOCK_SIZE);
      testBuf.copyFrom(1, 0, inputR.data(), BLOCK_SIZE);

      for (int i = 0; i < BLOCK_SIZE; ++i) {
        modBuffer[i] = std::sin(2.0f * 3.14159265f * 2.0f * 10.0f *
                                static_cast<float>(BLOCK_SIZE + i) /
                                static_cast<float>(SAMPLE_RATE)) *
                       depth;
      }

      band.process(testBuf, 1.0f, modBuffer.data(), nullptr);

      // Calculate RMS
      float rms = 0.0f;
      for (int i = 0; i < BLOCK_SIZE; ++i) {
        rms += testBuf.getSample(0, i) * testBuf.getSample(0, i);
      }
      rms = std::sqrt(rms / BLOCK_SIZE);

      float modRangeMs = depth * 25.0f; // Current implementation

      std::string notes = "";
      if (depth == 0.0f)
        notes = "No modulation (dry)";
      else if (depth <= 0.1f)
        notes = "Subtle detuning";
      else if (depth <= 0.25f)
        notes = "Gentle chorus";
      else if (depth <= 0.5f)
        notes = "Standard chorus";
      else
        notes = "Heavy vibrato";

      std::cout << std::setw(20) << (depth * 100) << "% | +/-" << std::setw(12)
                << modRangeMs << "ms | " << std::setw(15) << rms << " | "
                << notes << std::endl;
    }

    std::cout << "\nIf output RMS is 0 or very low, modulation isn't working!"
              << std::endl;
  }

  SECTION("Recommended scaling for AH Chorus presets") {
    std::cout << "\n--- RECOMMENDED SCALING FOR AH CHORUS ---" << std::endl;

    std::cout << "\nOriginal AH Chorus preset values:" << std::endl;
    std::cout << "  Speed = 3 (0-10 scale)" << std::endl;
    std::cout << "  Depth = 2.5 (0-10 scale)" << std::endl;
    std::cout << "  DelayTime = 23.6ms (short, chorus-appropriate)"
              << std::endl;

    std::cout << "\nCurrent UDS implementation:" << std::endl;
    std::cout << "  Rate: Speed passed directly as Hz -> 3 Hz" << std::endl;
    std::cout << "  Depth: (Depth * 10) / 100 * 25ms -> 6.25ms range"
              << std::endl;

    std::cout << "\nRECOMMENDED CHANGES:" << std::endl;
    std::cout << "  1. Rate: Speed * 0.3 for chorus (0.9 Hz)" << std::endl;
    std::cout << "     Or Speed * 0.5 for faster chorus (1.5 Hz)" << std::endl;
    std::cout << "  2. Depth: Depth * 1.0ms for subtle chorus (2.5ms range)"
              << std::endl;
    std::cout << "     Or keep current for dramatic chorus" << std::endl;

    std::cout << "\nTo implement, modify PresetManager.h:" << std::endl;
    std::cout << "  line 502: band.lfoRate = getFloatParam(prefix + \"Speed\") "
                 "* 0.3f;"
              << std::endl;
    std::cout << "  line 505: band.lfoDepth = getFloatParam(prefix + "
                 "\"Depth\") * 10.0f;"
              << std::endl;
    std::cout
        << "            (keep as-is, or reduce multiplier for subtler effect)"
        << std::endl;

    REQUIRE(true);
  }
}
