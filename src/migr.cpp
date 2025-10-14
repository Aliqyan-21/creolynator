#include "migr.h"
#include <algorithm>
#include <iostream>
#include <sstream>

size_t MIGRNode::next_id_ = 1;

MIGRNode::MIGRNode(MIGRNodeType type, const std::string &c)
    : type_(type), content_(c), version_(1) {
  generate_id();
  update_hash();
}

void MIGRNode::generate_id() { id_ = "node_" + std::to_string(next_id_++); }

void MIGRNode::update_hash() {
  std::hash<std::string> hasher;
  content_hash_ = std::to_string(
      hasher(content_ + std::to_string(static_cast<int>(type_))));
}

void MIGRNode::add_child(std::shared_ptr<MIGRNode> child) {
  if (child) {
    children_.push_back(child);
    child->parent_ = shared_from_this();
  }
}

void MIGRNode::add_semantic_link(std::shared_ptr<MIGRNode> target) {
  if (target && std::find(semantic_links_.begin(), semantic_links_.end(),
                          target) == semantic_links_.end()) {
    semantic_links_.push_back(target);
  }
}

void MIGRNode::remove_child(const std::string &child_id) {
  children_.erase(
      std::remove_if(children_.begin(), children_.end(),
                     [&child_id](const std::shared_ptr<MIGRNode> &child) {
                       return child && child->id_ == child_id;
                     }),
      children_.end());
}

std::shared_ptr<MIGRNode> MIGRNode::find_child(const std::string &id) {
  for (auto &child : children_) {
    if (child && child->id_ == id) {
      return child;
    }
  }
  return nullptr;
}

void MIGRNode::update_content(const std::string &new_content) {
  if (content_ != new_content) {
    content_ = new_content;
    version_++;
    update_hash();
  }
}

std::string MIGRNode::to_string() const {
  std::ostringstream oss;
  oss << "MIGRNode:\n"
      << "id: " << id_ << "\n"
      << "type: " << static_cast<int>(type_) << "\n"
      << "content: " << content_ << "\n"
      << "sementic_links: " << semantic_links_.size() << "\n"
      << "version: " << version_ << "\n";
  return oss.str();
}

void MIGRNode::print_tree(int depth) const {
  std::string indent(depth + 2, ' ');
  std::cout << indent << "- " << to_string() << std::endl;
  for (const auto &child : children_) {
    if (child) {
      child->print_tree(depth + 1);
    }
  }
}
