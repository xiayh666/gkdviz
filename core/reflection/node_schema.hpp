#pragma once

#include "annotation.hpp"
#include "port_type.hpp"

#include <string>
#include <variant>
#include <vector>

namespace gkdviz::core {

using FieldDefaultValue = std::variant<std::monostate, bool, int32_t, int64_t, float, double, std::string>;

struct FieldSchema {
  std::string name;
  PortType type;
  FieldDefaultValue default_value;
  FieldAnnotation annotation;
};

struct PortSchema {
  std::string name;
  PortType type;
  bool plottable{false};
  bool loggable{false};
};

struct NodeSchema {
  std::string type;
  std::string display_name;
  std::string category;
  std::vector<FieldSchema> config_fields;
  std::vector<PortSchema> input_ports;
  std::vector<PortSchema> output_ports;
  std::string runtime_type_id;
};

} // namespace gkdviz::core
