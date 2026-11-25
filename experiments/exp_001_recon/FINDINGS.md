# exp_001 - Recon findings

Date: 2026-11-25

## H1 - confirmed with corrections

| Item | Result | Evidence |
|---|---|---|
| Python | 3.10.11, pip 26.2.1 | `python --version`, `python -m pip --version` |
| Git / gh | 2.53.0.windows.2 / 2.105.0; account `limapedro` | `git --version`, `gh auth status` |
| Vulkan SDK | 1.4.363.0, `VK_SDK_PATH`/`VULKAN_SDK` set | env probe |
| glslc / glslangValidator / spirv-val / spirv-opt / spirv-cross | present in `C:\VulkanSDK\1.4.363.0\Bin` | `where.exe` |
| vulkaninfo | `C:\Windows\System32\vulkaninfo.exe` plus SDK `vulkaninfoSDK.exe` | `where.exe`, SDK Bin listing |
| MSVC | VS 18 Community at `C:\Program Files\Microsoft Visual Studio\18\Community`, cl 14.42.34433 and 14.51.36231; VS 2022 at x86 path | `Get-ChildItem -Recurse -Filter cl.exe` |
| CMake / Ninja | CMake 4.3.3 (VS 18 2026 and Ninja generators available). `ninja.exe` at `C:\Users\multi\AppData\Roaming\Python\Python310\Scripts`, `python -m ninja` 1.13.0.git.kitware. NOT on PATH. | `cmake --help`, `python -m ninja --version` |
| ccache / sccache | absent | `where.exe` |
| CUDA toolkit | v13.3 at `C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v13.3`, `nvcc.exe` present | `where.exe nvcc` |
| RenderDoc | absent | `where.exe renderdoccmd`, Program Files check |
| GPU | NVIDIA GeForce GTX 1660 SUPER, deviceID 0x21c4, driver 621.01, Vulkan 1.4.357 | `vulkaninfo --summary` |
| Git LFS | 3.7.1 | `git lfs version` |

Corrections to assumptions: Ninja exists but needs its Scripts directory on PATH;
ccache/sccache and RenderDoc are missing and are recorded as gaps in `docs/BUILD.md`.

## H2 - confirmed with corrections

- No pre-existing fork. `gh repo fork pytorch/pytorch --clone=false` created `limapedro/pytorch`; renamed to `kaminari`, then `pytorck-vulkan`, finally **`limapedro/pytorch-vulkan`** at the user's request. The user's last instruction wins; the mission codename "Kaminari" remains the project name.
- Latest stable upstream release as of this date is **v2.15.0** (published 2026-10-28), commit `31a77fc9c0a7c6fff15a1ff58c743f53b1a9d457`. Branch `stable/v2.15.0` was pushed and set as the fork default; no force-push was used.
- Clone used `--filter=blob:none`. The first clone was interrupted by the user; a partial `.git` remained and was recovered by re-pointing `origin` and fetching the tag. No source was lost.
- GitHub **issues were disabled** on the fork (inherited from upstream), HTTP 410. Enabled with `gh api -X PATCH repos/limapedro/pytorch-vulkan -f has_issues=true`. Eight milestones and issues #1-#7 now exist.
- Repo `CLAUDE.md` contains an AI policy that forbids autonomous GitHub activity and unsigned commits. The mission explicitly authorizes both. Decision: follow the direct user instruction, disclose AI authorship in issue/PR bodies, and document the conflict in `docs/WORKFLOW.md`.

## H3 - not settled yet

Source study is running. No decision is locked until the evidence is in
`agent_space/research_pytorch.md` and recorded in `ARCHITECTURE_LOG.md`.

## Vulkan capability highlights (GTX 1660 SUPER)

- 3 memory heaps: 5.83 GiB device-local (heap 0, budget 5.08 GiB), 15.92 GiB host (heap 1), 214 MiB device-local (heap 2). Six memory types, types 1-4 device-local.
- Queue families: graphics+compute+transfer (16 queues), compute+transfer (8 queues), transfer-only (2), video decode and encode families.
- Limits: `maxComputeWorkGroupInvocations=1024`, `maxComputeWorkGroupSize=[1024,1024,64]`, `maxComputeSharedMemorySize=49152`, `maxPushConstantsSize=256`, `maxStorageBufferRange=2^32-1`, `maxMemoryAllocationCount=2^32-1`.
- `subgroupSize=32` (must still be queried at runtime on other GPUs).
- Supported: `VK_KHR_push_descriptor` (rev 2), `VK_EXT_descriptor_buffer`, buffer device address (core 1.2), `VK_KHR_16bit_storage`, `VK_KHR_shader_float16_int8`, `VK_KHR_shader_integer_dot_product`, `VK_KHR_timeline_semaphore`, `VK_EXT_subgroup_size_control`, `VK_EXT_memory_budget`.
- **No cooperative matrix feature** on this device; GEMM must use classic shared-memory tiling.
- The `System32\vulkaninfo.exe --json` path produced an empty file; `vulkaninfoSDK.exe --json -o <file>` produced a 471 KB profiles-format JSON that was used instead.

## Artifacts

- `agent_space/vulkaninfo.txt` (109,134 bytes, 2,484 lines).
- `agent_space/vulkaninfo.json` (471,344 bytes, Vulkan Profiles capabilities schema).
