#pragma once

#include "../../../core/graph/graph_compiler.hpp"
#include "../../../core/integration/control_lib_adapter.hpp"

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#ifndef BOOST_JSON_HEADER_ONLY
#define BOOST_JSON_HEADER_ONLY
#endif
#include <boost/json.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <deque>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>

namespace gkdviz::backend {

namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;
using tcp = boost::asio::ip::tcp;

template <typename Response>
inline void set_cors_headers(Response& response) {
  response.set(http::field::access_control_allow_origin, "*");
  response.set(http::field::access_control_allow_methods, "GET, POST, OPTIONS");
  response.set(http::field::access_control_allow_headers, "content-type");
}

inline std::string json_escape(std::string_view value) {
  std::string out;
  out.reserve(value.size() + 8);
  for (char ch : value) {
    switch (ch) {
      case '\\':
        out += "\\\\";
        break;
      case '"':
        out += "\\\"";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        out += ch;
        break;
    }
  }
  return out;
}

inline std::string to_string(gkdviz::core::ValueType value) {
  using enum gkdviz::core::ValueType;
  switch (value) {
    case kBool:
      return "bool";
    case kInt32:
      return "int32";
    case kInt64:
      return "int64";
    case kFloat32:
      return "float32";
    case kFloat64:
      return "float64";
    case kString:
      return "string";
    case kBytes:
      return "bytes";
    case kUnknown:
    default:
      return "unknown";
  }
}

inline std::string to_string(gkdviz::core::Semantic value) {
  using enum gkdviz::core::Semantic;
  switch (value) {
    case kGain:
      return "gain";
    case kVelocity:
      return "velocity";
    case kVelocityError:
      return "velocity_error";
    case kVelocityCommand:
      return "velocity_command";
    case kCurrent:
      return "current";
    case kControlCommand:
      return "control_command";
    case kTime:
      return "time";
    case kDebugValue:
      return "debug_value";
    case kNone:
    default:
      return "none";
  }
}

inline std::string to_string(gkdviz::core::Unit value) {
  using enum gkdviz::core::Unit;
  switch (value) {
    case kSecond:
      return "second";
    case kRpm:
      return "rpm";
    case kAmpere:
      return "ampere";
    case kNormalized:
      return "normalized";
    case kNone:
    default:
      return "none";
  }
}

inline std::string json_bool(bool value) {
  return value ? "true" : "false";
}

inline std::string json_field_default(const gkdviz::core::FieldDefaultValue& value) {
  return std::visit(
      [](const auto& current) -> std::string {
        using T = std::decay_t<decltype(current)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
          return "null";
        } else if constexpr (std::is_same_v<T, bool>) {
          return current ? "true" : "false";
        } else if constexpr (std::is_same_v<T, std::string>) {
          return "\"" + json_escape(current) + "\"";
        } else {
          return std::to_string(current);
        }
      },
      value);
}

inline std::string serialize_config_field(const gkdviz::core::FieldSchema& field) {
  std::ostringstream out;
  out << "{\"name\":\"" << json_escape(field.name) << "\""
      << ",\"value_type\":\"" << to_string(field.type.value_type) << "\""
      << ",\"semantic\":\"" << to_string(field.type.semantic) << "\""
      << ",\"unit\":\"" << to_string(field.type.unit) << "\""
      << ",\"default\":" << json_field_default(field.default_value)
      << ",\"plottable\":" << json_bool(field.annotation.plottable)
      << ",\"loggable\":" << json_bool(field.annotation.loggable);
  if (field.annotation.range.has_value()) {
    out << ",\"range\":[" << field.annotation.range->min << "," << field.annotation.range->max << "]";
  } else {
    out << ",\"range\":null";
  }
  out << "}";
  return out.str();
}

inline std::string serialize_port(const gkdviz::core::PortSchema& port) {
  std::ostringstream out;
  out << "{\"name\":\"" << json_escape(port.name) << "\""
      << ",\"value_type\":\"" << to_string(port.type.value_type) << "\""
      << ",\"semantic\":\"" << to_string(port.type.semantic) << "\""
      << ",\"unit\":\"" << to_string(port.type.unit) << "\""
      << ",\"plottable\":" << json_bool(port.plottable)
      << ",\"loggable\":" << json_bool(port.loggable) << "}";
  return out.str();
}

