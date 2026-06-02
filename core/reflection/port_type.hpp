#pragma once

#include <cstdint>

namespace gkdviz::core {

enum class ValueType : uint8_t {
  kUnknown,
  kBool,
  kInt32,
  kInt64,
  kFloat32,
  kFloat64,
  kString,
  kBytes
};

enum class Semantic : uint16_t {
  kNone,
  kGain,
  kVelocity,
  kVelocityError,
  kVelocityCommand,
  kCurrent,
  kControlCommand,
  kTime,
  kDebugValue
};

enum class Unit : uint16_t {
  kNone,
  kSecond,
  kRpm,
  kAmpere,
  kNormalized
};

struct PortType {
  ValueType value_type{ValueType::kUnknown};
  Semantic semantic{Semantic::kNone};
  Unit unit{Unit::kNone};

  [[nodiscard]] bool is_numeric() const {
    return value_type == ValueType::kInt32 || value_type == ValueType::kInt64 ||
           value_type == ValueType::kFloat32 || value_type == ValueType::kFloat64;
  }
};

[[nodiscard]] inline bool is_compatible(const PortType& from, const PortType& to) {
  if (from.value_type != ValueType::kUnknown && to.value_type != ValueType::kUnknown &&
      from.value_type != to.value_type) {
    return false;
  }
  if (from.semantic != Semantic::kNone && to.semantic != Semantic::kNone &&
      from.semantic != to.semantic) {
    return false;
  }
  if (from.unit != Unit::kNone && to.unit != Unit::kNone && from.unit != to.unit) {
    return false;
  }
  return true;
}

} // namespace gkdviz::core
