#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/json/src.hpp>

#include <iostream>

#include "../../../core/algorithm/demo_pid.hpp"
#include "../../../core/command/command_channel.hpp"
#include "../../../core/graph/graph_compiler.hpp"
#include "../../../core/reflection/node_schema_generator.hpp"
#include "../../../core/reflection/static_reflection_provider.hpp"
#include "../../../core/safety/safety_manager.hpp"
#include "../../../core/telemetry/telemetry_buffer.hpp"
#include "node_catalog_http.hpp"

namespace {

gkdviz::core::GraphCompiler::Catalog build_catalog() {
  gkdviz::core::StaticReflectionProvider provider;
  provider.register_algorithm<gkdviz::core::DemoPid>();

  gkdviz::core::NodeSchemaGenerator generator(provider);
  gkdviz::core::GraphCompiler::Catalog catalog;
  auto pid_schema = generator.generate<gkdviz::core::DemoPid>();
  catalog.emplace(pid_schema.type, std::move(pid_schema));
  catalog.emplace(
      "VelocitySource",
      gkdviz::core::NodeSchema{
          .type = "VelocitySource",
          .display_name = "Velocity Source",
          .category = "Builtin",
          .output_ports =
              {
                  gkdviz::core::PortSchema{
                      .name = "out",
                      .type =
                          {
                              .value_type = gkdviz::core::ValueType::kFloat64,
                              .semantic = gkdviz::core::Semantic::kVelocity,
                              .unit = gkdviz::core::Unit::kRpm,
                          },
                  },
              },
      });
  catalog.emplace(
      "TimeSource",
      gkdviz::core::NodeSchema{
          .type = "TimeSource",
          .display_name = "Time Source",
          .category = "Builtin",
          .output_ports =
              {
                  gkdviz::core::PortSchema{
                      .name = "out",
                      .type =
                          {
                              .value_type = gkdviz::core::ValueType::kFloat64,
                              .semantic = gkdviz::core::Semantic::kTime,
                              .unit = gkdviz::core::Unit::kSecond,
                          },
                  },
              },
      });
  catalog.emplace(
      "NumberInput",
      gkdviz::core::NodeSchema{
          .type = "NumberInput",
          .display_name = "Number Input",
          .category = "Input",
          .config_fields =
              {
                  gkdviz::core::FieldSchema{
                      .name = "value",
                      .type = {.value_type = gkdviz::core::ValueType::kFloat64},
                      .default_value = 0.0,
                  },
              },
          .output_ports =
              {
                  gkdviz::core::PortSchema{
                      .name = "out",
                      .type =
                          {
                              .value_type = gkdviz::core::ValueType::kFloat64,
                              .semantic = gkdviz::core::Semantic::kNone,
                              .unit = gkdviz::core::Unit::kNone,
                          },
                  },
              },
      });
  catalog.emplace(
      "BoolInput",
      gkdviz::core::NodeSchema{
          .type = "BoolInput",
          .display_name = "Bool Input",
          .category = "Input",
          .config_fields =
              {
                  gkdviz::core::FieldSchema{
                      .name = "value",
                      .type = {.value_type = gkdviz::core::ValueType::kBool},
                      .default_value = false,
                  },
              },
          .output_ports =
              {
                  gkdviz::core::PortSchema{
                      .name = "out",
                      .type =
                          {
                              .value_type = gkdviz::core::ValueType::kBool,
                              .semantic = gkdviz::core::Semantic::kNone,
                              .unit = gkdviz::core::Unit::kNone,
                          },
                  },
              },
      });
  catalog.emplace(
      "TextInput",
      gkdviz::core::NodeSchema{
          .type = "TextInput",
          .display_name = "Text Input",
          .category = "Input",
          .config_fields =
              {
                  gkdviz::core::FieldSchema{
                      .name = "value",
                      .type = {.value_type = gkdviz::core::ValueType::kString},
                      .default_value = std::string(""),
                  },
              },
          .output_ports =
              {
                  gkdviz::core::PortSchema{
                      .name = "out",
                      .type =
                          {
                              .value_type = gkdviz::core::ValueType::kString,
                              .semantic = gkdviz::core::Semantic::kNone,
                              .unit = gkdviz::core::Unit::kNone,
                          },
                  },
              },
      });
  catalog.emplace(
      "DoublePrinter",
      gkdviz::core::NodeSchema{
          .type = "DoublePrinter",
          .display_name = "Double Printer",
          .category = "Test",
          .input_ports =
              {
                  gkdviz::core::PortSchema{
                      .name = "in",
                      .type =
                          {
                              .value_type = gkdviz::core::ValueType::kFloat64,
                              .semantic = gkdviz::core::Semantic::kNone,
                              .unit = gkdviz::core::Unit::kNone,
                          },
                  },
              },
      });
  catalog.emplace(
      "Comment",
      gkdviz::core::NodeSchema{
          .type = "Comment",
          .display_name = "Comment",
          .category = "Utility",
          .config_fields =
              {
                  gkdviz::core::FieldSchema{
                      .name = "title",
                      .type = {.value_type = gkdviz::core::ValueType::kString},
                      .default_value = std::string("Comment"),
                  },
                  gkdviz::core::FieldSchema{
                      .name = "text",
                      .type = {.value_type = gkdviz::core::ValueType::kString},
                      .default_value = std::string(""),
                  },
              },
      });
  catalog.emplace(
      "SignalAdapter",
      gkdviz::core::NodeSchema{
          .type = "SignalAdapter",
          .display_name = "Signal Adapter",
          .category = "Adapter",
          .config_fields =
              {
                  gkdviz::core::FieldSchema{
                      .name = "input_value_type",
                      .type = {.value_type = gkdviz::core::ValueType::kString},
                      .default_value = std::string("float64"),
                  },
                  gkdviz::core::FieldSchema{
                      .name = "input_semantic",
                      .type = {.value_type = gkdviz::core::ValueType::kString},
                      .default_value = std::string("none"),
                  },
                  gkdviz::core::FieldSchema{
                      .name = "input_unit",
                      .type = {.value_type = gkdviz::core::ValueType::kString},
                      .default_value = std::string("none"),
                  },
                  gkdviz::core::FieldSchema{
                      .name = "output_value_type",
                      .type = {.value_type = gkdviz::core::ValueType::kString},
                      .default_value = std::string("float64"),
                  },
                  gkdviz::core::FieldSchema{
                      .name = "output_semantic",
                      .type = {.value_type = gkdviz::core::ValueType::kString},
                      .default_value = std::string("none"),
                  },
                  gkdviz::core::FieldSchema{
                      .name = "output_unit",
                      .type = {.value_type = gkdviz::core::ValueType::kString},
                      .default_value = std::string("none"),
                  },
              },
          .input_ports =
              {
                  gkdviz::core::PortSchema{
                      .name = "in",
                      .type =
                          {
                              .value_type = gkdviz::core::ValueType::kUnknown,
                              .semantic = gkdviz::core::Semantic::kNone,
                              .unit = gkdviz::core::Unit::kNone,
                          },
                  },
              },
          .output_ports =
              {
                  gkdviz::core::PortSchema{
                      .name = "out",
                      .type =
                          {
                              .value_type = gkdviz::core::ValueType::kUnknown,
                              .semantic = gkdviz::core::Semantic::kNone,
                              .unit = gkdviz::core::Unit::kNone,
                          },
                  },
              },
      });
  catalog.emplace(
      "CommandSink",
      gkdviz::core::NodeSchema{
          .type = "CommandSink",
          .display_name = "Command Sink",
          .category = "Builtin",
          .input_ports =
              {
                  gkdviz::core::PortSchema{
                      .name = "in",
                      .type =
                          {
                              .value_type = gkdviz::core::ValueType::kFloat64,
                              .semantic = gkdviz::core::Semantic::kControlCommand,
                              .unit = gkdviz::core::Unit::kNormalized,
                          },
                  },
              },
      });
  return catalog;
}

} // namespace

