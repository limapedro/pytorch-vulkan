# exp_002 - Build findings

Date: 2026-11-25. Result: **build green** (attempt 5).

## Verdict

- **H1 confirmed:** a no-CUDA PyTorch v2.15.0 source build succeeds on this Windows host
  with Python 3.12.10, MSVC 14.51 and Ninja via the repo-mandated
  `pip install -e . -v --no-build-isolation`.
- **H2 confirmed:** CMake reported `USE_CUDA=0`, `USE_CUDNN=0`, `USE_DISTRIBUTED=0`,
  `USE_NCCL=OFF`, `USE_VULKAN=OFF`, `USE_FBGEMM=OFF`, `USE_NNPACK=OFF`,
  `USE_XNNPACK=OFF`; the built wheel reports `torch.version.cuda is None` and
  `torch.cuda.is_available() is False`.
- **H3 open:** ccache/sccache is absent and no incremental timing comparison was made.
  The 2-job retry did reuse ~1,700 existing objects, which suggests caching would help.

## Attempt 1 - PowerShell stderr handling (negative)

Running pip under `$ErrorActionPreference = "Stop"` in PowerShell 5.1 stopped immediately
with `NativeCommandError` on the first stderr line. `agent_space/build.log` had only three
lines. Fix: invoke pip via `cmd /c "... > build.log 2>&1"` and set
`$ErrorActionPreference = "Continue"` around that call (`agent_space/build_fork.ps1`,
git-ignored).

## Attempt 2 - missing submodules (negative)

Metadata generation failed with 54 `ConfigurationError`s:
`Every pattern in "project.license-files" must match at least one file:
'third_party/FP16/LICENSE' did not match any`. Root cause: `git clone --filter=blob:none
--no-checkout` left 36 submodules uninitialized. Fixed with recursive
`git submodule update --init --depth 1` plus `core.longpaths=true` for Windows MAX_PATH
issues in AMD's `composable_kernel`; all license patterns now match.

## Attempt 3 - CMake 4.4.5 SLEEF probe (negative)

Configuration failed after ~39 s with:
`CMake Error: Generator: build tool execution failed, command was: ...\ninja -v cmTC_80db8`
in SLEEF 3.8.0's `COMPILER_SUPPORTS_BUILTIN_MATH` check. A standalone CMake project using
the same ninja and MSVC completed `check_cxx_source_compiles`, so the toolchain was sound.
The transient nature was confirmed: a later configure with the same CMake 4.4.5 succeeded.
Note `pip install -r requirements-build.txt` reinstalls cmake 4.4.5 on every run, so
uninstalling it is not a durable fix.

## Attempt 4 - MSVC parallelism failure (negative)

With `MAX_JOBS=4`, the build ran 49 minutes to 1709/2866, then four
`torch/csrc/jit/passes/*.cpp` objects failed together (`constant_propagation`,
`constant_pooling`, `create_functional_graphs`, `create_autodiff_subgraphs`) with
`FAILED: [code=1]` and no compiler diagnostic. Free RAM was 21 GB afterwards; most likely
MSVC memory pressure or an unlogged compiler crash.

## Attempt 5 - success

With `MAX_JOBS=2` / `CMAKE_BUILD_PARALLEL_LEVEL=2` and the same MSVC 14.51 toolset:

- Started `2026-11-25T03:13:07-03:00`, exited 0 at `2026-11-25T04:35:37-03:00`
  (~1 h 22 min, reusing ~1,700 objects from the failed 4-job attempt).
- 1,156 final ninja targets; wheel
  `torch-2.15.0a0+git8f2938f-cp312-cp312-win_amd64.whl` (93,180,921 B,
  sha256 `dc34c802dc6356b1f49be3e3bd49317e897f2dfd2806095de66fe65eb2023374`).
- Runtime deps installed: mpmath, sympy, networkx, MarkupSafe, fsspec, filelock, jinja2.
- Verification from `agent_space/fork` (so the source tree does not shadow the wheel):
  `torch.__version__ = 2.15.0a0+git8f2938f`, `cuda.is_available() = False`,
  `torch.version.cuda = None`; `add` and `mm` sanity checks true.

## Environment corrections

- Python 3.12.10 installed from python.org (`pyproject.toml:350` requires >=3.11).
- Build venv `agent_space/fork/venv`: scikit-build-core 1.1.2, setuptools 81.0.0,
  cmake 4.4.5, ninja 1.13.2, numpy 2.5.5, pyyaml 6.0.4.
- VS DevShell (`Microsoft.VisualStudio.DevShell.dll`) supplies the MSVC environment.
- `agent_space/build_fork.ps1` is git-ignored; the exact successful environment is
  documented in `docs/BUILD.md`.
