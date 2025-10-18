#include "migr.h"
#include "globals.h"
#include <algorithm>
#include <iostream>
#include <sstream>

size_t MIGRNode::next_id_ = 1;

MIGRNode::MIGRNode(MIGRNodeType type, const std::string &c)
    : type_(type), content_(c), loc_(0), version_(1) {
  generate_id();
  update_hash();
}

/*
 * generates id in format node_${id} and increments the id
 */
void MIGRNode::generate_id() { id_ = "node_" + std::to_string(next_id_++); }

/*
 * Recomputes the content hash of this node.
 * Combines the node's content with its type and hashes the result.
 * Used in our code for detecting changes/versioning.
 * note: not perfect yet, does not use perfect hashing
 */
void MIGRNode::update_hash() {
  std::hash<std::string> hasher;
  content_hash_ = std::to_string(
      hasher(content_ + std::to_string(static_cast<int>(type_))));
}

/*
 * Adds a child node to this node.
 * If the child pointer is valid, it appends to the children list and sets this
 * node as the parent.
 */
void MIGRNode::add_child(std::shared_ptr<MIGRNode> child) {
  if (child) {
    children_.push_back(child);
    child->parent_ = shared_from_this();
  } else {
    _V_ << " [MIGRNode] [Warning] Child for id: " << shared_from_this()->id_
        << "Was null when add_child was called on it" << std::endl;
  }
}

/*
 * Removes a child node based on its ID.
 * Searches the children list and removes any node matching the given ID.
 */
void MIGRNode::remove_child(const std::string &child_id) {
  children_.erase(
      std::remove_if(children_.begin(), children_.end(),
                     [&child_id](const std::shared_ptr<MIGRNode> &child) {
                       return child && child->id_ == child_id;
                     }),
      children_.end());
}

/*
 * Finds a direct child node by ID.
 * Returns a shared_ptr to the child if found, otherwise nullptr.
 */
std::shared_ptr<MIGRNode> MIGRNode::find_child(const std::string &id) {
  for (auto &child : children_) {
    if (child && child->id_ == id) {
      return child;
    }
  }
  return nullptr;
}

/*
 * Updates the content of the node if it has changed.
 * Increments the version counter and updates the content hash accordingly.
 */
void MIGRNode::update_content(const std::string &new_content) {
  if (content_ != new_content) {
    content_ = new_content;
    version_++;
    update_hash();
  }
}

/*
 * Creates a human-readable string summary for the node.
 * Shows id, type, location, content, counts of children, and version.
 */
std::string MIGRNode::to_string() const {
  std::ostringstream oss;
  oss << "MIGRNode:\n"
      << "id: " << id_ << "\n"
      << "type: " << static_cast<int>(type_) << "\n"
      << "loc: " << loc_ << "\n"
      << "content: "
      << (content_.empty()
              ? "[empty]"
              : content_.substr(0, 50) + (content_.size() > 50 ? "..." : ""))
      << "\n"
      << "children: " << children_.size() << "\n"
      << "version: " << version_ << "\n";
  return oss.str();
}

/*
 * Recursively prints this node and its children in a tree format.
 * Indents output according to depth to visualize hierarchy.
 * todo: maybe make it generic to be used by any output stream
 */
void MIGRNode::print_tree(int depth) const {
  std::string indent(depth + 2, ' ');
  std::cout << indent << "- " << to_string() << std::endl;
  for (const auto &child : children_) {
    if (child) {
      child->print_tree(depth + 1);
    }
  }
}
