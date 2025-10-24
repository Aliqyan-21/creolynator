#include "migr_traversal.h"
#include "globals.h"
#include <queue>
#include <unordered_set>

MIGRTraversal::MIGRTraversal(MIGRGraphLayer &layer) : layer_(layer) {
  _V_ << " [MIGRTraversal] Traversal Interface Initialized." << std::endl;
}

//-----------------------------------//
//    Internal Traversal Engines     //
//-----------------------------------//

/*
 * Performs a depth-first search (DFS) to collect nodes starting from the
 * given start nodes.
 * Nodes are visited iteratively using a stack.
 * The traversal respects a max depth and traversal
 * direction (forward, backward, or bidirectional).
 *
 * Each visited node is tested against the provided predicate function; if
 * the predicate returns true, the node is added to result vector.
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
 * Performs a breadth-first search (BFS) to collect nodes starting from the
 * given start nodes.
 * Nodes are visited iteratively using a queue.
 * The traversal respects a max depth and traversal
 * direction (forward, backward, or bidirectional).
 *
 * Each visited node is tested against the provided predicate function; if
 * the predicate returns true, the node is added to result vector.
 */
std::vector<std::shared_ptr<MIGRNode>>
MIGRTraversal::bfs_collect(const std::vector<std::shared_ptr<MIGRNode>> &starts,
                           std::function<bool(const MIGRNode &)> predicate,
                           int max_depth, TraversalDirection direction) const {
  std::vector<std::shared_ptr<MIGRNode>> results;
  std::unordered_set<std::string> visited;
  std::queue<std::pair<std::shared_ptr<MIGRNode>, int>> qu; // [node : depth]

  for (const auto &start : starts) {
    if (start) {
      qu.push({start, 0});
    }
  }

  while (!qu.empty()) {
    auto [node, depth] = qu.front();
    qu.pop();

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

    auto neighbours = get_neighbours(node, direction);
    for (const auto &neighbour : neighbours) {
      if (neighbour && !visited.count(neighbour->id_)) {
        qu.push({neighbour, depth + 1});
      }
    }
  }
  _V_ << " [MIGRTraversal] BFS collected " << results.size() << " nodes"
      << std::endl;
  return results;
}

/*
 * Performs an iterative depth-first search (DFS) traversal to visit nodes
 * according to the visitor function Starting from the provided nodes and
 * max depth.
 *
 * If visitor returns false at some point, then it's early termination and it
 * returns false in that case, otherwise if traversal completes then returns
 * true.
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

/*
 * Performs an iterative breadth-first search (BFS) traversal to visit nodes
 * according to the visitor function Starting from the provided nodes and
 * max depth.
 *
 * If visitor returns false at some point, then it's early termination and it
 * returns false in that case, otherwise if traversal completes then returns
 * true.
 */
bool MIGRTraversal::bfs_visit(
    const std::vector<std::shared_ptr<MIGRNode>> &starts,
    std::function<bool(std::shared_ptr<MIGRNode>, int depth)> visitor,
    int max_depth, TraversalDirection direction) const {
  // TODO: implement
}

//-----------------------------//
//  Internal Transform Engine  //
//-----------------------------//

/*
 * Performs a depth-first search (DFS) traversal and applies the transformation
 * function to each visited node. The transformer function receives the
 * current node and depth, and returns a possibly transformed node according to
 * the tranformer function to be include in results vector.
 *
 * traversal respects max depth and direction. Transformed
 * nodes are collected into a result vector.
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
    auto neighbours = get_neighbours(node, direction);
    for (auto it = neighbours.rbegin(); it != neighbours.rend(); ++it) {
      if (*it && !visited.count((*it)->id_)) {
        st.push({*it, depth + 1});
      }
    }
  }
  _V_ << " [MIGRTraversal] DFS transformed " << results.size() << " nodes"
      << std::endl;
  return results;
}

/*
 * Performs a breadth-first search (BFS) traversal and applies the transformation
 * function to each visited node. The transformer function receives the
 * current node and depth, and returns a possibly transformed node according to
 * the tranformer function to be include in results vector.
 *
 * traversal respects max depth and direction. Transformed
 * nodes are collected into a result vector.
 */
std::vector<std::shared_ptr<MIGRNode>> MIGRTraversal::bfs_transform(
    const std::vector<std::shared_ptr<MIGRNode>> &starts,
    std::function<std::shared_ptr<MIGRNode>(std::shared_ptr<MIGRNode>,
                                            int depth)>
        transformer,
    int max_depth, TraversalDirection direction) const {
  // TODO: implement
}

//----------------------//
//   Neighbour Helpers  //
//----------------------//

/*
 * Returns neighbors of a node based on the specified traversal direction.
 * (forward, backward, or bidirectional)
 */
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

/*
 * Gets forward neighbors of a node.
 * - for semantic layers, returns outgoing semantic targets.
 * - for structural layers, returns children nodes.
 */
std::vector<std::shared_ptr<MIGRNode>> MIGRTraversal::get_forward_neighbours(
    const std::shared_ptr<MIGRNode> &node) const {
  /* seeing if semantic layer */
  if (SemanticLayer *semantic = dynamic_cast<SemanticLayer *>(&layer_)) {
    return semantic->get_semantic_targets(node->id_);
  }

  /* otherwise return structural children */
  return node->children_;
}

/*
 * Gets backward neighbors of a node.
 * - for semantic layers, returns incoming semantic sources.
 * - for structural layers, returns the parent node if present.
 */
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
