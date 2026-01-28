#pragma once

#include "GenerativeModulator.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <array>


namespace uds {

/**
 * @brief Centralized Engine for generating modulation signals
 *
 * Manages 8 Band Modulators + 1 Master Modulator.
 * Generates modulation buffers block-by-block for efficient processing.
 */
class ModulationEngine {
public:
  ModulationEngine() = default;

  void prepare(double sampleRate, size_t maxBlockSize) {
    // Output buffers
    // 8 Channels for bands
    localModBuffer_.setSize(8, static_cast<int>(maxBlockSize));
    // 1 Channel for master
    masterModBuffer_.setSize(1, static_cast<int>(maxBlockSize));

    // Prepare modulators
    for (auto& mod : bandModulators_) {
      mod.prepare(sampleRate);
    }
    masterModulator_.prepare(sampleRate);
  }

  void reset() {
    for (auto& mod : bandModulators_) {
      mod.reset();
    }
    masterModulator_.reset();

    localModBuffer_.clear();
    masterModBuffer_.clear();
  }

  // Parameter setters
  void setBandParams(int bandIndex, ModulationType type, float rate,
                     float depth) {
    if (bandIndex >= 0 && bandIndex < 8) {
      bandModulators_[static_cast<size_t>(bandIndex)].setParams(type, rate,
                                                                depth);
    }
  }

  void setMasterParams(ModulationType type, float rate, float depth) {
    masterModulator_.setParams(type, rate, depth);
  }

  /**
   * @brief Generate modulation signals for the current block
   */
  void process(int numSamples) {
    if (numSamples <= 0)
      return;

    // 1. Process Master Modulator
    auto* masterWrite = masterModBuffer_.getWritePointer(0);
    for (int i = 0; i < numSamples; ++i) {
      masterWrite[i] = masterModulator_.tick();
    }

    // 2. Process Band Modulators
    for (int ch = 0; ch < 8; ++ch) {
      auto* bandWrite = localModBuffer_.getWritePointer(ch);
      auto& modulator = bandModulators_[static_cast<size_t>(ch)];

      for (int i = 0; i < numSamples; ++i) {
        bandWrite[i] = modulator.tick();
      }
    }
  }

  // Accessors for DelayMatrix / BandNodes
  const juce::AudioBuffer<float>& getLocalBuffer() const {
    return localModBuffer_;
  }
  const juce::AudioBuffer<float>& getMasterBuffer() const {
    return masterModBuffer_;
  }

private:
  std::array<GenerativeModulator, 8> bandModulators_;
  GenerativeModulator masterModulator_;

  // Block Processing Buffers
  juce::AudioBuffer<float> localModBuffer_;

  // TYPO FIX: Member variable was declared as masterModBuffer_ but used as
  // masterModulatorBuffer_ in process() I will fix this in the code below by
  // using masterModBuffer_ consistently.
  juce::AudioBuffer<float> masterModBuffer_;
};

} // namespace uds
