#pragma once

#include <algorithm>
#include <cmath>

namespace uds {

/**
 * @brief Attack envelope for volume swell effects
 *
 * Creates pad-like textures by fading in the wet signal over a
 * configurable attack time. Used for Holdsworth-style volume swell delays.
 *
 * Behavior:
 * - When input exceeds threshold, envelope ramps from 0 to 1 over attackTimeMs
 * - Envelope holds at 1 while signal is present
 * - When signal drops below threshold, envelope releases over releaseTimeMs
 * - Exponential curves for natural-sounding swells
 */
class AttackEnvelope {
public:
  void prepare(double sampleRate) {
    sampleRate_ = sampleRate;
    updateCoefficients();
  }

  void reset() {
    envelope_ = 0.0f;
    triggered_ = false;
  }

  void setAttackTimeMs(float attackMs) {
    if (attackTimeMs_ != attackMs) {
      attackTimeMs_ = std::clamp(attackMs, 0.0f, 5000.0f);
      updateCoefficients();
    }
  }

  void setReleaseTimeMs(float releaseMs) {
    if (releaseTimeMs_ != releaseMs) {
      releaseTimeMs_ = std::clamp(releaseMs, 1.0f, 5000.0f);
      updateCoefficients();
    }
  }

  void setThreshold(float thresholdDb) {
    // Convert dB to linear amplitude
    threshold_ = std::pow(10.0f, thresholdDb / 20.0f);
  }

  /**
   * @brief Process a single sample and return the envelope value
   * @param inputLevel The absolute level of the input signal (use std::abs)
   * @return Envelope value [0, 1] to multiply with wet signal
   */
  float process(float inputLevel) {
    // Trigger detection
    bool inputActive = inputLevel > threshold_;

    if (inputActive) {
      triggered_ = true;
      // Attack phase: ramp up
      envelope_ += attackCoeff_ * (1.0f - envelope_);
    } else if (triggered_) {
      // Release phase: ramp down
      envelope_ -= releaseCoeff_ * envelope_;
      if (envelope_ < 0.001f) {
        envelope_ = 0.0f;
        triggered_ = false;
      }
    }

    return envelope_;
  }

  /**
   * @brief Apply envelope to a stereo sample pair
   * @param inputL Left channel input (for level detection)
   * @param inputR Right channel input (for level detection)
   * @param wetL Left wet signal to apply envelope to (modified in place)
   * @param wetR Right wet signal to apply envelope to (modified in place)
   */
  void processBlock(float inputL, float inputR, float& wetL, float& wetR) {
    // Use peak of stereo input for trigger detection
    float inputLevel = std::max(std::abs(inputL), std::abs(inputR));
    float env = process(inputLevel);
    wetL *= env;
    wetR *= env;
  }

  float getEnvelope() const { return envelope_; }
  float getAttackTimeMs() const { return attackTimeMs_; }
  float getReleaseTimeMs() const { return releaseTimeMs_; }
  bool isActive() const { return envelope_ > 0.001f; }

private:
  void updateCoefficients() {
    if (sampleRate_ <= 0.0)
      return;

    // Time constant for exponential envelope
    // Coefficient = 1 - exp(-1 / (time_in_seconds * sample_rate))
    // This gives ~63% of the way to target per time constant

    if (attackTimeMs_ > 0.0f) {
      float attackSamples = (attackTimeMs_ / 1000.0f) * static_cast<float>(sampleRate_);
      // Use 5 time constants to reach ~99% of target
      attackCoeff_ = 1.0f - std::exp(-5.0f / attackSamples);
    } else {
      attackCoeff_ = 1.0f; // Instant attack
    }

    float releaseSamples = (releaseTimeMs_ / 1000.0f) * static_cast<float>(sampleRate_);
    releaseCoeff_ = 1.0f - std::exp(-5.0f / releaseSamples);
  }

  double sampleRate_ = 44100.0;
  float attackTimeMs_ = 0.0f;    // 0 = no swell (instant)
  float releaseTimeMs_ = 100.0f; // Quick release by default
  float threshold_ = 0.001f;     // -60dB default threshold

  float attackCoeff_ = 1.0f;
  float releaseCoeff_ = 0.01f;
  float envelope_ = 0.0f;
  bool triggered_ = false;
};

} // namespace uds
