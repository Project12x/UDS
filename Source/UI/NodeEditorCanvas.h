#pragma once

#include "../Core/RoutingGraph.h"
#include "BandNodeComponent.h"
#include "NodeVisual.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>
#include <vector>


namespace uds {

/**
 * @brief Main canvas for the visual node editor.
 *
 * Features:
 * - Displays Input, Band (8), and Output nodes
 * - Draws bezier cables between connected nodes
 * - Handles cable creation via drag
 * - Syncs with RoutingGraph for audio routing
 */
class NodeEditorCanvas : public juce::Component, private juce::Timer {
public:
  // Callback when routing is ABOUT to change (for undo capture)
  std::function<void()> onRoutingWillChange;
  // Callback when routing changes (for audio engine update)
  std::function<void()> onRoutingChanged;

  NodeEditorCanvas() {
    setWantsKeyboardFocus(true);

    // Create Input node (far left, centered)
    inputNode_ = std::make_unique<BandNodeComponent>(
        static_cast<int>(NodeId::Input), "INPUT",
        BandNodeComponent::NodeType::Input);
    inputNode_->setTopLeftPosition(40, 180);
    setupNodeCallbacks(inputNode_.get());
    addAndMakeVisible(inputNode_.get());

    // Create 8 Band nodes in staggered layout (not a grid!)
    // Positions form a flowing pattern between Input and Output
    const std::array<juce::Point<int>, 8> bandPositions = {{
        {200, 60},  // Band 1 - top left
        {200, 180}, // Band 2 - middle left
        {200, 300}, // Band 3 - bottom left
        {380, 120}, // Band 4 - top center
        {380, 240}, // Band 5 - bottom center
        {560, 60},  // Band 6 - top right
        {560, 180}, // Band 7 - middle right
        {560, 300}, // Band 8 - bottom right
    }};

    for (int i = 0; i < kNumBands; ++i) {
      auto band = std::make_unique<BandNodeComponent>(
          i + 1, "Band " + juce::String(i + 1),
          BandNodeComponent::NodeType::Band);

      band->setTopLeftPosition(bandPositions[static_cast<size_t>(i)].x,
                               bandPositions[static_cast<size_t>(i)].y);
      setupNodeCallbacks(band.get());
      addAndMakeVisible(band.get());
      bandNodes_.push_back(std::move(band));
    }

    // Create Output node (far right, centered)
    outputNode_ = std::make_unique<BandNodeComponent>(
        static_cast<int>(NodeId::Output), "OUTPUT",
        BandNodeComponent::NodeType::Output);
    outputNode_->setTopLeftPosition(740, 180);
    setupNodeCallbacks(outputNode_.get());
    addAndMakeVisible(outputNode_.get());

    // Initialize with default parallel routing
    routingGraph_.setDefaultParallelRouting();
    updateCableVisuals();

    // Start timer for smooth cable updates (30fps)
    startTimerHz(30);
  }

  ~NodeEditorCanvas() override { stopTimer(); }

  void paint(juce::Graphics& g) override {
    // Background (drawn without transform)
    g.fillAll(theme_.background);

    // Apply zoom transform
    g.addTransform(juce::AffineTransform::scale(zoomLevel_, zoomLevel_));

    // Grid (scaled)
    g.setColour(theme_.gridLines);
    int gridSize = static_cast<int>(20 / zoomLevel_);
    for (int x = 0; x < static_cast<int>(getWidth() / zoomLevel_);
         x += gridSize) {
      g.drawVerticalLine(x, 0, static_cast<float>(getHeight() / zoomLevel_));
    }
    for (int y = 0; y < static_cast<int>(getHeight() / zoomLevel_);
         y += gridSize) {
      g.drawHorizontalLine(y, 0, static_cast<float>(getWidth() / zoomLevel_));
    }

    // Draw cables (behind nodes)
    for (const auto& cable : cables_) {
      drawCable(g, cable);
    }

    // Draw drag cable if active
    if (dragCable_.active) {
      drawDragCable(g);
    }
  }

  void mouseWheelMove(const juce::MouseEvent& event,
                      const juce::MouseWheelDetails& wheel) override {
    juce::ignoreUnused(event);

    // Zoom with mouse wheel (Ctrl + scroll for zoom, or just scroll if
    // preferred)
    float delta = wheel.deltaY * 0.1f;
    float newZoom = juce::jlimit(0.5f, 2.0f, zoomLevel_ + delta);

    if (newZoom != zoomLevel_) {
      zoomLevel_ = newZoom;

      // Scale node positions proportionally
      updateNodeTransforms();
      repaint();
    }
  }

  void mouseDown(const juce::MouseEvent& event) override {
    if (event.mods.isRightButtonDown()) {
      // Right-click: check if clicking on a cable to delete it
      auto cableIndex = findCableAtPosition(event.getPosition().toFloat());
      if (cableIndex >= 0) {
        // Delete the cable
        if (onRoutingWillChange)
          onRoutingWillChange();
        auto& cable = cables_[static_cast<size_t>(cableIndex)];
        routingGraph_.disconnect(cable.sourceNodeId, cable.destNodeId);
        updateCableVisuals();
        if (onRoutingChanged)
          onRoutingChanged();
        repaint();
      } else {
        // Start panning
        isPanning_ = true;
        lastMousePos_ = event.getPosition();
      }
    } else {
      // Left-click: deselect all nodes
      deselectAll();
    }
  }

