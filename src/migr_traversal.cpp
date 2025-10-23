#include "migr_traversal.h"
#include "globals.h"
#include <unordered_set>

MIGRTraversal::MIGRTraversal(MIGRGraphLayer &layer) : layer_(layer) {
  _V_ << " [MIGRTraversal] Traversal Interface Initialized." << std::endl;
}

/*
 * Iterative dfs engine for collect
 */
std::vector<std::shared_ptr<MIGRNode>>
MIGRTraversal::dfs_collect(const std::vector<std::shared_ptr<MIGRNode>> &starts,
                           std::function<bool(const MIGRNode &)> predicate,
                           int max_depth, TraversalDirection direction) const {
  std::vector<std::shared_ptr<MIGRNode>> results;
  std::unordered_set<std::string> visited;
  std::stack<std::pair<std::shared_ptr<MIGRNode>, int>> st; // [node : depth]

  // stack -> start nodes, with depth 0
  for (const auto &start : starts) {
    if (start) {
      st.push({start, 0});
    }
  }

  while (!st.empty()) {
    auto [node, depth] = st.top();
    st.pop();

    if (!node || visited.count(node->id_)) {
      continue;
    }

    if (max_depth >= 0 && depth > max_depth) {
      continue;
    }

    visited.insert(node->id_);

    if (predicate(*node)) {
      results.push_back(node);
    }

    /* pushing neighbours (in reverse order) */
    auto neighbours = get_neighbours(node, direction);
    for (auto it = neighbours.rbegin(); it != neighbours.rend(); ++it) {
      if (*it && !visited.count((*it)->id_)) {
        st.push({*it, depth + 1});
      }
    }
  }
  _V_ << " [MIGRTraversal] DFS collected " << results.size() << " nodes"
      << std::endl;
  return results;
}

std::vector<std::shared_ptr<MIGRNode>>
MIGRTraversal::get_neighbours(const std::shared_ptr<MIGRNode> &node,
                              TraversalDirection direction) const {
  // TODO: implement
}
