#include "Runtime.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <vector>

namespace kaminari {

namespace {

std::vector<char> readCacheFile(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary | std::ios::ate);
  if (!in) {
    return {};
  }
  const auto size = in.tellg();
  if (size <= 0) {
    return {};
  }
  std::vector<char> data(static_cast<std::size_t>(size));
  in.seekg(0);
  in.read(data.data(), size);
  return data;
}

void writeCacheFile(
    const std::filesystem::path& path,
    const std::vector<char>& data) {
  if (data.empty()) {
    return;
  }
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (out) {
    out.write(data.data(), static_cast<std::streamsize>(data.size()));
  }
}

} // namespace

Runtime::Runtime() {
  VkApplicationInfo app{
      VK_STRUCTURE_TYPE_APPLICATION_INFO,
      nullptr,
      "kaminari",
      1,
      "kaminari",
      1,
      VK_API_VERSION_1_3};
  VkInstanceCreateInfo instance_info{
      VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      nullptr,
      0,
      &app,
      0,
      nullptr,
      0,
      nullptr};
  if (vkCreateInstance(&instance_info, nullptr, &instance_) != VK_SUCCESS) {
    return;
  }

  std::uint32_t physical_count = 0;
  vkEnumeratePhysicalDevices(instance_, &physical_count, nullptr);
  std::vector<VkPhysicalDevice> physical_devices(physical_count);
  vkEnumeratePhysicalDevices(instance_, &physical_count, physical_devices.data());
  for (auto candidate : physical_devices) {
    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(candidate, &props);
    if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      physical_ = candidate;
      break;
    }
  }
  if (physical_ == VK_NULL_HANDLE && physical_count > 0) {
    physical_ = physical_devices[0];
  }
  if (physical_ == VK_NULL_HANDLE) {
    return;
  }

  VkPhysicalDeviceProperties props{};
  vkGetPhysicalDeviceProperties(physical_, &props);
  VkPhysicalDeviceProperties2 props2{
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
      nullptr};
  VkPhysicalDeviceSubgroupProperties subgroup{
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES,
      nullptr};
  props2.pNext = &subgroup;
  vkGetPhysicalDeviceProperties2(physical_, &props2);

  std::uint32_t family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physical_, &family_count, nullptr);
  std::vector<VkQueueFamilyProperties> families(family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(physical_, &family_count, families.data());
  for (std::uint32_t i = 0; i < family_count; ++i) {
    const auto flags = families[i].queueFlags;
    if (info_.compute_queue_family < 0 &&
        (flags & VK_QUEUE_COMPUTE_BIT) != 0) {
      info_.compute_queue_family = static_cast<int>(i);
      info_.timestamp_valid_bits = families[i].timestampValidBits;
    }
    if (info_.transfer_queue_family < 0 &&
        (flags & VK_QUEUE_TRANSFER_BIT) != 0 &&
        (flags & VK_QUEUE_COMPUTE_BIT) == 0 &&
        (flags & VK_QUEUE_GRAPHICS_BIT) == 0) {
      info_.transfer_queue_family = static_cast<int>(i);
    }
  }
  if (info_.compute_queue_family < 0) {
    return;
  }

  VkPhysicalDeviceFeatures2 features{
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
      nullptr};
  VkPhysicalDeviceVulkan11Features v11{
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
      nullptr};
  VkPhysicalDeviceVulkan12Features v12{
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
      nullptr};
  features.pNext = &v11;
  v11.pNext = &v12;
  vkGetPhysicalDeviceFeatures2(physical_, &features);
  v11.storageBuffer16BitAccess = VK_TRUE;
  v12.bufferDeviceAddress = VK_TRUE;
  v12.timelineSemaphore = VK_TRUE;
  v12.shaderFloat16 = VK_TRUE;
  v12.shaderInt8 = VK_TRUE;
  v12.descriptorIndexing = VK_TRUE;
  v12.storageBuffer8BitAccess = VK_TRUE;
  info_.buffer_device_address = v12.bufferDeviceAddress == VK_TRUE;
  info_.timeline_semaphore = v12.timelineSemaphore == VK_TRUE;
  info_.shader_float16 = v12.shaderFloat16 == VK_TRUE;
  info_.shader_int8 = v12.shaderInt8 == VK_TRUE;
  info_.storage_buffer16 = v11.storageBuffer16BitAccess == VK_TRUE;

  float priority = 1.0f;
  std::vector<VkDeviceQueueCreateInfo> queue_infos;
  queue_infos.push_back(VkDeviceQueueCreateInfo{
      VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      nullptr,
      0,
      static_cast<std::uint32_t>(info_.compute_queue_family),
      1,
      &priority});
  if (info_.transfer_queue_family >= 0 &&
      info_.transfer_queue_family != info_.compute_queue_family) {
    queue_infos.push_back(VkDeviceQueueCreateInfo{
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        nullptr,
        0,
        static_cast<std::uint32_t>(info_.transfer_queue_family),
        1,
        &priority});
  }

  VkDeviceCreateInfo device_info{
      VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      &features,
      0,
      static_cast<std::uint32_t>(queue_infos.size()),
      queue_infos.data(),
      0,
      nullptr,
      0,
      nullptr,
      nullptr};
  if (vkCreateDevice(physical_, &device_info, nullptr, &device_) != VK_SUCCESS) {
    return;
  }
  vkGetDeviceQueue(
      device_, static_cast<std::uint32_t>(info_.compute_queue_family), 0, &compute_queue_);

  VkCommandPoolCreateInfo pool_info{
      VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      nullptr,
      VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      static_cast<std::uint32_t>(info_.compute_queue_family)};
  if (vkCreateCommandPool(device_, &pool_info, nullptr, &command_pool_) != VK_SUCCESS) {
    return;
  }
  VkCommandBufferAllocateInfo allocate_info{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      nullptr,
      command_pool_,
      VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      1};
  if (vkAllocateCommandBuffers(device_, &allocate_info, &command_buffer_) != VK_SUCCESS) {
    return;
  }
  VkCommandBufferBeginInfo begin_info{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      nullptr,
      0,
      nullptr};
  vkBeginCommandBuffer(command_buffer_, &begin_info);

  VkFenceCreateInfo fence_info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, 0};
  vkCreateFence(device_, &fence_info, nullptr, &fence_);

  const char* cache_dir = std::getenv("KAMINARI_CACHE_DIR");
  std::filesystem::path cache_path =
      cache_dir ? std::filesystem::path(cache_dir)
                : std::filesystem::temp_directory_path() / "kaminari";
  std::filesystem::create_directories(cache_path);
  cache_path /= "pipeline.cache";
  auto cache_data = readCacheFile(cache_path);
  VkPipelineCacheCreateInfo cache_info{
      VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
      nullptr,
      0,
      cache_data.size(),
      cache_data.data()};
  vkCreatePipelineCache(device_, &cache_info, nullptr, &pipeline_cache_);
  cache_file_ = cache_path.string();

  info_.name = props.deviceName;
  info_.vendor_id = props.vendorID;
  info_.device_id = props.deviceID;
  info_.api_version = props.apiVersion;
  info_.driver_version = props.driverVersion;
  info_.subgroup_size = subgroup.subgroupSize;
  info_.subgroup_operations = subgroup.supportedOperations;
  info_.max_workgroup_invocations = props.limits.maxComputeWorkGroupInvocations;
  info_.max_shared_memory = props.limits.maxComputeSharedMemorySize;
  info_.max_push_constants = props.limits.maxPushConstantsSize;

  std::uint32_t extension_count = 0;
  vkEnumerateDeviceExtensionProperties(physical_, nullptr, &extension_count, nullptr);
  std::vector<VkExtensionProperties> extensions(extension_count);
  vkEnumerateDeviceExtensionProperties(
      physical_, nullptr, &extension_count, extensions.data());
  for (const auto& extension : extensions) {
    info_.extensions.emplace_back(extension.extensionName);
  }

  ready_ = device_ != VK_NULL_HANDLE && compute_queue_ != VK_NULL_HANDLE;
}

