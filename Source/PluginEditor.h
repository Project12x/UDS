#pragma once

#include "Core/PresetManager.h"
#include "PluginProcessor.h"
#include "UI/MainComponent.h"
#include "UI/SafetyMuteOverlay.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <memory>


/**
 * @brief UDS Plugin Editor
 */
class UDSAudioProcessorEditor : public juce::AudioProcessorEditor,
                                private juce::Timer {
public:
  explicit UDSAudioProcessorEditor(UDSAudioProcessor& p)
      : AudioProcessorEditor(&p),
        processorRef_(p),
        presetManager_(std::make_unique<uds::PresetManager>(p)) {
    mainComponent_ = std::make_unique<uds::MainComponent>(
        p.getParameters(), p.getRoutingGraph(), *presetManager_);
    addAndMakeVisible(*mainComponent_);

    // Wire expression pedal callbacks to processor
    mainComponent_->setExpressionCallbacks(
        [this](const juce::String& paramId, float minVal, float maxVal) {
          processorRef_.setExpressionMapping(paramId, minVal, maxVal);
        },
        [this]() { processorRef_.clearExpressionMapping(); },
        [this](const juce::String& paramId) {
          return processorRef_.hasExpressionMapping(paramId);
        },
        [this]() { return processorRef_.getExpressionValue(); });

    // Safety mute overlay (hidden by default)
    safetyOverlay_ = std::make_unique<uds::SafetyMuteOverlay>();
    safetyOverlay_->onUnlock = [this] {
      processorRef_.unlockSafetyMute();
      safetyOverlay_->hide();
    };
    addAndMakeVisible(*safetyOverlay_);

    setSize(1000, 500);
    setResizable(true, true);
    setResizeLimits(800, 400, 1600, 900);

    // Start timer to sync metronome BPM to processor
    startTimerHz(10);
  }

  ~UDSAudioProcessorEditor() override { stopTimer(); }

  void paint(juce::Graphics& g) override {
    g.fillAll(juce::Colour(0xff1a1a1a));
  }

  void resized() override {
    if (mainComponent_)
      mainComponent_->setBounds(getLocalBounds());

    // Overlay covers entire editor
    if (safetyOverlay_)
      safetyOverlay_->setBounds(getLocalBounds());
  }

private:
  void timerCallback() override {
    if (mainComponent_) {
      // Sync metronome BPM to processor for standalone tempo sync
      double metronomeBpm = mainComponent_->getMetronome().getBpm();
      processorRef_.setInternalBpm(metronomeBpm);

      // Poll band levels for activity indicators
      std::array<float, 12> levels;
      for (int i = 0; i < 12; ++i) {
        levels[static_cast<size_t>(i)] = processorRef_.getBandLevel(i);
      }
      mainComponent_->updateBandLevels(levels);
    }

    // Check for safety mute status
    if (safetyOverlay_) {
      if (processorRef_.isSafetyMuted()) {
        if (!safetyOverlay_->isVisible()) {
          safetyOverlay_->show(processorRef_.getSafetyMuteReason());
        }
      } else {
        if (safetyOverlay_->isVisible()) {
          safetyOverlay_->hide();
        }
      }
    }
  }

  UDSAudioProcessor& processorRef_;
  std::unique_ptr<uds::PresetManager> presetManager_;
  std::unique_ptr<uds::MainComponent> mainComponent_;
  std::unique_ptr<uds::SafetyMuteOverlay> safetyOverlay_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UDSAudioProcessorEditor)
};
