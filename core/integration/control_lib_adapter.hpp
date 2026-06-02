#pragma once

#include "../reflection/node_schema.hpp"

#include <pid.hpp>

namespace gkdviz::core {

struct ControlLibPidConfig {
  double kp{1.0};
  double ki{0.0};
  double kd{0.0};
  double max_out{100.0};
  double max_iout{100.0};
};

class ControlLibPidAdapter {
public:
  explicit ControlLibPidAdapter(const ControlLibPidConfig& config)
      : config_{
            .kp = static_cast<float>(config.kp),
            .ki = static_cast<float>(config.ki),
            .kd = static_cast<float>(config.kd),
            .max_out = static_cast<float>(config.max_out),
            .max_iout = static_cast<float>(config.max_iout),
        },
        pid_(config_, feedback_) {}

  [[nodiscard]] double process(double setpoint, double feedback) {
    feedback_ = static_cast<float>(feedback);
    pid_.set(static_cast<float>(setpoint));
    return static_cast<double>(pid_.out);
  }

private:
  float feedback_{0.0F};
  Pid::PidConfig config_{};
  Pid::PidPosition pid_;
};

[[nodiscard]] inline NodeSchema make_control_lib_pid_schema() {
  return NodeSchema{
      .type = "ControlLibPid",
      .display_name = "ControlLib PID",
      .category = "ControlLib",
      .config_fields =
          {
              FieldSchema{
                  .name = "kp",
                  .type = {.value_type = ValueType::kFloat64, .semantic = Semantic::kGain},
                  .default_value = 1.0,
              },
              FieldSchema{
                  .name = "ki",
                  .type = {.value_type = ValueType::kFloat64, .semantic = Semantic::kGain},
                  .default_value = 0.0,
              },
              FieldSchema{
                  .name = "kd",
                  .type = {.value_type = ValueType::kFloat64, .semantic = Semantic::kGain},
                  .default_value = 0.0,
              },
              FieldSchema{
                  .name = "max_out",
                  .type = {.value_type = ValueType::kFloat64},
                  .default_value = 100.0,
              },
              FieldSchema{
                  .name = "max_iout",
                  .type = {.value_type = ValueType::kFloat64},
                  .default_value = 100.0,
              },
          },
      .input_ports =
          {
              PortSchema{
                  .name = "setpoint",
                  .type = {.value_type = ValueType::kFloat64},
              },
              PortSchema{
                  .name = "feedback",
                  .type = {.value_type = ValueType::kFloat64},
              },
          },
      .output_ports =
          {
              PortSchema{
                  .name = "out",
                  .type = {.value_type = ValueType::kFloat64},
              },
          },
  };
}

} // namespace gkdviz::core
