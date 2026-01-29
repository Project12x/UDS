#pragma once

#include "NodeVisual.h"
#include "Typography.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>


namespace uds {

/**
 * @brief Parameter controls for a single delay band
 *
 * Features:
 * - Enable/bypass toggle
 * - Solo/Mute buttons
 * - Algorithm selector
 * - Time/Feedback/Level/Pan knobs
 * - Signal activity LED indicator
 */
class BandParameterPanel : public juce::Component,
                           public juce::Button::Listener,
                           public juce::Timer {
public:
  // Callback for solo state (global coordination)
  std::function<void(int bandIndex, bool solo)> onSoloChanged;
  std::function<void(int bandIndex, bool mute)> onMuteChanged;

  BandParameterPanel(juce::AudioProcessorValueTreeState& apvts, int bandIndex)
      : apvts_(apvts), bandIndex_(bandIndex) {
    juce::String prefix = "band" + juce::String(bandIndex) + "_";

    setupSlider(timeSlider_, timeLabel_, "Time", 250.0); // 250ms default
    setupSlider(feedbackSlider_, feedbackLabel_, "Feedback",
                30.0);                                    // 30% default
    setupSlider(levelSlider_, levelLabel_, "Level", 0.0); // 0dB default
    setupSlider(panSlider_, panLabel_, "Pan", 0.0);       // Center default

    // Band title with number
    titleLabel_.setText("Band " + juce::String(bandIndex + 1),
                        juce::dontSendNotification);
    titleLabel_.setJustificationType(juce::Justification::centred);
    titleLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
    titleLabel_.setFont(Typography::getInstance().getTitleFont(15.0f));
    addAndMakeVisible(titleLabel_);

    // Enable toggle
    enableButton_.setButtonText("ON");
    enableButton_.setClickingTogglesState(true);
    enableButton_.addListener(this);
    enableButton_.setColour(juce::TextButton::buttonOnColourId,
                            juce::Colour(0xff00b4d8));
    enableButton_.setColour(juce::TextButton::buttonColourId,
                            juce::Colour(0xff404040));
    addAndMakeVisible(enableButton_);

    // Solo button - yellow when active
    soloButton_.setButtonText("S");
    soloButton_.setClickingTogglesState(true);
    soloButton_.addListener(this);
    soloButton_.setColour(juce::TextButton::buttonOnColourId,
                          juce::Colour(0xfff9c74f)); // Yellow
    soloButton_.setColour(juce::TextButton::buttonColourId,
                          juce::Colour(0xff404040));
    soloButton_.setTooltip("Solo this band");
    addAndMakeVisible(soloButton_);

    // Mute button - red when active
    muteButton_.setButtonText("M");
    muteButton_.setClickingTogglesState(true);
    muteButton_.addListener(this);
    muteButton_.setColour(juce::TextButton::buttonOnColourId,
                          juce::Colour(0xfff94144)); // Red
    muteButton_.setColour(juce::TextButton::buttonColourId,
                          juce::Colour(0xff404040));
    muteButton_.setTooltip("Mute this band");
    addAndMakeVisible(muteButton_);

    // Algorithm selector
    algorithmBox_.addItem("Digital", 1);
    algorithmBox_.addItem("Analog", 2);
    algorithmBox_.addItem("Tape", 3);
    algorithmBox_.addItem("Lo-Fi", 4);
    algorithmBox_.setColour(juce::ComboBox::backgroundColourId,
                            juce::Colour(0xff303030));
    algorithmBox_.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    algorithmBox_.setColour(juce::ComboBox::outlineColourId,
                            juce::Colour(0xff505050));
    addAndMakeVisible(algorithmBox_);

    algorithmLabel_.setText("Mode", juce::dontSendNotification);
    algorithmLabel_.setJustificationType(juce::Justification::centredLeft);
    algorithmLabel_.setColour(juce::Label::textColourId,
                              juce::Colour(0xffe0e0e0));
    addAndMakeVisible(algorithmLabel_);

    // Create attachments
    timeAttachment_ =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts_, prefix + "time", timeSlider_);
    feedbackAttachment_ =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts_, prefix + "feedback", feedbackSlider_);
    levelAttachment_ =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts_, prefix + "level", levelSlider_);
    panAttachment_ =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts_, prefix + "pan", panSlider_);
    enableAttachment_ =
        std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts_, prefix + "enabled", enableButton_);
    algorithmAttachment_ = std::make_unique<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts_, prefix + "algorithm", algorithmBox_);

    // =========================================
    // Filter Controls (Hi-Cut, Lo-Cut)
    // =========================================
    setupSlider(hiCutSlider_, hiCutLabel_, "Hi-Cut");
    hiCutSlider_.setTextValueSuffix(" Hz");
    hiCutAttachment_ =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts_, prefix + "hiCut", hiCutSlider_);

    setupSlider(loCutSlider_, loCutLabel_, "Lo-Cut");
    loCutSlider_.setTextValueSuffix(" Hz");
    loCutAttachment_ =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts_, prefix + "loCut", loCutSlider_);

    // =========================================
    // LFO Controls (Rate, Depth, Waveform)
    // =========================================
    setupSlider(lfoRateSlider_, lfoRateLabel_, "LFO Rate");
    lfoRateSlider_.setTextValueSuffix(" Hz");
    lfoRateAttachment_ =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts_, prefix + "lfoRate", lfoRateSlider_);

    setupSlider(lfoDepthSlider_, lfoDepthLabel_, "LFO Depth");
    lfoDepthSlider_.setTextValueSuffix(" %");
    lfoDepthAttachment_ =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts_, prefix + "lfoDepth", lfoDepthSlider_);

    // LFO Waveform selector (with None option to disable LFO)
    lfoWaveformLabel_.setText("LFO", juce::dontSendNotification);
    lfoWaveformLabel_.setJustificationType(juce::Justification::centredLeft);
    lfoWaveformLabel_.setColour(juce::Label::textColourId,
                                juce::Colour(0xffe0e0e0));
    addAndMakeVisible(lfoWaveformLabel_);

    lfoWaveformBox_.addItem("None", 1);     // Index 0 = None (LFO disabled)
    lfoWaveformBox_.addItem("Sine", 2);     // Index 1 = Sine
    lfoWaveformBox_.addItem("Triangle", 3); // Index 2 = Triangle
    lfoWaveformBox_.addItem("Saw", 4);      // Index 3 = Saw
    lfoWaveformBox_.addItem("Square", 5);   // Index 4 = Square
    lfoWaveformBox_.addItem("Brownian", 6); // Index 5 = Brownian
    lfoWaveformBox_.addItem("Lorenz", 7);   // Index 6 = Lorenz
    lfoWaveformBox_.setColour(juce::ComboBox::backgroundColourId,
                              juce::Colour(0xff303030));
    lfoWaveformBox_.setColour(juce::ComboBox::textColourId,
                              juce::Colour(0xffe0e0e0));
    lfoWaveformBox_.onChange = [this]() { updateLfoVisibility(); };
    addAndMakeVisible(lfoWaveformBox_);

    lfoWaveformAttachment_ = std::make_unique<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts_, prefix + "lfoWaveform", lfoWaveformBox_);

    // =========================================
    // Attack Envelope (Volume Swell)
    // =========================================
    setupSlider(attackSlider_, attackLabel_, "Attack");
    attackSlider_.setTextValueSuffix(" ms");
    attackSlider_.setTooltip("Attack time for volume swell effect (0 = instant)");
    attackAttachment_ =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts_, prefix + "attack", attackSlider_);

    // Phase Invert toggle - using "INV" text for better rendering
    phaseInvertButton_.setButtonText("INV");
    phaseInvertButton_.setColour(juce::TextButton::buttonColourId,
                                 juce::Colour(0xff303030));
    phaseInvertButton_.setColour(
        juce::TextButton::buttonOnColourId,
        juce::Colour(0xff7b2cbf)); // Purple when active
    phaseInvertButton_.setColour(juce::TextButton::textColourOffId,
                                 juce::Colour(0xffa0a0a0));
    phaseInvertButton_.setColour(juce::TextButton::textColourOnId,
                                 juce::Colours::white);
    phaseInvertButton_.setClickingTogglesState(true);
    phaseInvertButton_.setTooltip("Phase Invert");
    addAndMakeVisible(phaseInvertButton_);

    phaseInvertAttachment_ =
        std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts_, prefix + "phaseInvert", phaseInvertButton_);

    // Ping-Pong toggle
    pingPongButton_.setButtonText("PP");
    pingPongButton_.setColour(juce::TextButton::buttonColourId,
                              juce::Colour(0xff303030));
    pingPongButton_.setColour(juce::TextButton::buttonOnColourId,
                              juce::Colour(0xff2e86ab)); // Blue when active
    pingPongButton_.setColour(juce::TextButton::textColourOffId,
                              juce::Colour(0xffa0a0a0));
    pingPongButton_.setColour(juce::TextButton::textColourOnId,
                              juce::Colours::white);
    pingPongButton_.setClickingTogglesState(true);
    pingPongButton_.setTooltip("Ping-Pong Delay (L/R alternation)");
    addAndMakeVisible(pingPongButton_);

    pingPongAttachment_ =
        std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts_, prefix + "pingPong", pingPongButton_);

    // =========================================
    // Tempo Sync Controls
    // =========================================
    tempoSyncButton_.setButtonText("Sync");
    tempoSyncButton_.setClickingTogglesState(true);
    tempoSyncButton_.setColour(juce::TextButton::buttonColourId,
                               juce::Colour(0xff303030));
    tempoSyncButton_.setColour(juce::TextButton::buttonOnColourId,
                               juce::Colour(0xff2e86ab)); // Blue when active
    tempoSyncButton_.setColour(juce::TextButton::textColourOffId,
                               juce::Colour(0xffa0a0a0));
    tempoSyncButton_.setColour(juce::TextButton::textColourOnId,
                               juce::Colours::white);
    tempoSyncButton_.setTooltip("Sync delay time to host tempo");
    tempoSyncButton_.onClick = [this]() { updateSyncVisibility(); };
    addAndMakeVisible(tempoSyncButton_);

    tempoSyncAttachment_ =
        std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts_, prefix + "tempoSync", tempoSyncButton_);

    noteDivisionBox_.addItem("1/1", 1);
    noteDivisionBox_.addItem("1/2", 2);
    noteDivisionBox_.addItem("1/4", 3);
    noteDivisionBox_.addItem("1/8", 4);
    noteDivisionBox_.addItem("1/16", 5);
    noteDivisionBox_.addItem("1/32", 6);
    noteDivisionBox_.addItem("1/4 D", 7); // Dotted
    noteDivisionBox_.addItem("1/8 D", 8);
    noteDivisionBox_.addItem("1/4 T", 9); // Triplet
    noteDivisionBox_.addItem("1/8 T", 10);
    noteDivisionBox_.setColour(juce::ComboBox::backgroundColourId,
                               juce::Colour(0xff303030));
    noteDivisionBox_.setColour(juce::ComboBox::textColourId,
                               juce::Colours::white);
    noteDivisionBox_.setColour(juce::ComboBox::outlineColourId,
                               juce::Colour(0xff505050));
    noteDivisionBox_.setTooltip("Note division for tempo sync");
    addAndMakeVisible(noteDivisionBox_);

    noteDivisionAttachment_ = std::make_unique<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts_, prefix + "noteDivision", noteDivisionBox_);

    // Solo/Mute button attachments
    soloAttachment_ =
        std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts_, prefix + "solo", soloButton_);

    muteAttachment_ =
        std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts_, prefix + "mute", muteButton_);

    // Initial visibility update
    updateSyncVisibility();

    // Start timer for LED decay
    startTimerHz(30);
  }

  ~BandParameterPanel() override {
    stopTimer();
    enableButton_.removeListener(this);
    soloButton_.removeListener(this);
    muteButton_.removeListener(this);
  }

  void buttonClicked(juce::Button* button) override {
    if (button == &enableButton_) {
      updateEnabledState();
    } else if (button == &soloButton_) {
      if (onSoloChanged)
        onSoloChanged(bandIndex_, soloButton_.getToggleState());
    } else if (button == &muteButton_) {
      if (onMuteChanged)
        onMuteChanged(bandIndex_, muteButton_.getToggleState());
    }
  }

  void timerCallback() override {
    // Decay the signal level for LED
    if (signalLevel_ > 0.0f) {
      signalLevel_ *= 0.85f;
      if (signalLevel_ < 0.01f)
        signalLevel_ = 0.0f;
      repaint();
    }
  }

  // Called by audio thread to update signal level
  void setSignalLevel(float level) {
    signalLevel_ = juce::jmax(signalLevel_, level);
  }

  void setSolo(bool solo) {
    soloButton_.setToggleState(solo, juce::dontSendNotification);
  }
  void setMute(bool mute) {
    muteButton_.setToggleState(mute, juce::dontSendNotification);
  }
  bool isSolo() const { return soloButton_.getToggleState(); }
  bool isMute() const { return muteButton_.getToggleState(); }

  void resized() override {
    auto bounds = getLocalBounds().reduced(5);

    // Title row: [Title] [S] [M] [ON]
    auto titleRow = bounds.removeFromTop(28);
    enableButton_.setBounds(titleRow.removeFromRight(40));
    titleRow.removeFromRight(3);
    muteButton_.setBounds(titleRow.removeFromRight(28));
    titleRow.removeFromRight(3);
    soloButton_.setBounds(titleRow.removeFromRight(28));
    titleLabel_.setBounds(titleRow);

    bounds.removeFromTop(5);

    const int knobSize = 50;
    const int labelHeight = 16;
    const int cellHeight = knobSize + labelHeight;
    const int cellWidth = bounds.getWidth() / 4;

    // =============== Row 1: Core delay parameters ===============
    auto row1 = bounds.removeFromTop(cellHeight);

    // Time cell - includes sync button + either time slider or note division
    auto cell = row1.removeFromLeft(cellWidth);
    auto timeLabelRow = cell.removeFromTop(labelHeight);
    tempoSyncButton_.setBounds(timeLabelRow.removeFromRight(36).reduced(2, 0));
    timeLabel_.setBounds(timeLabelRow);
    if (tempoSyncButton_.getToggleState()) {
      // Show note division dropdown when synced
      noteDivisionBox_.setBounds(cell.reduced(2, 5));
    } else {
      // Show time slider when not synced
      timeSlider_.setBounds(cell);
    }

    cell = row1.removeFromLeft(cellWidth);
    feedbackLabel_.setBounds(cell.removeFromTop(labelHeight));
    feedbackSlider_.setBounds(cell);

    cell = row1.removeFromLeft(cellWidth);
    levelLabel_.setBounds(cell.removeFromTop(labelHeight));
    levelSlider_.setBounds(cell);

    cell = row1.removeFromLeft(cellWidth);
    panLabel_.setBounds(cell.removeFromTop(labelHeight));
    panSlider_.setBounds(cell);

    bounds.removeFromTop(3);

    // =============== Row 2: Filter, Attack, and LFO parameters ===============
    auto row2 = bounds.removeFromTop(cellHeight);

    cell = row2.removeFromLeft(cellWidth);
    hiCutLabel_.setBounds(cell.removeFromTop(labelHeight));
    hiCutSlider_.setBounds(cell);

    cell = row2.removeFromLeft(cellWidth);
    loCutLabel_.setBounds(cell.removeFromTop(labelHeight));
    loCutSlider_.setBounds(cell);

    cell = row2.removeFromLeft(cellWidth);
    attackLabel_.setBounds(cell.removeFromTop(labelHeight));
    attackSlider_.setBounds(cell);

    cell = row2.removeFromLeft(cellWidth);
    lfoRateLabel_.setBounds(cell.removeFromTop(labelHeight));
    lfoRateSlider_.setBounds(cell);

    bounds.removeFromTop(3);

    // =============== Row 3: Algorithm and LFO Waveform selectors
    // ===============
    auto row3 = bounds.removeFromTop(28);

    // Algorithm selector (left half)
    auto algoArea = row3.removeFromLeft(row3.getWidth() / 2);
    algorithmLabel_.setBounds(algoArea.removeFromLeft(40));
    algorithmBox_.setBounds(algoArea.reduced(2, 3));

    // LFO Waveform selector + Phase Invert + Ping-Pong (right half)
    auto lfoArea = row3;
    pingPongButton_.setBounds(lfoArea.removeFromRight(34).reduced(2));
    lfoArea.removeFromRight(2);
    phaseInvertButton_.setBounds(lfoArea.removeFromRight(34).reduced(2));
    lfoArea.removeFromRight(3);
    lfoWaveformLabel_.setBounds(lfoArea.removeFromLeft(28));
    lfoWaveformBox_.setBounds(lfoArea.reduced(2, 3));
  }

  void paint(juce::Graphics& g) override {
    bool isEnabled = enableButton_.getToggleState();
    bool isMuted = muteButton_.getToggleState();

    // Get per-band color
    auto bandColor = NodeEditorTheme::getBandColor(bandIndex_);

    // Background - subtle tint based on band color when enabled
    if (isMuted) {
      g.setColour(juce::Colour(0xff2a1a1a)); // Red tint when muted
    } else if (isEnabled) {
      // Subtle band color tint
      g.setColour(bandColor.darker(0.85f).withAlpha(0.4f).overlaidWith(
          juce::Colour(0xff1a1a2e)));
    } else {
      g.setColour(juce::Colour(0xff1a1a1a)); // Dark when disabled
    }
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.0f);

    // Border - per-band color when enabled, red when muted
    if (isMuted) {
      g.setColour(juce::Colour(0xfff94144));
      g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 8.0f,
                             2.0f);
    } else if (isEnabled) {
      g.setColour(bandColor);
      g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 8.0f,
                             2.0f);
    } else {
      g.setColour(juce::Colour(0xff333333));
      g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 8.0f,
                             1.0f);
    }

    // Signal activity LED (top-right corner)
    auto ledBounds =
        getLocalBounds().reduced(8).removeFromTop(10).removeFromRight(10);

    // LED background
    g.setColour(juce::Colour(0xff202020));
    g.fillEllipse(ledBounds.toFloat());

    // LED glow based on signal level
    if (signalLevel_ > 0.0f) {
      auto glowColor = juce::Colour(0xff00ff88).withAlpha(signalLevel_);
      g.setColour(glowColor);
      g.fillEllipse(ledBounds.toFloat());
    }
  }

