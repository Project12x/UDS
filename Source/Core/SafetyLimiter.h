#pragma once

#include <algorithm>
#include <cmath>

namespace uds {

/**
 * @brief Comprehensive audio safety system to protect equipment and hearing
 *
 * Multi-stage protection chain with PERMANENT mute on danger detection:
 * 0. Sustained peak detection (+6dBFS for 100ms → PERMANENT MUTE)
 * 1. NaN/Inf detection and replacement
 * 2. DC offset detection (>0.5 for 500ms → PERMANENT MUTE)
 * 3. DC offset blocking (10Hz high-pass for speaker protection)
 * 4. Soft-knee limiting with fast attack/slow release
 * 5. Sustained loudness detection (feedback runaway protection)
 * 6. Slew rate limiting (ultrasonic protection)
 * 7. Hard clipping as final safety net
 *
 * CRITICAL: Once permanently muted, output stays silent until manually reset.
 * This is intentional - dangerous audio conditions should require human review.
 */
class SafetyLimiter {
public:
  SafetyLimiter() = default;

  void prepare(double sampleRate) {
    sampleRate_ = sampleRate;

    // Fast attack (0.1ms), slow release (50ms) for limiter
    attackCoeff_ = std::exp(-1.0 / (0.0001 * sampleRate));
    releaseCoeff_ = std::exp(-1.0 / (0.050 * sampleRate));

    // DC blocker coefficient (10Hz high-pass for speaker protection)
    dcBlockCoeff_ = 1.0 - (2.0 * 3.14159265359 * 10.0 / sampleRate);

    // Sustained peak detection (100ms window for +6dBFS trigger)
    sustainedPeakCoeff_ = std::exp(-1.0 / (0.1 * sampleRate));

    // DC offset detection (500ms window)
    dcDetectCoeff_ = std::exp(-1.0 / (0.5 * sampleRate));

    // Sustained loudness detection (500ms window)
    sustainedCoeff_ = std::exp(-1.0 / (0.5 * sampleRate));

    // Calculate sample thresholds
    sustainedPeakThresholdSamples_ =
        static_cast<int>(0.1 * sampleRate);                         // 100ms
    dcOffsetThresholdSamples_ = static_cast<int>(0.5 * sampleRate); // 500ms

    reset();
  }

  void reset() {
    envelope_ = 0.0f;
    dcBlockStateL_ = 0.0f;
    dcBlockStateR_ = 0.0f;
    dcBlockPrevL_ = 0.0f;
    dcBlockPrevR_ = 0.0f;
    sustainedLevel_ = 0.0f;
    sustainedPeakLevel_ = 0.0f;
    dcOffsetLevel_ = 0.0f;
    sustainedPeakCounter_ = 0;
    dcOffsetCounter_ = 0;
    prevOutputL_ = 0.0f;
    prevOutputR_ = 0.0f;
    // NOTE: permanentlyMuted_ is NOT reset here - requires explicit unlock
  }

