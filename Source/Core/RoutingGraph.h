#pragma once

#include "../UI/NodeVisual.h"

#include <algorithm>
#include <functional>
#include <queue>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>


namespace uds {

/**
 * @brief Manages routing connections between delay bands.
 *
 * Supports:
 * - Adding/removing connections
 * - Topological sorting for processing order
 * - Cycle detection for feedback protection
 */
class RoutingGraph {
public:
  RoutingGraph() {
    // Initialize with bands 1-8 active by default
    for (int i = 1; i <= kNumBands; ++i) {
      activeBands_.insert(i);
    }
    // Initialize with Input -> Output default connection
    connections_.push_back(
        {static_cast<int>(NodeId::Input), static_cast<int>(NodeId::Output)});
    rebuildProcessingOrder();
  }

  /**
   * @brief Add a connection between two nodes
   * @return true if connection was added (false if already exists)
   */
  bool connect(int sourceId, int destId) {
    // Can't connect to self
    if (sourceId == destId)
      return false;

    // Can't connect TO input node
    if (destId == static_cast<int>(NodeId::Input))
      return false;

    // Can't connect FROM output node
    if (sourceId == static_cast<int>(NodeId::Output))
      return false;

    // Check if connection already exists
    for (const auto& conn : connections_) {
      if (conn.sourceId == sourceId && conn.destId == destId) {
        return false;
      }
    }

    // Add connection
    connections_.push_back({sourceId, destId});
    rebuildProcessingOrder();
    return true;
  }

  /**
   * @brief Remove a connection
   * @return true if connection was removed
   */
  bool disconnect(int sourceId, int destId) {
    auto it =
        std::find_if(connections_.begin(), connections_.end(),
                     [sourceId, destId](const Connection& c) {
                       return c.sourceId == sourceId && c.destId == destId;
                     });

    if (it != connections_.end()) {
      connections_.erase(it);
      rebuildProcessingOrder();
      return true;
    }
    return false;
  }

  /**
   * @brief Remove all connections involving a node
   */
  void disconnectAll(int nodeId) {
    connections_.erase(std::remove_if(connections_.begin(), connections_.end(),
                                      [nodeId](const Connection& c) {
                                        return c.sourceId == nodeId ||
                                               c.destId == nodeId;
                                      }),
                       connections_.end());
    rebuildProcessingOrder();
  }

  /**
   * @brief Clear all connections and reset to default
   */
  void clearAllConnections() {
    connections_.clear();
    rebuildProcessingOrder();
  }

  /**
   * @brief Get all connections
   */
  const std::vector<Connection>& getConnections() const { return connections_; }

  /**
   * @brief Get nodes that feed into a given node
   */
  std::vector<int> getInputsFor(int nodeId) const {
    std::vector<int> inputs;
    for (const auto& conn : connections_) {
      if (conn.destId == nodeId) {
        inputs.push_back(conn.sourceId);
      }
    }
    return inputs;
  }

  /**
   * @brief Get nodes that a given node feeds into
   */
  std::vector<int> getOutputsFor(int nodeId) const {
    std::vector<int> outputs;
    for (const auto& conn : connections_) {
      if (conn.sourceId == nodeId) {
        outputs.push_back(conn.destId);
      }
    }
    return outputs;
  }

  /**
   * @brief Get processing order (topologically sorted)
   */
  const std::vector<int>& getProcessingOrder() const {
    return processingOrder_;
  }

  /**
   * @brief Check if adding a connection would create a cycle
   */
  bool wouldCreateCycle(int sourceId, int destId) const {
    std::unordered_map<int, std::vector<int>> adj;
    for (const auto& conn : connections_) {
      adj[conn.sourceId].push_back(conn.destId);
    }
    adj[sourceId].push_back(destId);
    return hasCycle(adj);
  }

  /**
   * @brief Check if graph currently has cycles
   */
  bool hasCycles() const {
    std::unordered_map<int, std::vector<int>> adj;
    for (const auto& conn : connections_) {
      adj[conn.sourceId].push_back(conn.destId);
    }
    return hasCycle(adj);
  }

  /**
   * @brief Clear all connections and reset to default
   */
  void clear() {
    connections_.clear();
    connections_.push_back(
        {static_cast<int>(NodeId::Input), static_cast<int>(NodeId::Output)});
    // Reset to default 8 active bands
    activeBands_.clear();
    for (int i = 1; i <= kNumBands; ++i) {
      activeBands_.insert(i);
    }
    rebuildProcessingOrder();
  }

  // ============== Dynamic Band Management ==============

  /**
   * @brief Add a band to the active set
   * @param bandId Band ID (1-12)
   * @return true if band was added (false if already active or invalid)
   */
  bool addBand(int bandId) {
    if (bandId < 1 || bandId > 12)
      return false;
    if (activeBands_.count(bandId) > 0)
      return false;
    activeBands_.insert(bandId);
    return true;
  }

  /**
   * @brief Remove a band from the active set
   * @param bandId Band ID (1-12)
   * @return true if band was removed (false if not active or invalid)
   */
  bool removeBand(int bandId) {
    if (bandId < 1 || bandId > 12)
      return false;
    if (activeBands_.count(bandId) == 0)
      return false;
    // Disconnect all connections to/from this band
    disconnectAll(bandId);
    activeBands_.erase(bandId);
    return true;
  }

  /**
   * @brief Check if a band is active
   */
  bool isBandActive(int bandId) const { return activeBands_.count(bandId) > 0; }

  /**
   * @brief Get count of active bands
   */
  int getActiveBandCount() const {
    return static_cast<int>(activeBands_.size());
  }

