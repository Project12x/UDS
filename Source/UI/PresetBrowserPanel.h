#pragma once

#include "../Core/PresetManager.h"
#include "Typography.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace uds {

/**
 * @brief Compact preset browser panel for the plugin header
 *
 * Layout: [<] [Preset Name Display] [>] [Save] [Menu]
 */
class PresetBrowserPanel : public juce::Component {
public:
  PresetBrowserPanel(PresetManager &presetManager)
      : presetManager_(presetManager) {
    // Previous button
    prevButton_.setButtonText("<");
    prevButton_.setColour(juce::TextButton::buttonColourId,
                          juce::Colour(0xff303030));
    prevButton_.setColour(juce::TextButton::textColourOffId,
                          juce::Colour(0xffe0e0e0));
    prevButton_.onClick = [this]() { presetManager_.loadPreviousPreset(); };
    addAndMakeVisible(prevButton_);

    // Next button
    nextButton_.setButtonText(">");
    nextButton_.setColour(juce::TextButton::buttonColourId,
                          juce::Colour(0xff303030));
    nextButton_.setColour(juce::TextButton::textColourOffId,
                          juce::Colour(0xffe0e0e0));
    nextButton_.onClick = [this]() { presetManager_.loadNextPreset(); };
    addAndMakeVisible(nextButton_);

    // Preset name label
    presetNameLabel_.setJustificationType(juce::Justification::centred);
    presetNameLabel_.setColour(juce::Label::textColourId,
                               juce::Colour(0xffffffff));
    presetNameLabel_.setColour(juce::Label::backgroundColourId,
                               juce::Colour(0xff252530));
    presetNameLabel_.setFont(Typography::getInstance().getHeaderFont(14.0f));
    presetNameLabel_.setEditable(false, true); // Click to show popup menu
    presetNameLabel_.onTextChange = [this]() { /* Handle rename if needed */ };
    addAndMakeVisible(presetNameLabel_);

    // Save button
    saveButton_.setButtonText("Save");
    saveButton_.setColour(juce::TextButton::buttonColourId,
                          juce::Colour(0xff303030));
    saveButton_.setColour(juce::TextButton::textColourOffId,
                          juce::Colour(0xffe0e0e0));
    saveButton_.onClick = [this]() { showSaveDialog(); };
    addAndMakeVisible(saveButton_);

    // Menu button
    menuButton_.setButtonText("...");
    menuButton_.setColour(juce::TextButton::buttonColourId,
                          juce::Colour(0xff303030));
    menuButton_.setColour(juce::TextButton::textColourOffId,
                          juce::Colour(0xffe0e0e0));
    menuButton_.onClick = [this]() { showPresetMenu(); };
    addAndMakeVisible(menuButton_);

    // Connect to preset manager updates
    presetManager_.onPresetChanged = [this]() { updatePresetName(); };

    updatePresetName();
  }

  void resized() override {
    auto bounds = getLocalBounds().reduced(4, 2);

    const int buttonWidth = 28;
    const int saveButtonWidth = 50;
    const int menuButtonWidth = 32;

    prevButton_.setBounds(bounds.removeFromLeft(buttonWidth));
    bounds.removeFromLeft(4);

    nextButton_.setBounds(bounds.removeFromRight(buttonWidth));
    bounds.removeFromRight(4);

    menuButton_.setBounds(bounds.removeFromRight(menuButtonWidth));
    bounds.removeFromRight(4);

    saveButton_.setBounds(bounds.removeFromRight(saveButtonWidth));
    bounds.removeFromRight(4);

    presetNameLabel_.setBounds(bounds);
  }

  void paint(juce::Graphics &g) override {
    g.setColour(juce::Colour(0xff1a1a1e));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 6.0f);

    g.setColour(juce::Colour(0xff3a3a40));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 6.0f,
                           1.0f);
  }

private:
  void updatePresetName() {
    auto name = presetManager_.getCurrentPresetName();
    if (presetManager_.isModified() &&
        presetManager_.getCurrentPresetIndex() >= 0) {
      name += " *";
    }
    presetNameLabel_.setText(name, juce::dontSendNotification);
  }

  void showSaveDialog() {
    auto *alertWindow = new juce::AlertWindow(
        "Save Preset",
        "Enter a name for the preset:", juce::MessageBoxIconType::NoIcon);

    alertWindow->addTextEditor(
        "presetName", presetManager_.getCurrentPresetName(), "Preset Name:");
    alertWindow->addButton("Save", 1);
    alertWindow->addButton("Cancel", 0);

    alertWindow->enterModalState(
        true,
        juce::ModalCallbackFunction::create([this, alertWindow](int result) {
          if (result == 1) {
            auto name = alertWindow->getTextEditorContents("presetName").trim();
            if (name.isNotEmpty()) {
              presetManager_.savePreset(name);
            }
          }
          delete alertWindow;
        }));
  }

  void showPresetMenu() {
    juce::PopupMenu menu;

    // Add presets
    const auto &presets = presetManager_.getPresets();
    if (!presets.empty()) {
      for (size_t i = 0; i < presets.size(); ++i) {
        menu.addItem(static_cast<int>(i + 1), presets[i].name,
                     true, // enabled
                     static_cast<int>(i) ==
                         presetManager_.getCurrentPresetIndex()); // ticked
      }
      menu.addSeparator();
    }

    // Utility actions
    menu.addItem(-1, "Open Preset Folder...");
    menu.addItem(-2, "Refresh Presets");

    menu.showMenuAsync(
        juce::PopupMenu::Options().withTargetComponent(&menuButton_),
        [this](int result) {
          if (result > 0) {
            presetManager_.loadPreset(result - 1);
          } else if (result == -1) {
            presetManager_.showPresetFolder();
          } else if (result == -2) {
            presetManager_.scanPresets();
          }
        });
  }

  PresetManager &presetManager_;

  juce::TextButton prevButton_;
  juce::TextButton nextButton_;
  juce::Label presetNameLabel_;
  juce::TextButton saveButton_;
  juce::TextButton menuButton_;
};

} // namespace uds
