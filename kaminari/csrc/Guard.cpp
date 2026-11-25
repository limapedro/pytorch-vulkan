#include <ATen/CPUGeneratorImpl.h>
#include <ATen/core/GeneratorForPrivateuseone.h>
#include <ATen/detail/PrivateUse1HooksInterface.h>
#include <c10/core/impl/DeviceGuardImplInterface.h>
#include <c10/util/Exception.h>

namespace kaminari {

class GuardImpl final : public c10::impl::DeviceGuardImplInterface {
 public:
  c10::DeviceType type() const override {
    return c10::DeviceType::PrivateUse1;
  }

  c10::Device exchangeDevice(c10::Device device) const override {
    TORCH_CHECK(
        device.index() == 0, "kaminari only supports device 0");
    return c10::Device(c10::DeviceType::PrivateUse1, 0);
  }

  c10::Device getDevice() const override {
    return c10::Device(c10::DeviceType::PrivateUse1, 0);
  }

  void setDevice(c10::Device device) const override {
    TORCH_CHECK(
        device.index() == 0, "kaminari only supports device 0");
  }

  void uncheckedSetDevice(c10::Device device) const noexcept override {
    (void)device;
  }

  c10::Stream getStream(c10::Device device) const override {
    return c10::Stream(c10::Stream::DEFAULT, device);
  }

  c10::Stream exchangeStream(c10::Stream stream) const override {
    return stream;
  }

  c10::DeviceIndex deviceCount() const noexcept override {
    return 1;
  }

  void synchronizeDevice(c10::DeviceIndex device_index) const override {
    TORCH_CHECK(
        device_index == 0, "kaminari only supports device 0");
  }
};

class Hooks final : public at::PrivateUse1HooksInterface {
 public:
  bool isBuilt() const override {
    return true;
  }

  bool isAvailable() const override {
    return true;
  }

  bool hasPrimaryContext(c10::DeviceIndex) const override {
    return true;
  }

  const at::Generator& getDefaultGenerator(c10::DeviceIndex) const override {
    return at::detail::getDefaultCPUGenerator();
  }

  at::Generator getNewGenerator(c10::DeviceIndex) const override {
    return at::detail::createCPUGenerator();
  }
};

static Hooks hooks_impl;

struct RegisterHooks {
  RegisterHooks() {
    at::RegisterPrivateUse1HooksInterface(&hooks_impl);
  }
};

static RegisterHooks register_hooks;

} // namespace kaminari

C10_REGISTER_GUARD_IMPL(PrivateUse1, kaminari::GuardImpl);
