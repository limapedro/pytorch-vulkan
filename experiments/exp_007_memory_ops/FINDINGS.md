# exp_007 - Allocator and first memory operations

Date: 2026-11-25. Result: **both hypotheses confirmed**; add remains a fallback.

## H1 - confirmed

`REGISTER_ALLOCATOR(c10::DeviceType::PrivateUse1, ...)` installs a `c10::Allocator`
backed by Vulkan buffers. `DataPtr` carries the buffer device address (BDA is enabled in
the runtime) and a heap-allocated `Allocation*` as context. `empty.memory_format` and
`empty_strided` use `at::detail::empty_generic`/`empty_strided_generic` with honest
strides; `view`, `as_strided`, `_reshape_alias`, `resize_` and
`_has_compatible_shallow_copy_type` are registered. Allocation reuses freed buffers by
4096-byte size class under a mutex. No core PyTorch file changed.

## H2 - confirmed

H2D/D2H/D2D `copy_` goes through host-visible staging buffers and `vkCmdCopyBuffer`
with explicit submit + `vkQueueWaitIdle`. `fill_`/`zero_` use `vkCmdFillBuffer` with the
float32 bit pattern. `_local_scalar_dense` (`item()`) copies one scalar to host.
Measured smoke values: `[1,2,3,4]` round-trip exact, `fill_(2.5)` exact, `zero_` exact,
`item() == 7.25` exact, `view(2,2)` contiguous. All 4 memory tests pass; full local
suite is 9/9 (3 registration + 2 runtime + 4 memory).

## Counted CPU fallback

`TORCH_LIBRARY_IMPL(_, PrivateUse1, m)` installs a global fallback that increments an
atomic counter and emits `TORCH_WARN_ONCE`. `a + b` on device produced
`fallback_delta = 1`, result `[3,3,3,3]`, device `kaminari:0`. Adding
`_copy_from_and_resize` was required for the fallback result to return to the device;
without it PyTorch raised `NotImplementedError` for the CPU dispatch of that op.

## Caveats / negative results

- `Allocator::allocate(size_t)` is **not const** in v2.15.0 (compile error C3668).
- `at::empty` needs `ATen/Functions.h`; `ATen/EmptyTensor.h` alone is insufficient.
- `copyFrom` takes three arguments; `_copy_from_and_resize` calls it with
  `non_blocking=false` after `resize_`.
- The v1 pool does no suballocation or coalescing: one `VkDeviceMemory` per cached
  buffer, freed buffers are retained forever. Fragmentation and `empty_cache` behavior
  are Phase 4 work; no memory stats are claimed here.
- `add` is **not** a Vulkan kernel yet. It uses the counted CPU fallback, as intended
  for early end-to-end coverage.
- No timings were taken; no performance claim is made.

## Artifacts

- `kaminari/csrc/Allocator.cpp`, `VulkanMemory.{h,cpp}`, `Ops.{h,cpp}`.
- `kaminari/tests/test_memory.py`.