template <typename T>
inline std::string join_serialized(const std::vector<T>& items, std::string (*serialize)(const T&)) {
  std::ostringstream out;
  out << "[";
  for (size_t i = 0; i < items.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << serialize(items[i]);
  }
  out << "]";
  return out.str();
}

inline std::string serialize_node_schema(const gkdviz::core::NodeSchema& schema) {
  std::ostringstream out;
  out << "{\"type\":\"" << json_escape(schema.type) << "\""
      << ",\"display_name\":\"" << json_escape(schema.display_name) << "\""
      << ",\"category\":\"" << json_escape(schema.category) << "\""
      << ",\"config\":" << join_serialized(schema.config_fields, serialize_config_field)
      << ",\"inputs\":" << join_serialized(schema.input_ports, serialize_port)
      << ",\"outputs\":" << join_serialized(schema.output_ports, serialize_port)
      << ",\"runtime_type_id\":\"" << json_escape(schema.runtime_type_id) << "\"}";
  return out.str();
}

inline std::string serialize_catalog(const gkdviz::core::GraphCompiler::Catalog& catalog) {
  std::vector<std::reference_wrapper<const gkdviz::core::NodeSchema>> nodes;
  nodes.reserve(catalog.size());
  for (const auto& [_, schema] : catalog) {
    nodes.emplace_back(std::cref(schema));
  }
  std::ranges::sort(nodes, [](const auto& lhs, const auto& rhs) {
    return lhs.get().type < rhs.get().type;
  });

  std::ostringstream out;
  out << "{\"nodes\":[";
  for (size_t i = 0; i < nodes.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << serialize_node_schema(nodes[i].get());
  }
  out << "]}";
  return out.str();
}

inline gkdviz::core::ConfigValue parse_config_value(const json::value& value) {
  if (value.is_bool()) {
    return value.as_bool();
  }
  if (value.is_int64()) {
    return value.as_int64();
  }
  if (value.is_uint64()) {
    return static_cast<int64_t>(value.as_uint64());
  }
  if (value.is_double()) {
    return value.as_double();
  }
  if (value.is_string()) {
    return std::string(value.as_string());
  }
  return std::monostate{};
}

inline std::optional<gkdviz::core::GraphConfig> parse_graph_config(std::string_view body,
                                                                   std::string& error_message) {
  beast::error_code ec;
  const auto parsed = json::parse(body, ec);
  if (ec) {
    error_message = "invalid_json";
    return std::nullopt;
  }
  if (!parsed.is_object()) {
    error_message = "graph_payload_must_be_object";
    return std::nullopt;
  }

  const auto& object = parsed.as_object();
  const auto nodes_it = object.find("nodes");
  const auto edges_it = object.find("edges");
  if (nodes_it == object.end() || !nodes_it->value().is_array()) {
    error_message = "nodes_must_be_array";
    return std::nullopt;
  }
  if (edges_it == object.end() || !edges_it->value().is_array()) {
    error_message = "edges_must_be_array";
    return std::nullopt;
  }

  gkdviz::core::GraphConfig graph;
  for (const auto& node_value : nodes_it->value().as_array()) {
    if (!node_value.is_object()) {
      error_message = "node_must_be_object";
      return std::nullopt;
    }
    const auto& node_object = node_value.as_object();
    const auto id_it = node_object.find("id");
    const auto type_it = node_object.find("type");
    if (id_it == node_object.end() || !id_it->value().is_string()) {
      error_message = "node.id_must_be_string";
      return std::nullopt;
    }
    if (type_it == node_object.end() || !type_it->value().is_string()) {
      error_message = "node.type_must_be_string";
      return std::nullopt;
    }

    gkdviz::core::GraphNodeConfig node{
        .id = std::string(id_it->value().as_string()),
        .type = std::string(type_it->value().as_string()),
    };

    const auto config_it = node_object.find("config");
    if (config_it != node_object.end()) {
      if (!config_it->value().is_object()) {
        error_message = "node.config_must_be_object";
        return std::nullopt;
      }
      for (const auto& [key, value] : config_it->value().as_object()) {
        node.config.emplace(std::string(key), parse_config_value(value));
      }
    }
    graph.nodes.push_back(std::move(node));
  }

  for (const auto& edge_value : edges_it->value().as_array()) {
    if (!edge_value.is_object()) {
      error_message = "edge_must_be_object";
      return std::nullopt;
    }
    const auto& edge_object = edge_value.as_object();
    const auto from_it = edge_object.find("from");
    const auto to_it = edge_object.find("to");
    if (from_it == edge_object.end() || !from_it->value().is_string()) {
      error_message = "edge.from_must_be_string";
      return std::nullopt;
    }
    if (to_it == edge_object.end() || !to_it->value().is_string()) {
      error_message = "edge.to_must_be_string";
      return std::nullopt;
    }
    graph.edges.push_back(
        {.from = std::string(from_it->value().as_string()), .to = std::string(to_it->value().as_string())});
  }

  return graph;
}

