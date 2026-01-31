#pragma once

#include "../Core/RoutingGraph.h"
#include "../Core/RoutingUndoManager.h"
#include "BandParameterPanel.h"
#include "ExpressionSlider.h"
#include "ExpressionTrainDialog.h"
#include "NodeEditorCanvas.h"
#include "PresetBrowserPanel.h"
#include "StandaloneMetronome.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <functional>
#include <memory>


namespace uds {

/**
 * @brief Main UI component for UDS with tabbed view (Bands / Routing)
 */
class MainComponent : public juce::Component {
public:
  MainComponent(juce::AudioProcessorValueTreeState& apvts,
                RoutingGraph& routingGraph, PresetManager& presetManager)
      : apvts_(apvts),
        routingGraph_(routingGraph),
        presetManager_(presetManager),
        presetBrowser_(presetManager) {
    // Sync UI when routing changes
    presetManager_.onRoutingChanged = [this] { syncRoutingFromProcessor(); };
    // Enable keyboard focus for shortcuts
    setWantsKeyboardFocus(true);


    // Preset browser panel
    addAndMakeVisible(presetBrowser_);

    // Global mix slider (with expression pedal support)
    mixSlider_.setParameterId("mix");
    mixSlider_.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    mixSlider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 18);
    mixSlider_.setColour(juce::Slider::rotarySliderFillColourId,
                         juce::Colour(0xff00b4d8));
    mixSlider_.setDoubleClickReturnValue(true,
                                         50.0); // Reset to 50% on double-click
    addAndMakeVisible(mixSlider_);
    mixAttachment_ =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, "mix", mixSlider_);

    mixLabel_.setText("Mix", juce::dontSendNotification);
    mixLabel_.setJustificationType(juce::Justification::centred);
    mixLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(mixLabel_);

