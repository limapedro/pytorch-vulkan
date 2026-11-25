# BUILD

Status: **recon complete, fork build not yet attempted** (2026-11-25). Nothing in this
file is a claimed successful build.

## Source layout

- Remote: `https://github.com/limapedro/pytorch-vulkan.git`
- Default branch: `stable/v2.15.0` (upstream tag `v2.15.0`, commit `31a77fc9c0a7c6fff15a1ff58c743f53b1a9d457`).
- Clone: `git clone --filter=blob:none --no-checkout <remote> .` then
  `git fetch --depth=1 origin refs/tags/v2.15.0:refs/tags/v2.15.0` and
  `git checkout -b stable/v2.15.0 v2.15.0`.

## Measured host environment

| Component | Version / path | Notes |
|---|---|---|
| OS | Windows (win32), i5-12400F, 32 GB RAM | GPU: GTX 1660 SUPER 6 GB, TU116, no Tensor Cores |
| Python | `C:\Program Files\Python310\python.exe`, 3.10.11; **Python 3.12.10 installed** at `C:\Users\multi\AppData\Local\Programs\Python\Python312\python.exe` | pip 26.2.1 (3.10). PyTorch v2.15.0 declares `requires-python = ">=3.11"` (`pyproject.toml:350`), so 3.10 cannot build the fork. The 3.12 install was downloaded from python.org (26,964,224 B) and installed silently; installer saved in `agent_space/` (git-ignored). |
| Git | 2.53.0.windows.2 | git-lfs 3.7.1 |
| gh | 2.105.0, account `limapedro` | repo scope available |
| CMake | 4.3.3 | generators include `Visual Studio 18 2026`, `Ninja` |
| MSVC | cl 14.42.34433 and 14.51.36231 under `C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC` | `cl.exe` is **not on PATH** by default |
| VS 2022 | `C:\Program Files (x86)\Microsoft Visual Studio\2022` | secondary installation |
| Ninja | `C:\Users\multi\AppData\Roaming\Python\Python310\Scripts\ninja.exe`, 1.13.0.git.kitware.jobserver-pipe-1 | **not on PATH**; `python -m ninja` works |
| Vulkan SDK | `C:\VulkanSDK\1.4.363.0` via `VK_SDK_PATH`/`VULKAN_SDK` | glslc, glslangValidator, spirv-val/opt/cross present in `Bin` |
| vulkaninfo | `C:\Windows\System32\vulkaninfo.exe`; SDK `vulkaninfoSDK.exe` | use `vulkaninfoSDK.exe --json -o out.json`; the System32 `--json` stdout path produced an empty file |
| CUDA | 13.3 at `C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v13.3` | nvcc present; intentionally disabled in the fork build |
| Driver | NVIDIA 621.01, Vulkan 1.4.357 | |
| ccache / sccache | **missing** | desired for the fork build |
| RenderDoc | **missing** | not a build blocker; needed for frame debugging later |

## Recovered gaps

- Ninja: add `C:\Users\multi\AppData\Roaming\Python\Python310\Scripts` to `PATH` for build shells.
- MSVC: invoke the build from a shell with `vcvars64.bat` applied, or add the
  `VC\Tools\MSVC\<version>\bin\Hostx64\x64` directory to `PATH` and set `INCLUDE`/`LIB`.
- Submodules: a blobless clone needs `git submodule update --init --depth 1 --jobs 4`
  (36 submodules). Without it, `pip install -e .` fails at metadata generation because
  `project.license-files` patterns such as `third_party/FP16/LICENSE` do not match.
- PowerShell 5.1: do not run the pip build with `$ErrorActionPreference = "Stop"`; native
  stderr becomes a terminating error. Use `cmd /c "... > build.log 2>&1"` instead.
- ccache/sccache: to be installed before the first full build; record the exact command here.
- RenderDoc: to be installed before GPU debugging; validate with `renderdoccmd version`.

## Build command policy

The repo's `CLAUDE.md` mandates one build entry point:

```
pip install -e . -v --no-build-isolation
```

No other command may be used to build PyTorch. The no-CUDA configuration will be set
with environment variables (`USE_CUDA=0`, `USE_DISTRIBUTED=0`, `BUILD_TESTING=0`,
`USE_CUDNN=0`, `CCACHE_WRAPPER` once installed) applied to that command. Exact successful
commands, timings and fixes go here after the first build (issue #5).

## Two-environment rule

The official CUDA wheel and the source fork cannot coexist in one process. The plan,
to be locked **after** measurement, is:

1. `agent_space/baseline/venv`: official CUDA wheel, reference numbers and serialized tensors.
2. Fork venv: built from source with `USE_CUDA=0`, plus in-process CPU comparison.

Baseline work is being done by a dedicated experiment; results land in
`docs/BENCHMARKS.md` and `benchmarks/results/`.

## Upstream policy

Never force-push `stable/v2.15.0`; all work arrives through squash-merged PRs. See
`docs/WORKFLOW.md` and `docs/UPSTREAM.md`.
