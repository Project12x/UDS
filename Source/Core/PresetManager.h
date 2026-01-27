#pragma once

#include "../PluginProcessor.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

namespace uds {

/**
 * @brief Preset metadata structure
 */
struct PresetInfo {
  juce::String name;
  juce::String author;
  juce::String category;
  juce::File file;
  bool isFactory = false;
};

/**
 * @brief Manages preset save/load operations
 *
 * Features:
 * - Scans factory and user preset folders
 * - Saves/loads complete plugin state (parameters + routing)
 * - Maintains current preset list for browser
 */
class PresetManager {
public:
  PresetManager(UDSAudioProcessor &processor) : processor_(processor) {
    // Initialize preset directories
    userPresetDirectory_ =
        juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("UDS")
            .getChildFile("Presets");
    userPresetDirectory_.createDirectory();

    // Ensure default preset exists
    ensureDefaultPresetExists();

    scanPresets();

    // Select Default preset on startup if available
    for (size_t i = 0; i < presets_.size(); ++i) {
      if (presets_[i].name == "Default") {
        currentPresetIndex_ = static_cast<int>(i);
        break;
      }
    }
  }

  /** @brief Get list of all available presets */
  const std::vector<PresetInfo> &getPresets() const { return presets_; }

  /** @brief Get current preset index (-1 if modified/unsaved) */
  int getCurrentPresetIndex() const { return currentPresetIndex_; }

  /** @brief Get current preset name */
  juce::String getCurrentPresetName() const {
    if (currentPresetIndex_ >= 0 &&
        currentPresetIndex_ < static_cast<int>(presets_.size())) {
      return presets_[static_cast<size_t>(currentPresetIndex_)].name;
    }
    return isModified_ ? "Modified" : "Init";
  }

  /** @brief Load a preset by index */
  bool loadPreset(int index) {
    if (index < 0 || index >= static_cast<int>(presets_.size()))
      return false;

    const auto &preset = presets_[static_cast<size_t>(index)];
    if (!preset.file.existsAsFile())
      return false;

    // Load XML from file
    auto xml = juce::XmlDocument::parse(preset.file);
    if (xml == nullptr)
      return false;

    // Convert to binary and restore state
    juce::MemoryBlock data;
    juce::AudioProcessor::copyXmlToBinary(*xml, data);
    processor_.setStateInformation(data.getData(),
                                   static_cast<int>(data.getSize()));

    currentPresetIndex_ = index;
    isModified_ = false;

    if (onPresetChanged)
      onPresetChanged();

    return true;
  }

  /** @brief Load next/previous preset */
  void loadNextPreset() {
    if (presets_.empty())
      return;
    int next = (currentPresetIndex_ + 1) % static_cast<int>(presets_.size());
    loadPreset(next);
  }

  void loadPreviousPreset() {
    if (presets_.empty())
      return;
    int prev = currentPresetIndex_ - 1;
    if (prev < 0)
      prev = static_cast<int>(presets_.size()) - 1;
    loadPreset(prev);
  }

  /** @brief Save current state as user preset */
  bool savePreset(const juce::String &name) {
    auto file = userPresetDirectory_.getChildFile(name + ".udspreset");

    // Get current state as XML
    juce::MemoryBlock data;
    processor_.getStateInformation(data);

    auto xml = juce::AudioProcessor::getXmlFromBinary(
        data.getData(), static_cast<int>(data.getSize()));
    if (xml == nullptr)
      return false;

    // Add metadata
    xml->setAttribute("presetName", name);
    xml->setAttribute("presetAuthor", "User");
    xml->setAttribute("presetCategory", "User");

    // Save to file
    if (!xml->writeTo(file))
      return false;

    // Refresh preset list
    scanPresets();

    // Find and set as current
    for (size_t i = 0; i < presets_.size(); ++i) {
      if (presets_[i].file == file) {
        currentPresetIndex_ = static_cast<int>(i);
        break;
      }
    }

    isModified_ = false;
    if (onPresetChanged)
      onPresetChanged();

    return true;
  }

  /** @brief Mark state as modified */
  void markModified() {
    isModified_ = true;
    if (onPresetChanged)
      onPresetChanged();
  }

  /** @brief Check if current state is modified */
  bool isModified() const { return isModified_; }

  /** @brief Re-scan preset folders */
  void scanPresets() {
    presets_.clear();

    // Scan user presets
    for (const auto &file : userPresetDirectory_.findChildFiles(
             juce::File::findFiles, false, "*.udspreset")) {
      PresetInfo info;
      info.file = file;
      info.name = file.getFileNameWithoutExtension();
      info.isFactory = false;

      // Try to read metadata from file
      if (auto xml = juce::XmlDocument::parse(file)) {
        info.name = xml->getStringAttribute("presetName", info.name);
        info.author = xml->getStringAttribute("presetAuthor", "User");
        info.category = xml->getStringAttribute("presetCategory", "User");
      }

      presets_.push_back(info);
    }

    // Sort alphabetically
    std::sort(presets_.begin(), presets_.end(),
              [](const PresetInfo &a, const PresetInfo &b) {
                return a.name.compareIgnoreCase(b.name) < 0;
              });
  }

  /** @brief Open user preset folder in system file browser */
  void showPresetFolder() { userPresetDirectory_.revealToUser(); }

  // Callback when preset changes
  std::function<void()> onPresetChanged;

private:
  /**
   * @brief Creates a Default preset if one doesn't exist
   *
   * The Default preset has:
   * - All bands disabled except Band 1
   * - Simple parallel routing (Input -> Band 1 -> Output)
   * - Clean delay settings
   */
  void ensureDefaultPresetExists() {
    auto defaultFile = userPresetDirectory_.getChildFile("Default.udspreset");
    if (defaultFile.existsAsFile())
      return;

    // Create a minimal default preset XML
    auto xml = std::make_unique<juce::XmlElement>("UDSState");
    xml->setAttribute("presetName", "Default");
    xml->setAttribute("presetAuthor", "Factory");
    xml->setAttribute("presetCategory", "Factory");

    // Create APVTS subtree with default values
    auto *apvtsXml = xml->createNewChildElement("Parameters");
    apvtsXml->setAttribute("mix", 0.5f);

    // Band 1 enabled, others disabled
    apvtsXml->setAttribute("band1_enabled", true);
    apvtsXml->setAttribute("band1_time", 250.0f);
    apvtsXml->setAttribute("band1_feedback", 0.3f);
    apvtsXml->setAttribute("band1_pan", 0.0f);
    apvtsXml->setAttribute("band1_level", 0.8f);
    apvtsXml->setAttribute("band1_algorithm", 0); // Digital

    for (int i = 2; i <= 8; ++i) {
      apvtsXml->setAttribute("band" + juce::String(i) + "_enabled", false);
    }

    // Create simple routing (Input -> Band 1 -> Output)
    auto *routingXml = xml->createNewChildElement("Routing");
    auto *conn1 = routingXml->createNewChildElement("Connection");
    conn1->setAttribute("source", 0); // Input
    conn1->setAttribute("dest", 1);   // Band 1
    auto *conn2 = routingXml->createNewChildElement("Connection");
    conn2->setAttribute("source", 1); // Band 1
    conn2->setAttribute("dest", 10);  // Output

    xml->writeTo(defaultFile);
  }

  UDSAudioProcessor &processor_;
  juce::File userPresetDirectory_;
  std::vector<PresetInfo> presets_;
  int currentPresetIndex_ = -1;
  bool isModified_ = false;
};

} // namespace uds
