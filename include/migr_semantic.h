#ifndef MIGR_SEMANTIC_H
#define MIGR_SEMANTIC_H

#include "migr.h"
#include "migr_structural.h"

class SemanticLayer : public MIGRGraphLayer {
public:
  SemanticLayer() = default;

  /* MIGR Graph Interface */
  void add_node(std::shared_ptr<MIGRNode> node) override;
  void remove_node(const std::string &node_id) override;
  std::vector<std::shared_ptr<MIGRNode>>
  query_nodes(std::function<bool(const MIGRNode &)> predicate) const override;
  void serialize(std::ostream &out) const override;
  void deserialize(std::istream &in) const override;

  /* Semantic Operations */
  void extract_semantics(const StructuralLayer &structural);
  void add_semantic_edge(const std::string &from_id, const std::string &to_id,
                         const std::string &relation_type);
  std::vector<std::shared_ptr<MIGRNode>>
  find_backlinks(const std::string &target_id) const;
  std::vector<std::shared_ptr<MIGRNode>> search_tag(const std::string &tag) const;
  std::vector<std::shared_ptr<MIGRNode>>
  find_all_links_to_target(const std::string &target_name) const;

  /* for debuggin' */
  void print_semantic_info(bool brief = false) const;

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

  /* Node Management */
  std::string get_or_create_reference_node(const std::string &target);
  std::string get_or_create_tag_node(const std::string &tag_name);
};

#endif //! MIGR_SEMANTIC_H
