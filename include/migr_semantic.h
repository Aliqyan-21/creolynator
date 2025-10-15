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

  /* Semantic Operations */
  void extract_semantics(const StructuralLayer &structural);
  void add_cross_reference(const std::string &from_id, const std::string &to_id,
                           const std::string &relation_type);
  std::vector<std::shared_ptr<MIGRNode>>
  find_backlinks(const std::string &target_id);

private:
  std::unordered_map<std::string, std::shared_ptr<MIGRNode>>
      semantic_nodes_; // [id : node]
  std::unordered_map<std::string, std::vector<std::string>>
      edges_; // [source : targets]
  std::unordered_map<std::string, std::vector<std::string>>
      backlinks_; // [target : sources]

  /* Helpers */
  void extract_links(std::shared_ptr<MIGRNode> node);
  void extract_tags(std::shared_ptr<MIGRNode> node);
  void build_backlinks();
};

#endif //! MIGR_SEMANTIC_H
