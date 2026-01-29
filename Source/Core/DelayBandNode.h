#pragma once

#include "AttackEnvelope.h"
#include "DelayAlgorithm.h"
#include "FilterSection.h"
#include "GenerativeModulator.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include <algorithm>
#include <cmath>
#include <memory>


namespace uds {

/**
 * @brief Parameters for a single delay band
 */
struct DelayBandParams {
  float delayTimeMs = 250.0f;
  float feedback = 0.3f;
  float level = 1.0f;
  float pan = 0.0f;
  float hiCutHz = 12000.0f;
  float loCutHz = 80.0f;
  float lfoRateHz = 1.0f;
  float lfoDepth = 0.0f;
  float attackTimeMs = 0.0f; // 0 = instant (no swell), >0 = volume swell effect

  ModulationType modulationType = ModulationType::Sine;
  // float masterLfoMod = 0.0f; // REMOVED: Managed externally via buffer
  bool phaseInvert = false;
  bool pingPong = false;
  bool enabled = true;
  DelayAlgorithmType algorithm = DelayAlgorithmType::Digital;
};

/**
 * @brief Single delay band with selectable algorithm, filters, and LFO
 * modulation
 *
 * Features:
 * - Algorithm selection (Digital, Analog, Tape, Lo-Fi)
 * - Hi-cut and Lo-cut filters in feedback path
 * - LFO modulation of delay time (chorus/flutter effects)
 * - Phase inversion option
 */
class DelayBandNode {
public:
  DelayBandNode() {
    // Default to digital algorithm
    algorithm_ = createDelayAlgorithm(DelayAlgorithmType::Digital);
  }

  void prepare(double sampleRate, size_t /*maxBlockSize*/) {
    sampleRate_ = sampleRate;

    // Max delay = 700ms + modulation headroom
    const int maxDelaySamples = static_cast<int>(0.75 * sampleRate) + 1;

    // Simple circular buffer
    bufferL_.resize(static_cast<size_t>(maxDelaySamples), 0.0f);
    bufferR_.resize(static_cast<size_t>(maxDelaySamples), 0.0f);
    writePos_ = 0;

    // Prepare algorithm
    if (algorithm_) {
      algorithm_->prepare(sampleRate);
    }

    // Prepare filter section
    filterSection_.prepare(sampleRate);

    // Prepare attack envelope for volume swell
    attackEnvelope_.prepare(sampleRate);

    prepared_ = true;
  }

  void reset() {
    std::fill(bufferL_.begin(), bufferL_.end(), 0.0f);
    std::fill(bufferR_.begin(), bufferR_.end(), 0.0f);
    writePos_ = 0;
    feedbackL_ = 0.0f;
    feedbackR_ = 0.0f;

    if (algorithm_) {
      algorithm_->reset();
    }

    filterSection_.reset();
    attackEnvelope_.reset();
  }

  void setParams(const DelayBandParams& params) {
    // Check if algorithm type changed
    if (params.algorithm != params_.algorithm) {
      algorithm_ = createDelayAlgorithm(params.algorithm);
      if (algorithm_ && prepared_) {
        algorithm_->prepare(sampleRate_);
      }
    }

    // Update filter frequencies
    filterSection_.setHiCutFrequency(params.hiCutHz);
    filterSection_.setLoCutFrequency(params.loCutHz);

    // Update LFO parameters: Handled by ModulationEngine now
    // lfo_.setRate(params.lfoRateHz);
    // lfo_.setDepth(params.lfoDepth);
    // lfo_.setWaveform(params.lfoWaveform);

    // Update attack envelope for volume swell
    attackEnvelope_.setAttackTimeMs(params.attackTimeMs);

    params_ = params;
  }

  /**
   * @brief Get current algorithm type
   */
  DelayAlgorithmType getAlgorithmType() const { return params_.algorithm; }

  /**
   * @brief Get algorithm display name
   */
  const char* getAlgorithmName() const {
    return algorithm_ ? algorithm_->getName() : "Unknown";
  }

