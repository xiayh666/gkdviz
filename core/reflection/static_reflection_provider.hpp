#pragma once

#include "auto_reflection.hpp"
#include "reflection_provider.hpp"

#include <string>
#include <typeinfo>
#include <unordered_map>

namespace gkdviz::core {

class StaticReflectionProvider final : public ReflectionProvider {
public:
  template <typename T>
  void register_struct() {
#if defined(__cpp_impl_reflection) && __has_include(<meta>)
    struct_registry_[typeid(T).name()] = StructSchema{
        .name = reflected_type_name<T>(),
        .fields = reflected_field_schemas<T>(),
    };
#else
    static_assert(kNativeReflectionAvailable, "native C++26 reflection is required");
#endif
  }

  template <typename Algo>
  void register_algorithm() {
#if defined(__cpp_impl_reflection) && __has_include(<meta>)
    using Traits = AlgorithmReflectionTraits<Algo>;
    using Config = typename Algo::Config;
    using Input = typename Algo::Input;
    using Output = typename Algo::Output;

    const auto reflected_name = reflected_type_name<Algo>();
    const auto display_name =
        Traits::display_name.empty() ? reflected_name : std::string(Traits::display_name);

    algo_registry_[typeid(Algo).name()] = NodeSchema{
        .type = reflected_name,
        .display_name = display_name,
        .category = std::string(Traits::category),
        .config_fields = reflected_field_schemas<Config>(),
        .input_ports = reflected_port_schemas<Input>(),
        .output_ports = reflected_port_schemas<Output>(),
        .runtime_type_id = typeid(Algo).name(),
    };
#else
    static_assert(kNativeReflectionAvailable, "native C++26 reflection is required");
#endif
  }

protected:
  [[nodiscard]] StructSchema reflect_struct_by_name(const std::string& type_name) const override {
    auto it = struct_registry_.find(type_name);
    if (it == struct_registry_.end()) {
      return StructSchema{};
    }
    return it->second;
  }

  [[nodiscard]] NodeSchema reflect_algorithm_by_name(const std::string& type_name) const override {
    auto it = algo_registry_.find(type_name);
    if (it == algo_registry_.end()) {
      return NodeSchema{};
    }
    return it->second;
  }

private:
  std::unordered_map<std::string, StructSchema> struct_registry_;
  std::unordered_map<std::string, NodeSchema> algo_registry_;
};

} // namespace gkdviz::core