    // Direct Level slider
    dryLevelSlider_.setParameterId("dryLevel");
    dryLevelSlider_.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    dryLevelSlider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 16);
    dryLevelSlider_.setColour(juce::Slider::rotarySliderFillColourId,
                              juce::Colour(0xff2ec4b6)); // Teal
    dryLevelSlider_.setDoubleClickReturnValue(true, 100.0);
    addAndMakeVisible(dryLevelSlider_);
    dryLevelAttachment_ =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, "dryLevel", dryLevelSlider_);

    dryLevelLabel_.setText("Direct", juce::dontSendNotification);
    dryLevelLabel_.setJustificationType(juce::Justification::centred);
    dryLevelLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
    dryLevelLabel_.setFont(juce::FontOptions(11.0f));
    addAndMakeVisible(dryLevelLabel_);

    // Direct Pan slider
    dryPanSlider_.setParameterId("dryPan");
    dryPanSlider_.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    dryPanSlider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 16);
    dryPanSlider_.setColour(juce::Slider::rotarySliderFillColourId,
                            juce::Colour(0xff2ec4b6)); // Teal
    dryPanSlider_.setDoubleClickReturnValue(true, 0.0);
    addAndMakeVisible(dryPanSlider_);
    dryPanAttachment_ =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, "dryPan", dryPanSlider_);

    dryPanLabel_.setText("D-Pan", juce::dontSendNotification);
    dryPanLabel_.setJustificationType(juce::Justification::centred);
    dryPanLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
    dryPanLabel_.setFont(juce::FontOptions(11.0f));
    addAndMakeVisible(dryPanLabel_);

    // Master LFO Rate slider
    masterLfoRateSlider_.setParameterId("masterLfoRate");
    masterLfoRateSlider_.setSliderStyle(
        juce::Slider::RotaryHorizontalVerticalDrag);
    masterLfoRateSlider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50,
                                         16);
    masterLfoRateSlider_.setColour(juce::Slider::rotarySliderFillColourId,
                                   juce::Colour(0xffe76f51));
    addAndMakeVisible(masterLfoRateSlider_);
    masterLfoRateAttachment_ =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, "masterLfoRate", masterLfoRateSlider_);

    masterLfoRateLabel_.setText("M-Rate", juce::dontSendNotification);
    masterLfoRateLabel_.setJustificationType(juce::Justification::centred);
    masterLfoRateLabel_.setColour(juce::Label::textColourId,
                                  juce::Colours::white);
    masterLfoRateLabel_.setFont(juce::FontOptions(11.0f));
    addAndMakeVisible(masterLfoRateLabel_);

    // Master LFO Depth slider
    masterLfoDepthSlider_.setParameterId("masterLfoDepth");
    masterLfoDepthSlider_.setSliderStyle(
        juce::Slider::RotaryHorizontalVerticalDrag);
    masterLfoDepthSlider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50,
                                          16);
    masterLfoDepthSlider_.setColour(juce::Slider::rotarySliderFillColourId,
                                    juce::Colour(0xffe76f51));
    addAndMakeVisible(masterLfoDepthSlider_);
    masterLfoDepthAttachment_ =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, "masterLfoDepth", masterLfoDepthSlider_);

    masterLfoDepthLabel_.setText("M-Depth", juce::dontSendNotification);
    masterLfoDepthLabel_.setJustificationType(juce::Justification::centred);
    masterLfoDepthLabel_.setColour(juce::Label::textColourId,
                                   juce::Colours::white);
    masterLfoDepthLabel_.setFont(juce::FontOptions(11.0f));
    addAndMakeVisible(masterLfoDepthLabel_);

    // Master LFO Waveform combo (0=None to disable)
    masterLfoWaveformCombo_.addItemList(
        {"None", "Sine", "Triangle", "Saw", "Square", "Brownian", "Lorenz"}, 1);
    addAndMakeVisible(masterLfoWaveformCombo_);
    masterLfoWaveformAttachment_ = std::make_unique<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, "masterLfoWaveform", masterLfoWaveformCombo_);

    masterLfoWaveformLabel_.setText("M-Wave", juce::dontSendNotification);
    masterLfoWaveformLabel_.setJustificationType(juce::Justification::centred);
    masterLfoWaveformLabel_.setColour(juce::Label::textColourId,
                                      juce::Colours::white);
    masterLfoWaveformLabel_.setFont(juce::FontOptions(11.0f));
    addAndMakeVisible(masterLfoWaveformLabel_);

    // I/O Mode combo (Auto, Mono, Mono→Stereo, Stereo)
    ioModeCombo_.addItemList({"Auto", "Mono", "Mono→Stereo", "Stereo"}, 1);
    addAndMakeVisible(ioModeCombo_);
    ioModeAttachment_ = std::make_unique<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "ioMode",
                                                                ioModeCombo_);

    ioModeLabel_.setText("I/O", juce::dontSendNotification);
    ioModeLabel_.setJustificationType(juce::Justification::centred);
    ioModeLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
    ioModeLabel_.setFont(juce::FontOptions(11.0f));
    addAndMakeVisible(ioModeLabel_);

    // Tab buttons
    bandsTabButton_.setButtonText("Bands");
    bandsTabButton_.onClick = [this] { showBandsView(); };
    addAndMakeVisible(bandsTabButton_);

    routingTabButton_.setButtonText("Routing");
    routingTabButton_.onClick = [this] { showRoutingView(); };
    routingTabButton_.setButtonText("Routing");
    routingTabButton_.onClick = [this] { showRoutingView(); };
    addAndMakeVisible(routingTabButton_);

    // Band Count Label (e.g. "8/12 Active")
    bandCountLabel_.setJustificationType(juce::Justification::centredLeft);
    bandCountLabel_.setColour(juce::Label::textColourId,
                              juce::Colours::white.withAlpha(0.6f));
    bandCountLabel_.setFont(juce::FontOptions(12.0f));
    addAndMakeVisible(bandCountLabel_);

    // Routing preset buttons
    parallelButton_.setButtonText("Parallel");
    parallelButton_.onClick = [this] {
      saveRoutingForUndo();
      nodeEditor_.setParallelRouting();
      syncRoutingToProcessor();
    };
    addAndMakeVisible(parallelButton_);

    seriesButton_.setButtonText("Series");
    seriesButton_.onClick = [this] {
      saveRoutingForUndo();
      nodeEditor_.setSeriesRouting();
      syncRoutingToProcessor();
    };
    addAndMakeVisible(seriesButton_);

    // Undo/Redo buttons
    undoButton_.setButtonText("Undo");
    undoButton_.onClick = [this] {
      if (undoManager_.undo(nodeEditor_.getRoutingGraph())) {
        syncRoutingToProcessor();
        nodeEditor_.rebuildCablesFromRouting();
      }
    };
    addAndMakeVisible(undoButton_);

    redoButton_.setButtonText("Redo");
    redoButton_.onClick = [this] {
      if (undoManager_.redo(nodeEditor_.getRoutingGraph())) {
        syncRoutingToProcessor();
        nodeEditor_.rebuildCablesFromRouting();
      }
    };
    addAndMakeVisible(redoButton_);

    // Create band panels (12 total, 9-12 hidden initially)
    for (int i = 0; i < 12; ++i) {
      bandPanels_[static_cast<size_t>(i)] =
          std::make_unique<BandParameterPanel>(apvts, i);
      if (i < 8) {
        addAndMakeVisible(*bandPanels_[static_cast<size_t>(i)]);
      } else {
        addChildComponent(*bandPanels_[static_cast<size_t>(i)]); // Hidden
      }
    }

    // Node editor canvas
    nodeEditor_.onRoutingWillChange = [this] { saveRoutingForUndo(); };
    nodeEditor_.onRoutingChanged = [this] {
      syncRoutingToProcessor();
      updateBandPanelVisibility(); // Sync band panels with active bands
    };
    addAndMakeVisible(nodeEditor_);

    // Initialize UI routing from processor's routing graph
    syncRoutingFromProcessor();

    // Metronome for standalone testing
    addAndMakeVisible(metronome_);

    // Start with bands view
    showBandsView();

    // Initialize band panel visibility based on active bands
    updateBandPanelVisibility();
  }

  ~MainComponent() override { presetManager_.onRoutingChanged = nullptr; }

  void resized() override {
    auto bounds = getLocalBounds().reduced(10);

    // Header row
    auto headerRow = bounds.removeFromTop(50);

    // Mix control on right
    auto mixArea = headerRow.removeFromRight(70);
    mixLabel_.setBounds(mixArea.removeFromTop(16));
    mixSlider_.setBounds(mixArea);

    // Direct Level control (next to mix)
    auto dryLevelArea = headerRow.removeFromRight(60);
    dryLevelLabel_.setBounds(dryLevelArea.removeFromTop(16));
    dryLevelSlider_.setBounds(dryLevelArea);

    // Direct Pan control (next to dry level)
    auto dryPanArea = headerRow.removeFromRight(60);
    dryPanLabel_.setBounds(dryPanArea.removeFromTop(16));
    dryPanSlider_.setBounds(dryPanArea);

    // I/O Mode control
    auto ioModeArea = headerRow.removeFromRight(80);
    ioModeLabel_.setBounds(ioModeArea.removeFromTop(16));
    ioModeCombo_.setBounds(ioModeArea.reduced(2, 8));

    // Master LFO controls (next to mix)
    auto masterLfoWaveArea = headerRow.removeFromRight(70);
    masterLfoWaveformLabel_.setBounds(masterLfoWaveArea.removeFromTop(16));
    masterLfoWaveformCombo_.setBounds(masterLfoWaveArea.reduced(2, 8));

    auto masterLfoDepthArea = headerRow.removeFromRight(60);
    masterLfoDepthLabel_.setBounds(masterLfoDepthArea.removeFromTop(16));
    masterLfoDepthSlider_.setBounds(masterLfoDepthArea);

    auto masterLfoRateArea = headerRow.removeFromRight(60);
    masterLfoRateLabel_.setBounds(masterLfoRateArea.removeFromTop(16));
    masterLfoRateSlider_.setBounds(masterLfoRateArea);

    // Tab buttons on left
    auto tabArea = headerRow.removeFromLeft(200);
    bandsTabButton_.setBounds(tabArea.removeFromLeft(80)); // Slightly smaller
    routingTabButton_.setBounds(tabArea.removeFromLeft(80));

    // Band Count Label next to tabs
    bandCountLabel_.setBounds(headerRow.removeFromLeft(80).reduced(5, 12));

    // Preset buttons (visible only in routing view)
    // Preset buttons (visible only in routing view)
    if (showRoutingView_) {
      auto presetArea = headerRow.removeFromRight(240);
      parallelButton_.setBounds(presetArea.removeFromLeft(60));
      seriesButton_.setBounds(presetArea.removeFromLeft(60));
      undoButton_.setBounds(presetArea.removeFromLeft(60));
      redoButton_.setBounds(presetArea.removeFromLeft(60));
    }

    parallelButton_.setVisible(showRoutingView_);
    seriesButton_.setVisible(showRoutingView_);
    undoButton_.setVisible(showRoutingView_);
    redoButton_.setVisible(showRoutingView_);


    // Metronome for standalone testing (before preset browser)
    metronome_.setBounds(headerRow.removeFromLeft(200).reduced(5, 8));

    // Preset browser in remaining center space
    presetBrowser_.setBounds(headerRow.reduced(10, 8));

    bounds.removeFromTop(10);

    if (showRoutingView_) {
      // Routing editor fills remaining space
      nodeEditor_.setBounds(bounds);
    } else {
      // Band panels in dynamic grid (only visible panels)
      // Count visible panels
      std::vector<BandParameterPanel*> visiblePanels;
      for (int i = 0; i < 12; ++i) {
        if (bandPanels_[static_cast<size_t>(i)] &&
            bandPanels_[static_cast<size_t>(i)]->isVisible()) {
          visiblePanels.push_back(bandPanels_[static_cast<size_t>(i)].get());
        }
      }

      // Layout in grid: 4 columns, rows as needed
      const int columns = 4;
      const int rows =
          (static_cast<int>(visiblePanels.size()) + columns - 1) / columns;
      if (rows > 0) {
        const int panelWidth = bounds.getWidth() / columns;
        const int panelHeight = bounds.getHeight() / rows;

        for (size_t i = 0; i < visiblePanels.size(); ++i) {
          int row = static_cast<int>(i) / columns;
          int col = static_cast<int>(i) % columns;
          visiblePanels[i]->setBounds(bounds.getX() + col * panelWidth,
                                      bounds.getY() + row * panelHeight,
                                      panelWidth - 5, panelHeight - 5);
        }
      }
    }
  }

  void paint(juce::Graphics& g) override {
    // 1. Background Gradient (Dark Industrial)
    juce::Colour color1 = juce::Colour(0xff2b2b2b); // Lighter Charcoal
    juce::Colour color2 = juce::Colour(0xff0d0d0d); // Deep Black
    // Diagonal gradient for depth
    juce::ColourGradient grad(color1, 0, 0, color2, (float)getWidth(),
                              (float)getHeight(), false);
    g.setGradientFill(grad);
    g.fillAll();

    // 2. Technical Grid Overlay
    g.setColour(
        juce::Colours::white.withAlpha(0.04f)); // Very faint technical lines
    const int gridSize = 40;

    // Vertical lines
    for (int x = 0; x < getWidth(); x += gridSize)
      g.drawVerticalLine(x, 0.0f, (float)getHeight());

    // Horizontal lines
    for (int y = 0; y < getHeight(); y += gridSize)
      g.drawHorizontalLine(y, 0.0f, (float)getWidth());
  }

  /**
   * @brief Handle keyboard shortcuts for undo/redo routing changes.
   */
  bool keyPressed(const juce::KeyPress& key) override {
    // Ctrl+Z = Undo (when in routing view)
    if (key.getModifiers().isCommandDown() && key.getKeyCode() == 'Z') {
      if (key.getModifiers().isShiftDown()) {
        // Ctrl+Shift+Z = Redo
        if (undoManager_.redo(nodeEditor_.getRoutingGraph())) {
          nodeEditor_.repaint();
        }
        return true;
      } else {
        // Ctrl+Z = Undo
        if (undoManager_.undo(nodeEditor_.getRoutingGraph())) {
          nodeEditor_.repaint();
        }
        return true;
      }
    }
    return false;
  }

  // Access routing graph for processor sync
  RoutingGraph& getRoutingGraph() { return nodeEditor_.getRoutingGraph(); }