Runtime::~Runtime() {
  if (!ready_) {
    return;
  }
  std::size_t cache_size = 0;
  vkGetPipelineCacheData(device_, pipeline_cache_, &cache_size, nullptr);
  std::vector<char> cache_data(cache_size);
  if (cache_size > 0) {
    vkGetPipelineCacheData(device_, pipeline_cache_, &cache_size, cache_data.data());
    cache_data.resize(cache_size);
    writeCacheFile(std::filesystem::path(cache_file_), cache_data);
  }
  vkDestroyPipelineCache(device_, pipeline_cache_, nullptr);
  vkDestroyFence(device_, fence_, nullptr);
  vkDestroyCommandPool(device_, command_pool_, nullptr);
  vkDestroyDevice(device_, nullptr);
  vkDestroyInstance(instance_, nullptr);
}

Runtime& Runtime::get() {
  static Runtime runtime;
  return runtime;
}

void Runtime::synchronize() {
  if (ready_) {
    vkQueueWaitIdle(compute_queue_);
  }
}

pybind11::dict deviceInfo() {
  auto& runtime = Runtime::get();
  const auto& info = runtime.info();
  pybind11::dict result;
  result["available"] = runtime.ready();
  result["name"] = info.name;
  result["vendor_id"] = info.vendor_id;
  result["device_id"] = info.device_id;
  result["api_version"] = info.api_version;
  result["driver_version"] = info.driver_version;
  result["subgroup_size"] = info.subgroup_size;
  result["subgroup_operations"] = info.subgroup_operations;
  result["max_workgroup_invocations"] = info.max_workgroup_invocations;
  result["max_shared_memory"] = info.max_shared_memory;
  result["max_push_constants"] = info.max_push_constants;
  result["timestamp_valid_bits"] = info.timestamp_valid_bits;
  result["compute_queue_family"] = info.compute_queue_family;
  result["transfer_queue_family"] = info.transfer_queue_family;
  result["buffer_device_address"] = info.buffer_device_address;
  result["timeline_semaphore"] = info.timeline_semaphore;
  result["shader_float16"] = info.shader_float16;
  result["shader_int8"] = info.shader_int8;
  result["storage_buffer16"] = info.storage_buffer16;
  pybind11::list extensions;
  for (const auto& extension : info.extensions) {
    extensions.append(extension);
  }
  result["extensions"] = extensions;
  return result;
}

bool isVulkanAvailable() {
  return Runtime::get().ready();
}

void synchronizeVulkan() {
  Runtime::get().synchronize();
}

} // namespace kaminari
