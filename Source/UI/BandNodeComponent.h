#pragma once

#include "NodeVisual.h"
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

namespace uds {

/**
 * @brief Visual component representing a single node in the routing editor.
 *
 * Features:
 * - Draggable positioning
 * - Input/output ports for cable connections
 * - Visual feedback for selection and enabled state
 */
class BandNodeComponent : public juce::Component {
public:
  enum class NodeType { Input, Band, Output };

  // Callbacks for node interactions
  std::function<void(int nodeId, juce::Point<int> position)> onDragEnd;
  std::function<void(int nodeId, bool isOutput, juce::Point<int> portPos)>
      onPortClicked;
  std::function<void(int nodeId)> onNodeSelected;

  BandNodeComponent(int nodeId, const juce::String &name,
                    NodeType type = NodeType::Band)
      : nodeId_(nodeId), name_(name), type_(type) {
    setSize(static_cast<int>(theme_.nodeWidth),
            static_cast<int>(theme_.nodeHeight));
  }

  void paint(juce::Graphics &g) override {
    auto bounds = getLocalBounds().toFloat();

    // Shadow
    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.fillRoundedRectangle(bounds.translated(3, 3), 8.0f);

    // Body color based on type and state
    juce::Colour bodyColor;
    if (type_ == NodeType::Input) {
      bodyColor = theme_.inputNode;
    } else if (type_ == NodeType::Output) {
      bodyColor = theme_.outputNode;
    } else {
      // Band nodes use per-band colors
      auto bandColor = NodeEditorTheme::getBandColor(nodeId_ - 1);
      if (!enabled_) {
        bodyColor = theme_.nodeBodyDisabled;
      } else if (selected_) {
        bodyColor = bandColor.brighter(0.2f);
      } else {
        bodyColor = bandColor.darker(0.3f);
      }
    }

    g.setColour(bodyColor);
    g.fillRoundedRectangle(bounds, 8.0f);

    // Border (glow when selected)
    if (selected_) {
      g.setColour(theme_.nodeBorder);
      g.drawRoundedRectangle(bounds.reduced(1), 8.0f, 2.5f);
    } else {
      g.setColour(theme_.nodeBorder.withAlpha(0.5f));
      g.drawRoundedRectangle(bounds.reduced(1), 8.0f, 1.5f);
    }

    // Node name
    g.setColour(theme_.nodeLabel);
    g.setFont(juce::FontOptions(14.0f).withStyle("Bold"));
    g.drawText(name_, bounds.reduced(10, 5), juce::Justification::centredTop);

    // Draw ports
    if (type_ != NodeType::Input) {
      // Input port (left side)
      auto inputPos = getInputPortCenter();
      g.setColour(portHovered_ && !hoveredPortIsOutput_ ? theme_.portHover
                                                        : theme_.inputPort);
      g.fillEllipse(inputPos.x - theme_.portRadius,
                    inputPos.y - theme_.portRadius, theme_.portRadius * 2,
                    theme_.portRadius * 2);
      g.setColour(juce::Colours::white.withAlpha(0.8f));
      g.drawEllipse(inputPos.x - theme_.portRadius,
                    inputPos.y - theme_.portRadius, theme_.portRadius * 2,
                    theme_.portRadius * 2, 1.5f);
    }

    if (type_ != NodeType::Output) {
      // Output port (right side)
      auto outputPos = getOutputPortCenter();
      g.setColour(portHovered_ && hoveredPortIsOutput_ ? theme_.portHover
                                                       : theme_.outputPort);
      g.fillEllipse(outputPos.x - theme_.portRadius,
                    outputPos.y - theme_.portRadius, theme_.portRadius * 2,
                    theme_.portRadius * 2);
      g.setColour(juce::Colours::white.withAlpha(0.8f));
      g.drawEllipse(outputPos.x - theme_.portRadius,
                    outputPos.y - theme_.portRadius, theme_.portRadius * 2,
                    theme_.portRadius * 2, 1.5f);
    }

    // Enabled/disabled indicator for band nodes
    if (type_ == NodeType::Band) {
      g.setColour(enabled_ ? juce::Colour(0xff00ff88)
                           : juce::Colour(0xffff4444));
      g.fillEllipse(bounds.getRight() - 16, bounds.getY() + 6, 10, 10);
    }
  }