  /**
   * @brief Process a stereo buffer with full safety chain
   */
  void process(float* left, float* right, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
      // === PERMANENT MUTE CHECK (highest priority) ===
      if (permanentlyMuted_) {
        left[i] = 0.0f;
        right[i] = 0.0f;
        continue;
      }

      // === Stage 0: Sustained Peak Detection (+6dBFS for 100ms) ===
      float instantPeak = std::max(std::abs(left[i]), std::abs(right[i]));

      // Track sustained peak level (100ms window)
      sustainedPeakLevel_ =
          static_cast<float>(sustainedPeakCoeff_) * sustainedPeakLevel_ +
          (1.0f - static_cast<float>(sustainedPeakCoeff_)) * instantPeak;

      // +6dBFS = 2.0 linear
      if (sustainedPeakLevel_ > dangerPeakThreshold_) {
        ++sustainedPeakCounter_;
        if (sustainedPeakCounter_ >= sustainedPeakThresholdSamples_) {
          triggerPermanentMute(MuteReason::SustainedPeak);
          ++dangerEventCount_;
        }
      } else {
        sustainedPeakCounter_ = 0;
      }

      // === Stage 1: NaN/Inf Protection ===
      if (!std::isfinite(left[i])) {
        left[i] = 0.0f;
        triggerPermanentMute(MuteReason::NaNInf);
      }
      if (!std::isfinite(right[i])) {
        right[i] = 0.0f;
        triggerPermanentMute(MuteReason::NaNInf);
      }

      // === Stage 2: DC Offset Detection (>0.5 for 500ms) ===
      // Track DC offset level (500ms window) - uses raw input before DC
      // blocking
      float dcLevel = std::abs(0.5f * (left[i] + right[i])); // Mono DC check
      dcOffsetLevel_ = static_cast<float>(dcDetectCoeff_) * dcOffsetLevel_ +
                       (1.0f - static_cast<float>(dcDetectCoeff_)) * dcLevel;

      if (dcOffsetLevel_ > dcOffsetThreshold_) {
        ++dcOffsetCounter_;
        if (dcOffsetCounter_ >= dcOffsetThresholdSamples_) {
          triggerPermanentMute(MuteReason::DCOffset);
          ++dangerEventCount_;
        }
      } else {
        dcOffsetCounter_ = 0;
      }

      // === Stage 3: DC Offset Blocking (10Hz HPF) ===
      float dcFreeL = left[i] - dcBlockPrevL_ +
                      static_cast<float>(dcBlockCoeff_) * dcBlockStateL_;
      float dcFreeR = right[i] - dcBlockPrevR_ +
                      static_cast<float>(dcBlockCoeff_) * dcBlockStateR_;
      dcBlockPrevL_ = left[i];
      dcBlockPrevR_ = right[i];
      dcBlockStateL_ = dcFreeL;
      dcBlockStateR_ = dcFreeR;
      left[i] = dcFreeL;
      right[i] = dcFreeR;

      // === Stage 4: Soft-Knee Limiting ===
      float peak = std::max(std::abs(left[i]), std::abs(right[i]));

      // Envelope follower
      if (peak > envelope_)
        envelope_ = static_cast<float>(attackCoeff_) * envelope_ +
                    (1.0f - static_cast<float>(attackCoeff_)) * peak;
      else
        envelope_ = static_cast<float>(releaseCoeff_) * envelope_ +
                    (1.0f - static_cast<float>(releaseCoeff_)) * peak;

      // Calculate gain reduction
      float gain = 1.0f;
      if (envelope_ > threshold_) {
        float overshoot = envelope_ / threshold_;
        gain = 1.0f / overshoot;
      }

      // Apply limiting
      left[i] *= gain;
      right[i] *= gain;

      // === Stage 5: Sustained Loudness Detection (feedback runaway) ===
      float postPeak = std::max(std::abs(left[i]), std::abs(right[i]));
      sustainedLevel_ = static_cast<float>(sustainedCoeff_) * sustainedLevel_ +
                        (1.0f - static_cast<float>(sustainedCoeff_)) * postPeak;

      if (sustainedLevel_ > sustainedThreshold_) {
        float sustainGain = sustainedThreshold_ / sustainedLevel_;
        left[i] *= sustainGain;
        right[i] *= sustainGain;
      }

      // === Stage 6: Slew Rate Limiting (Ultrasonic Protection) ===
      float slewL = left[i] - prevOutputL_;
      float slewR = right[i] - prevOutputR_;

      if (std::abs(slewL) > maxSlewRate_) {
        left[i] = prevOutputL_ + (slewL > 0 ? maxSlewRate_ : -maxSlewRate_);
      }
      if (std::abs(slewR) > maxSlewRate_) {
        right[i] = prevOutputR_ + (slewR > 0 ? maxSlewRate_ : -maxSlewRate_);
      }

      prevOutputL_ = left[i];
      prevOutputR_ = right[i];

      // === Stage 7: Hard Clip (Final Safety Net) ===
      left[i] = std::clamp(left[i], -1.0f, 1.0f);
      right[i] = std::clamp(right[i], -1.0f, 1.0f);
    }
  }

