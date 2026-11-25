# exp_006 - Vulkan runtime findings

Date: 2026-11-25. Result: **both hypotheses confirmed**. Raw data:
`benchmarks/kaminari/results/2026-11-25_vulkan_capabilities.json`.

## H1 - confirmed

`kaminari::Runtime` creates a Vulkan 1.3 instance, selects the discrete GPU, enables
buffer device address, timeline semaphores, 16-bit storage, fp16/int8 and descriptor
indexing, and creates a compute queue, a transfer-only queue, a command pool, one
lazily recorded primary command buffer, a fence and an on-disk pipeline cache.

Queried values from `_C._device_info()` match the `vulkaninfo` dump in exp_001:

| Field | Measured via runtime | vulkaninfo |
|---|---|---|
| device | NVIDIA GeForce GTX 1660 SUPER | same |
| vendor/device ID | 4318 / 8644 (0x10de / 0x21c4) | same |
| subgroup size | 32 | 32 |
| max workgroup invocations | 1024 | 1024 |
| max shared memory | 49,152 B | 49,152 B |
| max push constants | 256 B | 256 B |
| timestamp valid bits | 64 | 64 |
| compute / transfer queue family | 0 / 1 | compute+graphics 0, transfer-only 1 |
| device extensions | 272 | (matrix in docs/VULKAN.md) |
| BDA / timeline / fp16 / int8 / 16-bit storage | all true | all true |

## H2 - confirmed

`find_package(Vulkan)` found SDK 1.4.363.0 (`C:/VulkanSDK/1.4.363.0/Lib/vulkan-1.lib`,
glslc and glslangValidator). The standalone extension built and linked. `_C._device_info`,
`_C._is_vulkan_available` and `_C._synchronize_vulkan` work; `test_runtime.py` passes
2/2 and `test_smoke.py` remains 3/3. The command buffer is begin-recorded once and
submission will be added with the first kernels (issue #11); `synchronize()` currently
uses `vkQueueWaitIdle`, which is conservative and correct.

## Negative results / caveats

- No kernel has been submitted yet, so there are no GPU timestamp measurements in this
  experiment. Timestamp queries are only confirmed as *available* (64 valid bits).
- The pipeline cache is created from/read to a file under `KAMINARI_CACHE_DIR` or
  `%TEMP%/kaminari`; its effectiveness will be measured in Phase 4.
- Validation layers are not enabled yet; the device is created with core features only
  plus the extensions listed above.

## Artifacts

- `kaminari/csrc/Runtime.h`, `Runtime.cpp`; `kaminari/CMakeLists.txt` updated.
- `benchmarks/kaminari/results/2026-11-25_vulkan_capabilities.json`.