inline std::string serialize_compile_result(const gkdviz::core::GraphConfig& graph,
                                            const gkdviz::core::GraphCompileResult& result) {
  std::ostringstream out;
  out << "{\"ok\":" << json_bool(result.ok()) << ",\"node_count\":" << graph.nodes.size()
      << ",\"edge_count\":" << graph.edges.size() << ",\"errors\":[";
  for (size_t i = 0; i < result.errors.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"message\":\"" << json_escape(result.errors[i].message) << "\"}";
  }
  out << "]}";
  return out.str();
}

struct GraphRunResult {
  bool ok{false};
  std::vector<std::string> logs;
  std::vector<gkdviz::core::GraphCompileError> errors;
};

inline std::string serialize_run_result(const GraphRunResult& result) {
  std::ostringstream out;
  out << "{\"ok\":" << json_bool(result.ok) << ",\"logs\":[";
  for (size_t i = 0; i < result.logs.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "\"" << json_escape(result.logs[i]) << "\"";
  }
  out << "],\"errors\":[";
  for (size_t i = 0; i < result.errors.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"message\":\"" << json_escape(result.errors[i].message) << "\"}";
  }
  out << "]}";
  return out.str();
}

inline std::optional<double> read_config_f64(const gkdviz::core::GraphNodeConfig& node,
                                             std::string_view key) {
  auto it = node.config.find(std::string(key));
  if (it == node.config.end()) {
    return std::nullopt;
  }
  if (const auto* v = std::get_if<double>(&it->second)) {
    return *v;
  }
  if (const auto* v = std::get_if<int64_t>(&it->second)) {
    return static_cast<double>(*v);
  }
  return std::nullopt;
}

inline std::optional<bool> read_config_bool(const gkdviz::core::GraphNodeConfig& node,
                                            std::string_view key) {
  auto it = node.config.find(std::string(key));
  if (it == node.config.end()) {
    return std::nullopt;
  }
  if (const auto* v = std::get_if<bool>(&it->second)) {
    return *v;
  }
  return std::nullopt;
}

inline std::optional<std::string> read_config_string(const gkdviz::core::GraphNodeConfig& node,
                                                     std::string_view key) {
  auto it = node.config.find(std::string(key));
  if (it == node.config.end()) {
    return std::nullopt;
  }
  if (const auto* v = std::get_if<std::string>(&it->second)) {
    return *v;
  }
  return std::nullopt;
}

