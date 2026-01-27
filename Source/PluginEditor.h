#pragma once

#include "Core/PresetManager.h"
#include "PluginProcessor.h"
#include "UI/MainComponent.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>

/**
 * @brief UDS Plugin Editor
 */
class UDSAudioProcessorEditor : public juce::AudioProcessorEditor,
                                private juce::Timer {
public:
  explicit UDSAudioProcessorEditor(UDSAudioProcessor &p)
      : AudioProcessorEditor(&p), processorRef_(p),
        presetManager_(std::make_unique<uds::PresetManager>(p)) {
    mainComponent_ = std::make_unique<uds::MainComponent>(
        p.getParameters(), p.getRoutingGraph(), *presetManager_);
    addAndMakeVisible(*mainComponent_);

    setSize(1000, 500);
    setResizable(true, true);
    setResizeLimits(800, 400, 1600, 900);

    // Start timer to sync metronome BPM to processor
    startTimerHz(10);
  }

  ~UDSAudioProcessorEditor() override { stopTimer(); }

  void paint(juce::Graphics &g) override {
    g.fillAll(juce::Colour(0xff1a1a1a));
  }

  void resized() override {
    if (mainComponent_)
      mainComponent_->setBounds(getLocalBounds());
  }

private:
  void timerCallback() override {
    if (mainComponent_) {
      // Sync metronome BPM to processor for standalone tempo sync
      double metronomeBpm = mainComponent_->getMetronome().getBpm();
      processorRef_.setInternalBpm(metronomeBpm);

      // Poll band levels for activity indicators
      std::array<float, 8> levels;
      for (int i = 0; i < 8; ++i) {
        levels[static_cast<size_t>(i)] = processorRef_.getBandLevel(i);
      }
      mainComponent_->updateBandLevels(levels);
    }
  }

  UDSAudioProcessor &processorRef_;
  std::unique_ptr<uds::PresetManager> presetManager_;
  std::unique_ptr<uds::MainComponent> mainComponent_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UDSAudioProcessorEditor)
};
