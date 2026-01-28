#pragma once

#include <juce_core/juce_core.h>

#include <algorithm>
#include <cmath>


namespace uds {

enum class ModulationType {
  Sine = 0,
  Triangle,
  Saw,
  Square,
  Brownian, // Random Walk (Tape Drift)
  Lorenz    // Chaotic Attractor (Orbit)
};

/**
 * @brief Unified Modulator for standard LFOs and Generative signals
 */
class GenerativeModulator {
public:
  GenerativeModulator() : rng_(juce::Time::currentTimeMillis()) {}

  void prepare(double sampleRate) { sampleRate_ = sampleRate; }

  void reset() {
    phase_ = 0.0f;
    brownianValue_ = 0.0f;
    // Lorenz initial state (must not be 0,0,0)
    lorenzX_ = 0.1f;
    lorenzY_ = 0.0f;
    lorenzZ_ = 0.0f;
  }

  void setParams(ModulationType type, float rateHz, float depth) {
    type_ = type;
    rateHz_ = std::clamp(rateHz, 0.01f, 20.0f);
    depth_ = std::clamp(depth, 0.0f, 1.0f);
  }

  /**
   * @brief Advance state and return current value (-1.0 to 1.0) * depth
   */
  float tick() {
    float rawValue = 0.0f;

    // 1. Calculate phase increment
    float phaseInc = rateHz_ / static_cast<float>(sampleRate_);

    switch (type_) {
    case ModulationType::Sine:
      rawValue = std::sin(phase_ * 2.0f * juce::MathConstants<float>::pi);
      advancePhase(phaseInc);
      break;

    case ModulationType::Triangle:
      rawValue =
          (phase_ < 0.5f) ? (4.0f * phase_ - 1.0f) : (3.0f - 4.0f * phase_);
      advancePhase(phaseInc);
      break;

    case ModulationType::Saw:
      rawValue = 2.0f * phase_ - 1.0f;
      advancePhase(phaseInc);
      break;

    case ModulationType::Square:
      rawValue = (phase_ < 0.5f) ? 1.0f : -1.0f;
      advancePhase(phaseInc);
      break;

    case ModulationType::Brownian:
      // Random Walk with smooth interpolation
      // Updates target value at rateHz, smoothly approaches it
      {
        // Advance phase for timing the random walk steps
        float prevPhase = phase_;
        advancePhase(phaseInc);

        // When phase wraps, pick a new random step
        if (phase_ < prevPhase) {
          // New random target: current + random offset
          float step = (rng_.nextFloat() - 0.5f) * 0.4f; // ±0.2 step
          brownianTarget_ += step;

          // Tether towards center to prevent drift
          brownianTarget_ *= 0.92f;

          // Hard clamp
          brownianTarget_ = std::clamp(brownianTarget_, -1.0f, 1.0f);
        }

        // Smooth interpolation towards target (slew)
        float slewRate = 0.001f; // Smooth transition
        brownianValue_ += (brownianTarget_ - brownianValue_) * slewRate;

        rawValue = brownianValue_;
      }
      break;

    case ModulationType::Lorenz:
      // Lorenz Attractor with smoothed output
      // The chaotic system runs at its internal rate, output is smoothed
      {
        // Standard Lorenz constants
        const float sigma = 10.0f;
        const float rho = 28.0f;
        const float beta = 8.0f / 3.0f;

        // Run Lorenz at a fixed internal rate for stability
        // Scale iterations based on rateHz (more rate = faster chaos)
        float dt = 0.01f; // Fixed dt for stability
        int iterations = std::max(1, static_cast<int>(rateHz_ * 0.5f));

        for (int i = 0; i < iterations; ++i) {
          float dx = sigma * (lorenzY_ - lorenzX_);
          float dy = lorenzX_ * (rho - lorenzZ_) - lorenzY_;
          float dz = lorenzX_ * lorenzY_ - beta * lorenzZ_;

          lorenzX_ += dx * dt;
          lorenzY_ += dy * dt;
          lorenzZ_ += dz * dt;
        }

        // Raw Lorenz output
        float lorenzRaw = std::clamp(lorenzX_ / 20.0f, -1.0f, 1.0f);

        // Smooth the output to prevent harsh artifacts
        float slewRate = 0.0005f + (rateHz_ * 0.0001f);
        lorenzSmoothed_ += (lorenzRaw - lorenzSmoothed_) * slewRate;

        rawValue = lorenzSmoothed_;
      }
      break;
    }

    return rawValue * depth_;
  }

private:
  void advancePhase(float inc) {
    phase_ += inc;
    if (phase_ >= 1.0f)
      phase_ -= 1.0f;
  }

  double sampleRate_ = 44100.0;
  ModulationType type_ = ModulationType::Sine;

  float rateHz_ = 1.0f;
  float depth_ = 0.0f;
  float phase_ = 0.0f;

  // Generative States
  juce::Random rng_;
  float brownianValue_ = 0.0f;
  float brownianTarget_ = 0.0f; // Target value for smooth interpolation

  // Lorenz State
  float lorenzX_ = 0.1f;
  float lorenzY_ = 0.0f;
  float lorenzZ_ = 0.0f;
  float lorenzSmoothed_ = 0.0f; // Smoothed output for artifacts prevention
};

} // namespace uds
