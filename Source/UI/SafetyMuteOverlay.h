#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace uds {

/**
 * @brief Full-screen overlay shown when safety mute is triggered.
 *
 * Displays the reason for the mute and provides an unlock button.
 * This is a modal overlay that blocks all other interaction until dismissed.
 */
class SafetyMuteOverlay : public juce::Component {
public:
  SafetyMuteOverlay() {
    // Warning label
    warningLabel_.setText("AUDIO MUTED", juce::dontSendNotification);
    warningLabel_.setJustificationType(juce::Justification::centred);
    warningLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
    warningLabel_.setFont(juce::FontOptions(48.0f).withStyle("Bold"));
    addAndMakeVisible(warningLabel_);

    // Reason label
    reasonLabel_.setJustificationType(juce::Justification::centred);
    reasonLabel_.setColour(juce::Label::textColourId, juce::Colour(0xffcccccc));
    reasonLabel_.setFont(juce::FontOptions(18.0f));
    addAndMakeVisible(reasonLabel_);

    // Unlock button
    unlockButton_.setButtonText("UNLOCK AUDIO");
    unlockButton_.setColour(juce::TextButton::buttonColourId,
                            juce::Colour(0xff00b4d8));
    unlockButton_.setColour(juce::TextButton::textColourOnId,
                            juce::Colours::white);
    unlockButton_.onClick = [this] {
      if (onUnlock)
        onUnlock();
    };
    addAndMakeVisible(unlockButton_);

    setVisible(false);
  }

  void paint(juce::Graphics& g) override {
    // Semi-transparent red overlay
    g.fillAll(juce::Colour(0xe0661111));

    // Warning icon (triangle with exclamation)
    auto center = getLocalBounds().getCentre();
    juce::Path triangle;
    float triSize = 80.0f;
    triangle.addTriangle(
        static_cast<float>(center.x), static_cast<float>(center.y) - 180.0f,
        static_cast<float>(center.x) - triSize,
        static_cast<float>(center.y) - 180.0f + triSize * 1.5f,
        static_cast<float>(center.x) + triSize,
        static_cast<float>(center.y) - 180.0f + triSize * 1.5f);
    g.setColour(juce::Colours::yellow);
    g.fillPath(triangle);

    // Exclamation mark
    g.setColour(juce::Colour(0xff661111));
    g.setFont(juce::FontOptions(60.0f).withStyle("Bold"));
    g.drawText("!", juce::Rectangle<int>(center.x - 20, center.y - 160, 40, 80),
               juce::Justification::centred);
  }

  void resized() override {
    auto bounds = getLocalBounds();
    auto center = bounds.getCentre();

    warningLabel_.setBounds(center.x - 200, center.y - 60, 400, 60);
    reasonLabel_.setBounds(center.x - 250, center.y, 500, 30);
    unlockButton_.setBounds(center.x - 100, center.y + 60, 200, 50);
  }

  void setReason(int reasonCode) {
    juce::String reasonText;
    switch (reasonCode) {
    case 1:
      reasonText = "Sustained peak level exceeded +6dBFS for 100ms";
      break;
    case 2:
      reasonText = "DC offset exceeded 0.5 for 500ms";
      break;
    case 3:
      reasonText = "NaN or Infinity detected in audio stream";
      break;
    default:
      reasonText = "Unknown safety condition triggered";
      break;
    }
    reasonLabel_.setText(reasonText, juce::dontSendNotification);
  }

  void show(int reasonCode) {
    setReason(reasonCode);
    setVisible(true);
    toFront(true);
  }

  void hide() { setVisible(false); }

  std::function<void()> onUnlock;

private:
  juce::Label warningLabel_;
  juce::Label reasonLabel_;
  juce::TextButton unlockButton_;
};

} // namespace uds
