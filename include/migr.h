#ifndef MIGR_H
#define MIGR_H

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <memory>
enum class MIGRNodeType {
  /* structural */
  DOCUMENT_ROOT,
  HEADING,
  PARAGRAPH,
  LIST,
  LIST_ITEM,
  VERBATIM,

  /* inline */
  TEXT,
  BOLD,
  ITALIC,
  LINK,
  IMAGE,

  /* semantic */
  TAG,
  REFERENCE,
  FOOTNOTE,
  COMMENT,

  /* across docs */
  BACKLINK,
  EXTERNAL_REF,
};

enum class MIGREdgeType {
  STRUCTURAL_CHILD,
  SEMANTIC_LINK,
  BACKLINK,
  CROSS_REFERENCE,
  TAG_RELATION,
};

class MIGRNode : public std::enable_shared_from_this<MIGRNode> {
public:
  std::string id_;
  MIGRNodeType type_;
  std::string content_;
  std::unordered_map<std::string, std::string> metadata_;

  /* structural edges (tree) */
  std::vector<std::shared_ptr<MIGRNode>> children_;
  std::weak_ptr<MIGRNode> parent_;

  /* sementic edges (graph) */
  std::vector<std::shared_ptr<MIGRNode>> semantic_links_;

  /* versioning */
  size_t version_{0};
  std::string content_hash_;

  MIGRNode(MIGRNodeType type, const std::string &c = "");

  /* core operations */
  void add_child(std::shared_ptr<MIGRNode> child);
  void add_semantic_link(std::shared_ptr<MIGRNode> target);
  void remove_child(const std::string &child_id);
  std::shared_ptr<MIGRNode> find_child(const std::string &id);
  void update_content(const std::string &new_content);

  /* utility */
  std::string to_string() const;
  void print_tree(int depth = 0) const;

private:
  void generate_id();
  void update_hash();
  static size_t next_id_;
};

/* MIGRGraphLayer: Interface for Layer Management */
class MIGRGraphLayer {
public:
  virtual ~MIGRGraphLayer() = default;
  virtual void add_node(std::shared_ptr<MIGRNode> node) = 0;
  virtual void remove_node(std::shared_ptr<MIGRNode> node_id) = 0;
  virtual std::vector<std::shared_ptr<MIGRNode>>
  query_nodes(std::function<bool(const MIGRNode &)> predicate) = 0;
  virtual void serialize(std::ostream &out) const = 0;
  virtual void deserialize(std::istream &in) const = 0;
};

#endif //! MIGR_H