  void mouseDrag(const juce::MouseEvent& event) override {
    if (isPanning_) {
      auto delta = event.getPosition() - lastMousePos_;
      panOffset_ += delta;
      lastMousePos_ = event.getPosition();

      // Move all nodes by delta
      inputNode_->setTopLeftPosition(inputNode_->getPosition() + delta);
      outputNode_->setTopLeftPosition(outputNode_->getPosition() + delta);
      for (auto& band : bandNodes_) {
        band->setTopLeftPosition(band->getPosition() + delta);
      }
      repaint();
    } else if (dragCable_.active) {
      dragCable_.currentPos = event.getPosition();
      repaint();
    }
  }

  void mouseUp(const juce::MouseEvent& event) override {
    if (isPanning_) {
      isPanning_ = false;
    } else if (dragCable_.active) {
      // Check if we're over a valid port
      auto targetNode = findNodeAtPort(event.getPosition());
      if (targetNode.first >= 0) {
        bool targetIsOutput = targetNode.second;

        // Valid connection: output -> input
        if (dragCable_.fromOutput && !targetIsOutput) {
          if (onRoutingWillChange)
            onRoutingWillChange();
          routingGraph_.connect(dragCable_.sourceNodeId, targetNode.first);
          updateCableVisuals();
          if (onRoutingChanged)
            onRoutingChanged();
        }
        // Or input -> output (reverse direction)
        else if (!dragCable_.fromOutput && targetIsOutput) {
          if (onRoutingWillChange)
            onRoutingWillChange();
          routingGraph_.connect(targetNode.first, dragCable_.sourceNodeId);
          updateCableVisuals();
          if (onRoutingChanged)
            onRoutingChanged();
        }
      }

      dragCable_.active = false;
      repaint();
    }
  }

  void resized() override {
    // Nodes maintain their positions from construction
  }

  // Get routing graph for audio processing
  RoutingGraph& getRoutingGraph() { return routingGraph_; }
  const RoutingGraph& getRoutingGraph() const { return routingGraph_; }

  // Preset routing buttons
  void setParallelRouting() {
    routingGraph_.setDefaultParallelRouting();
    updateCableVisuals();
    if (onRoutingChanged)
      onRoutingChanged();
    repaint();
  }

  void setSeriesRouting() {
    routingGraph_.setSeriesRouting();
    updateCableVisuals();
    if (onRoutingChanged)
      onRoutingChanged();
    repaint();
  }

  void clearRouting() {
    routingGraph_.clear();
    updateCableVisuals();
    if (onRoutingChanged)
      onRoutingChanged();
    repaint();
  }

  /**
   * @brief Rebuild visual cables from the routing graph
   *
   * Call this after externally modifying the routing graph to update the UI.
   */
  void rebuildCablesFromRouting() {
    updateCableVisuals();
    repaint();
  }

  // Update band enabled state
  void setBandEnabled(int bandIndex, bool enabled) {
    if (bandIndex >= 0 && bandIndex < static_cast<int>(bandNodes_.size())) {
      bandNodes_[static_cast<size_t>(bandIndex)]->setEnabled(enabled);
    }
  }

private:
  NodeEditorTheme theme_;
  RoutingGraph routingGraph_;
  std::vector<CableVisual> cables_;
  DragCable dragCable_;
  float zoomLevel_ = 1.0f;

  // Pan state
  bool isPanning_ = false;
  juce::Point<int> lastMousePos_;
  juce::Point<int> panOffset_{0, 0};

  std::unique_ptr<BandNodeComponent> inputNode_;
  std::vector<std::unique_ptr<BandNodeComponent>> bandNodes_;
  std::unique_ptr<BandNodeComponent> outputNode_;

  void timerCallback() override {
    // Update cable positions every frame for smooth animation
    updateCableVisuals();
    repaint();
  }

  // Find cable index at given position, returns -1 if none found
  int findCableAtPosition(juce::Point<float> pos) const {
    const float hitDistance = 8.0f; // Pixels from cable center

    for (size_t i = 0; i < cables_.size(); ++i) {
      const auto& cable = cables_[i];

      // Sample points along bezier curve and check distance
      for (float t = 0.0f; t <= 1.0f; t += 0.05f) {
        auto pt = bezierPoint(cable.start, cable.control1, cable.control2,
                              cable.end, t);
        if (pt.getDistanceFrom(pos) < hitDistance) {
          return static_cast<int>(i);
        }
      }
    }
    return -1;
  }

  // Evaluate cubic bezier at t
  static juce::Point<float> bezierPoint(juce::Point<float> p0,
                                        juce::Point<float> p1,
                                        juce::Point<float> p2,
                                        juce::Point<float> p3, float t) {
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;

    return p0 * uuu + p1 * (3 * uu * t) + p2 * (3 * u * tt) + p3 * ttt;
  }

