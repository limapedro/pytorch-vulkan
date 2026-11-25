#pragma once

#include <vulkan/vulkan.h>

#include <pybind11/pybind11.h>

#include <cstdint>
#include <string>
#include <vector>

namespace kaminari {

struct DeviceInfo {
  std::string name;
  std::uint32_t vendor_id = 0;
  std::uint32_t device_id = 0;
  std::uint32_t api_version = 0;
  std::uint32_t driver_version = 0;
  std::uint32_t subgroup_size = 0;
  std::uint32_t subgroup_operations = 0;
  std::uint32_t max_workgroup_invocations = 0;
  std::uint32_t max_shared_memory = 0;
  std::uint32_t max_push_constants = 0;
  std::uint32_t timestamp_valid_bits = 0;
  int compute_queue_family = -1;
  int transfer_queue_family = -1;
  bool buffer_device_address = false;
  bool timeline_semaphore = false;
  bool shader_float16 = false;
  bool shader_int8 = false;
  bool storage_buffer16 = false;
  std::vector<std::string> extensions;
};

class Runtime {
 public:
  static Runtime& get();

  const DeviceInfo& info() const {
    return info_;
  }
  bool ready() const {
    return ready_;
  }
  VkDevice device() const {
    return device_;
  }
  VkPhysicalDevice physicalDevice() const {
    return physical_;
  }
  VkQueue computeQueue() const {
    return compute_queue_;
  }
  VkCommandBuffer commandBuffer() const {
    return command_buffer_;
  }
  VkPipelineCache pipelineCache() const {
    return pipeline_cache_;
  }

  void synchronize();

 private:
  Runtime();
  ~Runtime();
  Runtime(const Runtime&) = delete;
  Runtime& operator=(const Runtime&) = delete;

  VkInstance instance_ = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT messenger_ = VK_NULL_HANDLE;
  VkPhysicalDevice physical_ = VK_NULL_HANDLE;
  VkDevice device_ = VK_NULL_HANDLE;
  VkQueue compute_queue_ = VK_NULL_HANDLE;
  VkCommandPool command_pool_ = VK_NULL_HANDLE;
  VkCommandBuffer command_buffer_ = VK_NULL_HANDLE;
  VkFence fence_ = VK_NULL_HANDLE;
  VkPipelineCache pipeline_cache_ = VK_NULL_HANDLE;
  std::string cache_file_;
  bool ready_ = false;
  DeviceInfo info_;
};

pybind11::dict deviceInfo();
bool isVulkanAvailable();
void synchronizeVulkan();

} // namespace kaminari
