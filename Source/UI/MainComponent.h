#pragma once

#include "../Core/RoutingGraph.h"
#include "../Core/RoutingUndoManager.h"
#include "BandParameterPanel.h"
#include "NodeEditorCanvas.h"
#include "PresetBrowserPanel.h"
#include "StandaloneMetronome.h"
#include <array>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

namespace uds {

/**
 * @brief Main UI component for UDS with tabbed view (Bands / Routing)
 */
class MainComponent : public juce::Component {
public:
  MainComponent(juce::AudioProcessorValueTreeState &apvts,
                RoutingGraph &routingGraph, PresetManager &presetManager)
      : apvts_(apvts), routingGraph_(routingGraph),
        presetBrowser_(presetManager) {
    // Enable keyboard focus for shortcuts
    setWantsKeyboardFocus(true);

    // Title
    titleLabel_.setText("UDS - Universal Delay System",
                        juce::dontSendNotification);
    titleLabel_.setJustificationType(juce::Justification::centred);
    titleLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
    titleLabel_.setFont(juce::FontOptions(18.0f).withStyle("Bold"));
    addAndMakeVisible(titleLabel_);

    // Preset browser panel
    addAndMakeVisible(presetBrowser_);

    // Global mix slider
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

    // Tab buttons
    bandsTabButton_.setButtonText("Bands");
    bandsTabButton_.onClick = [this] { showBandsView(); };
    addAndMakeVisible(bandsTabButton_);

    routingTabButton_.setButtonText("Routing");
    routingTabButton_.onClick = [this] { showRoutingView(); };
    addAndMakeVisible(routingTabButton_);

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

    // Create band panels
    for (int i = 0; i < 8; ++i) {
      bandPanels_[static_cast<size_t>(i)] =
          std::make_unique<BandParameterPanel>(apvts, i);
      addAndMakeVisible(*bandPanels_[static_cast<size_t>(i)]);
    }

    // Node editor canvas
    nodeEditor_.onRoutingWillChange = [this] { saveRoutingForUndo(); };
    nodeEditor_.onRoutingChanged = [this] { syncRoutingToProcessor(); };
    addAndMakeVisible(nodeEditor_);

    // Initialize UI routing from processor's routing graph
    syncRoutingFromProcessor();

    // Metronome for standalone testing
    addAndMakeVisible(metronome_);

    // Start with bands view
    showBandsView();
  }

  ~MainComponent() override = default;

  void resized() override {
    auto bounds = getLocalBounds().reduced(10);

    // Header row
    auto headerRow = bounds.removeFromTop(50);

    // Mix control on right
    auto mixArea = headerRow.removeFromRight(80);
    mixLabel_.setBounds(mixArea.removeFromTop(18));
    mixSlider_.setBounds(mixArea);

    // Tab buttons on left
    auto tabArea = headerRow.removeFromLeft(200);
    bandsTabButton_.setBounds(tabArea.removeFromLeft(100));
    routingTabButton_.setBounds(tabArea.removeFromLeft(100));

    // Preset buttons (visible only in routing view)
    auto presetArea = headerRow.removeFromRight(240);
    parallelButton_.setBounds(presetArea.removeFromLeft(60));
    seriesButton_.setBounds(presetArea.removeFromLeft(60));
    undoButton_.setBounds(presetArea.removeFromLeft(60));
    redoButton_.setBounds(presetArea.removeFromLeft(60));
    parallelButton_.setVisible(showRoutingView_);
    seriesButton_.setVisible(showRoutingView_);
    undoButton_.setVisible(showRoutingView_);
    redoButton_.setVisible(showRoutingView_);

    // Title in center
    titleLabel_.setBounds(headerRow.removeFromLeft(220));

    // Metronome for standalone testing (before preset browser)
    metronome_.setBounds(headerRow.removeFromLeft(200).reduced(5, 8));

    // Preset browser in remaining center space
    presetBrowser_.setBounds(headerRow.reduced(10, 8));

    bounds.removeFromTop(10);

    if (showRoutingView_) {
      // Routing editor fills remaining space
      nodeEditor_.setBounds(bounds);
    } else {
      // Band panels in 2x4 grid
      const int panelWidth = bounds.getWidth() / 4;
      const int panelHeight = bounds.getHeight() / 2;

      for (int row = 0; row < 2; ++row) {
        for (int col = 0; col < 4; ++col) {
          int index = row * 4 + col;
          bandPanels_[static_cast<size_t>(index)]->setBounds(
              bounds.getX() + col * panelWidth,
              bounds.getY() + row * panelHeight, panelWidth - 5,
              panelHeight - 5);
        }
      }
    }
  }

  void paint(juce::Graphics &g) override {
    g.fillAll(juce::Colour(0xff1a1a1a));
  }

  /**
   * @brief Handle keyboard shortcuts for undo/redo routing changes.
   */
  bool keyPressed(const juce::KeyPress &key) override {
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
  RoutingGraph &getRoutingGraph() { return nodeEditor_.getRoutingGraph(); }

private:
  void showBandsView() {
    showRoutingView_ = false;
    nodeEditor_.setVisible(false);
    for (auto &panel : bandPanels_) {
      panel->setVisible(true);
    }
    updateTabButtonColors();
    resized();
  }

  void showRoutingView() {
    showRoutingView_ = true;
    nodeEditor_.setVisible(true);
    for (auto &panel : bandPanels_) {
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
    const auto &uiConnections = nodeEditor_.getRoutingGraph().getConnections();

    // Clear and rebuild processor's routing graph
    routingGraph_.clearAllConnections();

    for (const auto &conn : uiConnections) {
      routingGraph_.connect(conn.sourceId, conn.destId);
    }
  }

  /**
   * @brief Initialize UI routing from processor's routing graph
   */
  void syncRoutingFromProcessor() {
    // Get connections from processor
    const auto &procConnections = routingGraph_.getConnections();

    // Update node editor's routing graph
    auto &uiRouting = nodeEditor_.getRoutingGraph();
    uiRouting.clearAllConnections();

    for (const auto &conn : procConnections) {
      uiRouting.connect(conn.sourceId, conn.destId);
    }

    // Update the visual cables in the canvas
    nodeEditor_.rebuildCablesFromRouting();
  }

  juce::AudioProcessorValueTreeState &apvts_;
  RoutingGraph &routingGraph_;

  juce::Label titleLabel_;
  juce::Slider mixSlider_;
  juce::Label mixLabel_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      mixAttachment_;

  juce::TextButton bandsTabButton_;
  juce::TextButton routingTabButton_;
  juce::TextButton parallelButton_;
  juce::TextButton seriesButton_;
  juce::TextButton undoButton_;
  juce::TextButton redoButton_;
  RoutingUndoManager undoManager_;
  bool showRoutingView_ = false;

  std::array<std::unique_ptr<BandParameterPanel>, 8> bandPanels_;
  NodeEditorCanvas nodeEditor_;
  PresetBrowserPanel presetBrowser_;
  StandaloneMetronome metronome_;

public:
  // Access metronome for BPM and click generation
  StandaloneMetronome &getMetronome() { return metronome_; }

  // Update band signal levels for LED activity indicators
  void updateBandLevels(const std::array<float, 8> &levels) {
    for (size_t i = 0; i < 8; ++i) {
      if (bandPanels_[i])
        bandPanels_[i]->setSignalLevel(levels[i]);
    }
  }
};

} // namespace uds
