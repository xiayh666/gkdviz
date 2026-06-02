#include "../../../core/reflection/auto_reflection.hpp"
#include "../../../core/graph/graph_compiler.hpp"
#include "../../../core/reflection/node_schema_generator.hpp"
#include "../../../core/reflection/static_reflection_provider.hpp"

#include <cmath>
#include <iostream>
#include <string>
#include <typeinfo>

namespace {

struct Pid {
  struct Config {
    [[= gkdviz::core::SemanticAnnotation{gkdviz::core::Semantic::kGain}]]
    double kp{1.0};
    [[= gkdviz::core::SemanticAnnotation{gkdviz::core::Semantic::kGain}]]
    double ki{0.1};
    [[= gkdviz::core::SemanticAnnotation{gkdviz::core::Semantic::kGain}]]
    double kd{0.01};
  };

  struct Input {
    [[= gkdviz::core::SemanticAnnotation{gkdviz::core::Semantic::kVelocity}]]
    [[= gkdviz::core::UnitAnnotation{gkdviz::core::Unit::kRpm}]]
    double target{0.0};
    [[= gkdviz::core::SemanticAnnotation{gkdviz::core::Semantic::kVelocity}]]
    [[= gkdviz::core::UnitAnnotation{gkdviz::core::Unit::kRpm}]]
    double actual{0.0};
    [[= gkdviz::core::SemanticAnnotation{gkdviz::core::Semantic::kTime}]]
    [[= gkdviz::core::UnitAnnotation{gkdviz::core::Unit::kSecond}]]
    double dt{0.01};
  };

  struct Output {
    [[= gkdviz::core::SemanticAnnotation{gkdviz::core::Semantic::kControlCommand}]]
    [[= gkdviz::core::UnitAnnotation{gkdviz::core::Unit::kNormalized}]]
    [[= gkdviz::core::PlottableAnnotation{}]]
    [[= gkdviz::core::LoggableAnnotation{}]]
    double control{0.0};
    [[= gkdviz::core::SemanticAnnotation{gkdviz::core::Semantic::kVelocityError}]]
    [[= gkdviz::core::UnitAnnotation{gkdviz::core::Unit::kRpm}]]
    [[= gkdviz::core::PlottableAnnotation{}]]
    [[= gkdviz::core::LoggableAnnotation{}]]
    double error{0.0};
  };

  explicit Pid(const Config& cfg) : cfg_(cfg) {}

  Output process(const Input& in) {
    Output out{};
    out.error = in.target - in.actual;
    integral_ += out.error * in.dt;
    const double derivative = in.dt > 0.0 ? (out.error - last_error_) / in.dt : 0.0;
    out.control = cfg_.kp * out.error + cfg_.ki * integral_ + cfg_.kd * derivative;
    last_error_ = out.error;
    return out;
  }

private:
  Config cfg_;
  double integral_{0.0};
  double last_error_{0.0};
};

bool approx(double a, double b, double eps = 1e-9) {
  return std::fabs(a - b) < eps;
}

} // namespace

template <>
struct gkdviz::core::AlgorithmReflectionTraits<Pid> {
  static constexpr std::string_view display_name = "PID Controller";
  static constexpr std::string_view category = "Control";
};

int main() {
  gkdviz::core::StaticReflectionProvider provider;
  provider.register_algorithm<Pid>();

  gkdviz::core::NodeSchemaGenerator generator(provider);

  const auto schema = generator.generate<Pid>();

  if (schema.type != "Pid") {
    std::cerr << "identify failed: type mismatch\n";
    return 1;
  }
  if (schema.input_ports.size() != 3 || schema.output_ports.size() != 2) {
    std::cerr << "identify failed: ports mismatch\n";
    return 1;
  }
  if (schema.input_ports[0].type.semantic != gkdviz::core::Semantic::kVelocity) {
    std::cerr << "identify failed: semantic mismatch\n";
    return 1;
  }
  if (!schema.output_ports[0].plottable || !schema.output_ports[0].loggable) {
    std::cerr << "identify failed: output flags mismatch\n";
    return 1;
  }

  gkdviz::core::GraphCompiler::Catalog catalog;
  catalog.emplace(schema.type, schema);
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

  gkdviz::core::GraphConfig graph{
      .nodes =
          {
              {.id = "target1", .type = "VelocitySource"},
              {.id = "actual1", .type = "VelocitySource"},
              {.id = "dt1", .type = "TimeSource"},
              {.id = "pid1", .type = "Pid"},
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

  gkdviz::core::GraphCompiler compiler;
  const auto compile_result = compiler.compile(graph, catalog);
  if (!compile_result.ok()) {
    std::cerr << "graph compile failed from reflected pid schema\n";
    return 1;
  }
  if (!compile_result.plan.has_value() || compile_result.plan->nodes_in_order.size() != graph.nodes.size()) {
    std::cerr << "graph compile failed: runtime plan mismatch\n";
    return 1;
  }

  Pid pid(Pid::Config{});
  const auto out = pid.process(Pid::Input{.target = 300.0, .actual = 280.0, .dt = 0.01});
  if (!approx(out.error, 20.0)) {
    std::cerr << "pid process failed: error mismatch\n";
    return 1;
  }

  std::cout << "pid reflected node smoke: PASS\n";
  return 0;
}
