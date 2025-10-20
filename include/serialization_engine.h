#ifndef SERIALIZATION_ENGINE_H
#define SERIALIZATION_ENGINE_H

#include "migr.h"
#include "rapidjson/writer.h"

using Writer = rapidjson::Writer<rapidjson::StringBuffer>;

class SerialzationEngine {
public:
  /* Node Serialzation */
  static void write_node(Writer &w, const std::shared_ptr<MIGRNode> &node);

  /* Node Map Serialzation */
  static void write_nodes(
      Writer &w,
      const std::unordered_map<std::string, std::shared_ptr<MIGRNode>> &nodes);

  /* Edge Serialzation */
  template <typename EdgeType>
  static void write_edge(Writer &w, const EdgeType &edge);

  /* Edge Array Serialzation */
  template <typename EdgeType>
  static void write_edges(Writer &w, const std::vector<EdgeType> &edges);

  /* String-To-String Map */
  static void
  write_map(Writer &w, const char *key,
            const std::unordered_map<std::string, std::string> &map);

  /* String-To-Vector Map (for indexes) */
  static void
  write_index(Writer &w, const char *key,
              const std::unordered_map<std::string, std::vector<size_t>> &idx);
};

#endif //! SERIALIZATION_ENGINE_H