inline GraphRunResult execute_graph_once(const gkdviz::core::GraphConfig& graph,
                                         const gkdviz::core::GraphCompiler::Catalog& catalog) {
  GraphRunResult result;
  gkdviz::core::GraphCompiler compiler;
  auto compile_result = compiler.compile(graph, catalog);
  if (!compile_result.ok()) {
    result.errors = compile_result.errors;
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

  std::unordered_map<std::string, const gkdviz::core::GraphNodeConfig*> node_by_id;
  std::unordered_map<std::string, std::vector<std::string>> out_adj;
  std::unordered_map<std::string, uint32_t> indegree;
  std::unordered_map<std::string, ParsedEndpoint> input_sources;
  for (const auto& node : graph.nodes) {
    node_by_id.emplace(node.id, &node);
    out_adj[node.id] = {};
    indegree[node.id] = 0;
  }
  for (const auto& edge : graph.edges) {
    auto from = parse_endpoint(edge.from);
    auto to = parse_endpoint(edge.to);
    if (!from || !to) {
      continue;
    }
    out_adj[from->node_id].push_back(to->node_id);
    indegree[to->node_id] += 1;
    input_sources[to->node_id + "." + to->port_name] = *from;
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

  using RuntimeValue = std::variant<std::monostate, bool, double, std::string>;
  std::unordered_map<std::string, RuntimeValue> outputs;

  auto read_input = [&](const std::string& node_id, const std::string& port_name) -> RuntimeValue {
    auto it = input_sources.find(node_id + "." + port_name);
    if (it == input_sources.end()) {
      return std::monostate{};
    }
    const auto& source = it->second;
    auto out_it = outputs.find(source.node_id + "." + source.port_name);
    if (out_it == outputs.end()) {
      return std::monostate{};
    }
    return out_it->second;
  };

  for (const auto& node_id : topo) {
    const auto& node = *node_by_id.at(node_id);
    if (node.type == "NumberInput") {
      outputs[node.id + ".out"] = read_config_f64(node, "value").value_or(0.0);
      continue;
    }
    if (node.type == "BoolInput") {
      outputs[node.id + ".out"] = read_config_bool(node, "value").value_or(false);
      continue;
    }
    if (node.type == "TextInput") {
      outputs[node.id + ".out"] = read_config_string(node, "value").value_or("");
      continue;
    }
    if (node.type == "SignalAdapter") {
      outputs[node.id + ".out"] = read_input(node.id, "in");
      continue;
    }
    if (node.type == "Comment") {
      continue;
    }
    if (node.type == "DoublePrinter") {
      auto value = read_input(node.id, "in");
      if (const auto* input = std::get_if<double>(&value)) {
        const auto doubled = (*input) * 2.0;
        std::ostringstream line;
        line << "[DoublePrinter] " << node.id << " input=" << *input << " doubled=" << doubled;
        result.logs.push_back(line.str());
        std::cout << line.str() << std::endl;
      } else {
        result.errors.push_back({"DoublePrinter requires float64 input on " + node.id + ".in"});
      }
      continue;
    }
    if (node.type == "ControlLibPid") {
      auto setpoint = read_input(node.id, "setpoint");
      auto feedback = read_input(node.id, "feedback");
      const auto* setpoint_value = std::get_if<double>(&setpoint);
      const auto* feedback_value = std::get_if<double>(&feedback);
      if (setpoint_value == nullptr || feedback_value == nullptr) {
        result.errors.push_back({"ControlLibPid requires float64 setpoint and feedback inputs on " + node.id});
        continue;
      }
      const gkdviz::core::ControlLibPidConfig config{
          .kp = read_config_f64(node, "kp").value_or(1.0),
          .ki = read_config_f64(node, "ki").value_or(0.0),
          .kd = read_config_f64(node, "kd").value_or(0.0),
          .max_out = read_config_f64(node, "max_out").value_or(100.0),
          .max_iout = read_config_f64(node, "max_iout").value_or(100.0),
      };
      gkdviz::core::ControlLibPidAdapter adapter(config);
      const auto out = adapter.process(*setpoint_value, *feedback_value);
      outputs[node.id + ".out"] = out;
      continue;
    }

    result.errors.push_back({"runtime execution not implemented for node type: " + node.type});
  }

  result.ok = result.errors.empty();
  return result;
}

template <typename Send>
void handle_request(const gkdviz::core::GraphCompiler::Catalog& catalog,
                    http::request<http::string_body>&& request, Send&& send) {
  if (request.method() == http::verb::options) {
    http::response<http::string_body> response{http::status::no_content, request.version()};
    set_cors_headers(response);
    response.prepare_payload();
    return send(std::move(response));
  }

  if (request.method() == http::verb::get && request.target() == "/node_catalog") {
    http::response<http::string_body> response{http::status::ok, request.version()};
    response.set(http::field::content_type, "application/json");
    set_cors_headers(response);
    response.body() = serialize_catalog(catalog);
    response.prepare_payload();
    return send(std::move(response));
  }

  if (request.method() == http::verb::post && request.target() == "/graph/compile") {
    std::string error_message;
    auto graph = parse_graph_config(request.body(), error_message);
    if (!graph.has_value()) {
      http::response<http::string_body> response{http::status::bad_request, request.version()};
      response.set(http::field::content_type, "application/json");
      set_cors_headers(response);
      response.body() = "{\"ok\":false,\"errors\":[{\"message\":\"" + json_escape(error_message) + "\"}]}";
      response.prepare_payload();
      return send(std::move(response));
    }

    gkdviz::core::GraphCompiler compiler;
    const auto result = compiler.compile(*graph, catalog);
    http::response<http::string_body> response{
        result.ok() ? http::status::ok : http::status::unprocessable_entity, request.version()};
    response.set(http::field::content_type, "application/json");
    set_cors_headers(response);
    response.body() = serialize_compile_result(*graph, result);
    response.prepare_payload();
    return send(std::move(response));
  }

  if (request.method() == http::verb::post && request.target() == "/graph/run") {
    std::string error_message;
    auto graph = parse_graph_config(request.body(), error_message);
    if (!graph.has_value()) {
      http::response<http::string_body> response{http::status::bad_request, request.version()};
      response.set(http::field::content_type, "application/json");
      set_cors_headers(response);
      response.body() = "{\"ok\":false,\"logs\":[],\"errors\":[{\"message\":\"" +
                        json_escape(error_message) + "\"}]}";
      response.prepare_payload();
      return send(std::move(response));
    }

    const auto result = execute_graph_once(*graph, catalog);
    http::response<http::string_body> response{
        result.ok ? http::status::ok : http::status::unprocessable_entity, request.version()};
    response.set(http::field::content_type, "application/json");
    set_cors_headers(response);
    response.body() = serialize_run_result(result);
    response.prepare_payload();
    return send(std::move(response));
  }

  if (request.method() != http::verb::get && request.method() != http::verb::post) {
    http::response<http::string_body> response{http::status::method_not_allowed, request.version()};
    response.set(http::field::content_type, "application/json");
    set_cors_headers(response);
    response.body() = "{\"error\":\"method_not_allowed\"}";
    response.prepare_payload();
    return send(std::move(response));
  }

  http::response<http::string_body> response{http::status::not_found, request.version()};
  response.set(http::field::content_type, "application/json");
  set_cors_headers(response);
  response.body() = "{\"error\":\"not_found\"}";
  response.prepare_payload();
  return send(std::move(response));
}

inline void serve_session(tcp::socket socket, const gkdviz::core::GraphCompiler::Catalog& catalog) {
  beast::flat_buffer buffer;
  http::request<http::string_body> request;
  beast::error_code ec;
  http::read(socket, buffer, request, ec);
  if (ec) {
    return;
  }

  auto sender = [&](auto&& response) {
    http::write(socket, response, ec);
  };
  handle_request(catalog, std::move(request), sender);
  socket.shutdown(tcp::socket::shutdown_send, ec);
}

inline void do_accept(tcp::acceptor& acceptor, const gkdviz::core::GraphCompiler::Catalog& catalog) {
  acceptor.async_accept([&acceptor, &catalog](beast::error_code ec, tcp::socket socket) {
    if (!ec) {
      serve_session(std::move(socket), catalog);
    }
    do_accept(acceptor, catalog);
  });
}

} // namespace gkdviz::backend