private:
  void showBandsView() {
    showRoutingView_ = false;
    nodeEditor_.setVisible(false);
    updateBandPanelVisibility(); // Only show active bands
    updateTabButtonColors();
    resized();
  }

  void showRoutingView() {
    showRoutingView_ = true;
    nodeEditor_.setVisible(true);
    for (auto& panel : bandPanels_) {
      panel->setVisible(false);
    }
    updateTabButtonColors();
    resized();
  }

  void updateTabButtonColors() {
    auto activeColor = juce::Colour(0xff00b4d8);
    auto inactiveColor = juce::Colour(0xff444444);

    bandsTabButton_.setColour(juce::TextButton::buttonColourId,
                              showRoutingView_ ? inactiveColor : activeColor);
    routingTabButton_.setColour(juce::TextButton::buttonColourId,
                                showRoutingView_ ? activeColor : inactiveColor);
  }

  /**
   * @brief Save current routing state for undo before making changes.
   */
  void saveRoutingForUndo() {
    undoManager_.saveState(nodeEditor_.getRoutingGraph());
  }

  /**
   * @brief Sync UI routing changes to the processor's routing graph
   */
  void syncRoutingToProcessor() {
    // Get connections from the node editor canvas
    const auto& uiConnections = nodeEditor_.getRoutingGraph().getConnections();

    // Clear and rebuild processor's routing graph
    routingGraph_.clearAllConnections();

    for (const auto& conn : uiConnections) {
      routingGraph_.connect(conn.sourceId, conn.destId);
    }

    // Also sync the active bands from UI to processor
    routingGraph_.setActiveBands(
        nodeEditor_.getRoutingGraph().getActiveBands());
  }

  /**
   * @brief Initialize UI routing from processor's routing graph
   */
  void syncRoutingFromProcessor() {
    // Get connections from processor
    const auto& procConnections = routingGraph_.getConnections();

    // Update node editor's routing graph
    auto& uiRouting = nodeEditor_.getRoutingGraph();
    uiRouting.clearAllConnections();

    for (const auto& conn : procConnections) {
      uiRouting.connect(conn.sourceId, conn.destId);
    }

    // Also sync the active bands
    uiRouting.setActiveBands(routingGraph_.getActiveBands());

    // Update the visual cables in the canvas
    nodeEditor_.rebuildCablesFromRouting();

    // Update band visibility and panel visibility
    nodeEditor_.updateBandVisibility();
    updateBandPanelVisibility();
  }

  juce::AudioProcessorValueTreeState& apvts_;
  RoutingGraph& routingGraph_;
  PresetManager& presetManager_;


  ExpressionSlider mixSlider_;
  juce::Label mixLabel_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      mixAttachment_;

  // Direct signal controls
  ExpressionSlider dryLevelSlider_;
  ExpressionSlider dryPanSlider_;
  juce::Label dryLevelLabel_;
  juce::Label dryPanLabel_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      dryLevelAttachment_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      dryPanAttachment_;

  // Master LFO controls
  ExpressionSlider masterLfoRateSlider_;
  ExpressionSlider masterLfoDepthSlider_;
  juce::ComboBox masterLfoWaveformCombo_;
  juce::Label masterLfoRateLabel_;
  juce::Label masterLfoDepthLabel_;
  juce::Label masterLfoWaveformLabel_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      masterLfoRateAttachment_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      masterLfoDepthAttachment_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
      masterLfoWaveformAttachment_;

  // I/O Mode controls
  juce::ComboBox ioModeCombo_;
  juce::Label ioModeLabel_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
      ioModeAttachment_;

  juce::TextButton bandsTabButton_;
  juce::TextButton routingTabButton_;
  juce::TextButton parallelButton_;
  juce::TextButton seriesButton_;
  juce::TextButton undoButton_;
  juce::TextButton redoButton_;
  juce::Label bandCountLabel_;
  RoutingUndoManager undoManager_;
  bool showRoutingView_ = false;

  std::array<std::unique_ptr<BandParameterPanel>, 12> bandPanels_;
  NodeEditorCanvas nodeEditor_;
  PresetBrowserPanel presetBrowser_;
  StandaloneMetronome metronome_;

