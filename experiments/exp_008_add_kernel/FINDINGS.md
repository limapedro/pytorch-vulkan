# exp_008 - Vulkan add kernel findings

Date: 2026-11-25. Result: **both hypotheses confirmed**.

## H1 - confirmed

`shaders/add.comp` compiles with glslc (`--target-env=vulkan1.3`) into SPIR-V, embedded
as a C++ header by `kaminari/tools/embed_spirv.py` (dataclass config, no argparse). The
pipeline uses BDA buffer references in push constants and an empty descriptor-set layout:
no per-op descriptor allocation. `add.out` and `add.Tensor` are registered on PrivateUse1.
Measured fp32 outputs matched CPU exactly: `[1,2,3,4] + [10,20,30,40] = [11,22,33,44]`.

## H2 - confirmed

`a + b` on device no longer uses the CPU fallback (`fallback delta = 0` over 64 elements).
Full local suite: 11/11 (3 registration + 2 runtime + 4 memory + 2 add). No core file was
modified.

## Negative results and fixes

- GLSL reserves `out` as a keyword; the result buffer reference was renamed to `result`.
- Validation (enabled via `KAMINARI_VALIDATE=1`) found two real bugs:
  1. buffers queried with `vkGetBufferDeviceAddress` lacked
     `VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT`; fixed, plus
     `VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT` on the memory allocation;
  2. `vkCmdDispatch` was issued without `vkCmdBindPipeline`; fixed.
- The descriptor set layout was destroyed while the pipeline layout still referenced it;
  it is now retained for process lifetime.
- Vulkan calls in a static destructor crashed after validation-layer teardown at exit;
  the Runtime destructor now intentionally leaks handles and the pipeline cache is no
  longer flushed there (explicit flush is Phase 4 work).
- `mul.out` (e.g. `2.0 * b`) still uses the counted CPU fallback; it is outside this PR.

## Timing

No latency benchmark was run, so no speed ratio is claimed. Shader/pipeline creation is
lazily triggered on first add; its cost will be measured in Phase 4.
