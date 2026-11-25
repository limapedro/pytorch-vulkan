# BUILD

Status: **fork build green** (2026-11-25). `pip install -e . -v --no-build-isolation`
exited 0 with `USE_CUDA=0`; the built wheel imports and reports no CUDA. Full build log
is `agent_space/build.log` (git-ignored); evidence in `experiments/exp_002_build/FINDINGS.md`.

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
- CMake 4.4.5 from the pip `cmake` wheel failed once in a SLEEF 3.8.0 `try_compile`
  probe (`ninja -v cmTC_80db8`); uninstalling it let the system CMake 4.3.3 be used for
  one attempt. Note `pip install -r requirements-build.txt` reinstalls 4.4.5 on every
  build run, so the pin is not sticky. The later configure with 4.4.5 succeeded.
- Use `MAX_JOBS=2` / `CMAKE_BUILD_PARALLEL_LEVEL=2`: with 4 jobs, four large
  `torch/csrc/jit/passes/*.cpp` objects failed together with no diagnostic after
  ~49 minutes. A 2-job retry is in progress to reuse the ~1,700 existing objects.
- ccache/sccache: to be installed before the first full build; record the exact command here.
- RenderDoc: to be installed before GPU debugging; validate with `renderdoccmd version`.

## Build command policy

The repo's `CLAUDE.md` mandates one build entry point:

```
pip install -e . -v --no-build-isolation
```

No other command may be used to build PyTorch. The successful no-CUDA configuration used environment variables
`USE_CUDA=0`, `USE_CUDNN=0`, `USE_DISTRIBUTED=0`, `BUILD_TESTING=0`, `MAX_JOBS=2`,
`CMAKE_BUILD_PARALLEL_LEVEL=2`, applied to that command from a VS DevShell.
Build time: 03:13:07 -> 04:35:37 (1 h 22 min, reusing objects from an earlier failed
attempt). Wheel: `torch-2.15.0a0+git8f2938f`, 93,180,921 B. Verification:
`torch.version.cuda is None`, `torch.cuda.is_available() is False`, basic `add`/`mm`
sanity checks pass. ccache/sccache remains uninstalled (H3 in exp_002).

## Building the Kaminari extension

The backend is a standalone extension and is **not** part of the PyTorch build. With the
fork venv active and a VS DevShell loaded:

```
cmake -S kaminari -B <build-dir> -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="<venv>/Lib/site-packages/torch;<venv>/Lib/site-packages/pybind11/share/cmake/pybind11"
cmake --build <build-dir> --config Release
pip install -e kaminari --no-build-isolation
```

`setup.py` runs the same CMake steps and registers the `torch.backends` autoload entry
point. `pybind11` 3.1.0 and `expecttest` 0.3.0 were installed into the fork venv for the
extension and its tests. Verification:

```
python -c "import torch; print(torch.kaminari.is_available(), torch.kaminari.device_count())"
python kaminari/tests/test_smoke.py
```

Build artifacts (`*.pyd`, `build_cmake/`) are ignored by `kaminari/.gitignore` and are
never committed. Evidence: `experiments/exp_005_device_registration/FINDINGS.md`.

## Baseline environments (measured)

| Venv | Interpreter | torch | Purpose |
|---|---|---|---|
| `agent_space/baseline/venv312` | Python 3.12.10 | `2.15.0+cu132` from `https://download.pytorch.org/whl/cu132` | primary CUDA baseline, version-matched to the fork |
| `agent_space/baseline/venv` | Python 3.10.11 | `2.14.1+cu130` | secondary CUDA check; 3.10 cannot install 2.15.0 |
| `agent_space/baseline/venv_dml` | Python 3.10.11 | `2.4.1+cpu` + `torch-directml 0.2.5.dev240914` | DirectML baseline; DML pins torch 2.4.1, so it needs its own venv |
| `agent_space/fork/venv` | Python 3.12.10 | fork built from source, `USE_CUDA=0` | Kaminari build (not yet successful) |

Install command for the primary baseline (exact):

```
& '...\venv312\Scripts\python.exe' -m pip install --no-cache-dir --progress-bar off --index-url https://download.pytorch.org/whl/cu132 --extra-index-url https://pypi.org/simple 'torch==2.15.0'
```

`--extra-index-url` works around a bad `typing_extensions` sdist on the PyTorch-only
index. The PyPI Windows 2.15.0 wheel is CPU-only (126.7 MB); the `cu132` wheel is
2108.3 MB. `pip index versions torch` on 3.10 tops out at 2.14.1.

All baseline verification and benchmark commands must run with cwd at
`agent_space/baseline`, not the repo root: from the repo root Python imports the
unbuilt `torch/` source directory and fails with
`ModuleNotFoundError: No module named 'torch.version'`.

## Two-environment rule

The official CUDA wheel and the source fork cannot coexist in one process. Decision
D010 in `ARCHITECTURE_LOG.md` (locked after measurement in exp_004): keep the separate
venvs, serialize fixed reference tensors from the baseline venv, and compare them
cross-process against the fork's CPU result and (once built) the Vulkan device, with
in-process CPU as a second control. Numbers and commands are in `docs/BENCHMARKS.md`
and `benchmarks/kaminari/`.

## Upstream policy

Never force-push `stable/v2.15.0`; all work arrives through squash-merged PRs. See
`docs/WORKFLOW.md` and `docs/UPSTREAM.md`.