  void updateNodeTransforms() {
    // Apply zoom to node components via setTransform
    auto transform = juce::AffineTransform::scale(zoomLevel_);
    inputNode_->setTransform(transform);
    outputNode_->setTransform(transform);
    for (auto& band : bandNodes_) {
      band->setTransform(transform);
    }
  }

  void setupNodeCallbacks(BandNodeComponent* node) {
    node->onPortClicked = [this](int nodeId, bool isOutput,
                                 juce::Point<int> pos) {
      // Start cable drag
      dragCable_.active = true;
      dragCable_.sourceNodeId = nodeId;
      dragCable_.fromOutput = isOutput;
      dragCable_.startPos = pos;
      dragCable_.currentPos = pos;
    };

    node->onNodeSelected = [this](int nodeId) {
      deselectAll();
      if (auto* n = findNodeById(nodeId)) {
        n->setSelected(true);
      }
    };

    node->onDragEnd = [this](int /*nodeId*/, juce::Point<int> /*newPos*/) {
      // Cable positions already updated by timer
    };
  }

  BandNodeComponent* findNodeById(int nodeId) {
    if (nodeId == static_cast<int>(NodeId::Input))
      return inputNode_.get();
    if (nodeId == static_cast<int>(NodeId::Output))
      return outputNode_.get();
    if (nodeId >= 1 && nodeId <= kNumBands) {
      return bandNodes_[static_cast<size_t>(nodeId - 1)].get();
    }
    return nullptr;
  }

  void deselectAll() {
    inputNode_->setSelected(false);
    outputNode_->setSelected(false);
    for (auto& band : bandNodes_) {
      band->setSelected(false);
    }
  }

  // Returns {nodeId, isOutputPort} or {-1, false} if no port found
  std::pair<int, bool> findNodeAtPort(juce::Point<int> pos) {
    const float hitRadius = theme_.portRadius + 8;

    // Check input node's output port
    auto outPos = inputNode_->getOutputPortPosition();
    if (pos.getDistanceFrom(outPos) < hitRadius) {
      return {static_cast<int>(NodeId::Input), true};
    }

    // Check output node's input port
    auto inPos = outputNode_->getInputPortPosition();
    if (pos.getDistanceFrom(inPos) < hitRadius) {
      return {static_cast<int>(NodeId::Output), false};
    }

    // Check band nodes
    for (size_t i = 0; i < bandNodes_.size(); ++i) {
      auto& band = bandNodes_[i];
      int nodeId = static_cast<int>(i) + 1;

      auto bandInPos = band->getInputPortPosition();
      if (pos.getDistanceFrom(bandInPos) < hitRadius) {
        return {nodeId, false};
      }

      auto bandOutPos = band->getOutputPortPosition();
      if (pos.getDistanceFrom(bandOutPos) < hitRadius) {
        return {nodeId, true};
      }
    }

    return {-1, false};
  }

  void updateCableVisuals() {
    cables_.clear();

    for (const auto& conn : routingGraph_.getConnections()) {
      CableVisual cable;
      cable.sourceNodeId = conn.sourceId;
      cable.destNodeId = conn.destId;
      // Use per-band color based on source node
      cable.color = theme_.getCableColorForSource(conn.sourceId);

      // Get port positions
      if (auto* srcNode = findNodeById(conn.sourceId)) {
        auto pos = srcNode->getOutputPortPosition();
        cable.start = pos.toFloat();
      }
      if (auto* dstNode = findNodeById(conn.destId)) {
        auto pos = dstNode->getInputPortPosition();
        cable.end = pos.toFloat();
      }

      cable.updateControlPoints();
      cables_.push_back(cable);
    }
  }

  void drawCable(juce::Graphics& g, const CableVisual& cable) {
    juce::Path path;
    path.startNewSubPath(cable.start);
    path.cubicTo(cable.control1, cable.control2, cable.end);

    // Shadow
    g.setColour(theme_.cableShadow);
    g.strokePath(path, juce::PathStrokeType(theme_.cableThickness + 2),
                 juce::AffineTransform::translation(2, 2));

    // Cable
    g.setColour(cable.selected ? theme_.cableSelected : cable.color);
    g.strokePath(path, juce::PathStrokeType(theme_.cableThickness));
  }

  void drawDragCable(juce::Graphics& g) {
    juce::Path path;
    auto start = dragCable_.startPos.toFloat();
    auto end = dragCable_.currentPos.toFloat();

    float dx = std::abs(end.x - start.x);
    juce::Point<float> ctrl1(start.x + dx * 0.5f, start.y);
    juce::Point<float> ctrl2(end.x - dx * 0.5f, end.y);

    path.startNewSubPath(start);
    path.cubicTo(ctrl1, ctrl2, end);

    g.setColour(theme_.cableSelected);
    g.strokePath(path, juce::PathStrokeType(theme_.cableThickness));
  }
};

} // namespace uds