  void mouseDown(const juce::MouseEvent &event) override {
    // Check if clicking on a port
    auto inputPos = getInputPortCenter();
    auto outputPos = getOutputPortCenter();
    auto clickPos = event.getPosition().toFloat();

    // Check input port (not on Input node type)
    if (type_ != NodeType::Input &&
        clickPos.getDistanceFrom(inputPos) < theme_.portRadius + 6) {
      portDragActive_ = true;
      if (onPortClicked) {
        onPortClicked(
            nodeId_, false,
            event.getEventRelativeTo(getParentComponent()).getPosition());
      }
      return;
    }

    // Check output port (not on Output node type)
    if (type_ != NodeType::Output &&
        clickPos.getDistanceFrom(outputPos) < theme_.portRadius + 6) {
      portDragActive_ = true;
      if (onPortClicked) {
        onPortClicked(
            nodeId_, true,
            event.getEventRelativeTo(getParentComponent()).getPosition());
      }
      return;
    }

    // Otherwise start node dragging
    dragging_ = true;
    dragOffset_ = event.getPosition();

    if (onNodeSelected) {
      onNodeSelected(nodeId_);
    }
  }

  void mouseDrag(const juce::MouseEvent &event) override {
    if (portDragActive_) {
      // Forward drag events to parent for cable drawing
      if (auto *parent = getParentComponent()) {
        auto parentEvent = event.getEventRelativeTo(parent);
        parent->mouseDrag(parentEvent);
      }
    } else if (dragging_) {
      auto parentPos =
          event.getEventRelativeTo(getParentComponent()).getPosition();
      setTopLeftPosition(parentPos - dragOffset_);
      repaint();
    }
  }

  void mouseUp(const juce::MouseEvent &event) override {
    if (portDragActive_) {
      portDragActive_ = false;
      // Forward mouseUp to parent to complete cable connection
      if (auto *parent = getParentComponent()) {
        auto parentEvent = event.getEventRelativeTo(parent);
        parent->mouseUp(parentEvent);
      }
    } else if (dragging_) {
      dragging_ = false;
      if (onDragEnd) {
        onDragEnd(nodeId_, getPosition());
      }
    }
  }

  void mouseMove(const juce::MouseEvent &event) override {
    auto inputPos = getInputPortCenter();
    auto outputPos = getOutputPortCenter();
    auto mousePos = event.getPosition().toFloat();

    bool wasHovered = portHovered_;
    portHovered_ = false;

    if (type_ != NodeType::Input &&
        mousePos.getDistanceFrom(inputPos) < theme_.portRadius + 4) {
      portHovered_ = true;
      hoveredPortIsOutput_ = false;
    } else if (type_ != NodeType::Output &&
               mousePos.getDistanceFrom(outputPos) < theme_.portRadius + 4) {
      portHovered_ = true;
      hoveredPortIsOutput_ = true;
    }

    if (wasHovered != portHovered_)
      repaint();
  }

  void mouseExit(const juce::MouseEvent &) override {
    if (portHovered_) {
      portHovered_ = false;
      repaint();
    }
  }

  // Accessors
  int getNodeId() const { return nodeId_; }
  void setSelected(bool selected) {
    selected_ = selected;
    repaint();
  }
  bool isSelected() const { return selected_; }
  void setEnabled(bool enabled) {
    enabled_ = enabled;
    repaint();
  }
  bool isNodeEnabled() const { return enabled_; }

  juce::Point<float> getInputPortCenter() const {
    return {theme_.portRadius + 2, getHeight() / 2.0f};
  }

  juce::Point<float> getOutputPortCenter() const {
    return {getWidth() - theme_.portRadius - 2, getHeight() / 2.0f};
  }

  juce::Point<int> getInputPortPosition() const {
    auto center = getInputPortCenter();
    return getPosition() + juce::Point<int>(static_cast<int>(center.x),
                                            static_cast<int>(center.y));
  }

  juce::Point<int> getOutputPortPosition() const {
    auto center = getOutputPortCenter();
    return getPosition() + juce::Point<int>(static_cast<int>(center.x),
                                            static_cast<int>(center.y));
  }

private:
  int nodeId_;
  juce::String name_;
  NodeType type_;
  NodeEditorTheme theme_;

  bool selected_ = false;
  bool enabled_ = true;
  bool dragging_ = false;
  bool portDragActive_ = false;
  bool portHovered_ = false;
  bool hoveredPortIsOutput_ = false;
  juce::Point<int> dragOffset_;
};

} // namespace uds
