#pragma once

#include "graph_config.hpp"
#include "runtime_plan.hpp"
#include "../reflection/node_schema.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <unordered_map>
#include <vector>

namespace gkdviz::core {

struct GraphCompileError {
  std::string message;
};

struct GraphCompileResult {
  std::optional<RuntimePlan> plan;
  std::vector<GraphCompileError> errors;

  [[nodiscard]] bool ok() const { return plan.has_value() && errors.empty(); }
};

class GraphCompiler {
public:
  using Catalog = std::unordered_map<std::string, NodeSchema>;

  [[nodiscard]] GraphCompileResult compile(const GraphConfig& graph, const Catalog& catalog) const {
    GraphCompileResult result;
    std::unordered_map<std::string, const GraphNodeConfig*> node_by_id;
    node_by_id.reserve(graph.nodes.size());

    for (const auto& node : graph.nodes) {
      if (node.id.empty()) {
        result.errors.push_back({"node id must not be empty"});
      }
      if (!node_by_id.emplace(node.id, &node).second) {
        result.errors.push_back({"duplicate node id: " + node.id});
      }
      if (!catalog.contains(node.type)) {
        result.errors.push_back({"unknown node type: " + node.type});
      }
    }

    if (!result.errors.empty()) {
      return result;
    }

    std::unordered_map<std::string, NodeSchema> resolved_schemas;
    resolved_schemas.reserve(graph.nodes.size());
    for (const auto& node : graph.nodes) {
      resolved_schemas.emplace(node.id, resolve_node_schema(node, catalog.at(node.type), result.errors));
    }

    if (!result.errors.empty()) {
      return result;
    }

    struct ParsedEndpoint {
      std::string node_id;
      std::string port_name;
    };
    auto parse_endpoint = [](const std::string& value) -> std::optional<ParsedEndpoint> {
      auto dot = value.find('.');
      if (dot == std::string::npos || dot == 0 || dot + 1 >= value.size()) {
        return std::nullopt;
      }
      return ParsedEndpoint{value.substr(0, dot), value.substr(dot + 1)};
    };

    struct ParsedEdge {
      ParsedEndpoint from;
      ParsedEndpoint to;
    };
    std::vector<ParsedEdge> parsed_edges;
    parsed_edges.reserve(graph.edges.size());

    std::unordered_map<std::string, std::vector<std::string>> out_adj;
    std::unordered_map<std::string, uint32_t> indegree;
    for (const auto& node : graph.nodes) {
      out_adj[node.id] = {};
      indegree[node.id] = 0;
    }

    for (const auto& edge : graph.edges) {
      auto from = parse_endpoint(edge.from);
      auto to = parse_endpoint(edge.to);
      if (!from.has_value() || !to.has_value()) {
        result.errors.push_back({"invalid edge endpoint format, expected node.port"});
        continue;
      }
      if (!node_by_id.contains(from->node_id)) {
        result.errors.push_back({"edge source node not found: " + from->node_id});
        continue;
      }
      if (!node_by_id.contains(to->node_id)) {
        result.errors.push_back({"edge target node not found: " + to->node_id});
        continue;
      }

      const auto& from_schema = resolved_schemas.at(from->node_id);
      const auto& to_schema = resolved_schemas.at(to->node_id);

      auto out_it = std::find_if(
          from_schema.output_ports.begin(), from_schema.output_ports.end(),
          [&](const PortSchema& port) { return port.name == from->port_name; });
      if (out_it == from_schema.output_ports.end()) {
        result.errors.push_back({"source output port not found: " + edge.from});
        continue;
      }

      auto in_it = std::find_if(
          to_schema.input_ports.begin(), to_schema.input_ports.end(),
          [&](const PortSchema& port) { return port.name == to->port_name; });
      if (in_it == to_schema.input_ports.end()) {
        result.errors.push_back({"target input port not found: " + edge.to});
        continue;
      }

      if (!is_compatible(out_it->type, in_it->type)) {
        result.errors.push_back({"incompatible edge types: " + edge.from + " -> " + edge.to});
        continue;
      }

      parsed_edges.push_back({*from, *to});
      out_adj[from->node_id].push_back(to->node_id);
      indegree[to->node_id] += 1;
    }

    if (!result.errors.empty()) {
      return result;
    }

    std::deque<std::string> ready;
    for (const auto& [node_id, degree] : indegree) {
      if (degree == 0) {
        ready.push_back(node_id);
      }
    }

    std::vector<std::string> topo;
    topo.reserve(graph.nodes.size());
    while (!ready.empty()) {
      auto id = ready.front();
      ready.pop_front();
      topo.push_back(id);
      for (const auto& next : out_adj[id]) {
        auto& degree = indegree[next];
        if (--degree == 0) {
          ready.push_back(next);
        }
      }
    }

    if (topo.size() != graph.nodes.size()) {
      result.errors.push_back({"graph contains cycle"});
      return result;
    }

    RuntimePlan plan;
    plan.slots.reserve(parsed_edges.size());
    plan.nodes_in_order.reserve(topo.size());

    for (size_t i = 0; i < topo.size(); ++i) {
      CompiledNode node{};
      node.id = static_cast<NodeInstanceId>(i + 1);
      plan.nodes_in_order.push_back(node);
    }

    for (size_t slot = 0; slot < parsed_edges.size(); ++slot) {
      (void)slot; // slot ids are allocated by edge order in current skeleton.
    }

    result.plan = std::move(plan);
    return result;
  }

private:
  [[nodiscard]] static NodeSchema resolve_node_schema(const GraphNodeConfig& node,
                                                      const NodeSchema& base_schema,
                                                      std::vector<GraphCompileError>& errors) {
    if (node.type != "SignalAdapter") {
      return base_schema;
    }

    NodeSchema resolved = base_schema;

    auto input_value_type = parse_value_type(read_string_config(node, "input_value_type", "float64"));
    auto input_semantic = parse_semantic(read_string_config(node, "input_semantic", "none"));
    auto input_unit = parse_unit(read_string_config(node, "input_unit", "none"));
    auto output_value_type = parse_value_type(read_string_config(node, "output_value_type", "float64"));
    auto output_semantic = parse_semantic(read_string_config(node, "output_semantic", "none"));
    auto output_unit = parse_unit(read_string_config(node, "output_unit", "none"));

    if (!input_value_type.has_value()) {
      errors.push_back({"invalid SignalAdapter input_value_type on node: " + node.id});
      input_value_type = ValueType::kUnknown;
    }
    if (!input_semantic.has_value()) {
      errors.push_back({"invalid SignalAdapter input_semantic on node: " + node.id});
      input_semantic = Semantic::kNone;
    }
    if (!input_unit.has_value()) {
      errors.push_back({"invalid SignalAdapter input_unit on node: " + node.id});
      input_unit = Unit::kNone;
    }
    if (!output_value_type.has_value()) {
      errors.push_back({"invalid SignalAdapter output_value_type on node: " + node.id});
      output_value_type = ValueType::kUnknown;
    }
    if (!output_semantic.has_value()) {
      errors.push_back({"invalid SignalAdapter output_semantic on node: " + node.id});
      output_semantic = Semantic::kNone;
    }
    if (!output_unit.has_value()) {
      errors.push_back({"invalid SignalAdapter output_unit on node: " + node.id});
      output_unit = Unit::kNone;
    }

    if (!resolved.input_ports.empty()) {
      resolved.input_ports[0].type = {
          .value_type = *input_value_type,
          .semantic = *input_semantic,
          .unit = *input_unit,
      };
    }
    if (!resolved.output_ports.empty()) {
      resolved.output_ports[0].type = {
          .value_type = *output_value_type,
          .semantic = *output_semantic,
          .unit = *output_unit,
      };
    }
    return resolved;
  }

