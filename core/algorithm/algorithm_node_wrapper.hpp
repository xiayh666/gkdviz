#pragma once

#include "../graph/runtime_plan.hpp"

namespace gkdviz::core {

template <typename Algo>
requires requires(typename Algo::Config cfg) {
  typename Algo::Input;
  typename Algo::Output;
  { Algo(cfg) } -> std::same_as<Algo>;
  { std::declval<Algo>().process(std::declval<typename Algo::Input>()) } -> std::same_as<typename Algo::Output>;
}
class AlgorithmNodeWrapper 
{
public:
  using Config = typename Algo::Config;
  using Input = typename Algo::Input;
  using Output = typename Algo::Output;

  explicit AlgorithmNodeWrapper(const Config& cfg) : algo_(cfg) {}

  void process(RuntimeContext& ctx) {
    Input in = read_input_from_slots(ctx);
    Output out = algo_.process(in);
    write_output_to_slots(ctx, out);
    emit_telemetry_from_output(ctx, out);
  }

private:
  [[nodiscard]] Input read_input_from_slots(RuntimeContext&) const { return Input{}; }
  void write_output_to_slots(RuntimeContext&, const Output&) const {}
  void emit_telemetry_from_output(RuntimeContext&, const Output&) const {}

  Algo algo_;
};

} // namespace gkdviz::core