private:
  void updateEnabledState() {
    bool isEnabled = enableButton_.getToggleState();
    float alpha = isEnabled ? 1.0f : 0.4f;

    timeSlider_.setAlpha(alpha);
    feedbackSlider_.setAlpha(alpha);
    levelSlider_.setAlpha(alpha);
    panSlider_.setAlpha(alpha);
    algorithmBox_.setAlpha(alpha);
    soloButton_.setAlpha(alpha);
    muteButton_.setAlpha(alpha);

    // Filter and LFO controls
    hiCutSlider_.setAlpha(alpha);
    loCutSlider_.setAlpha(alpha);
    lfoRateSlider_.setAlpha(alpha);
    lfoDepthSlider_.setAlpha(alpha);
    lfoWaveformBox_.setAlpha(alpha);
    phaseInvertButton_.setAlpha(alpha);

    repaint();
  }

  // Hide LFO controls when waveform is set to "None"
  void updateLfoVisibility() {
    bool lfoEnabled = lfoWaveformBox_.getSelectedId() > 1; // >1 means not None
    lfoRateSlider_.setVisible(lfoEnabled);
    lfoRateLabel_.setVisible(lfoEnabled);
    lfoDepthSlider_.setVisible(lfoEnabled);
    lfoDepthLabel_.setVisible(lfoEnabled);
  }

  // Show/hide time slider vs note division based on tempo sync mode
  void updateSyncVisibility() {
    bool synced = tempoSyncButton_.getToggleState();
    timeSlider_.setVisible(!synced);
    timeLabel_.setVisible(!synced);
    noteDivisionBox_.setVisible(synced);
    resized(); // Re-layout to adjust positions
  }

  void setupSlider(juce::Slider& slider, juce::Label& label,
                   const juce::String& name, double defaultValue = 0.0) {
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
    slider.setColour(juce::Slider::rotarySliderFillColourId,
                     juce::Colour(0xff00b4d8));
    slider.setColour(juce::Slider::rotarySliderOutlineColourId,
                     juce::Colour(0xff404040));
    slider.setPopupDisplayEnabled(true, true, this);
    slider.setTooltip(name);
    slider.setDoubleClickReturnValue(true, defaultValue);
    addAndMakeVisible(slider);

    label.setText(name, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colour(0xffe0e0e0));
    addAndMakeVisible(label);
  }

  juce::AudioProcessorValueTreeState& apvts_;
  int bandIndex_;
  float signalLevel_ = 0.0f;

  juce::Label titleLabel_;
  juce::TextButton enableButton_;
  juce::TextButton soloButton_;
  juce::TextButton muteButton_;

  juce::Slider timeSlider_, feedbackSlider_, levelSlider_, panSlider_;
  juce::Label timeLabel_, feedbackLabel_, levelLabel_, panLabel_;
  juce::ComboBox algorithmBox_;
  juce::Label algorithmLabel_;

  // Filter controls
  juce::Slider hiCutSlider_, loCutSlider_;
  juce::Label hiCutLabel_, loCutLabel_;

  // LFO controls
  juce::Slider lfoRateSlider_, lfoDepthSlider_;
  juce::Label lfoRateLabel_, lfoDepthLabel_;
  juce::ComboBox lfoWaveformBox_;
  juce::Label lfoWaveformLabel_;
  juce::TextButton phaseInvertButton_;

  // Attack envelope (volume swell)
  juce::Slider attackSlider_;
  juce::Label attackLabel_;

  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      timeAttachment_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      feedbackAttachment_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      levelAttachment_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      panAttachment_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
      enableAttachment_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
      algorithmAttachment_;

  // Filter attachments
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      hiCutAttachment_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      loCutAttachment_;

  // LFO attachments
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      lfoRateAttachment_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      lfoDepthAttachment_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
      lfoWaveformAttachment_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
      phaseInvertAttachment_;

  // Attack envelope attachment
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      attackAttachment_;

  juce::TextButton pingPongButton_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
      pingPongAttachment_;

  // Tempo sync controls
  juce::TextButton tempoSyncButton_;
  juce::ComboBox noteDivisionBox_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
      tempoSyncAttachment_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
      noteDivisionAttachment_;

  // Solo/Mute attachments
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
      soloAttachment_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
      muteAttachment_;
};

} // namespace uds
