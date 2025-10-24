#ifndef MIGR_TRAVERSAL_H
#define MIGR_TRAVERSAL_H

#include "migr.h"
#include "migr_semantic.h"

/*
Rules:
- class and struct will be named in PascalCase
- class member functions and members will be named in snake_case
*/

enum class TraversalType {
  DFS,
  BFS,
};

enum class TraversalDirection {
  FORWARD,       // structural: children, semantic: outgoing edges
  BACKWARD,      // structural: parent, semantic: incoming edges
  BIDIRECTIONAL, // both
};

/*
 * A struct to represent path between any two connected nodes
 * with full edge context
 */
struct TraversalPath {
  std::vector<std::shared_ptr<MIGRNode>> nodes;
  std::vector<SemanticEdge> edges;
  int total_depth;
  double path_weight; // NOTE: will not implemented in this iteration; for
                      // future weighted traversals (heuristics)

  TraversalPath() : total_depth(0), path_weight(0.0) {}
};

/*
 * A layer-agnostic traversal implementation interface
 * for all MIGRGraphLayer interface.
 * Currently proviedes DFS/BFS with filtering, visiting and transformation
 * capabilities meanwhile maintaining MIGR's pure IR nature.
 */
class MIGRTraversal {
public:
  explicit MIGRTraversal(MIGRGraphLayer &layer);

private:
  MIGRGraphLayer &layer_;

  /* Internal Traversal Engines */
  std::vector<std::shared_ptr<MIGRNode>>
  dfs_collect(const std::vector<std::shared_ptr<MIGRNode>> &starts,
              std::function<bool(const MIGRNode &)> predicate, int max_depth,
              TraversalDirection direction) const;

  bool
  dfs_visit(const std::vector<std::shared_ptr<MIGRNode>> &starts,
            std::function<bool(std::shared_ptr<MIGRNode>, int depth)> visitor,
            int max_depth, TraversalDirection direction) const;

  /* Internal Transform Engine */
  std::vector<std::shared_ptr<MIGRNode>> dfs_transform(
      const std::vector<std::shared_ptr<MIGRNode>> &starts,
      std::function<std::shared_ptr<MIGRNode>(std::shared_ptr<MIGRNode>,
                                              int depth)>
          transformer,
      int max_depth, TraversalDirection direction) const;

  /* Neighbour Helpers */
  std::vector<std::shared_ptr<MIGRNode>>
  get_neighbours(const std::shared_ptr<MIGRNode> &node,
                 TraversalDirection direction) const;

  std::vector<std::shared_ptr<MIGRNode>>
  get_forward_neighbours(const std::shared_ptr<MIGRNode> &node) const;

  std::vector<std::shared_ptr<MIGRNode>>
  get_backward_neighbours(const std::shared_ptr<MIGRNode> &node) const;
};

#endif //! MIGR_TRAVERSAL_H
