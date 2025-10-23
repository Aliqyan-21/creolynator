#include "migr_traversal.h"
#include "globals.h"
#include <unordered_set>

MIGRTraversal::MIGRTraversal(MIGRGraphLayer &layer) : layer_(layer) {
  _V_ << " [MIGRTraversal] Traversal Interface Initialized." << std::endl;
}

//-----------------------------------//
//    Internal Traversal Engines     //
//-----------------------------------//

/*
 * Iterative DFS engine for collect
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

/*
 * Iterative DFS engine for visit
 */
bool MIGRTraversal::dfs_visit(
    const std::vector<std::shared_ptr<MIGRNode>> &starts,
    std::function<bool(std::shared_ptr<MIGRNode>, int depth)> visitor,
    int max_depth, TraversalDirection direction) const {
  std::unordered_set<std::string> visited;
  std::stack<std::pair<std::shared_ptr<MIGRNode>, int>> st;

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

    if (!visitor(node, depth)) {
      _V_ << " [MIGRTraversal] DFS visit terminated early by visitor"
          << std::endl;
      return false;
    }

    auto neighbours = get_neighbours(node, direction);
    for (auto it = neighbours.rbegin(); it != neighbours.rend(); ++it) {
      if (*it && !visited.count((*it)->id_)) {
        st.push({*it, depth + 1});
      }
    }
  }

  return true;
}

//-----------------------------//
//  Internal Transform Engine  //
//-----------------------------//

/*
 * Iterative DFS engine for transform
 */
std::vector<std::shared_ptr<MIGRNode>> MIGRTraversal::dfs_transform(
    const std::vector<std::shared_ptr<MIGRNode>> &starts,
    std::function<std::shared_ptr<MIGRNode>(std::shared_ptr<MIGRNode>,
                                            int depth)>
        transformer,
    int max_depth, TraversalDirection direction) const {
  std::vector<std::shared_ptr<MIGRNode>> results;
  std::unordered_set<std::string> visited;
  std::stack<std::pair<std::shared_ptr<MIGRNode>, int>> st;

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

    auto transformed = transformer(node, depth);
    if (transformed) {
      results.push_back(transformed);
    }
    auto neighbors = get_neighbours(node, direction);
    for (auto it = neighbors.rbegin(); it != neighbors.rend(); ++it) {
      if (*it && !visited.count((*it)->id_)) {
        st.push({*it, depth + 1});
      }
    }
  }
  _V_ << " [MIGRTraversal] DFS transformed " << results.size() << " nodes"
      << std::endl;
  return results;
}

std::vector<std::shared_ptr<MIGRNode>>
MIGRTraversal::get_neighbours(const std::shared_ptr<MIGRNode> &node,
                              TraversalDirection direction) const {
  if (!node) {
    return {};
  }

  if (direction == TraversalDirection::FORWARD) {
    return get_forward_neighbours(node);
  } else if (direction == TraversalDirection::BACKWARD) {
    return get_backward_neighbours(node);
  } else {
    // both..,
    auto forward = get_forward_neighbours(node);
    auto backward = get_backward_neighbours(node);
    forward.insert(forward.end(), backward.begin(), backward.end());
    return forward;
  }
}

std::vector<std::shared_ptr<MIGRNode>> MIGRTraversal::get_forward_neighbours(
    const std::shared_ptr<MIGRNode> &node) const {
  /* seeing if semantic layer */
  if (SemanticLayer *semantic = dynamic_cast<SemanticLayer *>(&layer_)) {
    return semantic->get_semantic_targets(node->id_);
  }

  /* otherwise return structural children */
  return node->children_;
}

std::vector<std::shared_ptr<MIGRNode>> MIGRTraversal::get_backward_neighbours(
    const std::shared_ptr<MIGRNode> &node) const {
  std::vector<std::shared_ptr<MIGRNode>> neighbours;

  /* seeing if semantic layer */
  if (SemanticLayer *semantic = dynamic_cast<SemanticLayer *>(&layer_)) {
    return semantic->get_semantic_sources(node->id_);
  }

  /* if structural get parent if there */
  if (auto parent = node->parent_.lock()) {
    neighbours.push_back(parent);
  }

  return neighbours;
}
