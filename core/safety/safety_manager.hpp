#pragma once

#include "../command/command_channel.hpp"

namespace gkdviz::core {

struct SafetyDecision {
  bool allowed{true};
  const char* reason{"ok"};
};

class SafetyManager {
public:
  [[nodiscard]] SafetyDecision evaluate(const RuntimeCommand& command) const {
    if (emergency_locked_ && command.type != RuntimeCommandType::kReleaseControl) {
      return {false, "emergency locked"};
    }
    return {true, "ok"};
  }

  void set_emergency_locked(bool locked) { emergency_locked_ = locked; }

private:
  bool emergency_locked_{false};
};

} // namespace gkdviz::core
