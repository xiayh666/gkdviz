#pragma once

#include "slot_storage.hpp"

#include <cstdint>
#include <vector>

namespace gkdviz::core {

using NodeInstanceId = uint32_t;

struct RuntimeContext {
  double dt_seconds{0.0};
  SlotStorage* slots{nullptr};
};

struct CompiledNode {
  NodeInstanceId id{0};
  void* instance{nullptr};
  void (*process_fn)(void* instance, RuntimeContext& ctx){nullptr};
  std::vector<SlotId> input_slots;
  std::vector<SlotId> output_slots;
};

struct RuntimePlan {
  std::vector<CompiledNode> nodes_in_order;
  SlotStorage slots;

  void process(RuntimeContext& ctx) {
    ctx.slots = &slots;
    for (auto& node : nodes_in_order) {
      if (node.process_fn != nullptr) {
        node.process_fn(node.instance, ctx);
      }
    }
  }
};

} // namespace gkdviz::core
