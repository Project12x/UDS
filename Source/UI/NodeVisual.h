#pragma once

#include <array>
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace uds {

/**
 * @brief Node IDs for the routing graph
 */
enum class NodeId : int {
  Input = 0,
  Band1 = 1,
  Band2 = 2,
  Band3 = 3,
  Band4 = 4,
  Band5 = 5,
  Band6 = 6,
  Band7 = 7,
  Band8 = 8,
  Output = 9
};

constexpr int kNumBands = 8;
constexpr int kNumNodes = 10; // Input + 8 bands + Output

/**
 * @brief Visual representation of a node in the routing editor
 */
struct NodeVisual {
  int id = 0;
  juce::String name;
  juce::Point<int> position{0, 0};
  juce::Rectangle<int> bounds;
  bool selected = false;
  bool dragging = false;
  bool enabled = true;

  // Port positions (relative to node)
  juce::Point<int> inputPortOffset{0, 40};
  juce::Point<int> outputPortOffset{100, 40};

  juce::Point<int> getInputPortPosition() const {
    return position + inputPortOffset;
  }

  juce::Point<int> getOutputPortPosition() const {
    return position + outputPortOffset;
  }
};

/**
 * @brief Visual representation of a cable connection
 */
struct CableVisual {
  int sourceNodeId = 0;
  int destNodeId = 0;
  bool selected = false;
  juce::Colour color{0xff00b4d8}; // Cyan

  // Bezier control points (calculated from node positions)
  juce::Point<float> start;
  juce::Point<float> end;
  juce::Point<float> control1;
  juce::Point<float> control2;

  void updateControlPoints() {
    float dx = std::abs(end.x - start.x);
    control1 = {start.x + dx * 0.5f, start.y};
    control2 = {end.x - dx * 0.5f, end.y};
  }
};

/**
 * @brief State for in-progress cable drag
 */
struct DragCable {
  bool active = false;
  int sourceNodeId = -1;
  bool fromOutput = true; // true = dragging from output port
  juce::Point<int> startPos;
  juce::Point<int> currentPos;
};

/**
 * @brief A connection in the routing graph
 */
struct Connection {
  int sourceId;
  int destId;

  bool operator==(const Connection &other) const {
    return sourceId == other.sourceId && destId == other.destId;
  }
};

/**
 * @brief Theme colors for the node editor
 */
struct NodeEditorTheme {
  // Background
  juce::Colour background{0xff1a1a2e};
  juce::Colour gridLines{0xff333333};

  // Nodes
  juce::Colour nodeBody{0xff0f3460};
  juce::Colour nodeBodyActive{0xff533483};
  juce::Colour nodeBodyDisabled{0xff2a2a2a};
  juce::Colour nodeBorder{0xffe94560};
  juce::Colour nodeLabel{0xffffffff};

  // Ports
  juce::Colour inputPort{0xff00d9ff};
  juce::Colour outputPort{0xff00ff88};
  juce::Colour portHover{0xffff6b6b};

  // Cables
  juce::Colour cableDefault{0xff00b4d8};
  juce::Colour cableSelected{0xffff6b6b};
  juce::Colour cableShadow{0x4d000000};

  // Per-band cable colors (8 distinct colors)
  std::array<juce::Colour, 8> bandColors = {{
      juce::Colour(0xffff6b6b), // Band 1: Coral red
      juce::Colour(0xffffd93d), // Band 2: Yellow
      juce::Colour(0xff6bcb77), // Band 3: Green
      juce::Colour(0xff4d96ff), // Band 4: Blue
      juce::Colour(0xffc084fc), // Band 5: Purple
      juce::Colour(0xffff8fab), // Band 6: Pink
      juce::Colour(0xff00d9ff), // Band 7: Cyan
      juce::Colour(0xffffb347), // Band 8: Orange
  }};

  // Special nodes
  juce::Colour inputNode{0xff2d6a4f};
  juce::Colour outputNode{0xff9d4edd};

  // Sizing
  float nodeWidth = 100.0f;
  float nodeHeight = 80.0f;
  float portRadius = 8.0f;
  float cableThickness = 3.0f;

  // Static band colors - shared by all UI components
  static constexpr std::array<uint32_t, 8> kBandColorValues = {{
      0xffff6b6b, // Band 1: Coral red
      0xffffd93d, // Band 2: Yellow
      0xff6bcb77, // Band 3: Green
      0xff4d96ff, // Band 4: Blue
      0xffc084fc, // Band 5: Purple
      0xffff8fab, // Band 6: Pink
      0xff00d9ff, // Band 7: Cyan
      0xffffb347, // Band 8: Orange
  }};

  static juce::Colour getBandColor(int bandIndex) {
    if (bandIndex >= 0 && bandIndex < 8) {
      return juce::Colour(kBandColorValues[static_cast<size_t>(bandIndex)]);
    }
    return juce::Colour(0xff00b4d8); // Default cyan
  }

  juce::Colour getCableColorForSource(int sourceNodeId) const {
    if (sourceNodeId >= 1 && sourceNodeId <= 8) {
      return getBandColor(sourceNodeId - 1);
    }
    return cableDefault; // Input node uses default
  }
};

} // namespace uds
