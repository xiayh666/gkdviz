#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace gkdviz::core {

struct TelemetrySample {
  std::string signal;
  double value{0.0};
  std::chrono::steady_clock::time_point ts;
};

class TelemetryBuffer {
public:
  explicit TelemetryBuffer(size_t capacity) : capacity_(capacity) { buffer_.reserve(capacity); }

  void push(TelemetrySample sample) {
    if (buffer_.size() == capacity_) {
      buffer_[head_] = std::move(sample);
      head_ = (head_ + 1) % capacity_;
      full_ = true;
      return;
    }
    buffer_.push_back(std::move(sample));
  }

  [[nodiscard]] std::vector<TelemetrySample> snapshot() const { return buffer_; }

private:
  size_t capacity_{0};
  size_t head_{0};
  bool full_{false};
  std::vector<TelemetrySample> buffer_;
};

} // namespace gkdviz::core
