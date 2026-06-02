#pragma once

#include "reflection_provider.hpp"

namespace gkdviz::core {

class NodeSchemaGenerator {
public:
  explicit NodeSchemaGenerator(const ReflectionProvider& reflection_provider)
      : reflection_provider_(reflection_provider) {}

  template <typename Algo>
  [[nodiscard]] NodeSchema generate() const {
    return reflection_provider_.reflect_algorithm<Algo>();
  }

private:
  const ReflectionProvider& reflection_provider_;
};

} // namespace gkdviz::core
