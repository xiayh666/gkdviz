#pragma once

#include "port_type.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace gkdviz::core {

struct Range {
  double min{0.0};
  double max{0.0};

  constexpr auto operator<=>(const Range&) const = default;
};

struct FieldAnnotation {
  std::string label;
  Semantic semantic{Semantic::kNone};
  Unit unit{Unit::kNone};
  std::optional<Range> range;
  bool plottable{false};
  bool loggable{false};
};

// Reflection-friendly annotation payloads used by [[= ... ]] metadata.
struct SemanticAnnotation {
  Semantic value{Semantic::kNone};

  constexpr auto operator<=>(const SemanticAnnotation&) const = default;
};

struct UnitAnnotation {
  Unit value{Unit::kNone};

  constexpr auto operator<=>(const UnitAnnotation&) const = default;
};

struct RangeAnnotation {
  Range value{};

  constexpr auto operator<=>(const RangeAnnotation&) const = default;
};

struct PlottableAnnotation {
  bool value{true};

  constexpr auto operator<=>(const PlottableAnnotation&) const = default;
};

struct LoggableAnnotation {
  bool value{true};

  constexpr auto operator<=>(const LoggableAnnotation&) const = default;
};

} // namespace gkdviz::core
