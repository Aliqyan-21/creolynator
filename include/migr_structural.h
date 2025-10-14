#ifndef MIGR_STRUCTURAL_H
#define MIGR_STRUCTURAL_H

#include "migr.h"
#include "b_lexer.h"
#include <stack>

/* Represent Document Outline As A Tree */
class StructuralLayer : public MIGRGraphLayer {
public:
  StructuralLayer();

  /* from MIGRGraphLayer interface */
  void add_node(std::shared_ptr<MIGRNode> node) override;
  void remove_node(const std::string &node_id) override;
  std::vector<std::shared_ptr<MIGRNode>>
  query_nodes(std::function<bool(const MIGRNode &)> predicate) override;
  void serialize(std::ostream &out) const override;
  void deserialize(std::istream &in) const override;

  /* core functionality */
  void build_from_tokens(const std::vector<BToken> &tokens);
  std::shared_ptr<MIGRNode> get_root() const;

private:
  std::shared_ptr<MIGRNode> root_;
  std::unordered_map<std::string, std::shared_ptr<MIGRNode>>
      nodes_; // [id : node]

  /* parsing state */
  std::stack<std::shared_ptr<MIGRNode>> parent_stack_;
  std::stack<std::shared_ptr<MIGRNode>> list_stack_;

  /* Token Processing Helpers */
  void process_heading_token(const BToken &token);
  void process_paragraph_token(const BToken &token);
  void process_ulist_token(const BToken &token);
  void process_olist_token(const BToken &token);
};

#endif //! MIGR_STRUCTURAL_H
