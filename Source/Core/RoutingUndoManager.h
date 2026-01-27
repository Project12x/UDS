#pragma once

#include "RoutingGraph.h"
#include <vector>

namespace uds {

/**
 * @brief Simple undo/redo manager for routing graph connections.
 *
 * Stores snapshots of routing state and allows navigation through history.
 */
class RoutingUndoManager {
public:
  static constexpr size_t kMaxHistorySize = 32;

  RoutingUndoManager() = default;

  /**
   * @brief Save current routing state before making changes.
   */
  void saveState(const RoutingGraph &graph) {
    // Clear any redo states when new action is taken
    if (currentIndex_ < history_.size()) {
      history_.erase(history_.begin() + static_cast<long>(currentIndex_),
                     history_.end());
    }

    // Capture current connections
    std::vector<Connection> state;
    for (const auto &conn : graph.getConnections()) {
      state.push_back(conn);
    }
    history_.push_back(state);

    // Enforce max history size
    while (history_.size() > kMaxHistorySize) {
      history_.erase(history_.begin());
    }

    currentIndex_ = history_.size();
  }

  /**
   * @brief Undo to previous routing state.
   * @return true if undo was performed, false if no history.
   */
  bool undo(RoutingGraph &graph) {
    if (!canUndo())
      return false;

    // If at end, save current state first so we can redo back to it
    if (currentIndex_ == history_.size()) {
      std::vector<Connection> state;
      for (const auto &conn : graph.getConnections()) {
        state.push_back(conn);
      }
      history_.push_back(state);
    }

    --currentIndex_;
    restoreState(graph, history_[currentIndex_]);
    return true;
  }

  /**
   * @brief Redo to next routing state.
   * @return true if redo was performed, false if no forward history.
   */
  bool redo(RoutingGraph &graph) {
    if (!canRedo())
      return false;

    ++currentIndex_;
    restoreState(graph, history_[currentIndex_]);
    return true;
  }

  bool canUndo() const { return currentIndex_ > 0; }
  bool canRedo() const { return currentIndex_ < history_.size() - 1; }

  void clear() {
    history_.clear();
    currentIndex_ = 0;
  }

private:
  void restoreState(RoutingGraph &graph, const std::vector<Connection> &state) {
    graph.clearAllConnections();
    for (const auto &conn : state) {
      graph.connect(conn.sourceId, conn.destId);
    }
  }

  std::vector<std::vector<Connection>> history_;
  size_t currentIndex_ = 0;
};

} // namespace uds