  void process(juce::AudioBuffer<float>& buffer, float wetMix,
               const float* modSignal = nullptr,
               const float* masterModSignal = nullptr) {
    if (!params_.enabled || !prepared_ || bufferL_.empty())
      return;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel =
        numChannels > 1 ? buffer.getWritePointer(1) : leftChannel;

    const int bufferSize = static_cast<int>(bufferL_.size());

    for (int i = 0; i < numSamples; ++i) {
      // Get LFO-modulated delay time
      float modulatedTimeMs = params_.delayTimeMs;

      // Calculate total modulation (Local + Master)
      float totalMod = 0.0f;

      if (modSignal) {
        totalMod += modSignal[i];
      }

      if (masterModSignal) {
        totalMod += masterModSignal[i];
      }

      // Apply modulation (scale by 25ms for audible chorus/vibrato effect)
      if (totalMod != 0.0f) {
        modulatedTimeMs += (totalMod * 25.0f); // ±25ms modulation range
        modulatedTimeMs = std::max(1.0f, modulatedTimeMs); // Ensure positive
      }

      // Calculate delay in samples (with interpolation for smooth modulation)
      float delaySamplesF =
          (modulatedTimeMs / 1000.0f) * static_cast<float>(sampleRate_);
      int delaySamples = static_cast<int>(delaySamplesF);
      float frac = delaySamplesF - static_cast<float>(delaySamples);

      // Calculate read positions for cubic Hermite interpolation (4 points)
      int readPos0 = writePos_ - delaySamples + 1; // One sample ahead
      int readPos1 = writePos_ - delaySamples;
      int readPos2 = readPos1 - 1;
      int readPos3 = readPos1 - 2;

      // Wrap positions
      if (readPos0 < 0)
        readPos0 += bufferSize;
      if (readPos1 < 0)
        readPos1 += bufferSize;
      if (readPos2 < 0)
        readPos2 += bufferSize;
      if (readPos3 < 0)
        readPos3 += bufferSize;
      if (readPos0 >= bufferSize)
        readPos0 -= bufferSize;

      // Get 4 sample points for cubic interpolation
      float y0L = bufferL_[static_cast<size_t>(readPos0)];
      float y1L = bufferL_[static_cast<size_t>(readPos1)];
      float y2L = bufferL_[static_cast<size_t>(readPos2)];
      float y3L = bufferL_[static_cast<size_t>(readPos3)];

      float y0R = bufferR_[static_cast<size_t>(readPos0)];
      float y1R = bufferR_[static_cast<size_t>(readPos1)];
      float y2R = bufferR_[static_cast<size_t>(readPos2)];
      float y3R = bufferR_[static_cast<size_t>(readPos3)];

      // Cubic Hermite interpolation coefficients
      float c0L = y1L;
      float c1L = 0.5f * (y2L - y0L);
      float c2L = y0L - 2.5f * y1L + 2.0f * y2L - 0.5f * y3L;
      float c3L = 0.5f * (y3L - y0L) + 1.5f * (y1L - y2L);

      float c0R = y1R;
      float c1R = 0.5f * (y2R - y0R);
      float c2R = y0R - 2.5f * y1R + 2.0f * y2R - 0.5f * y3R;
      float c3R = 0.5f * (y3R - y0R) + 1.5f * (y1R - y2R);

      // Evaluate cubic polynomial: y = c0 + c1*t + c2*t^2 + c3*t^3
      float delayedL = ((c3L * frac + c2L) * frac + c1L) * frac + c0L;
      float delayedR = ((c3R * frac + c2R) * frac + c1R) * frac + c0R;

      // Get input
      float inputL = leftChannel[i];
      float inputR = rightChannel[i];

      // Apply algorithm to feedback signal (this is what creates the character)
      float feedbackL = delayedL * params_.feedback;
      float feedbackR = delayedR * params_.feedback;

      if (algorithm_) {
        feedbackL = algorithm_->processSample(feedbackL);
        feedbackR = algorithm_->processSample(feedbackR);
      }

      // Apply filters to feedback path
      filterSection_.processSample(feedbackL, feedbackR);

      // Write to buffer (input + processed feedback)
      // For ping-pong: cross-feed feedback to create L/R bounce
      if (params_.pingPong) {
        // Ping-pong: L feedback goes to R buffer, R feedback goes to L buffer
        bufferL_[static_cast<size_t>(writePos_)] = inputL + feedbackR;
        bufferR_[static_cast<size_t>(writePos_)] = inputR + feedbackL;
      } else {
        bufferL_[static_cast<size_t>(writePos_)] = inputL + feedbackL;
        bufferR_[static_cast<size_t>(writePos_)] = inputR + feedbackR;
      }

      // Advance write position
      writePos_ = (writePos_ + 1) % bufferSize;

      // Apply level and pan
      float panL = std::cos((params_.pan + 1.0f) * 0.25f * 3.14159f);
      float panR = std::sin((params_.pan + 1.0f) * 0.25f * 3.14159f);

      float wetL = delayedL * params_.level * panL;
      float wetR = delayedR * params_.level * panR;

      // Apply phase inversion if enabled
      if (params_.phaseInvert) {
        wetL = -wetL;
        wetR = -wetR;
      }

      // Apply attack envelope for volume swell effect
      // Uses input level to trigger, applies gain to wet signal
      if (params_.attackTimeMs > 0.0f) {
        attackEnvelope_.processBlock(inputL, inputR, wetL, wetR);
      }

      // Output: dry + wet
      leftChannel[i] = inputL + wetL * wetMix;
      if (numChannels > 1)
        rightChannel[i] = inputR + wetR * wetMix;
    }
  }

private:
  DelayBandParams params_;
  std::unique_ptr<DelayAlgorithm> algorithm_;
  double sampleRate_ = 44100.0;
  bool prepared_ = false;

  // Simple circular buffer
  std::vector<float> bufferL_;
  std::vector<float> bufferR_;
  int writePos_ = 0;

  float feedbackL_ = 0.0f;
  float feedbackR_ = 0.0f;

  // Filter section for feedback path
  FilterSection filterSection_;

  // Attack envelope for volume swell effects
  AttackEnvelope attackEnvelope_;

  // LFO now external
  // LFOModulator lfo_;
};

} // namespace uds
