#ifndef MIGR_STRUCTURAL_H
#define MIGR_STRUCTURAL_H

#include "b_lexer.h"
#include "error.h"
#include "migr.h"
#include <stack>

enum class RecoveryStrategy {
  SKIP,
  ATTACH_TO_PARENT,
  CREATE_PLACEHOLDER,
};

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

  /* Error Recovery */
  void set_recovery_stratgegy(RecoveryStrategy strategy);
  const std::vector<MIGRError> &ger_errors();
  void clear_errors();

private:
  std::shared_ptr<MIGRNode> root_;
  std::unordered_map<std::string, std::shared_ptr<MIGRNode>>
      nodes_; // [id : node]
  RecoveryStrategy recovery_strategy_;

  /* parsing state */
  std::stack<std::shared_ptr<MIGRNode>> parent_stack_;
  std::stack<std::shared_ptr<MIGRNode>> list_stack_;
  std::vector<MIGRError> errors_;

  /* Token Processing Helpers */
  void process_heading_token(const BToken &token);
  void process_paragraph_token(const BToken &token);
  void process_ulist_token(const BToken &token);
  void process_olist_token(const BToken &token);
  void process_horizontal_rule_token(const BToken &token);
  void process_verbatim_token(const BToken &token);
  void process_image_token(const BToken &token);
  void process_newline_token(const BToken &token);

  /* stack manangement */
  void manage_heading_stack(int heading_level);
  void enter_list_context(MIGRNodeType list_type);
  void exit_list_context();
  bool in_list_context() const;

  /* inline processing */
  void process_inline_content(std::shared_ptr<MIGRNode> parent,
                              const std::string &content);

  /* Error Handling */
  void handle_error(const std::string &message, size_t line);
  bool attempt_recovery(const BToken &token);
};

#endif //! MIGR_STRUCTURAL_H
