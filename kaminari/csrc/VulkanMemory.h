#pragma once

#include <vulkan/vulkan.h>

#include <cstddef>
#include <cstdint>

namespace kaminari {

struct Allocation {
  VkBuffer buffer = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;
  VkDeviceAddress address = 0;
  std::uint64_t size = 0;
  std::uint64_t size_class = 0;
};

Allocation* allocateBuffer(std::uint64_t bytes);
void freeBuffer(Allocation* allocation);
Allocation* allocationFromAddress(const void* address);

void copyHostToDevice(Allocation* dst, const void* src, std::uint64_t bytes);
void copyDeviceToHost(void* dst, const Allocation* src, std::uint64_t bytes);
void copyDeviceToDevice(
    Allocation* dst,
    const Allocation* src,
    std::uint64_t bytes);
void fillBuffer(Allocation* dst, std::uint32_t pattern, std::uint64_t bytes);
void submitAndWait();

} // namespace kaminari
