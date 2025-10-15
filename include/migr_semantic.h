#ifndef MIGR_SEMANTIC_H
#define MIGR_SEMANTIC_H

#include "migr.h"
#include "migr_structural.h"

class SemanticLayer : public MIGRGraphLayer {
public:
  SemanticLayer();

  /* MIGR Graph Interface */
  void add_node(std::shared_ptr<MIGRNode> node) override;
  void remove_node(const std::string &node_id) override;
  std::vector<std::shared_ptr<MIGRNode>>
  query_nodes(std::function<bool(const MIGRNode &)> predicate) override;
  void serialize(std::ostream &out) const override;
  void deserialize(std::istream &in) const override;

private:
  std::unordered_map<std::string, std::shared_ptr<MIGRNode>>
      semantic_nodes_; // [id : node]
  std::unordered_map<std::string, std::vector<std::string>>
      edges_; // [source : targets]
  std::unordered_map<std::string, std::vector<std::string>>
      backlinks_; // [source : targets]
};

#endif //! MIGR_SEMANTIC_H