public:
  // Access metronome for BPM and click generation
  StandaloneMetronome& getMetronome() { return metronome_; }

  // Update band signal levels for LED activity indicators
  void updateBandLevels(const std::array<float, 12>& levels) {
    for (size_t i = 0; i < 12; ++i) {
      if (bandPanels_[i])
        bandPanels_[i]->setSignalLevel(levels[i]);
    }
  }

  // Update band panel visibility based on active bands in routing graph
  void updateBandPanelVisibility() {
    const auto& routingGraph = nodeEditor_.getRoutingGraph();
    int activeCount = 0;
    for (int i = 0; i < 12; ++i) {
      int bandId = i + 1; // Band IDs are 1-based
      bool isActive = routingGraph.isBandActive(bandId);
      if (isActive)
        activeCount++;
      if (bandPanels_[static_cast<size_t>(i)]) {
        bandPanels_[static_cast<size_t>(i)]->setVisible(isActive);
      }
    }

    // Update count label
    bandCountLabel_.setText(juce::String(activeCount) + "/12 Active",
                            juce::dontSendNotification);

    resized(); // Re-layout to fill gaps
  }

  // Wire expression pedal callbacks for all ExpressionSliders
  void setExpressionCallbacks(
      std::function<void(const juce::String&, float, float)> onAssign,
      std::function<void()> onClear,
      std::function<bool(const juce::String&)> hasMapping,
      std::function<float()> getExprValue) {

    // Helper to wire a slider with Train Range support
    auto wireSlider = [&](ExpressionSlider& slider, float defaultMin,
                          float defaultMax) {
      slider.onAssignExpression = [onAssign, defaultMin,
                                   defaultMax](const juce::String& paramId) {
        onAssign(paramId, defaultMin, defaultMax);
      };
      slider.onClearExpression = onClear;
      slider.isExpressionAssigned = [hasMapping, &slider]() {
        return hasMapping(slider.getParameterId());
      };
      slider.onTrainRange = [this, onAssign,
                             getExprValue](const juce::String& paramId) {
        showExpressionTrainDialog(
            this, getExprValue,
            [onAssign, paramId](float minVal, float maxVal) {
              onAssign(paramId, minVal, maxVal);
            });
      };
    };

    // Wire all ExpressionSliders with their default ranges
    wireSlider(mixSlider_, 0.0f, 100.0f);            // Mix: 0-100%
    wireSlider(dryLevelSlider_, 0.0f, 100.0f);       // Direct Level: 0-100%
    wireSlider(dryPanSlider_, -100.0f, 100.0f);      // Direct Pan: L100-R100
    wireSlider(masterLfoRateSlider_, 0.0f, 10.0f);   // LFO Rate: 0-10 Hz
    wireSlider(masterLfoDepthSlider_, 0.0f, 100.0f); // LFO Depth: 0-100%

    // Wire all band panels (9 sliders per band × 8 bands = 72 expression
    // targets)
    for (auto& panel : bandPanels_) {
      if (panel)
        panel->setExpressionCallbacks(onAssign, onClear, hasMapping,
                                      getExprValue);
    }
  }
};

} // namespace uds
