#include "VulkanMemory.h"

#include <c10/core/Allocator.h>
#include <c10/util/Exception.h>

namespace kaminari {
namespace {

void deleteAllocation(void* context) noexcept {
  freeBuffer(static_cast<Allocation*>(context));
}

class KaminariAllocator final : public c10::Allocator {
 public:
  c10::DataPtr allocate(std::size_t bytes) override {
    const c10::Device device(c10::DeviceType::PrivateUse1, 0);
    if (bytes == 0) {
      return {nullptr, nullptr, &deleteAllocation, device};
    }
    auto* allocation = allocateBuffer(bytes);
    return {
        reinterpret_cast<void*>(allocation->address),
        allocation,
        &deleteAllocation,
        device};
  }

  c10::DeleterFnPtr raw_deleter() const override {
    return &deleteAllocation;
  }

  void copy_data(void* dest, const void* src, std::size_t bytes) const override {
    auto* dst = allocationFromAddress(dest);
    auto* from = allocationFromAddress(src);
    TORCH_CHECK(dst && from, "kaminari: copy_data on unknown buffers");
    copyDeviceToDevice(dst, from, bytes);
  }
};

KaminariAllocator allocator;

} // namespace
} // namespace kaminari

REGISTER_ALLOCATOR(c10::DeviceType::PrivateUse1, &kaminari::allocator);
