#include "Ops.h"

#include "Kernel.h"
#include "VulkanMemory.h"

#include <ATen/EmptyTensor.h>
#include <ATen/Functions.h>
#include <ATen/native/CPUFallback.h>
#include <ATen/ops/_local_scalar_dense_native.h>
#include <ATen/ops/_reshape_alias_native.h>
#include <ATen/ops/as_strided_cpu_dispatch.h>
#include <ATen/ops/copy_native.h>
#include <ATen/ops/resize_native.h>
#include <ATen/ops/view_native.h>
#include <torch/library.h>

#include <atomic>
#include <cstring>

namespace kaminari {
namespace {

std::atomic<std::uint64_t> g_fallback_count{0};

constexpr c10::DispatchKeySet pu1_dks(c10::DispatchKey::PrivateUse1);

at::Tensor emptyMemoryFormat(
    c10::IntArrayRef size,
    std::optional<c10::ScalarType> dtype_opt,
    std::optional<c10::Layout> layout_opt,
    std::optional<c10::Device> device_opt,
    std::optional<bool> pin_memory_opt,
    std::optional<c10::MemoryFormat> memory_format_opt) {
  const auto device = c10::device_or_default(device_opt);
  TORCH_CHECK(device.is_privateuseone(), "expected a kaminari device");
  TORCH_CHECK(
      c10::layout_or_default(layout_opt) == c10::Layout::Strided,
      "non-strided layout unsupported");
  TORCH_CHECK(!c10::pinned_memory_or_default(pin_memory_opt), "no pinned memory");
  const c10::DeviceGuard guard(device);
  return at::detail::empty_generic(
      size,
      at::GetAllocator(at::kPrivateUse1),
      pu1_dks,
      c10::dtype_or_default(dtype_opt),
      memory_format_opt);
}

at::Tensor emptyStrided(
    c10::IntArrayRef size,
    c10::IntArrayRef stride,
    std::optional<c10::ScalarType> dtype_opt,
    std::optional<c10::Layout> layout_opt,
    std::optional<c10::Device> device_opt,
    std::optional<bool> pin_memory_opt) {
  const auto device = c10::device_or_default(device_opt);
  TORCH_CHECK(device.is_privateuseone(), "expected a kaminari device");
  TORCH_CHECK(
      c10::layout_or_default(layout_opt) == c10::Layout::Strided,
      "non-strided layout unsupported");
  TORCH_CHECK(!c10::pinned_memory_or_default(pin_memory_opt), "no pinned memory");
  const c10::DeviceGuard guard(device);
  return at::detail::empty_strided_generic(
      size,
      stride,
      at::GetAllocator(at::kPrivateUse1),
      pu1_dks,
      c10::dtype_or_default(dtype_opt));
}

at::Tensor asStrided(
    const at::Tensor& self,
    c10::SymIntArrayRef size,
    c10::SymIntArrayRef stride,
    std::optional<c10::SymInt> storage_offset) {
  return at::cpu::as_strided_symint(self, size, stride, storage_offset);
}

const at::Tensor& resize_(
    const at::Tensor& self,
    c10::SymIntArrayRef size,
    std::optional<at::MemoryFormat> memory_format) {
  return at::native::resize_(self, C10_AS_INTARRAYREF_SLOW(size), memory_format);
}

at::Tensor reshapeAlias(
    const at::Tensor& self,
    c10::SymIntArrayRef size,
    c10::SymIntArrayRef stride) {
  return at::native::_reshape_alias(
      self, C10_AS_INTARRAYREF_SLOW(size), C10_AS_INTARRAYREF_SLOW(stride));
}

at::Tensor view(const at::Tensor& self, c10::SymIntArrayRef size) {
  return at::native::view(self, C10_AS_INTARRAYREF_SLOW(size));
}

bool hasCompatibleShallowCopyType(const at::Tensor& self, const at::Tensor& other) {
  return self.device() == other.device();
}

at::Tensor copyFrom(
    const at::Tensor& self,
    const at::Tensor& dst,
    bool /* non_blocking */) {
  TORCH_CHECK(self.defined() && dst.defined(), "undefined tensor in copy_");
  TORCH_CHECK(self.is_contiguous() && dst.is_contiguous(), "only contiguous copy_");
  const auto bytes = static_cast<std::uint64_t>(self.nbytes());
  const bool src_device = self.device().is_privateuseone();
  const bool dst_device = dst.device().is_privateuseone();

  if (src_device && dst_device) {
    auto* src = allocationFromAddress(self.data_ptr());
    auto* target = allocationFromAddress(dst.data_ptr());
    TORCH_CHECK(src && target, "unknown kaminari buffer in copy_");
    copyDeviceToDevice(target, src, bytes);
  } else if (!src_device && dst_device) {
    auto* target = allocationFromAddress(dst.data_ptr());
    TORCH_CHECK(target, "unknown kaminari buffer in copy_");
    copyHostToDevice(target, self.data_ptr(), bytes);
  } else if (src_device && !dst_device) {
    auto* src = allocationFromAddress(self.data_ptr());
    TORCH_CHECK(src, "unknown kaminari buffer in copy_");
    copyDeviceToHost(dst.data_ptr(), src, bytes);
  } else {
    at::Tensor mutable_dst = dst;
    at::native::copy_(mutable_dst, self, false);
  }
  return dst;
}

at::Tensor copyFromAndResize(
    const at::Tensor& self,
    const at::Tensor& dst) {
  at::Tensor mutable_dst = dst;
  at::native::resize_(mutable_dst, self.sizes(), std::nullopt);
  return copyFrom(self, mutable_dst, false);
}

at::Scalar localScalarDense(const at::Tensor& self) {
  TORCH_CHECK(self.is_contiguous(), "only contiguous scalar read");
  auto host = at::empty({1}, self.options().device(at::kCPU));
  auto* src = allocationFromAddress(self.data_ptr());
  TORCH_CHECK(src, "unknown kaminari buffer in item()");
  copyDeviceToHost(host.data_ptr(), src, static_cast<std::uint64_t>(self.nbytes()));
  return at::native::_local_scalar_dense_cpu(host);
}

std::uint32_t fillPattern(const at::Tensor& self, const at::Scalar& value) {
  if (self.scalar_type() == c10::ScalarType::Float) {
    const float f = value.to<float>();
    std::uint32_t bits = 0;
    std::memcpy(&bits, &f, sizeof(bits));
    return bits;
  }
  if (self.scalar_type() == c10::ScalarType::Int) {
    return static_cast<std::uint32_t>(value.to<std::int32_t>());
  }
  TORCH_CHECK(false, "kaminari fill_ supports float32 and int32 only");
}

at::Tensor& fillScalar(at::Tensor& self, const at::Scalar& value) {
  TORCH_CHECK(self.is_contiguous(), "only contiguous fill_");
  auto* allocation = allocationFromAddress(self.data_ptr());
  TORCH_CHECK(allocation, "unknown kaminari buffer in fill_");
  fillBuffer(allocation, fillPattern(self, value), self.nbytes());
  return self;
}

at::Tensor& zero_(at::Tensor& self) {
  return fillScalar(self, 0);
}

at::Tensor& addOut(
    const at::Tensor& self,
    const at::Tensor& other,
    const at::Scalar& alpha,
    at::Tensor& out) {
  TORCH_CHECK(
      self.is_contiguous() && other.is_contiguous() && out.is_contiguous(),
      "kaminari add supports contiguous tensors only");
  TORCH_CHECK(
      self.sizes() == other.sizes() && self.sizes() == out.sizes(),
      "kaminari add requires equal shapes");
  TORCH_CHECK(
      self.scalar_type() == c10::ScalarType::Float &&
          other.scalar_type() == c10::ScalarType::Float &&
          out.scalar_type() == c10::ScalarType::Float,
      "kaminari add supports float32 only");
  auto* a = allocationFromAddress(self.data_ptr());
  auto* b = allocationFromAddress(other.data_ptr());
  auto* result = allocationFromAddress(out.data_ptr());
  TORCH_CHECK(a && b && result, "unknown kaminari buffer in add");
  addBuffers(a, b, result, static_cast<std::uint64_t>(self.numel()), alpha.to<float>());
  return out;
}

at::Tensor addTensor(
    const at::Tensor& self,
    const at::Tensor& other,
    const at::Scalar& alpha) {
  auto out = at::empty_like(self);
  addOut(self, other, alpha, out);
  return out;
}

void cpuFallback(const c10::OperatorHandle& op, torch::jit::Stack* stack) {
  g_fallback_count.fetch_add(1, std::memory_order_relaxed);
  TORCH_WARN_ONCE(
      "Kaminari CPU fallback for ",
      op.schema().operator_name(),
      " (count available via torch.kaminari._C._fallback_count())");
  at::native::cpu_fallback(op, stack);
}

} // namespace

std::uint64_t fallbackCount() {
  return g_fallback_count.load(std::memory_order_relaxed);
}

} // namespace kaminari

TORCH_LIBRARY_IMPL(aten, PrivateUse1, m) {
  m.impl("empty.memory_format", kaminari::emptyMemoryFormat);
  m.impl("empty_strided", kaminari::emptyStrided);
  m.impl("as_strided", kaminari::asStrided);
  m.impl("resize_", kaminari::resize_);
  m.impl("_reshape_alias", kaminari::reshapeAlias);
  m.impl("view", kaminari::view);
  m.impl("_has_compatible_shallow_copy_type", kaminari::hasCompatibleShallowCopyType);
  m.impl("_copy_from", kaminari::copyFrom);
  m.impl("_copy_from_and_resize", kaminari::copyFromAndResize);
  m.impl("_local_scalar_dense", kaminari::localScalarDense);
  m.impl("fill_.Scalar", kaminari::fillScalar);
  m.impl("zero_", kaminari::zero_);
  m.impl("add.out", kaminari::addOut);
  m.impl("add.Tensor", kaminari::addTensor);
}

TORCH_LIBRARY_IMPL(_, PrivateUse1, m) {
  m.fallback(torch::CppFunction::makeFromBoxedFunction<&kaminari::cpuFallback>());
}
