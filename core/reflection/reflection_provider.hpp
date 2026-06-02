#pragma once

#include "node_schema.hpp"

#include <string>
#include <vector>

namespace gkdviz::core {

struct StructSchema {
  std::string name;
  std::vector<FieldSchema> fields;
};

class ReflectionProvider {
public:
  virtual ~ReflectionProvider() = default;

  template <typename T>
  [[nodiscard]] StructSchema reflect_struct() const {
    return reflect_struct_by_name(typeid(T).name());
  }

  template <typename Algo>
  [[nodiscard]] NodeSchema reflect_algorithm() const {
    return reflect_algorithm_by_name(typeid(Algo).name());
  }

protected:
  [[nodiscard]] virtual StructSchema reflect_struct_by_name(const std::string& type_name) const = 0;
  [[nodiscard]] virtual NodeSchema reflect_algorithm_by_name(const std::string& type_name) const = 0;
};

} // namespace gkdviz::core
