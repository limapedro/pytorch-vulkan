# VULKAN

Everything Vulkan-specific that Kaminari relies on. All numbers in the matrix are
**measured on this machine** with `vulkaninfoSDK.exe --json -o agent_space/vulkaninfo.json`
on 2026-11-25; other GPUs must be filled in by the standalone probe script (Phase 6).

## Capability matrix

| GPU | Vendor / device | Driver | VK API | subgroupSize | max workgroup invocations | shared mem | push constants | float16 | int8 | 16-bit storage | BDA | push descriptors | timeline semaphores | cooperative matrix |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| GTX 1660 SUPER (TU116) | 0x10de / 0x21c4 | 621.01 | 1.4.357 (SDK 1.4.363) | 32 | 1024 | 49,152 B | 256 B | yes (`shaderFloat16`) | yes (`shaderInt8`) | yes (`storageBuffer16BitAccess`) | yes | yes (rev 2) | yes (rev 2) | **absent** |

Per-GPU results for AMD/Intel/other NVIDIA are tracked in `docs/DEVICES.md`.

## Instance and device

- Instance API 1.4.363 available; validation, synchronization2, shader-object and
  profiling layers installed. Two third-party 1.3 layers (OBS hook, MediaSDK hook)
  warn when an app requests 1.4; do not enable them in the backend.
- Device heaps measured: heap 0 = 5.83 GiB device-local (budget 5.08 GiB), heap 1 =
  15.92 GiB host, heap 2 = 214 MiB device-local. Six memory types; types 1-4 are
  device-local.
- Queue families: 16 graphics+compute+transfer, 8 compute-only+transfer, 2 transfer-only,
  plus video decode/encode families. Async compute and transfer are possible on
  separate families; ownership transfer policy is still to be measured.
- `VK_EXT_memory_budget` is present, so allocator pressure can use real budgets rather
  than spec-sheet VRAM.

## Memory and storage

- Tensor storage will be **plain strided buffers**, not textures (design guidance 3;
  rationale to be locked in `ARCHITECTURE_LOG.md` after source study).
- `maxMemoryAllocationCount = 2^32-1`, so a suballocating caching allocator is about
  fragmentation and fast reuse, not a hard allocation-count cap.
- `minStorageBufferOffsetAlignment = 16`; `maxStorageBufferRange = 2^32-1`.
- Push constants are only 256 B: use them for shapes/tiles/scalars, never for tensor data.
- Staging path: host-visible heap 1 -> device-local heap 0 via `vkCmdCopyBuffer`, with
  a transfer queue if it measurably helps.

## Descriptors and binding

- Options present: plain descriptor sets, `VK_KHR_push_descriptor` rev 2,
  `VK_EXT_descriptor_buffer`, and buffer device address (core 1.2 feature enabled).
- Avoid per-op descriptor-set allocation (design guidance 1). Decision between push
  descriptors and BDA is an experiment, not an assumption; both are available here.
- `maxBoundDescriptorSets=32`, `maxPerStageDescriptorStorageBuffers=1048576`.

## Compute and subgroups

- `maxComputeWorkGroupSize=[1024,1024,64]`, `maxComputeWorkGroupInvocations=1024`,
  48 KiB shared memory. Kernels must be subgroup-size agnostic; this card reports 32
  but `VK_EXT_subgroup_size_control` exists and other vendors differ.
- Useful extensions measured present: `VK_KHR_shader_subgroup_extended_types`,
  `VK_KHR_shader_integer_dot_product`, `VK_KHR_shader_float_controls` (rev 4),
  `VK_KHR_16bit_storage`, `VK_KHR_shader_float16_int8`, `VK_KHR_8bit_storage`,
  `VK_EXT_shader_atomic_float`, `VK_KHR_workgroup_memory_explicit_layout`.
- **No cooperative matrix** feature: GEMM must use classic shared-memory tiling with
  subgroup-aware loads, not tensor cores. TU116 has no Tensor Cores, so fp16 will not
  have the throughput advantage it has on newer NVIDIA GPUs; this must be measured.

## Synchronization and submission

- `VK_KHR_timeline_semaphore` rev 2 and synchronization2 layer available.
- Lazy command-buffer recording with per-buffer read/write hazard tracking is the
  planned approach; flush only at synchronization points (`item()`, `.cpu()`, print,
  explicit synchronize). This is a hypothesis to be measured in Phase 4.
- Timestamp queries: queue families report `timestampValidBits=64` for graphics/compute.
- Windows TDR: long dispatches are killed by the driver; large reductions/GEMMs must be
  split into bounded submissions. Thresholds to be measured, not guessed.

## Toolchain

- GLSL -> SPIR-V via `glslc`; validation with `glslangValidator` and `spirv-val`;
  optimization with `spirv-opt`; inspection with `spirv-cross`.
- Pipeline cache on disk is planned to reduce startup time; specialization constants
  for tile sizes and dtypes avoid shader permutation explosions.
- Validation layers and RenderDoc (currently missing) for debugging; see `docs/BUILD.md`.

## Reproduce the matrix

```
vulkaninfoSDK.exe --json -o agent_space/vulkaninfo.json
vulkaninfo > agent_space/vulkaninfo.txt
```

A portable probe script that dumps this matrix plus a mini benchmark is a Phase 6
deliverable so other users can contribute rows to `docs/DEVICES.md`.
