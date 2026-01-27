#pragma once

#include "../UI/NodeVisual.h"
#include "DelayBandNode.h"
#include "RoutingGraph.h"
#include "SafetyLimiter.h"
#include <array>
#include <juce_audio_basics/juce_audio_basics.h>
#include <memory>
#include <unordered_map>
#include <vector>

namespace uds {

/**
 * @brief Container for 8 delay bands with graph-based routing.
 *
 * Processes bands in topological order based on RoutingGraph connections.
 * Supports series, parallel, and complex feedback routing.
 */
class DelayMatrix {
public:
  static constexpr int NUM_BANDS = 8;

  DelayMatrix() = default;

  void prepare(double sampleRate, size_t maxBlockSize) {
    sampleRate_ = sampleRate;
    maxBlockSize_ = maxBlockSize;

    // Create bands
    bands_.clear();
    for (int i = 0; i < NUM_BANDS; ++i) {
      bands_.push_back(std::make_unique<DelayBandNode>());
      bands_.back()->prepare(sampleRate, maxBlockSize);
    }

    // Prepare safety limiter
    limiter_.prepare(sampleRate);

    // Allocate node buffers (Input=0, Bands=1-8, Output=9)
    for (int i = 0; i < kNumNodes; ++i) {
      nodeBuffers_[i].setSize(2, static_cast<int>(maxBlockSize));
    }

    prepared_ = true;
  }

  void reset() {
    for (auto &band : bands_) {
      if (band)
        band->reset();
    }
    limiter_.reset();
  }

  void setBandParams(int bandIndex, const DelayBandParams &params) {
    if (bandIndex >= 0 && bandIndex < static_cast<int>(bands_.size())) {
      if (bands_[static_cast<size_t>(bandIndex)])
        bands_[static_cast<size_t>(bandIndex)]->setParams(params);
    }
  }

  /**
   * @brief Get the routing graph for external manipulation
   */
  RoutingGraph &getRoutingGraph() { return routingGraph_; }
  const RoutingGraph &getRoutingGraph() const { return routingGraph_; }

  /**
   * @brief Process audio through the delay matrix using routing graph
   */
  void process(juce::AudioBuffer<float> &buffer, float globalMix) {
    if (!prepared_ || bands_.empty())
      return;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = std::min(2, buffer.getNumChannels());
    if (numSamples == 0 || numChannels == 0)
      return;

    // Clear all node buffers
    for (auto &[id, buf] : nodeBuffers_) {
      buf.clear();
    }

    // Store dry signal
    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer);

    // Copy input to Input node buffer
    for (int ch = 0; ch < numChannels; ++ch) {
      nodeBuffers_[0].copyFrom(ch, 0, buffer, ch, 0, numSamples);
    }

    // Process nodes in topological order
    const auto &order = routingGraph_.getProcessingOrder();

    for (int nodeId : order) {
      if (nodeId == static_cast<int>(NodeId::Input)) {
        // Input node: already filled with input buffer
        continue;
      }

      if (nodeId == static_cast<int>(NodeId::Output)) {
        // Output node: sum all inputs into output
        auto inputs = routingGraph_.getInputsFor(nodeId);
        for (int srcId : inputs) {
          for (int ch = 0; ch < numChannels; ++ch) {
            nodeBuffers_[nodeId].addFrom(ch, 0, nodeBuffers_[srcId], ch, 0,
                                         numSamples);
          }
        }
        continue;
      }

      // Band node (1-8): sum inputs, process through delay, store output
      int bandIndex = nodeId - 1;
      if (bandIndex < 0 || bandIndex >= static_cast<int>(bands_.size()))
        continue;

      auto &band = bands_[static_cast<size_t>(bandIndex)];
      if (!band)
        continue;

      // Sum all inputs for this band
      auto inputs = routingGraph_.getInputsFor(nodeId);
      juce::AudioBuffer<float> bandInput(numChannels, numSamples);
      bandInput.clear();

      for (int srcId : inputs) {
        for (int ch = 0; ch < numChannels; ++ch) {
          bandInput.addFrom(ch, 0, nodeBuffers_[srcId], ch, 0, numSamples);
        }
      }

      // Process through delay band (wet only, no dry mix)
      band->process(bandInput, 1.0f);

      // Store in node buffer
      for (int ch = 0; ch < numChannels; ++ch) {
        nodeBuffers_[nodeId].copyFrom(ch, 0, bandInput, ch, 0, numSamples);
      }
    }

    // Get output node result (this is the wet signal)
    auto &wetBuffer = nodeBuffers_[static_cast<int>(NodeId::Output)];

    // Apply safety limiter to wet signal
    if (numChannels >= 2) {
      limiter_.process(wetBuffer.getWritePointer(0),
                       wetBuffer.getWritePointer(1), numSamples);
    }

