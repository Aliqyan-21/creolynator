#ifndef MIGR_SEMANTIC_H
#define MIGR_SEMANTIC_H

#include "migr.h"
#include "migr_structural.h"

struct SemanticEdge {
  std::string source_id;
  std::string target_id;
  MIGREdgeType edge_type;
  std::string relation_label; // "references", "tagged_with"
};

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
  void add_semantic_edge(const std::shared_ptr<MIGRNode> &source,
                         const std::shared_ptr<MIGRNode> &target,
                         MIGREdgeType edge_type,
                         const std::string &relation_label = "");
  std::vector<std::shared_ptr<MIGRNode>>
  find_backlinks(const std::string &target_id) const;
  std::vector<std::shared_ptr<MIGRNode>>
  search_tag(const std::string &tag) const;
  std::vector<std::shared_ptr<MIGRNode>>
  find_all_links_to_target(const std::string &target_name) const;

  /* for debuggin' */
  void print_semantic_info(bool detailed = false) const;

private:
  std::unordered_map<std::string, std::shared_ptr<MIGRNode>>
      semantic_nodes_;              // [id : node]
  std::vector<SemanticEdge> edges_; // efficient querying
  std::unordered_map<std::string, std::vector<std::string>>
      backlink_index_; // [target : sources]

  /* Cache for reference nodes and tag nodes */
  std::unordered_map<std::string, std::string>
      reference_cache_;                                    // [target : node_id]
  std::unordered_map<std::string, std::string> tag_cache_; // [tag_name : id]

  /* Helpers */
  void reset();
  void extract_links(std::shared_ptr<MIGRNode> node);
  void extract_tags(std::shared_ptr<MIGRNode> node);
  void build_backlink_index();
  std::string classify_link_type(const std::string &target) const;

  /* Node Management */
  std::shared_ptr<MIGRNode>
  get_or_create_reference_node(const std::string &target);
  std::shared_ptr<MIGRNode> get_or_create_tag_node(const std::string &tag_name);
};

#endif //! MIGR_SEMANTIC_H
