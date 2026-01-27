#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>

// Include headers under test
#include "../Source/Core/FilterSection.h"
#include "../Source/Core/LFOModulator.h"
#include "../Source/Core/SafetyLimiter.h"


//==============================================================================
// SafetyLimiter Tests
//==============================================================================

TEST_CASE("SafetyLimiter prevents clipping", "[safety]") {
  uds::SafetyLimiter limiter;
  limiter.prepare(44100.0);

  SECTION("Normal signals pass through") {
    float left[512], right[512];
    for (int i = 0; i < 512; ++i) {
      left[i] = 0.5f * std::sin(2.0f * 3.14159f * 440.0f * i / 44100.0f);
      right[i] = left[i];
    }

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

//==============================================================================
// FilterSection Tests
//==============================================================================

TEST_CASE("FilterSection processes audio", "[dsp]") {
  uds::FilterSection filter;
  filter.prepare(44100.0, 512);

  SECTION("Passthrough at extreme frequencies") {
    // With very high hi-cut and very low lo-cut, should be near passthrough
    filter.setHiCut(20000.0f);
    filter.setLoCut(20.0f);

    float testSignal[512];
    for (int i = 0; i < 512; ++i) {
      testSignal[i] = 0.7f * std::sin(2.0f * 3.14159f * 1000.0f * i / 44100.0f);
    }

    float original[512];
    std::copy(testSignal, testSignal + 512, original);

    filter.process(testSignal, 512);

    // Signal should be mostly preserved (allowing for filter settling)
    float sumOriginal = 0, sumFiltered = 0;
    for (int i = 256; i < 512; ++i) {
      sumOriginal += std::abs(original[i]);
      sumFiltered += std::abs(testSignal[i]);
    }

    // Should preserve at least 90% of energy
    REQUIRE(sumFiltered > sumOriginal * 0.9f);
  }

  SECTION("Lo-cut removes DC offset") {
    filter.setLoCut(80.0f);
    filter.setHiCut(20000.0f);

    float testSignal[512];
    for (int i = 0; i < 512; ++i) {
      testSignal[i] = 0.5f; // Pure DC
    }

    filter.process(testSignal, 512);

    // After settling, DC should be significantly reduced
    float dcLevel = std::abs(testSignal[511]);
    REQUIRE(dcLevel < 0.1f);
  }
}

//==============================================================================
// LFOModulator Tests
//==============================================================================

TEST_CASE("LFOModulator generates waveforms", "[modulation]") {
  uds::LFOModulator lfo;
  lfo.prepare(44100.0);
  lfo.setRate(1.0f); // 1 Hz

  SECTION("Sine wave stays in range [-1, 1]") {
    lfo.setWaveform(uds::LFOModulator::Waveform::Sine);
    lfo.reset();

    for (int i = 0; i < 44100; ++i) {
      float value = lfo.getNextSample();
      REQUIRE(value >= -1.0f);
      REQUIRE(value <= 1.0f);
    }
  }

  SECTION("Triangle wave stays in range [-1, 1]") {
    lfo.setWaveform(uds::LFOModulator::Waveform::Triangle);
    lfo.reset();

    for (int i = 0; i < 44100; ++i) {
      float value = lfo.getNextSample();
      REQUIRE(value >= -1.0f);
      REQUIRE(value <= 1.0f);
    }
  }

  SECTION("Square wave is exactly -1 or 1") {
    lfo.setWaveform(uds::LFOModulator::Waveform::Square);
    lfo.reset();

    for (int i = 0; i < 44100; ++i) {
      float value = lfo.getNextSample();
      bool isValid = (std::abs(value - 1.0f) < 0.001f) ||
                     (std::abs(value + 1.0f) < 0.001f);
      REQUIRE(isValid);
    }
  }
}
