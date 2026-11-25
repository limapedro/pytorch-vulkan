#include "VulkanMemory.h"

#include "Runtime.h"

#include <c10/util/Exception.h>

#include <algorithm>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace kaminari {

namespace {

std::mutex g_mutex;
std::unordered_map<std::uint64_t, std::vector<Allocation*>> g_pool;
std::unordered_map<VkDeviceAddress, Allocation*> g_by_address;

std::uint32_t findMemoryType(
    VkPhysicalDevice physical,
    std::uint32_t type_filter,
    VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties memory_properties{};
  vkGetPhysicalDeviceMemoryProperties(physical, &memory_properties);
  for (std::uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i) {
    const auto flags = memory_properties.memoryTypes[i].propertyFlags;
    if ((type_filter & (1u << i)) != 0 && (flags & properties) == properties) {
      return i;
    }
  }
  TORCH_CHECK(false, "kaminari: no compatible Vulkan memory type found");
}

std::uint64_t sizeClass(std::uint64_t bytes) {
  constexpr std::uint64_t kGranularity = 4096;
  return std::max(kGranularity, ((bytes + kGranularity - 1) / kGranularity) * kGranularity);
}

Allocation* createBuffer(std::uint64_t bytes) {
  auto& runtime = Runtime::get();
  TORCH_CHECK(runtime.ready(), "kaminari Vulkan runtime is not ready");

  VkBufferCreateInfo buffer_info{
      VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      nullptr,
      0,
      bytes,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
          VK_BUFFER_USAGE_TRANSFER_DST_BIT |
          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      VK_SHARING_MODE_EXCLUSIVE,
      0,
      nullptr};
  auto* allocation = new Allocation();
  allocation->size = bytes;
  allocation->size_class = sizeClass(bytes);
  auto result = vkCreateBuffer(runtime.device(), &buffer_info, nullptr, &allocation->buffer);
  TORCH_CHECK(result == VK_SUCCESS, "kaminari: vkCreateBuffer failed");

  VkMemoryRequirements requirements{};
  vkGetBufferMemoryRequirements(runtime.device(), allocation->buffer, &requirements);
  const auto memory_type = findMemoryType(
      runtime.physicalDevice(),
      requirements.memoryTypeBits,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  VkMemoryAllocateFlagsInfo flags_info{
      VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
      nullptr,
      VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
      0};
  VkMemoryAllocateInfo allocate_info{
      VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      &flags_info,
      requirements.size,
      memory_type};
  result = vkAllocateMemory(runtime.device(), &allocate_info, nullptr, &allocation->memory);
  TORCH_CHECK(result == VK_SUCCESS, "kaminari: vkAllocateMemory failed");
  result = vkBindBufferMemory(
      runtime.device(), allocation->buffer, allocation->memory, 0);
  TORCH_CHECK(result == VK_SUCCESS, "kaminari: vkBindBufferMemory failed");

  VkBufferDeviceAddressInfo address_info{
      VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
      nullptr,
      allocation->buffer};
  allocation->address = vkGetBufferDeviceAddress(runtime.device(), &address_info);
  TORCH_CHECK(allocation->address != 0, "kaminari: buffer device address is null");
  g_by_address[allocation->address] = allocation;
  return allocation;
}

void destroyBuffer(Allocation* allocation) {
  auto& runtime = Runtime::get();
  g_by_address.erase(allocation->address);
  vkDestroyBuffer(runtime.device(), allocation->buffer, nullptr);
  vkFreeMemory(runtime.device(), allocation->memory, nullptr);
  delete allocation;
}

void submitAndWaitImpl() {
  auto& runtime = Runtime::get();
  auto buffer = runtime.commandBuffer();
  VkSubmitInfo submit_info{
      VK_STRUCTURE_TYPE_SUBMIT_INFO,
      nullptr,
      0,
      nullptr,
      nullptr,
      1,
      &buffer,
      0,
      nullptr};
  auto result = vkEndCommandBuffer(buffer);
  TORCH_CHECK(result == VK_SUCCESS, "kaminari: vkEndCommandBuffer failed");
  result = vkQueueSubmit(runtime.computeQueue(), 1, &submit_info, VK_NULL_HANDLE);
  TORCH_CHECK(result == VK_SUCCESS, "kaminari: vkQueueSubmit failed");
  result = vkQueueWaitIdle(runtime.computeQueue());
  TORCH_CHECK(result == VK_SUCCESS, "kaminari: vkQueueWaitIdle failed");
  vkResetCommandBuffer(buffer, 0);
  VkCommandBufferBeginInfo begin_info{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      nullptr,
      0,
      nullptr};
  result = vkBeginCommandBuffer(buffer, &begin_info);
  TORCH_CHECK(result == VK_SUCCESS, "kaminari: vkBeginCommandBuffer failed");
}

std::pair<VkBuffer, VkDeviceMemory> createStaging(std::uint64_t bytes) {
  auto& runtime = Runtime::get();
  VkBuffer buffer = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;
  VkBufferCreateInfo buffer_info{
      VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      nullptr,
      0,
      bytes,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_SHARING_MODE_EXCLUSIVE,
      0,
      nullptr};
  auto result = vkCreateBuffer(runtime.device(), &buffer_info, nullptr, &buffer);
  TORCH_CHECK(result == VK_SUCCESS, "kaminari: staging vkCreateBuffer failed");
  VkMemoryRequirements requirements{};
  vkGetBufferMemoryRequirements(runtime.device(), buffer, &requirements);
  const auto memory_type = findMemoryType(
      runtime.physicalDevice(),
      requirements.memoryTypeBits,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  VkMemoryAllocateInfo allocate_info{
      VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      nullptr,
      requirements.size,
      memory_type};
  result = vkAllocateMemory(runtime.device(), &allocate_info, nullptr, &memory);
  TORCH_CHECK(result == VK_SUCCESS, "kaminari: staging vkAllocateMemory failed");
  vkBindBufferMemory(runtime.device(), buffer, memory, 0);
  return {buffer, memory};
}

void destroyStaging(VkBuffer buffer, VkDeviceMemory memory) {
  auto& runtime = Runtime::get();
  vkDestroyBuffer(runtime.device(), buffer, nullptr);
  vkFreeMemory(runtime.device(), memory, nullptr);
}

} // namespace

void submitAndWait() {
  submitAndWaitImpl();
}

Allocation* allocateBuffer(std::uint64_t bytes) {
  const auto class_id = sizeClass(bytes);
  {
    std::lock_guard<std::mutex> lock(g_mutex);
    auto it = g_pool.find(class_id);
    if (it != g_pool.end() && !it->second.empty()) {
      auto* allocation = it->second.back();
      it->second.pop_back();
      return allocation;
    }
  }
  return createBuffer(class_id);
}

void freeBuffer(Allocation* allocation) {
  if (allocation == nullptr) {
    return;
  }
  std::lock_guard<std::mutex> lock(g_mutex);
  g_pool[allocation->size_class].push_back(allocation);
}

Allocation* allocationFromAddress(const void* address) {
  std::lock_guard<std::mutex> lock(g_mutex);
  auto it = g_by_address.find(reinterpret_cast<VkDeviceAddress>(address));
  return it == g_by_address.end() ? nullptr : it->second;
}

void copyHostToDevice(Allocation* dst, const void* src, std::uint64_t bytes) {
  auto [staging, memory] = createStaging(bytes);
  void* mapped = nullptr;
  vkMapMemory(Runtime::get().device(), memory, 0, bytes, 0, &mapped);
  std::memcpy(mapped, src, bytes);
  vkUnmapMemory(Runtime::get().device(), memory);
  VkBufferCopy region{0, 0, bytes};
  vkCmdCopyBuffer(
      Runtime::get().commandBuffer(), staging, dst->buffer, 1, &region);
  submitAndWait();
  destroyStaging(staging, memory);
}

void copyDeviceToHost(void* dst, const Allocation* src, std::uint64_t bytes) {
  auto [staging, memory] = createStaging(bytes);
  VkBufferCopy region{0, 0, bytes};
  vkCmdCopyBuffer(
      Runtime::get().commandBuffer(), src->buffer, staging, 1, &region);
  submitAndWait();
  void* mapped = nullptr;
  vkMapMemory(Runtime::get().device(), memory, 0, bytes, 0, &mapped);
  std::memcpy(dst, mapped, bytes);
  vkUnmapMemory(Runtime::get().device(), memory);
  destroyStaging(staging, memory);
}

void copyDeviceToDevice(
    Allocation* dst,
    const Allocation* src,
    std::uint64_t bytes) {
  VkBufferCopy region{0, 0, bytes};
  vkCmdCopyBuffer(
      Runtime::get().commandBuffer(), src->buffer, dst->buffer, 1, &region);
  submitAndWait();
}

void fillBuffer(Allocation* dst, std::uint32_t pattern, std::uint64_t bytes) {
  vkCmdFillBuffer(Runtime::get().commandBuffer(), dst->buffer, 0, bytes, pattern);
  submitAndWait();
}

} // namespace kaminari
