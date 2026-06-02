#pragma once

#include <cstdint>
#include <queue>
#include <string>

namespace gkdviz::core {

enum class RuntimeCommandType : uint8_t {
  kStartGraph,
  kStopGraph,
  kSetParameter,
  kEnable,
  kDisable,
  kStop,
  kEmergencyStop,
  kAcquireControl,
  kReleaseControl
};

struct RuntimeCommand {
  RuntimeCommandType type{RuntimeCommandType::kStop};
  std::string payload;
};

class CommandChannel {
public:
  void push(RuntimeCommand command) { queue_.push(std::move(command)); }

  [[nodiscard]] bool try_pop(RuntimeCommand& out) {
    if (queue_.empty()) {
      return false;
    }
    out = std::move(queue_.front());
    queue_.pop();
    return true;
  }

private:
  std::queue<RuntimeCommand> queue_;
};

} // namespace gkdviz::core
