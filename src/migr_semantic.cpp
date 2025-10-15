#include "migr_semantic.h"
#include "globals.h"
#include <algorithm>
#include <memory>

void SemanticLayer::add_node(std::shared_ptr<MIGRNode> node) {
  if (node) {
    semantic_nodes_[node->id_] = node;
  }
}

void SemanticLayer::remove_node(const std::string &node_id) {
  semantic_nodes_.erase(node_id);
  edges_.erase(node_id);
  backlinks_.erase(node_id);

  for (auto &edge_pair : edges_) {
    auto &edge_list = edge_pair.second;
    edge_list.erase(std::remove(edge_list.begin(), edge_list.end(), node_id),
                    edge_list.end());
  }
}

std::vector<std::shared_ptr<MIGRNode>>
SemanticLayer::query_nodes(std::function<bool(const MIGRNode &)> predicate) {
  std::vector<std::shared_ptr<MIGRNode>> query_results;
  for (const auto &[_, node] : semantic_nodes_) {
    if (node && predicate(*node)) {
      query_results.push_back(node);
    }
  }
  return query_results;
}

void SemanticLayer::serialize(std::ostream &out) const {
  out << "  \"semantic_layer\": {\n";
  out << "    \"edges\": [\n";

  bool first{true};
  for (const auto &[source, targets] : edges_) {
    for (const std::string &target : targets) {
      if (!first) {
        out << ",\n";
      }
      first = false;
      out << "      {\"source\": \"" << source << "\", \"target\": \"" << target
          << "\", \"type\": \"SEMANTIC_LINK\"}";
    }
  }
  out << "\n    ]\n";
  out << "  }\n";
}

void SemanticLayer::deserialize(std::istream &in) const {
  // todo: implement deserialization of data - would implement
  // json parsing as data will be in json format
  SPEAK << "Deserialization is not implemented yet" << std::endl;
}

void SemanticLayer::extract_semantics(const StructuralLayer &structural) {
  _V_ << "Extracting semantic info..." << std::endl;

  auto root = structural.get_root();
  if (!root) {
    _V_ << "No root node found while extracting semantics!" << std::endl;
    return;
  }

  extract_links(root);
  extract_tags(root);
  build_backlinks();
}

void SemanticLayer::add_cross_reference(const std::string &from_id,
                                        const std::string &to_id,
                                        const std::string &relation_type) {
  edges_[from_id].push_back(to_id);
  backlinks_[to_id].push_back(from_id);

  /* we can store relation type in metadata */
  auto from_it = semantic_nodes_.find(from_id);
  if (from_it != semantic_nodes_.end()) {
    from_it->second->metadata_["relation_type"] = relation_type;
  }
}

std::vector<std::shared_ptr<MIGRNode>>
SemanticLayer::find_backlinks(const std::string &target_id) {
  std::vector<std::shared_ptr<MIGRNode>> results;
  auto it = backlinks_.find(target_id);
  if (it != backlinks_.end()) {
    for (const std::string &bl_id : it->second) {
      auto node_it = semantic_nodes_.find(bl_id);
      if (node_it != semantic_nodes_.end()) {
        results.push_back(node_it->second);
      }
    }
  }
  return results;
}

void SemanticLayer::extract_links(std::shared_ptr<MIGRNode> node) {
  if (!node) {
    return;
  }

  if (node->type_ == MIGRNodeType::LINK) {
    std::string target_url = node->metadata_["url"];

    /* a reference semantic node */
    auto ref_node =
        std::make_shared<MIGRNode>(MIGRNodeType::REFERENCE, target_url);
    ref_node->metadata_["source_node"] = node->id_;
    ref_node->metadata_["target"] = target_url;

    add_node(ref_node);

    // linking by creating edge link_node -> ref_node
    add_cross_reference(node->id_, ref_node->id_, "references");

    /* if link is internal, considering links not starting with 'http/https' */
    if (target_url.find("http://") != 0 && target_url.find("https://") != 0) {
      // internal wiki-link
      ref_node->metadata_["link_type"] = "internal";

      /* backlink resolution (cross document)
       * format: target_document -> list of source nodes */
      backlinks_[target_url].push_back(node->id_);
    } else {
      ref_node->metadata_["link_type"] = "external";
    }
  }

  // recursively processing the children
  for (const auto &child : node->children_) {
    extract_links(child);
  }
}

void SemanticLayer::extract_tags(std::shared_ptr<MIGRNode> node) {
  if (!node)
    return;

  // let's do this: tag will be like this [[#tag]] Creole-style tag links
  if (node->type_ == MIGRNodeType::LINK && !node->metadata_["url"].empty()) {
    std::string url = node->metadata_["url"];
    if (url.length() > 0 && url[0] == '#') {
      std::string tag_name = url.substr(1); // Remove #

      auto tag_node = std::make_shared<MIGRNode>(MIGRNodeType::TAG, tag_name);
      tag_node->metadata_["tag_name"] = tag_name;
      tag_node->metadata_["source_node"] = node->id_;

      add_node(tag_node);
      add_cross_reference(node->id_, tag_node->id_, "tagged_with");
    }
  }

  // recursively processing the children
  for (const auto &child : node->children_) {
    extract_tags(child);
  }
}

void SemanticLayer::build_backlinks() {
  backlinks_.clear();
  for (const auto &[source, targets] : edges_) {
    for (const auto &target : targets) {
      backlinks_[target].push_back(source);
    }
  }
}
