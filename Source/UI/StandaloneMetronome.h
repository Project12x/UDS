#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace uds {

/**
 * @brief Internal metronome for standalone testing
 *
 * Provides BPM control and audible click for testing tempo sync
 * when running outside a DAW host.
 */
class StandaloneMetronome : public juce::Component, public juce::Timer {
public:
  StandaloneMetronome() {
    // BPM Label
    bpmLabel_.setText("BPM", juce::dontSendNotification);
    bpmLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
    bpmLabel_.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(bpmLabel_);

    // BPM Slider
    bpmSlider_.setRange(40.0, 240.0, 1.0);
    bpmSlider_.setValue(120.0);
    bpmSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    bpmSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 45, 20);
    bpmSlider_.setColour(juce::Slider::trackColourId, juce::Colour(0xff00b4d8));
    bpmSlider_.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    bpmSlider_.setColour(juce::Slider::textBoxTextColourId,
                         juce::Colours::white);
    bpmSlider_.setColour(juce::Slider::textBoxBackgroundColourId,
                         juce::Colour(0xff303030));
    bpmSlider_.setColour(juce::Slider::textBoxOutlineColourId,
                         juce::Colours::transparentBlack);
    bpmSlider_.onValueChange = [this]() {
      bpm_ = bpmSlider_.getValue();
      if (isPlaying_) {
        updateTimerInterval();
      }
    };
    addAndMakeVisible(bpmSlider_);

    // Play/Stop button
    playButton_.setButtonText("▶");
    playButton_.setColour(juce::TextButton::buttonColourId,
                          juce::Colour(0xff404040));
    playButton_.setColour(juce::TextButton::buttonOnColourId,
                          juce::Colour(0xff2e86ab));
    playButton_.setColour(juce::TextButton::textColourOffId,
                          juce::Colours::white);
    playButton_.setColour(juce::TextButton::textColourOnId,
                          juce::Colours::white);
    playButton_.setClickingTogglesState(true);
    playButton_.onClick = [this]() {
      isPlaying_ = playButton_.getToggleState();
      if (isPlaying_) {
        playButton_.setButtonText("■");
        beatCounter_ = 0;
        updateTimerInterval();
        startTimer(static_cast<int>(60000.0 / bpm_));
      } else {
        playButton_.setButtonText("▶");
        stopTimer();
      }
    };
    addAndMakeVisible(playButton_);

    // Beat indicator LED
    beatLed_.setColour(juce::Label::backgroundColourId,
                       juce::Colour(0xff303030));
    addAndMakeVisible(beatLed_);
  }

  ~StandaloneMetronome() override { stopTimer(); }

  void resized() override {
    auto bounds = getLocalBounds().reduced(5, 2);

    playButton_.setBounds(bounds.removeFromLeft(28));
    bounds.removeFromLeft(5);

    beatLed_.setBounds(bounds.removeFromLeft(12).reduced(0, 6));
    bounds.removeFromLeft(5);

    bpmLabel_.setBounds(bounds.removeFromLeft(35));
    bounds.removeFromLeft(3);

    bpmSlider_.setBounds(bounds);
  }

  void paint(juce::Graphics &g) override {
    g.setColour(juce::Colour(0xff252525));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);
  }

  void timerCallback() override {
    // Flash the beat LED
    beatLed_.setColour(juce::Label::backgroundColourId,
                       (beatCounter_ % 4 == 0) ? juce::Colour(0xfff94144)
                                               : juce::Colour(0xff00b4d8));
    beatLed_.repaint();

    // Trigger click sound
    clickPending_ = true;

    beatCounter_++;

    // Fade LED after short delay
    juce::Timer::callAfterDelay(50, [this]() {
      beatLed_.setColour(juce::Label::backgroundColourId,
                         juce::Colour(0xff303030));
      beatLed_.repaint();
    });
  }

  double getBpm() const { return bpm_; }
  bool isPlaying() const { return isPlaying_; }

  // Called from audio thread to check if click should play
  bool consumeClick() {
    bool shouldClick = clickPending_;
    clickPending_ = false;
    return shouldClick;
  }

  // Generate a short click sample (simple sine burst)
  void generateClick(juce::AudioBuffer<float> &buffer, double sampleRate) {
    if (!clickPending_)
      return;
    clickPending_ = false;

    const int clickSamples = static_cast<int>(sampleRate * 0.015); // 15ms click
    const int samplesToWrite = juce::jmin(clickSamples, buffer.getNumSamples());

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
      float *data = buffer.getWritePointer(ch);
      for (int i = 0; i < samplesToWrite; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(samplesToWrite);
        float envelope = (1.0f - t) * (1.0f - t); // Quick decay
        float wave =
            std::sin(2.0f * juce::MathConstants<float>::pi * 880.0f *
                     static_cast<float>(i) / static_cast<float>(sampleRate));
        data[i] += wave * envelope * 0.3f; // Mix in click
      }
    }
  }

private:
  void updateTimerInterval() {
    if (isPlaying_) {
      int intervalMs = static_cast<int>(60000.0 / bpm_);
      startTimer(intervalMs);
    }
  }

  juce::Label bpmLabel_;
  juce::Slider bpmSlider_;
  juce::TextButton playButton_;
  juce::Label beatLed_;

  double bpm_ = 120.0;
  bool isPlaying_ = false;
  int beatCounter_ = 0;
  std::atomic<bool> clickPending_{false};
};

} // namespace uds