  [[nodiscard]] static std::string read_string_config(const GraphNodeConfig& node,
                                                      const std::string& key,
                                                      const std::string& fallback) {
    auto it = node.config.find(key);
    if (it == node.config.end()) {
      return fallback;
    }
    if (const auto* value = std::get_if<std::string>(&it->second)) {
      return *value;
    }
    return fallback;
  }

  [[nodiscard]] static std::optional<ValueType> parse_value_type(const std::string& value) {
    static constexpr std::array<std::pair<std::string_view, ValueType>, 8> kMap{{
        {"unknown", ValueType::kUnknown},
        {"bool", ValueType::kBool},
        {"int32", ValueType::kInt32},
        {"int64", ValueType::kInt64},
        {"float32", ValueType::kFloat32},
        {"float64", ValueType::kFloat64},
        {"string", ValueType::kString},
        {"bytes", ValueType::kBytes},
    }};
    for (const auto& [name, mapped] : kMap) {
      if (value == name) {
        return mapped;
      }
    }
    return std::nullopt;
  }

  [[nodiscard]] static std::optional<Semantic> parse_semantic(const std::string& value) {
    static constexpr std::array<std::pair<std::string_view, Semantic>, 9> kMap{{
        {"none", Semantic::kNone},
        {"gain", Semantic::kGain},
        {"velocity", Semantic::kVelocity},
        {"velocity_error", Semantic::kVelocityError},
        {"velocity_command", Semantic::kVelocityCommand},
        {"current", Semantic::kCurrent},
        {"control_command", Semantic::kControlCommand},
        {"time", Semantic::kTime},
        {"debug_value", Semantic::kDebugValue},
    }};
    for (const auto& [name, mapped] : kMap) {
      if (value == name) {
        return mapped;
      }
    }
    return std::nullopt;
  }

  [[nodiscard]] static std::optional<Unit> parse_unit(const std::string& value) {
    static constexpr std::array<std::pair<std::string_view, Unit>, 5> kMap{{
        {"none", Unit::kNone},
        {"second", Unit::kSecond},
        {"rpm", Unit::kRpm},
        {"ampere", Unit::kAmpere},
        {"normalized", Unit::kNormalized},
    }};
    for (const auto& [name, mapped] : kMap) {
      if (value == name) {
        return mapped;
      }
    }
    return std::nullopt;
  }
};

} // namespace gkdviz::core
