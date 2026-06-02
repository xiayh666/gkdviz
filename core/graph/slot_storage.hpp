#pragma once

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace gkdviz::core {

using SlotId = uint32_t;

class SlotStorage {
public:
  SlotStorage() = default;
  explicit SlotStorage(size_t initial_slots) : slots_(initial_slots) {}

  void reserve(size_t slot_count) { slots_.resize(slot_count); }

  template <typename T>
  void set(SlotId id, T value) {
    ensure_slot(id);
    slots_[id] = SlotValue::template make<T>(std::move(value));
  }

  template <typename T>
  [[nodiscard]] T get(SlotId id) const {
    if (id >= slots_.size()) {
      throw std::runtime_error("slot not found");
    }
    return slots_[id].template as<T>();
  }

private:
  struct SlotValue {
    enum class Kind : uint8_t { kEmpty, kI64, kF64, kBool };

    Kind kind{Kind::kEmpty};
    int64_t i64{0};
    double f64{0.0};
    bool b{false};

    template <typename T>
    static SlotValue make(T value);

    template <typename T>
    [[nodiscard]] T as() const;
  };

  void ensure_slot(SlotId id) {
    if (id < slots_.size()) {
      return;
    }
    if (id == std::numeric_limits<SlotId>::max()) {
      throw std::runtime_error("slot id overflow");
    }
    slots_.resize(static_cast<size_t>(id) + 1);
  }

  std::vector<SlotValue> slots_;
};

template <>
inline SlotStorage::SlotValue SlotStorage::SlotValue::make<int64_t>(int64_t value) {
  SlotValue out;
  out.kind = Kind::kI64;
  out.i64 = value;
  return out;
}

template <>
inline SlotStorage::SlotValue SlotStorage::SlotValue::make<double>(double value) {
  SlotValue out;
  out.kind = Kind::kF64;
  out.f64 = value;
  return out;
}

template <>
inline SlotStorage::SlotValue SlotStorage::SlotValue::make<bool>(bool value) {
  SlotValue out;
  out.kind = Kind::kBool;
  out.b = value;
  return out;
}

template <>
inline int64_t SlotStorage::SlotValue::as<int64_t>() const {
  if (kind != Kind::kI64) {
    throw std::runtime_error("slot type mismatch: expected int64");
  }
  return i64;
}

template <>
inline double SlotStorage::SlotValue::as<double>() const {
  if (kind != Kind::kF64) {
    throw std::runtime_error("slot type mismatch: expected float64");
  }
  return f64;
}

template <>
inline bool SlotStorage::SlotValue::as<bool>() const {
  if (kind != Kind::kBool) {
    throw std::runtime_error("slot type mismatch: expected bool");
  }
  return b;
}

} // namespace gkdviz::core
