#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>


namespace uds {

/**
 * @brief Modal dialog for training expression pedal range
 *
 * Shows instructions to move pedal to MIN/MAX positions and captures values.
 */
class ExpressionTrainDialog : public juce::Component, private juce::Timer {
public:
  std::function<float()>
      getExpressionValue; // Returns current 0-1 expression value
  std::function<void(float, float)>
      onRangeSet; // Called with (min, max) when complete

  ExpressionTrainDialog() {
    setSize(300, 180);

    instructionLabel_.setText(
        "Move pedal to MINIMUM position\nthen click 'Capture Min'",
        juce::dontSendNotification);
    instructionLabel_.setJustificationType(juce::Justification::centred);
    instructionLabel_.setColour(juce::Label::textColourId,
                                juce::Colours::white);
    addAndMakeVisible(instructionLabel_);

    valueLabel_.setText("Current: --", juce::dontSendNotification);
    valueLabel_.setJustificationType(juce::Justification::centred);
    valueLabel_.setColour(juce::Label::textColourId, juce::Colour(0xff00b4d8));
    valueLabel_.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    addAndMakeVisible(valueLabel_);

    captureButton_.setButtonText("Capture Min");
    captureButton_.onClick = [this] { captureValue(); };
    addAndMakeVisible(captureButton_);

    cancelButton_.setButtonText("Cancel");
    cancelButton_.onClick = [this] {
      if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
        dw->closeButtonPressed();
    };
    addAndMakeVisible(cancelButton_);

    startTimerHz(10);
  }

  ~ExpressionTrainDialog() override { stopTimer(); }

  void paint(juce::Graphics& g) override {
    g.fillAll(juce::Colour(0xff2a2a2a));
  }

  void resized() override {
    auto bounds = getLocalBounds().reduced(20);
    instructionLabel_.setBounds(bounds.removeFromTop(50));
    bounds.removeFromTop(10);
    valueLabel_.setBounds(bounds.removeFromTop(30));
    bounds.removeFromTop(20);
    auto buttonRow = bounds.removeFromTop(35);
    captureButton_.setBounds(buttonRow.removeFromLeft(120));
    buttonRow.removeFromLeft(20);
    cancelButton_.setBounds(buttonRow.removeFromLeft(80));
  }

private:
  juce::Label instructionLabel_;
  juce::Label valueLabel_;
  juce::TextButton captureButton_;
  juce::TextButton cancelButton_;

  bool capturedMin_ = false;
  float minValue_ = 0.0f;
  float maxValue_ = 1.0f;

  void timerCallback() {
    if (getExpressionValue) {
      float val = getExpressionValue();
      valueLabel_.setText("Current: " + juce::String(val * 100.0f, 1) + "%",
                          juce::dontSendNotification);
    }
  }

  void captureValue() {
    if (!getExpressionValue)
      return;

    float val = getExpressionValue();

    if (!capturedMin_) {
      // Capture minimum
      minValue_ = val * 100.0f; // Store as parameter value (0-100 for Mix)
      capturedMin_ = true;
      instructionLabel_.setText(
          "Move pedal to MAXIMUM position\nthen click 'Capture Max'",
          juce::dontSendNotification);
      captureButton_.setButtonText("Capture Max");
    } else {
      // Capture maximum and finish
      maxValue_ = val * 100.0f;
      if (onRangeSet)
        onRangeSet(minValue_, maxValue_);
      if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
        dw->closeButtonPressed();
    }
  }

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExpressionTrainDialog)
};

// Helper to show the dialog
inline void
showExpressionTrainDialog(juce::Component* parent,
                          std::function<float()> getExprValue,
                          std::function<void(float, float)> onComplete) {
  auto* dialog = new ExpressionTrainDialog();
  dialog->getExpressionValue = std::move(getExprValue);
  dialog->onRangeSet = std::move(onComplete);

  juce::DialogWindow::LaunchOptions options;
  options.content.setOwned(dialog);
  options.dialogTitle = "Train Expression Range";
  options.dialogBackgroundColour = juce::Colour(0xff2a2a2a);
  options.escapeKeyTriggersCloseButton = true;
  options.useNativeTitleBar = false;
  options.resizable = false;
  options.launchAsync();
}

} // namespace uds
