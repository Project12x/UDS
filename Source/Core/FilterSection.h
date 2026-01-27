#pragma once

#include <cmath>

namespace uds {

/**
 * @brief Simple biquad filter coefficients
 */
struct BiquadCoeffs {
  float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
  float a1 = 0.0f, a2 = 0.0f;
};

/**
 * @brief Biquad filter state
 */
struct BiquadState {
  float z1 = 0.0f, z2 = 0.0f;

  void reset() { z1 = z2 = 0.0f; }

  float process(float input, const BiquadCoeffs &c) {
    float output = c.b0 * input + z1;
    z1 = c.b1 * input - c.a1 * output + z2;
    z2 = c.b2 * input - c.a2 * output;
    return output;
  }
};

/**
 * @brief Hi-cut and Lo-cut filter section for delay feedback path
 *
 * Uses 2nd order Butterworth filters for smooth frequency response.
 * Hi-cut: Low-pass filter (removes highs)
 * Lo-cut: High-pass filter (removes lows)
 */
class FilterSection {
public:
  void prepare(double sampleRate) {
    sampleRate_ = sampleRate;
    updateCoefficients();
  }

  void reset() {
    hiCutStateL_.reset();
    hiCutStateR_.reset();
    loCutStateL_.reset();
    loCutStateR_.reset();
  }

  void setHiCutFrequency(float freqHz) {
    if (hiCutHz_ != freqHz) {
      hiCutHz_ = freqHz;
      updateHiCut();
    }
  }

  void setLoCutFrequency(float freqHz) {
    if (loCutHz_ != freqHz) {
      loCutHz_ = freqHz;
      updateLoCut();
    }
  }

  // Process stereo pair
  void processSample(float &left, float &right) {
    // Apply hi-cut (low-pass)
    left = hiCutStateL_.process(left, hiCutCoeffs_);
    right = hiCutStateR_.process(right, hiCutCoeffs_);

    // Apply lo-cut (high-pass)
    left = loCutStateL_.process(left, loCutCoeffs_);
    right = loCutStateR_.process(right, loCutCoeffs_);
  }

  float getHiCutHz() const { return hiCutHz_; }
  float getLoCutHz() const { return loCutHz_; }

private:
  void updateCoefficients() {
    updateHiCut();
    updateLoCut();
  }

  // Low-pass (hi-cut) Butterworth calculation
  void updateHiCut() {
    if (sampleRate_ <= 0.0)
      return;

    // Clamp frequency
    float freq =
        std::clamp(hiCutHz_, 20.0f, static_cast<float>(sampleRate_ * 0.49));

    // Butterworth low-pass
    float omega = 2.0f * 3.14159265f * freq / static_cast<float>(sampleRate_);
    float sinOmega = std::sin(omega);
    float cosOmega = std::cos(omega);
    float alpha = sinOmega / (2.0f * 0.7071f); // Q = sqrt(2)/2 for Butterworth

    float a0 = 1.0f + alpha;
    hiCutCoeffs_.b0 = ((1.0f - cosOmega) / 2.0f) / a0;
    hiCutCoeffs_.b1 = (1.0f - cosOmega) / a0;
    hiCutCoeffs_.b2 = ((1.0f - cosOmega) / 2.0f) / a0;
    hiCutCoeffs_.a1 = (-2.0f * cosOmega) / a0;
    hiCutCoeffs_.a2 = (1.0f - alpha) / a0;
  }

  // High-pass (lo-cut) Butterworth calculation
  void updateLoCut() {
    if (sampleRate_ <= 0.0)
      return;

    // Clamp frequency
    float freq =
        std::clamp(loCutHz_, 20.0f, static_cast<float>(sampleRate_ * 0.49));

    // Butterworth high-pass
    float omega = 2.0f * 3.14159265f * freq / static_cast<float>(sampleRate_);
    float sinOmega = std::sin(omega);
    float cosOmega = std::cos(omega);
    float alpha = sinOmega / (2.0f * 0.7071f);

    float a0 = 1.0f + alpha;
    loCutCoeffs_.b0 = ((1.0f + cosOmega) / 2.0f) / a0;
    loCutCoeffs_.b1 = (-(1.0f + cosOmega)) / a0;
    loCutCoeffs_.b2 = ((1.0f + cosOmega) / 2.0f) / a0;
    loCutCoeffs_.a1 = (-2.0f * cosOmega) / a0;
    loCutCoeffs_.a2 = (1.0f - alpha) / a0;
  }

  double sampleRate_ = 44100.0;
  float hiCutHz_ = 12000.0f;
  float loCutHz_ = 80.0f;

  BiquadCoeffs hiCutCoeffs_;
  BiquadCoeffs loCutCoeffs_;

  BiquadState hiCutStateL_, hiCutStateR_;
  BiquadState loCutStateL_, loCutStateR_;
};

} // namespace uds