int main() {
  boost::asio::io_context io;

  auto catalog = build_catalog();
  gkdviz::core::GraphCompiler compiler;
  gkdviz::core::GraphConfig graph{
      .nodes =
          {
              {.id = "target1", .type = "VelocitySource"},
              {.id = "actual1", .type = "VelocitySource"},
              {.id = "dt1", .type = "TimeSource"},
              {.id = "pid1", .type = "DemoPid"},
              {.id = "sink1", .type = "CommandSink"},
          },
      .edges =
          {
              {.from = "target1.out", .to = "pid1.target"},
              {.from = "actual1.out", .to = "pid1.actual"},
              {.from = "dt1.out", .to = "pid1.dt"},
              {.from = "pid1.control", .to = "sink1.in"},
          },
  };
  auto compile_result = compiler.compile(graph, catalog);
  if (!compile_result.ok()) {
    std::cerr << "graph compile failed during startup validation" << std::endl;
    return 1;
  }

  gkdviz::core::CommandChannel cmd_channel;
  gkdviz::core::SafetyManager safety;
  gkdviz::core::RuntimeCommand cmd{gkdviz::core::RuntimeCommandType::kStartGraph, "{}"};
  cmd_channel.push(cmd);
  auto verdict = safety.evaluate(cmd);
  (void)verdict;

  gkdviz::core::TelemetryBuffer telemetry(64);
  telemetry.push({"backend.heartbeat", 1.0, std::chrono::steady_clock::now()});

  boost::asio::ip::tcp::acceptor acceptor(
      io, {boost::asio::ip::make_address("127.0.0.1"), 8080});
  gkdviz::backend::do_accept(acceptor, catalog);

  boost::asio::signal_set signals(io, SIGINT, SIGTERM);
  signals.async_wait([&](const boost::system::error_code&, int signal_number) {
    std::cout << "gkdviz-backend: received signal " << signal_number << ", shutting down" << std::endl;
    acceptor.close();
    io.stop();
  });

  std::cout << "gkdviz-backend started on http://127.0.0.1:8080/node_catalog" << std::endl;
  io.run();
  std::cout << "gkdviz-backend stopped" << std::endl;
  return 0;
}
