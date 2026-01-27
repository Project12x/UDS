#pragma once

#include "../PluginProcessor.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

#include <nlohmann/json.hpp>


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
  PresetManager(UDSAudioProcessor& processor) : processor_(processor) {
    // Initialize preset directories
    userPresetDirectory_ =
        juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("UDS")
            .getChildFile("Presets");
    userPresetDirectory_.createDirectory();

    // Create factory presets on first run
    createFactoryPresets();

    scanPresets();

    // Select first preset on startup if available
    for (size_t i = 0; i < presets_.size(); ++i) {
      if (presets_[i].name.startsWith("01")) {
        currentPresetIndex_ = static_cast<int>(i);
        break;
      }
    }
  }

  /** @brief Get list of all available presets */
  const std::vector<PresetInfo>& getPresets() const { return presets_; }

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

    const auto& preset = presets_[static_cast<size_t>(index)];
    if (!preset.file.existsAsFile())
      return false;

    // Load XML from file
    auto xml = juce::XmlDocument::parse(preset.file);
    if (xml == nullptr)
      return false;

    // Parse our custom UDSState format and apply to APVTS
    auto* paramsXml = xml->getChildByName("UDSParameters");
    if (paramsXml != nullptr) {
      auto& apvts = processor_.getAPVTS();

      // === First pass: Find parent band (first with tapOnly=false) ===
      // These values will be used for tapOnly=true bands
      float parentTime = 250.0f;
      float parentFeedback = 30.0f;
      float parentHiCut = 12000.0f;
      float parentLoCut = 80.0f;
      int parentBand = -1;

      for (int band = 0; band < 8; ++band) {
        juce::String prefix = "band" + juce::String(band) + "_";
        bool isTapOnly = paramsXml->getBoolAttribute(prefix + "tapOnly", false);

        if (!isTapOnly) {
          parentBand = band;
          parentTime = static_cast<float>(
              paramsXml->getDoubleAttribute(prefix + "time", 250.0));
          parentFeedback = static_cast<float>(
              paramsXml->getDoubleAttribute(prefix + "feedback", 30.0));
          parentHiCut = static_cast<float>(
              paramsXml->getDoubleAttribute(prefix + "hiCut", 12000.0));
          parentLoCut = static_cast<float>(
              paramsXml->getDoubleAttribute(prefix + "loCut", 80.0));
          break; // Found our parent
        }
      }

      // === Second pass: Apply all parameters with tapOnly inheritance ===
      for (int i = 0; i < paramsXml->getNumAttributes(); ++i) {
        auto attrName = paramsXml->getAttributeName(i);
        auto attrValue = paramsXml->getAttributeValue(i);

        // Skip tapOnly attribute itself (not an APVTS param)
        if (attrName.endsWith("_tapOnly"))
          continue;

        // Try to get the parameter from APVTS
        if (auto* param = apvts.getParameter(attrName)) {
          float value = attrValue.getFloatValue();
          param->setValueNotifyingHost(param->convertTo0to1(value));
        }
      }

      // === Third pass: Override inherited params for tapOnly bands ===
      for (int band = 0; band < 8; ++band) {
        juce::String prefix = "band" + juce::String(band) + "_";
        bool isTapOnly = paramsXml->getBoolAttribute(prefix + "tapOnly", false);

        if (isTapOnly && parentBand >= 0) {
          // Get this band's tap percentage (0-100)
          float tapPercentage = static_cast<float>(
              paramsXml->getDoubleAttribute(prefix + "tapPercentage", 100.0));

          // Calculate actual delay time: parentTime × (tapPercent / 100)
          float actualTime = parentTime * (tapPercentage / 100.0f);

          // Apply parent's Filter/Feedback and calculated Time to this tapOnly
          // band
          auto setParam = [&](const juce::String& name, float value) {
            if (auto* param = apvts.getParameter(prefix + name)) {
              param->setValueNotifyingHost(param->convertTo0to1(value));
            }
          };

          setParam("time", actualTime);
          setParam("feedback", parentFeedback);
          setParam("hiCut", parentHiCut);
          setParam("loCut", parentLoCut);
        }
      }
    }

    // Load routing if present
    auto* routingXml = xml->getChildByName("Routing");
    if (routingXml != nullptr) {
      processor_.getRoutingGraph().fromXml(routingXml);
      if (onRoutingChanged)
        onRoutingChanged();
    }

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
  bool savePreset(const juce::String& name) {
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
    for (const auto& file : userPresetDirectory_.findChildFiles(
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
              [](const PresetInfo& a, const PresetInfo& b) {
                return a.name.compareIgnoreCase(b.name) < 0;
              });
  }

  /** @brief Open user preset folder in system file browser */
  void showPresetFolder() { userPresetDirectory_.revealToUser(); }

  // Callback when preset changes
  std::function<void()> onPresetChanged;

  // Callback when routing changes (for UI sync)
  std::function<void()> onRoutingChanged;

  /**
   * @brief Helper struct for band configuration
   */
  struct BandConfig {
    bool enabled = false;
    float timeMs = 250.0f;
    float feedbackPct = 30.0f;
    float pan = 0.0f;
    float levelDb = 0.0f;
    int algorithm = 0; // 0=Digital, 1=Analog, 2=Tape, 3=LoFi
    float hiCut = 12000.0f;
    float loCut = 80.0f;
    float lfoRate = 0.0f;
    float lfoDepth = 0.0f;
    int lfoWaveform = 0; // 0=None, 1=Sine, 2=Tri, 3=Saw, 4=Square
    bool phaseInvert = false;
    bool pingPong = false;
    bool tapOnly =
        false; // True if this band shares Time/Filter/Feedback from parent
    // Tempo sync parameters
    bool tempoSync = false;
    int noteDivision = 3;    // 0=1/1, 1=1/2, 2=1/4D, 3=1/4, 4=1/8D, 5=1/8,
                             // 6=1/16D, 7=1/16, 8=1/32
    int tapPercentage = 100; // 0-100 for tap offset within note division
  };

  /**
   * @brief Write a preset file from band and routing configuration
   */
  void writePresetFile(const juce::File& file, const juce::String& name,
                       const juce::String& author, const juce::String& category,
                       const std::vector<BandConfig>& bands,
                       const std::vector<std::pair<int, int>>& routing,
                       int masterLfoWaveform = 0, float effectLevel = 50.0f,
                       float directLevel = 100.0f, float directPan = 0.0f) {
    auto xml = std::make_unique<juce::XmlElement>("UDSState");
    xml->setAttribute("presetName", name);
    xml->setAttribute("presetAuthor", author);
    xml->setAttribute("presetCategory", category);

    // Create APVTS subtree
    auto* apvtsXml = xml->createNewChildElement("UDSParameters");
    apvtsXml->setAttribute("mix", effectLevel);
    apvtsXml->setAttribute("dryLevel", directLevel);
    apvtsXml->setAttribute("dryPan", directPan);
    apvtsXml->setAttribute("masterLfoWaveform", masterLfoWaveform);

    // Configure all 8 bands
    int bandIdx = 0;
    for (const auto& band : bands) {
      juce::String prefix = "band" + juce::String(bandIdx) + "_";
      apvtsXml->setAttribute(prefix + "enabled", band.enabled);
      apvtsXml->setAttribute(prefix + "time", band.timeMs);
      apvtsXml->setAttribute(prefix + "feedback", band.feedbackPct);
      apvtsXml->setAttribute(prefix + "pan", band.pan);
      apvtsXml->setAttribute(prefix + "level", band.levelDb);
      apvtsXml->setAttribute(prefix + "algorithm", band.algorithm);
      apvtsXml->setAttribute(prefix + "hiCut", band.hiCut);
      apvtsXml->setAttribute(prefix + "loCut", band.loCut);
      apvtsXml->setAttribute(prefix + "lfoRate", band.lfoRate);
      apvtsXml->setAttribute(prefix + "lfoDepth", band.lfoDepth);
      apvtsXml->setAttribute(prefix + "lfoWaveform", band.lfoWaveform);
      apvtsXml->setAttribute(prefix + "phaseInvert", band.phaseInvert);
      apvtsXml->setAttribute(prefix + "pingPong", band.pingPong);
      apvtsXml->setAttribute(prefix + "tapOnly", band.tapOnly);
      apvtsXml->setAttribute(prefix + "tempoSync", band.tempoSync);
      apvtsXml->setAttribute(prefix + "noteDivision", band.noteDivision);
      apvtsXml->setAttribute(prefix + "tapPercentage", band.tapPercentage);
      ++bandIdx;
    }

    // Disable remaining bands
    for (int i = bandIdx; i < 8; ++i) {
      juce::String prefix = "band" + juce::String(i) + "_";
      apvtsXml->setAttribute(prefix + "enabled", false);
    }

    // Create routing
    auto* routingXml = xml->createNewChildElement("Routing");
    for (const auto& conn : routing) {
      auto* connXml = routingXml->createNewChildElement("Connection");
      connXml->setAttribute("source", conn.first);
      connXml->setAttribute("dest", conn.second);
    }

    xml->writeTo(file);
  }

  /**
   * @brief Import presets from MagicStomp Frenzy JSON export
   *
   * Imports "8 Band Parallel Delay" (id=1) and "8 Multi Tap Mod. Delay" (id=5)
   * presets, mapping parameters to UDS format. Skips other effect types.
   *
   * @param jsonFile Path to the JSON export file
   * @return Number of presets successfully imported
   */
  int importFromMagicStompJson(const juce::File& jsonFile) {
    if (!jsonFile.existsAsFile())
      return 0;

    // Read JSON file
    auto jsonText = jsonFile.loadFileAsString();
    if (jsonText.isEmpty())
      return 0;

    nlohmann::json patches;
    try {
      patches = nlohmann::json::parse(jsonText.toStdString());
    } catch (...) {
      return 0;
    }

    if (!patches.is_array())
      return 0;

    int imported = 0;

    for (const auto& patch : patches) {
      // Check effect type - only import 8-band delay variants
      int effectId = 0;
      if (patch.contains("effectType") && patch["effectType"].contains("id"))
        effectId = patch["effectType"]["id"].get<int>();

      // id=1: "8 Band Parallel Delay", id=5: "8 Multi Tap Mod. Delay"
      if (effectId != 1 && effectId != 5)
        continue;

      // Determine category based on effect type
      juce::String category = (effectId == 5) ? "Multi-Tap" : "Parallel";

      // Get patch name
      juce::String rawName;
      if (patch.contains("patchName"))
        rawName = juce::String(patch["patchName"].get<std::string>());
      else
        rawName = "Preset " + juce::String(imported + 1);

      // Skip empty/init presets
      if (rawName.containsIgnoreCase("Init Voice"))
        continue;

      // Extract base name for numbered presets (e.g., "Faderhead 2" ->
      // "Faderhead") and use as sub-category if there are numbered variations
      juce::String baseName = rawName.trimEnd();
      // Strip trailing numbers and spaces
      while (baseName.isNotEmpty() && (baseName.getLastCharacter() >= '0' &&
                                       baseName.getLastCharacter() <= '9'))
        baseName = baseName.dropLastCharacters(1);
      baseName = baseName.trimEnd();

      // If base name is different from raw name, we have a numbered variant
      // Use base name as sub-category: "Parallel/Faderhead" or "Multi-Tap/Sand"
      if (baseName.isNotEmpty() && baseName != rawName.trimEnd()) {
        category = category + "/" + baseName;
      }

      juce::String patchName = "AH - " + rawName;

      const auto& params = patch["parameters"];
      if (!params.is_object())
        continue;

      // Determine parameter prefix based on effect type
      // New schema uses "Band{N}_Property" format (underscores, no spaces)
      std::string bandPrefix = (effectId == 5) ? "Tap" : "Band";

      // Helper to get float param from new JSON schema
      auto getFloatParam = [&](const std::string& key) -> float {
        if (params.contains(key)) {
          if (params[key].is_number())
            return params[key].get<float>();
          else if (params[key].is_string())
            return 0.0f; // String values handled separately
        }
        return 0.0f;
      };

      // Helper to get string param
      auto getStringParam = [&](const std::string& key) -> std::string {
        if (params.contains(key) && params[key].is_string())
          return params[key].get<std::string>();
        return "";
      };

      // Map parameters for each of 8 bands
      std::vector<BandConfig> bands;
      for (int i = 1; i <= 8; ++i) {
        std::string prefix = bandPrefix + std::to_string(i) + "_";

        BandConfig band;

        // Level=0 means band is disabled (MagicStomp's way to disable a band)
        float levelVal = getFloatParam(prefix + "Level");
        band.enabled = levelVal > 0;

        // Delay time from DelayTime{N} (already in ms)
        band.timeMs = getFloatParam("DelayTime" + std::to_string(i));

        // Feedback (0-10 scale → percentage)
        band.feedbackPct = getFloatParam(prefix + "Feedback") * 10.0f;

        // Level: 0-10 → -24 to 0 dB (scaled)
        band.levelDb = (levelVal / 10.0f) * 24.0f - 24.0f;

        // Pan: Already -10 to +10, scale to -1 to +1
        float panRaw = getFloatParam(prefix + "Pan");
        band.pan = panRaw / 10.0f;

        // Hi-Cut Filter (0-100 scale → frequency)
        float hiCutPct = getFloatParam(prefix + "HighCutFilter");
        band.hiCut =
            static_cast<int>(20000.0f - (hiCutPct / 100.0f) * 19000.0f);

        // Lo-Cut Filter (0-100 scale → frequency)
        float loCutPct = getFloatParam(prefix + "LowCutFilter");
        band.loCut = static_cast<int>(20.0f + (loCutPct / 100.0f) * 980.0f);

        // LFO Rate from Speed (0-10 Hz scale)
        band.lfoRate = getFloatParam(prefix + "Speed");

        // LFO Depth (0-10 scale → percentage)
        band.lfoDepth = getFloatParam(prefix + "Depth") * 10.0f;

        // LFO Waveform from Wave string
        // APVTS: 0=None, 1=Sine, 2=Triangle, 3=Saw, 4=Square
        std::string waveStr = getStringParam(prefix + "Wave");
        if (waveStr == "Sine")
          band.lfoWaveform = 1;
        else if (waveStr == "Triangle")
          band.lfoWaveform = 2;
        else if (waveStr == "Saw")
          band.lfoWaveform = 3;
        else if (waveStr == "Square")
          band.lfoWaveform = 4;
        else
          band.lfoWaveform = 0; // None/default

        // Phase invert from Phase string
        std::string phaseStr = getStringParam(prefix + "Phase");
        band.phaseInvert = (phaseStr == "Invert");

        // Tempo sync parameters - Sync is 1-8 (band routing index)
        // In new schema, Sync is just a number 1-8
        int syncValue = static_cast<int>(getFloatParam(prefix + "Sync"));
        // Note: In the new schema, Sync=1-8 appears to be routing order,
        // not note division. Use default values for now.
        band.tempoSync = false;
        band.noteDivision = 3; // Default 1/4

        // Tap: 0-100 percentage
        band.tapPercentage = static_cast<int>(getFloatParam(prefix + "Tap"));

        // Algorithm (default Digital)
        band.algorithm = 0;

        // Read tapOnly from JSON
        if (params.contains(prefix + "tapOnly") &&
            params[prefix + "tapOnly"].is_boolean()) {
          band.tapOnly = params[prefix + "tapOnly"].get<bool>();
        } else {
          band.tapOnly = false;
        }

        // Ping pong inferred from patch name (hidden param not exportable from
        // MagicStomp)
        band.pingPong = rawName.containsIgnoreCase("ping pong");

        bands.push_back(band);
      }

      // Get mix level from EffectLevel (0-10 scale)
      float effectLevel =
          getFloatParam("EffectLevel") * 10.0f; // Convert to 0-100

      // Create routing based on effect type
      std::vector<std::pair<int, int>> routing;
      // Determine routing from _delayStructure.type
      // Values: "8 Independent Bands (Parallel)", "8 Independent Bands
      // (Series)",
      //         "8 Multi-Tap (1 Band)", "4 Bands x 2 Taps", etc.
      bool isSeriesRouting = false;
      if (params.contains("_delayStructure") &&
          params["_delayStructure"].is_object()) {
        auto& delayStruct = params["_delayStructure"];
        if (delayStruct.contains("type") && delayStruct["type"].is_string()) {
          std::string structType = delayStruct["type"].get<std::string>();
          // Check if the type contains "Series"
          isSeriesRouting = (structType.find("Series") != std::string::npos);
        }
      }

      if (isSeriesRouting) {
        // Series/Cascade routing: Input → B1 → B2 → ... → B8 → Output
        // Find first and last enabled bands
        int firstEnabled = -1;
        int lastEnabled = -1;
        for (int i = 0; i < 8; ++i) {
          if (bands[static_cast<size_t>(i)].enabled) {
            if (firstEnabled < 0)
              firstEnabled = i;
            lastEnabled = i;
          }
        }

        if (firstEnabled >= 0) {
          // Input → first band
          routing.push_back({0, firstEnabled + 1});

          // Chain enabled bands together
          int prevEnabled = firstEnabled;
          for (int i = firstEnabled + 1; i <= lastEnabled; ++i) {
            if (bands[static_cast<size_t>(i)].enabled) {
              routing.push_back({prevEnabled + 1, i + 1});
              prevEnabled = i;
            }
          }

          // Last band → Output
          routing.push_back({lastEnabled + 1, 9});
        }
      } else {
        // Parallel routing (default): Input → All Bands → Output
        for (int i = 0; i < 8; ++i) {
          if (bands[static_cast<size_t>(i)].enabled) {
            routing.push_back({0, i + 1}); // Input → Band
            routing.push_back({i + 1, 9}); // Band → Output
          }
        }
      }

      // Get master LFO waveform from WaveForm string (no space)
      int masterLfoWaveform = 0;
      std::string waveFormStr = getStringParam("WaveForm");
      if (waveFormStr == "Triangle")
        masterLfoWaveform = 1;
      else if (waveFormStr == "Saw")
        masterLfoWaveform = 2;
      else if (waveFormStr == "Square")
        masterLfoWaveform = 3;
      // else Sine = 0

      // Get DirectLevel (0-10 scale → 0-100%)
      float directLevel = getFloatParam("DirectLevel") * 10.0f;

      // Get DirectPan (-10 to +10 scale → -1 to +1)
      float directPan = getFloatParam("DirectPan") / 10.0f;

      // Generate preset file (skip if already exists)
      auto file = userPresetDirectory_.getChildFile(patchName + ".udspreset");
      if (file.existsAsFile())
        continue;

      writePresetFile(file, patchName, "Allan Holdsworth", category, bands,
                      routing, masterLfoWaveform, effectLevel, directLevel,
                      directPan);

      ++imported;
    }

    // Refresh preset list
    scanPresets();

    if (onPresetChanged)
      onPresetChanged();

    return imported;
  }

  /**
   * @brief Creates factory presets if they don't exist
   *
   * 12 presets inspired by Allan Holdsworth's UD Stomp programming style:
   * - Stereo Enhanced Lead (4): Short delays for stereo widening
   * - Rhythmic Echoes (4): Medium delays for rhythmic patterns
   * - Vintage Textures (4): Long delays with character algorithms
   */
  void createFactoryPresets() {
    // === STEREO ENHANCED LEAD ===

    // 1. Stereo Widener - subtle L/R spread
    createPreset("01 - Stereo Widener", "Factory", "Stereo Lead",
                 {{true, 15.0f, 20.0f, -1.0f, -3.0f, 0}, // Band 1: 15ms hard L
                  {true, 25.0f, 15.0f, 1.0f, -3.0f, 0}}, // Band 2: 25ms hard R
                 {{0, 1}, {0, 2}, {1, 9}, {2, 9}});      // Parallel to output

    // 2. Chorus Lead - multi-tap with modulation
    createPreset(
        "02 - Chorus Lead", "Factory", "Stereo Lead",
        {{true, 12.0f, 10.0f, -0.7f, -4.0f, 0, 12000, 80, 0.5f, 15.0f, 1},
         {true, 18.0f, 10.0f, 0.3f, -4.0f, 0, 12000, 80, 0.6f, 12.0f, 1},
         {true, 24.0f, 10.0f, -0.3f, -5.0f, 0, 12000, 80, 0.4f, 18.0f, 1},
         {true, 30.0f, 10.0f, 0.7f, -5.0f, 0, 12000, 80, 0.5f, 10.0f, 1}},
        {{0, 1}, {0, 2}, {0, 3}, {0, 4}, {1, 9}, {2, 9}, {3, 9}, {4, 9}});

    // 3. Wide Shimmer - sparkly modulated
    createPreset(
        "03 - Wide Shimmer", "Factory", "Stereo Lead",
        {{true, 20.0f, 25.0f, -1.0f, -2.0f, 0, 15000, 60, 0.8f, 20.0f, 2},
         {true, 35.0f, 20.0f, 1.0f, -2.0f, 0, 15000, 60, 0.7f, 25.0f, 2}},
        {{0, 1}, {0, 2}, {1, 9}, {2, 9}});

    // 4. Synth Lead - saw mod for synth texture
    createPreset(
        "04 - Synth Lead", "Factory", "Stereo Lead",
        {{true, 10.0f, 15.0f, -0.8f, -3.0f, 0, 10000, 100, 1.0f, 30.0f, 3},
         {true, 20.0f, 12.0f, 0.0f, -4.0f, 0, 10000, 100, 1.2f, 25.0f, 3},
         {true, 40.0f, 10.0f, 0.8f, -5.0f, 0, 10000, 100, 0.8f, 35.0f, 3}},
        {{0, 1}, {0, 2}, {0, 3}, {1, 9}, {2, 9}, {3, 9}});

    // === RHYTHMIC ECHOES ===

    // 5. Dotted Eighth - classic Edge/U2 delay
    createPreset("05 - Dotted Eighth", "Factory", "Rhythmic",
                 {{true, 375.0f, 40.0f, 0.0f, -2.0f, 0, 8000, 100}},
                 {{0, 1}, {1, 9}});

    // 6. Ping Pong 8th - bouncing stereo
    createPreset("06 - Ping Pong 8th", "Factory", "Rhythmic",
                 {{true, 250.0f, 50.0f, -1.0f, -3.0f, 0, 10000, 80, 0, 0, 0,
                   false, true},
                  {true, 250.0f, 50.0f, 1.0f, -3.0f, 0, 10000, 80, 0, 0, 0,
                   false, true}},
                 {{0, 1}, {1, 2}, {2, 1}, {1, 9}, {2, 9}}); // Cross-feedback

    // 7. Polyrhythm - complex pattern
    createPreset("07 - Polyrhythm", "Factory", "Rhythmic",
                 {{true, 200.0f, 30.0f, -0.5f, -4.0f, 0},
                  {true, 300.0f, 30.0f, 0.0f, -4.0f, 0},
                  {true, 400.0f, 30.0f, 0.5f, -4.0f, 0}},
                 {{0, 1}, {0, 2}, {0, 3}, {1, 9}, {2, 9}, {3, 9}});

    // 8. Analog Echo - warm filtered
    createPreset(
        "08 - Analog Echo", "Factory", "Rhythmic",
        {{true, 350.0f, 55.0f, 0.0f, -1.0f, 1, 6000, 150}}, // Analog algo
        {{0, 1}, {1, 9}});

    // === VINTAGE TEXTURES ===

    // 9. Tape Echo - wow/flutter warmth
    createPreset(
        "09 - Tape Echo", "Factory", "Vintage",
        {{true, 400.0f, 45.0f, -0.3f, -2.0f, 2, 5000, 120, 0.3f, 8.0f, 1}},
        {{0, 1}, {1, 9}});

    // 10. Space Echo - dub-style series
    createPreset("10 - Space Echo", "Factory", "Vintage",
                 {{true, 250.0f, 35.0f, -0.5f, -3.0f, 1, 4000, 200},
                  {true, 500.0f, 40.0f, 0.5f, -4.0f, 1, 3500, 250}},
                 {{0, 1}, {1, 2}, {2, 9}}); // Series routing

    // 11. Ambient Wash - clean long tails
    createPreset("11 - Ambient Wash", "Factory", "Vintage",
                 {{true, 600.0f, 60.0f, -0.7f, -4.0f, 0, 12000, 40},
                  {true, 700.0f, 55.0f, 0.7f, -4.0f, 0, 12000, 40}},
                 {{0, 1}, {0, 2}, {1, 9}, {2, 9}});

    // 12. Lo-Fi Dreams - crushed and noisy
    createPreset(
        "12 - Lo-Fi Dreams", "Factory", "Vintage",
        {{true, 300.0f, 50.0f, 0.0f, -2.0f, 3, 4000, 300, 0.2f, 5.0f, 4}},
        {{0, 1}, {1, 9}});
  }

  /**
   * @brief Create a preset file with given configuration
   */
  void createPreset(const juce::String& name, const juce::String& author,
                    const juce::String& category,
                    std::initializer_list<BandConfig> bands,
                    std::initializer_list<std::pair<int, int>> routing) {
    auto file = userPresetDirectory_.getChildFile(name + ".udspreset");
    if (file.existsAsFile())
      return; // Don't overwrite existing presets

    auto xml = std::make_unique<juce::XmlElement>("UDSState");
    xml->setAttribute("presetName", name);
    xml->setAttribute("presetAuthor", author);
    xml->setAttribute("presetCategory", category);

    // Create APVTS subtree
    auto* apvtsXml = xml->createNewChildElement("UDSParameters");
    apvtsXml->setAttribute("mix", 50.0f);

    // Configure all 8 bands
    int bandIdx = 0;
    for (const auto& band : bands) {
      juce::String prefix = "band" + juce::String(bandIdx) + "_";
      apvtsXml->setAttribute(prefix + "enabled", band.enabled);
      apvtsXml->setAttribute(prefix + "time", band.timeMs);
      apvtsXml->setAttribute(prefix + "feedback", band.feedbackPct);
      apvtsXml->setAttribute(prefix + "pan", band.pan);
      apvtsXml->setAttribute(prefix + "level", band.levelDb);
      apvtsXml->setAttribute(prefix + "algorithm", band.algorithm);
      apvtsXml->setAttribute(prefix + "hiCut", band.hiCut);
      apvtsXml->setAttribute(prefix + "loCut", band.loCut);
      apvtsXml->setAttribute(prefix + "lfoRate", band.lfoRate);
      apvtsXml->setAttribute(prefix + "lfoDepth", band.lfoDepth);
      apvtsXml->setAttribute(prefix + "lfoWaveform", band.lfoWaveform);
      apvtsXml->setAttribute(prefix + "phaseInvert", band.phaseInvert);
      apvtsXml->setAttribute(prefix + "pingPong", band.pingPong);
      ++bandIdx;
    }

    // Disable remaining bands
    for (int i = bandIdx; i < 8; ++i) {
      juce::String prefix = "band" + juce::String(i) + "_";
      apvtsXml->setAttribute(prefix + "enabled", false);
    }

    // Create routing
    auto* routingXml = xml->createNewChildElement("Routing");
    for (const auto& conn : routing) {
      auto* connXml = routingXml->createNewChildElement("Connection");
      connXml->setAttribute("source", conn.first);
      connXml->setAttribute("dest", conn.second);
    }

    xml->writeTo(file);
  }

  UDSAudioProcessor& processor_;
  juce::File userPresetDirectory_;
  std::vector<PresetInfo> presets_;
  int currentPresetIndex_ = -1;
  bool isModified_ = false;
};

} // namespace uds
