#pragma once

#include "../reflection/annotation.hpp"
#include "../reflection/auto_reflection.hpp"

namespace gkdviz::core {

struct DemoPid {
  struct Config {
    [[= SemanticAnnotation{Semantic::kGain}]]
    double kp{1.0};

    [[= SemanticAnnotation{Semantic::kGain}]]
    double ki{0.1};

    [[= SemanticAnnotation{Semantic::kGain}]]
    double kd{0.01};
  };

  struct Input {
    [[= SemanticAnnotation{Semantic::kVelocity}]]
    [[= UnitAnnotation{Unit::kRpm}]]
    double target{0.0};

    [[= SemanticAnnotation{Semantic::kVelocity}]]
    [[= UnitAnnotation{Unit::kRpm}]]
    double actual{0.0};

    [[= SemanticAnnotation{Semantic::kTime}]]
    [[= UnitAnnotation{Unit::kSecond}]]
    double dt{0.01};
  };

  struct Output {
    [[= SemanticAnnotation{Semantic::kControlCommand}]]
    [[= UnitAnnotation{Unit::kNormalized}]]
    [[= PlottableAnnotation{}]]
    [[= LoggableAnnotation{}]]
    double control{0.0};

    [[= SemanticAnnotation{Semantic::kVelocityError}]]
    [[= UnitAnnotation{Unit::kRpm}]]
    [[= PlottableAnnotation{}]]
    [[= LoggableAnnotation{}]]
    double error{0.0};
  };

  explicit DemoPid(const Config& cfg) : cfg_(cfg) {}

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

template <>
struct AlgorithmReflectionTraits<DemoPid> {
  static constexpr std::string_view display_name = "PID Controller";
  static constexpr std::string_view category = "Control";
};

} // namespace gkdviz::core
