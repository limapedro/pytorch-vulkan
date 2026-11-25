# exp_006 - Vulkan runtime skeleton

Date: 2026-11-25

## H1

A minimal Vulkan compute context can be created on the GTX 1660 Super with a discrete-GPU
selection policy, buffer device address, timeline semaphores and fp16/int8 features, and
its queried limits match `vulkaninfo` (subgroup size 32, 49 KiB shared memory, 1024
invocations, 256 B push constants).

**Falsifier:** instance/device creation fails, or queried limits disagree with the
`vulkaninfo` dump recorded in exp_001.

## H2

The context can expose a lazily recorded compute command buffer and a synchronization
entry point without allocating descriptors per op, and this can be built as part of the
same standalone extension with `find_package(Vulkan)`.

**Falsifier:** CMake cannot find Vulkan, or queue/command-buffer setup fails at runtime.

## Measurement plan

- `_C._device_info()` dumps name, driver, API version, subgroup size, limits, extension
  list and queue family indices; save as raw JSON in `benchmarks/kaminari/results/`.
- `_C._is_vulkan_available()` and `_C._synchronize()` smoke checks.
- No kernels yet, so no GPU timestamp numbers are claimed.
