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
  PresetBrowserPanel(PresetManager& presetManager)
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

    // A/B toggle button
    abButton_.setButtonText("A");
    abButton_.setColour(juce::TextButton::buttonColourId,
                        juce::Colour(0xff2a5a2a)); // Green tint for A
    abButton_.setColour(juce::TextButton::textColourOffId,
                        juce::Colour(0xffe0e0e0));
    abButton_.onClick = [this]() {
      // Right-click to store, left-click to toggle
      presetManager_.toggleAB();
      updateABButton();
    };
    addAndMakeVisible(abButton_);

    // Connect to preset manager updates
    presetManager_.onPresetChanged = [this]() {
      updatePresetName();
      updateABButton();
    };

    updatePresetName();
    updateABButton();
  }

  void resized() override {
    auto bounds = getLocalBounds().reduced(4, 2);

    const int buttonWidth = 28;
    const int saveButtonWidth = 50;
    const int menuButtonWidth = 32;
    const int abButtonWidth = 32;

    prevButton_.setBounds(bounds.removeFromLeft(buttonWidth));
    bounds.removeFromLeft(4);

    nextButton_.setBounds(bounds.removeFromRight(buttonWidth));
    bounds.removeFromRight(4);

    menuButton_.setBounds(bounds.removeFromRight(menuButtonWidth));
    bounds.removeFromRight(4);

    abButton_.setBounds(bounds.removeFromRight(abButtonWidth));
    bounds.removeFromRight(4);

    saveButton_.setBounds(bounds.removeFromRight(saveButtonWidth));
    bounds.removeFromRight(4);

    presetNameLabel_.setBounds(bounds);
  }

  void paint(juce::Graphics& g) override {
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

  void updateABButton() {
    int slot = presetManager_.getCurrentABSlot();
    abButton_.setButtonText(slot == 0 ? "A" : "B");
    abButton_.setColour(juce::TextButton::buttonColourId,
                        slot == 0 ? juce::Colour(0xff2a5a2a)   // Green for A
                                  : juce::Colour(0xff5a2a2a)); // Red for B
  }

  void showSaveDialog() {
    auto* alertWindow = new juce::AlertWindow(
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

    // Organize presets by category
    const auto& presets = presetManager_.getPresets();
    auto categories = presetManager_.getCategories();

    if (!presets.empty()) {
      // Add "All Presets" submenu
      juce::PopupMenu allMenu;
      for (size_t i = 0; i < presets.size(); ++i) {
        allMenu.addItem(static_cast<int>(i + 1), presets[i].name, true,
                        static_cast<int>(i) ==
                            presetManager_.getCurrentPresetIndex());
      }
      menu.addSubMenu("All Presets", allMenu);
      menu.addSeparator();

      // Add category submenus
      for (const auto& category : categories) {
        juce::PopupMenu catMenu;
        auto filtered = presetManager_.getPresetsFiltered(category);
        for (const auto& [idx, preset] : filtered) {
          catMenu.addItem(idx + 1, preset.name, true,
                          idx == presetManager_.getCurrentPresetIndex());
        }
        menu.addSubMenu(category, catMenu);
      }
      menu.addSeparator();
    }

    // A/B Comparison
    juce::PopupMenu abMenu;
    abMenu.addItem(-10, "Store to A", true, presetManager_.hasSlotData(0));
    abMenu.addItem(-11, "Store to B", true, presetManager_.hasSlotData(1));
    abMenu.addSeparator();
    abMenu.addItem(-12, "Recall A", presetManager_.hasSlotData(0));
    abMenu.addItem(-13, "Recall B", presetManager_.hasSlotData(1));
    menu.addSubMenu("A/B Comparison", abMenu);
    menu.addSeparator();

    // Utility actions
    menu.addItem(-1, "Open Preset Folder...");
    menu.addItem(-2, "Refresh Presets");
    menu.addSeparator();
    menu.addItem(-3, "Import MagicStomp JSON...");

    menu.showMenuAsync(
        juce::PopupMenu::Options().withTargetComponent(&menuButton_),
        [this](int result) {
          if (result > 0) {
            presetManager_.loadPreset(result - 1);
          } else if (result == -1) {
            presetManager_.showPresetFolder();
          } else if (result == -2) {
            presetManager_.scanPresets();
          } else if (result == -3) {
            showImportDialog();
          } else if (result == -10) {
            presetManager_.storeToSlot(0);
            updateABButton();
          } else if (result == -11) {
            presetManager_.storeToSlot(1);
            updateABButton();
          } else if (result == -12) {
            presetManager_.recallFromSlot(0);
            updateABButton();
          } else if (result == -13) {
            presetManager_.recallFromSlot(1);
            updateABButton();
          }
        });
  }

  void showImportDialog() {
    fileChooser_ = std::make_unique<juce::FileChooser>(
        "Select MagicStomp JSON Export",
        juce::File::getSpecialLocation(juce::File::userDesktopDirectory),
        "*.json");

    fileChooser_->launchAsync(
        juce::FileBrowserComponent::openMode |
            juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc) {
          auto results = fc.getResults();
          if (results.isEmpty())
            return;

          int imported = presetManager_.importFromMagicStompJson(results[0]);

          juce::String msg;
          if (imported > 0) {
            msg =
                "Successfully imported " + juce::String(imported) + " presets.";
          } else {
            msg = "No compatible delay presets found in file.";
          }

          juce::AlertWindow::showMessageBoxAsync(
              juce::MessageBoxIconType::InfoIcon, "Import Complete", msg);
        });
  }

  PresetManager& presetManager_;

  juce::TextButton prevButton_;
  juce::TextButton nextButton_;
  juce::Label presetNameLabel_;
  juce::TextButton saveButton_;
  juce::TextButton menuButton_;
  juce::TextButton abButton_;
  std::unique_ptr<juce::FileChooser> fileChooser_;
};

} // namespace uds
