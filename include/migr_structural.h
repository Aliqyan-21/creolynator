#ifndef MIGR_STRUCTURAL_H
#define MIGR_STRUCTURAL_H

#include "migr.h"
#include <stack>

/* Represent Document Outline As A Tree */
class StructuralLayer : public MIGRGraphLayer {
public:
  StructuralLayer();

  /* from MIGRGraphLayer interface */
  void add_node(std::shared_ptr<MIGRNode> node) override;
  void remove_node(std::shared_ptr<MIGRNode> node_id) override;
  std::vector<std::shared_ptr<MIGRNode>>
  query_nodes(std::function<bool(const MIGRNode &)> predicate) override;
  void serialize(std::ostream &out) const override;
  void deserialize(std::istream &in) const override;

private:
  std::shared_ptr<MIGRNode> root;
  std::unordered_map<std::string, std::shared_ptr<MIGRNode>>
      nodes; // [id : node]

  /* parsing state */
  std::stack<std::shared_ptr<MIGRNode>> parent_stack;
  std::stack<std::shared_ptr<MIGRNode>> list_stack;
};

#endif //! MIGR_STRUCTURAL_H
