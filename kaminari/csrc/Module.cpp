#include "Runtime.h"

#include <ATen/Context.h>
#include <c10/core/DeviceType.h>
#include <c10/util/Exception.h>
#include <torch/csrc/utils/device_lazy_init.h>

#include <pybind11/pybind11.h>

namespace py = pybind11;

namespace kaminari {

void init() {
  torch::utils::register_fork_handler_for_device_init(at::kPrivateUse1);
  at::globalContext().lazyInitDevice(c10::DeviceType::PrivateUse1);
}

int64_t deviceCount() {
  return 1;
}

int64_t currentDevice() {
  return 0;
}

void setDevice(int64_t index) {
  TORCH_CHECK(index == 0, "kaminari only supports device 0, got ", index);
}

int64_t exchangeDevice(int64_t index) {
  TORCH_CHECK(index == -1 || index == 0, "invalid kaminari device: ", index);
  return 0;
}

} // namespace kaminari

PYBIND11_MODULE(TORCH_EXTENSION_NAME, m) {
  m.def("_init", &kaminari::init);
  m.def("_get_device_count", &kaminari::deviceCount);
  m.def("_get_device", &kaminari::currentDevice);
  m.def("_set_device", &kaminari::setDevice);
  m.def("_exchange_device", &kaminari::exchangeDevice);
  m.def("_device_info", &kaminari::deviceInfo);
  m.def("_is_vulkan_available", &kaminari::isVulkanAvailable);
  m.def("_synchronize_vulkan", &kaminari::synchronizeVulkan);
}
