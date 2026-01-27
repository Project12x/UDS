#pragma once

#include <cmath>

namespace uds {

/**
 * @brief LFO waveform types
 */
enum class LFOWaveform {
  None = 0, // No modulation
  Sine,     // Smooth sinusoidal
  Triangle, // Linear ramp up/down
  Saw,      // Linear ramp
  Square    // On/off
};

/**
 * @brief Low Frequency Oscillator for delay time modulation
 *
 * Provides smooth modulation for creating chorus-like effects,
 * vibrato, and the tape wow/flutter character.
 */
class LFOModulator {
public:
  void prepare(double sampleRate) { sampleRate_ = sampleRate; }

  void reset() { phase_ = 0.0f; }

  void setRate(float rateHz) { rateHz_ = std::clamp(rateHz, 0.01f, 20.0f); }

  void setDepth(float depth) { depth_ = std::clamp(depth, 0.0f, 1.0f); }

  void setWaveform(LFOWaveform waveform) { waveform_ = waveform; }

  /**
   * @brief Get the next LFO value and advance phase
   * @return Value in range [-depth, +depth]
   */
  float tick() {
    float value = 0.0f;

    switch (waveform_) {
    case LFOWaveform::None:
      return 0.0f; // No modulation

    case LFOWaveform::Sine:
      value = std::sin(phase_ * 2.0f * 3.14159265f);
      break;

    case LFOWaveform::Triangle:
      // Triangle: ramps up 0->1 in first half, down 1->0 in second half
      if (phase_ < 0.5f) {
        value = 4.0f * phase_ - 1.0f;
      } else {
        value = 3.0f - 4.0f * phase_;
      }
      break;

    case LFOWaveform::Saw:
      // Sawtooth: ramps from -1 to +1
      value = 2.0f * phase_ - 1.0f;
      break;

    case LFOWaveform::Square:
      value = phase_ < 0.5f ? 1.0f : -1.0f;
      break;
    }

    // Advance phase
    float phaseIncrement = rateHz_ / static_cast<float>(sampleRate_);
    phase_ += phaseIncrement;
    if (phase_ >= 1.0f) {
      phase_ -= 1.0f;
    }

    return value * depth_;
  }

  /**
   * @brief Get modulated delay time in milliseconds
   * @param baseTimeMs The base delay time
   * @param maxModMs Maximum modulation amount in milliseconds
   * @return Modulated delay time
   */
  float getModulatedTime(float baseTimeMs, float maxModMs = 5.0f) {
    float modulation = tick();
    return baseTimeMs + (modulation * maxModMs);
  }

  float getRate() const { return rateHz_; }
  float getDepth() const { return depth_; }
  LFOWaveform getWaveform() const { return waveform_; }

private:
  double sampleRate_ = 44100.0;
  float phase_ = 0.0f;
  float rateHz_ = 1.0f;
  float depth_ = 0.0f;
  LFOWaveform waveform_ = LFOWaveform::Sine;
};

} // namespace uds
