#pragma once

#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace gkdviz::core {

using ConfigValue = std::variant<std::monostate, bool, int64_t, double, std::string>;

struct GraphNodeConfig {
  std::string id;
  std::string type;
  std::unordered_map<std::string, ConfigValue> config;
};

struct GraphEdgeConfig {
  std::string from;
  std::string to;
};

struct GraphConfig {
  std::vector<GraphNodeConfig> nodes;
  std::vector<GraphEdgeConfig> edges;
};

} // namespace gkdviz::core