    // Final mix: output = dry + wet * globalMix
    for (int ch = 0; ch < numChannels; ++ch) {
      const float *dry = dryBuffer.getReadPointer(ch);
      const float *wet = wetBuffer.getReadPointer(ch);
      float *out = buffer.getWritePointer(ch);

      for (int s = 0; s < numSamples; ++s) {
        out[s] = dry[s] + wet[s] * globalMix;
      }
    }
  }

  /**
   * @brief Process audio using an external routing graph
   *
   * This allows the processor to own the routing graph while DelayMatrix
   * handles the DSP.
   */
  void processWithRouting(juce::AudioBuffer<float> &buffer, float globalMix,
                          const RoutingGraph &externalRouting) {
    if (!prepared_ || bands_.empty())
      return;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = std::min(2, buffer.getNumChannels());
    if (numSamples == 0 || numChannels == 0)
      return;

    // Clear all node buffers
    for (auto &[id, buf] : nodeBuffers_) {
      buf.clear();
    }

    // Store dry signal
    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer);

    // Copy input to Input node buffer
    for (int ch = 0; ch < numChannels; ++ch) {
      nodeBuffers_[0].copyFrom(ch, 0, buffer, ch, 0, numSamples);
    }

    // Process nodes in topological order from external routing
    const auto &order = externalRouting.getProcessingOrder();

    for (int nodeId : order) {
      if (nodeId == static_cast<int>(NodeId::Input)) {
        continue;
      }

      if (nodeId == static_cast<int>(NodeId::Output)) {
        auto inputs = externalRouting.getInputsFor(nodeId);
        for (int srcId : inputs) {
          for (int ch = 0; ch < numChannels; ++ch) {
            nodeBuffers_[nodeId].addFrom(ch, 0, nodeBuffers_[srcId], ch, 0,
                                         numSamples);
          }
        }
        continue;
      }

      // Band node (1-8)
      int bandIndex = nodeId - 1;
      if (bandIndex < 0 || bandIndex >= static_cast<int>(bands_.size()))
        continue;

      auto &band = bands_[static_cast<size_t>(bandIndex)];
      if (!band)
        continue;

      // Sum all inputs for this band
      auto inputs = externalRouting.getInputsFor(nodeId);
      juce::AudioBuffer<float> bandInput(numChannels, numSamples);
      bandInput.clear();

      for (int srcId : inputs) {
        for (int ch = 0; ch < numChannels; ++ch) {
          bandInput.addFrom(ch, 0, nodeBuffers_[srcId], ch, 0, numSamples);
        }
      }

      // Process through delay band
      band->process(bandInput, 1.0f);

      // Calculate peak level for activity indicator
      float peak = 0.0f;
      for (int ch = 0; ch < numChannels; ++ch) {
        auto range = juce::FloatVectorOperations::findMinAndMax(
            bandInput.getReadPointer(ch), numSamples);
        peak = std::max(peak, std::max(std::abs(range.getStart()),
                                       std::abs(range.getEnd())));
      }
      bandLevels_[static_cast<size_t>(bandIndex)] = peak;

      // Store in node buffer
      for (int ch = 0; ch < numChannels; ++ch) {
        nodeBuffers_[nodeId].copyFrom(ch, 0, bandInput, ch, 0, numSamples);
      }
    }

    // Get output node result
    auto &wetBuffer = nodeBuffers_[static_cast<int>(NodeId::Output)];

    // Apply safety limiter
    if (numChannels >= 2) {
      limiter_.process(wetBuffer.getWritePointer(0),
                       wetBuffer.getWritePointer(1), numSamples);
    }

    // Final mix
    for (int ch = 0; ch < numChannels; ++ch) {
      const float *dry = dryBuffer.getReadPointer(ch);
      const float *wet = wetBuffer.getReadPointer(ch);
      float *out = buffer.getWritePointer(ch);

      for (int s = 0; s < numSamples; ++s) {
        out[s] = dry[s] + wet[s] * globalMix;
      }
    }
  }

  // Serialization for state save/restore
  juce::String getRoutingState() const {
    // TODO: Serialize routing graph connections
    return "{}";
  }

  void setRoutingState(const juce::String & /*state*/) {
    // TODO: Deserialize routing graph connections
  }

  // Get output level for a band (for activity indicators)
  float getBandLevel(int bandIndex) const {
    if (bandIndex >= 0 && bandIndex < 8)
      return bandLevels_[static_cast<size_t>(bandIndex)];
    return 0.0f;
  }

private:
  std::vector<std::unique_ptr<DelayBandNode>> bands_;
  RoutingGraph routingGraph_;
  SafetyLimiter limiter_;
  std::unordered_map<int, juce::AudioBuffer<float>> nodeBuffers_;

  double sampleRate_ = 44100.0;
  size_t maxBlockSize_ = 512;
  bool prepared_ = false;
  std::array<float, 8> bandLevels_{
      {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}};
};

} // namespace uds