  /**
   * @brief Get all active band IDs (sorted)
   */
  std::vector<int> getActiveBands() const {
    return std::vector<int>(activeBands_.begin(), activeBands_.end());
  }

  /**
   * @brief Set the active bands (replaces existing set)
   */
  void setActiveBands(const std::vector<int>& bands) {
    activeBands_.clear();
    for (int bandId : bands) {
      if (bandId >= 1 && bandId <= 12) {
        activeBands_.insert(bandId);
      }
    }
  }

  /**
   * @brief Set up default routing (Input -> all ACTIVE bands in parallel ->
   * Output)
   */
  void setDefaultParallelRouting() {
    connections_.clear();
    for (int bandId : activeBands_) {
      connections_.push_back({static_cast<int>(NodeId::Input), bandId});
      connections_.push_back({bandId, static_cast<int>(NodeId::Output)});
    }
    rebuildProcessingOrder();
  }

  /**
   * @brief Set up series routing (Input -> active bands in order -> Output)
   */
  void setSeriesRouting() {
    connections_.clear();
    auto bands = getActiveBands(); // sorted
    if (bands.empty()) {
      rebuildProcessingOrder();
      return;
    }
    connections_.push_back({static_cast<int>(NodeId::Input), bands.front()});
    for (size_t i = 0; i < bands.size() - 1; ++i) {
      connections_.push_back({bands[i], bands[i + 1]});
    }
    connections_.push_back({bands.back(), static_cast<int>(NodeId::Output)});
    rebuildProcessingOrder();
  }

  /**
   * @brief Serialize routing to XML for preset saving
   */
  std::unique_ptr<juce::XmlElement> toXml() const {
    auto xml = std::make_unique<juce::XmlElement>("Routing");

    // Serialize active bands
    juce::String activeBandsStr;
    for (int bandId : activeBands_) {
      if (activeBandsStr.isNotEmpty())
        activeBandsStr += ",";
      activeBandsStr += juce::String(bandId);
    }
    xml->setAttribute("activeBands", activeBandsStr);

    // Serialize connections
    for (const auto& conn : connections_) {
      auto* connXml = xml->createNewChildElement("Connection");
      connXml->setAttribute("source", conn.sourceId);
      connXml->setAttribute("dest", conn.destId);
    }
    return xml;
  }

  /**
   * @brief Restore routing from XML
   */
  void fromXml(const juce::XmlElement* xml) {
    if (xml == nullptr || !xml->hasTagName("Routing"))
      return;

    // Deserialize active bands (default to 1-8 if not present)
    activeBands_.clear();
    juce::String activeBandsStr = xml->getStringAttribute("activeBands", "");
    if (activeBandsStr.isNotEmpty()) {
      juce::StringArray bandIds;
      bandIds.addTokens(activeBandsStr, ",", "");
      for (const auto& idStr : bandIds) {
        int bandId = idStr.getIntValue();
        if (bandId >= 1 && bandId <= 12) {
          activeBands_.insert(bandId);
        }
      }
    } else {
      // Default to 8 bands for backwards compatibility
      for (int i = 1; i <= kNumBands; ++i) {
        activeBands_.insert(i);
      }
    }

    // Deserialize connections
    connections_.clear();
    for (auto* connXml : xml->getChildWithTagNameIterator("Connection")) {
      Connection conn;
      conn.sourceId = connXml->getIntAttribute("source", 0);
      conn.destId = connXml->getIntAttribute("dest", 10);
      connections_.push_back(conn);
    }
    rebuildProcessingOrder();
  }

  /**
   * @brief Batch-set all connections (for preset loading)
   */
  void setConnections(const std::vector<Connection>& newConnections) {
    connections_ = newConnections;
    rebuildProcessingOrder();
  }

private:
  std::vector<Connection> connections_;
  std::vector<int> processingOrder_;
  std::set<int> activeBands_; // Active band IDs (1-12)

  void rebuildProcessingOrder() {
    processingOrder_.clear();

    std::unordered_map<int, std::vector<int>> adj;
    std::unordered_map<int, int> inDegree;
    std::unordered_set<int> allNodes;

    // Collect all nodes from connections
    for (const auto& conn : connections_) {
      allNodes.insert(conn.sourceId);
      allNodes.insert(conn.destId);
      adj[conn.sourceId].push_back(conn.destId);
    }

    // Initialize inDegree for all nodes
    for (int node : allNodes) {
      inDegree[node] = 0;
    }

    // Calculate inDegree
    for (const auto& conn : connections_) {
      inDegree[conn.destId]++;
    }

    std::queue<int> queue;
    for (int node : allNodes) {
      if (inDegree[node] == 0)
        queue.push(node);
    }

    while (!queue.empty()) {
      int node = queue.front();
      queue.pop();
      processingOrder_.push_back(node);
      for (int neighbor : adj[node]) {
        if (--inDegree[neighbor] == 0)
          queue.push(neighbor);
      }
    }
  }

  static bool hasCycle(const std::unordered_map<int, std::vector<int>>& adj) {
    std::unordered_set<int> visited, inStack;

    std::function<bool(int)> dfs = [&](int node) -> bool {
      visited.insert(node);
      inStack.insert(node);
      auto it = adj.find(node);
      if (it != adj.end()) {
        for (int n : it->second) {
          if (inStack.count(n) > 0)
            return true;
          if (visited.count(n) == 0 && dfs(n))
            return true;
        }
      }
      inStack.erase(node);
      return false;
    };

    for (const auto& [node, _] : adj) {
      if (visited.count(node) == 0 && dfs(node))
        return true;
    }
    return false;
  }
};

} // namespace uds