  void setThreshold(float thresholdDb) {
    threshold_ = std::pow(10.0f, thresholdDb / 20.0f);
  }

  void setSustainedThreshold(float level) {
    sustainedThreshold_ = std::clamp(level, 0.1f, 0.95f);
  }

  // ==================== PERMANENT MUTE CONTROL ====================

  enum class MuteReason {
    None,
    SustainedPeak, // +6dBFS for 100ms
    DCOffset,      // DC > 0.5 for 500ms
    NaNInf         // NaN or Inf detected
  };

  /**
   * @brief Check if output is permanently muted
   */
  bool isPermanentlyMuted() const { return permanentlyMuted_; }

  /**
   * @brief Get the reason for permanent mute
   */
  MuteReason getMuteReason() const { return muteReason_; }

  /**
   * @brief Manually unlock the permanent mute (requires user action)
   * Call this from UI when user acknowledges the safety event
   */
  void unlockPermanentMute() {
    permanentlyMuted_ = false;
    muteReason_ = MuteReason::None;
    sustainedPeakCounter_ = 0;
    dcOffsetCounter_ = 0;
    sustainedPeakLevel_ = 0.0f;
    dcOffsetLevel_ = 0.0f;
  }

  /**
   * @brief Get current envelope level (for metering)
   */
  float getEnvelopeLevel() const { return envelope_; }

  /**
   * @brief Get count of danger events (for diagnostics)
   */
  int getDangerEventCount() const { return dangerEventCount_; }

  void resetDangerEventCount() { dangerEventCount_ = 0; }

private:
  void triggerPermanentMute(MuteReason reason) {
    permanentlyMuted_ = true;
    muteReason_ = reason;
  }

  double sampleRate_ = 44100.0;

  // Limiter coefficients
  double attackCoeff_ = 0.0;
  double releaseCoeff_ = 0.0;
  float envelope_ = 0.0f;
  float threshold_ = 0.9f; // ~-1dB default

  // DC blocker state (10Hz HPF)
  double dcBlockCoeff_ = 0.999;
  float dcBlockStateL_ = 0.0f;
  float dcBlockStateR_ = 0.0f;
  float dcBlockPrevL_ = 0.0f;
  float dcBlockPrevR_ = 0.0f;

  // Sustained loudness detection
  double sustainedCoeff_ = 0.0;
  float sustainedLevel_ = 0.0f;
  float sustainedThreshold_ = 0.7f; // ~-3dB

  // Sustained peak detection (+6dBFS for 100ms)
  double sustainedPeakCoeff_ = 0.0;
  float sustainedPeakLevel_ = 0.0f;
  float dangerPeakThreshold_ = 2.0f; // +6dBFS
  int sustainedPeakCounter_ = 0;
  int sustainedPeakThresholdSamples_ = 4410; // 100ms @ 44.1kHz

  // DC offset detection (>0.5 for 500ms)
  double dcDetectCoeff_ = 0.0;
  float dcOffsetLevel_ = 0.0f;
  float dcOffsetThreshold_ = 0.5f;
  int dcOffsetCounter_ = 0;
  int dcOffsetThresholdSamples_ = 22050; // 500ms @ 44.1kHz

  // Permanent mute state
  bool permanentlyMuted_ = false;
  MuteReason muteReason_ = MuteReason::None;
  int dangerEventCount_ = 0;

  // Slew rate limiting
  float maxSlewRate_ = 0.5f;
  float prevOutputL_ = 0.0f;
  float prevOutputR_ = 0.0f;
};

} // namespace uds
