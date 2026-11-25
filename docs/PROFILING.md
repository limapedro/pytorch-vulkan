# PROFILING

Tooling and recipes. Nothing has been profiled yet.

## Available on this host

- Vulkan SDK 1.4.363.0 validation, synchronization2, shader-object, api_dump,
  gfxreconstruct, monitor, crash-diagnostic and profiles layers (`vulkaninfo --summary`).
- `spirv-cross`, `spirv-val`, `spirv-opt`, `glslc`, `glslangValidator`.
- Validation can be enabled with `KAMINARI_VALIDATE=1`: the runtime installs
  `VK_LAYER_KHRONOS_validation`, `VK_EXT_debug_utils` and a debug messenger that prints
  warnings/errors to stderr. This caught two real bugs in the add kernel (exp_008).
- RenderDoc is **not installed** (see `docs/BUILD.md`); install before GPU frame debugging.
- Timestamp queries: 64 valid bits on graphics/compute queues.

## Plan

- Add GPU timestamp ranges around command-buffer submission and per-kernel dispatch.
- Wall-clock numbers always use explicit sync; never mix timestamp and wall-clock without
  labeling which is which.
- Use validation layers in debug builds; never in benchmarks.
- Capture a frame with gfxreconstruct for hazard review when lazy submission lands.
- Write raw profiler output to `agent_space/` (git-ignored), summaries to results JSON.
