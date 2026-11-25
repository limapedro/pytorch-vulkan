# Phase 0 baseline report (2026-11-25)

All files are under `E:\__PROJECTS__\pytorch-vulkan\agent_space\baseline\` (git-ignored scratch).
No tracked files were modified. No commit, push, issue, or PR was made. Pre-existing worktree changes
(`HYPOTHESIS.md`, `third_party/composable_kernel`, untracked `experiments/exp_004_cross_process/`) were left untouched.

## 1. Interpreters and installed versions

| Environment | Interpreter | torch | `torch.version.cuda` | Verification |
| --- | --- | --- | --- | --- |
| `venv312` (primary) | `C:\Users\multi\AppData\Local\Programs\Python\Python312\python.exe` — Python 3.12.10 | `2.15.0+cu132` | `13.2` | `torch.cuda.is_available()=True`, 1 device, `NVIDIA GeForce GTX 1660 SUPER`, capability `(7,5)`, cuDNN 92600 |
| `venv` (secondary 3.10 check) | `C:\Program Files\Python310\python.exe` — Python 3.10.11 | `2.14.1+cu130` | `13.0` | CUDA available, same device name/capability, cuDNN 92400 |
| `venv_dml` | `C:\Program Files\Python310\python.exe` — Python 3.10.11 | `2.4.1+cpu` | `None` | `torch-directml 0.2.5.dev240914`, `privateuseone:0`, `is_available()=True`, device name `NVIDIA GeForce GTX 1660 SUPER` |

Every number below is labeled with the environment that produced it. Driver 621.01 ran the `cu132` build successfully.

### Version decision and negative results

- Repo `pyproject.toml:350` declares `requires-python = ">=3.11"` and `version.txt` is `2.15.0a0`, so Python 3.10 cannot be used for the 2.15.0 baseline. Python 3.12.10 was provided during this task and was used for the primary venv.
- With Python 3.10, `pip index versions torch` reported latest `2.14.1`; with Python 3.12 it reported `2.15.0`.
- Official index probes (`pip index versions torch --index-url ...`): `cu130` -> `2.14.1+cu130`, `cu126` -> `2.14.1+cu126`, `cu128` -> `2.11.0+cu128`, `cu131` -> no distribution, **`cu132` -> `2.15.0+cu132`**. The primary install therefore uses `2.15.0+cu132` from `cu132`.
- PyPI `torch==2.15.0` on Windows is a **126.7 MB CPU wheel** (dry run showed no `nvidia-*` dependencies), so it was not used as a CUDA baseline. The `+cu132` wheel is 2108.3 MB.
- `torch-directml` pins `torch==2.4.1` and `torchvision==0.19.1`, hence the separate `venv_dml` (installing it into the primary venv would downgrade torch).

## 2. Exact commands run

```powershell
# Baseline dirs and venvs
New-Item -ItemType Directory -Force -Path 'E:\__PROJECTS__\pytorch-vulkan\agent_space\baseline\results' | Out-Null
& 'C:\Program Files\Python310\python.exe' -m venv 'E:\__PROJECTS__\pytorch-vulkan\agent_space\baseline\venv'
& 'C:\Program Files\Python310\python.exe' -m venv 'E:\__PROJECTS__\pytorch-vulkan\agent_space\baseline\venv_dml'
& 'C:\Users\multi\AppData\Local\Programs\Python\Python312\python.exe' -m venv 'E:\__PROJECTS__\pytorch-vulkan\agent_space\baseline\venv312'

# Index probes (abbreviated to the meaningful variants)
& 'C:\Program Files\Python310\python.exe' -m pip index versions torch --index-url https://download.pytorch.org/whl/cu130 --no-cache-dir
& 'C:\Program Files\Python310\python.exe' -m pip index versions torch --index-url https://download.pytorch.org/whl/cu128 --no-cache-dir
& 'C:\Users\multi\AppData\Local\Programs\Python\Python312\python.exe' -m pip index versions torch --no-cache-dir
& 'C:\Users\multi\AppData\Local\Programs\Python\Python312\python.exe' -m pip install --dry-run --no-cache-dir 'torch==2.15.0'
& 'C:\Users\multi\AppData\Local\Programs\Python\Python312\python.exe' -m pip index versions torch --index-url https://download.pytorch.org/whl/cu132 --no-cache-dir

# Installs (--extra-index-url avoids a bad typing_extensions sdist on the PyTorch index)
& 'E:\__PROJECTS__\pytorch-vulkan\agent_space\baseline\venv\Scripts\python.exe' -m pip install --no-cache-dir --progress-bar off --index-url https://download.pytorch.org/whl/cu130 --extra-index-url https://pypi.org/simple 'torch==2.14.1'
& 'E:\__PROJECTS__\pytorch-vulkan\agent_space\baseline\venv312\Scripts\python.exe' -m pip install --no-cache-dir --progress-bar off --index-url https://download.pytorch.org/whl/cu132 --extra-index-url https://pypi.org/simple 'torch==2.15.0'
& 'E:\__PROJECTS__\pytorch-vulkan\agent_space\baseline\venv_dml\Scripts\python.exe' -m pip install --no-cache-dir --progress-bar off torch-directml
& 'E:\__PROJECTS__\pytorch-vulkan\agent_space\baseline\venv312\Scripts\python.exe' -m pip install --no-cache-dir --progress-bar off numpy
& 'E:\__PROJECTS__\pytorch-vulkan\agent_space\baseline\venv\Scripts\python.exe' -m pip install --no-cache-dir --progress-bar off numpy

# Verification (run with cwd = baseline dir so the repo's torch/ source does not shadow the wheel)
& '...\venv312\Scripts\python.exe' -c "import torch; print(torch.__version__, torch.version.cuda, torch.cuda.is_available(), torch.cuda.get_device_name(0), torch.cuda.get_device_capability(0))"
& '...\venv\Scripts\python.exe' -c "import torch; print(torch.__version__, torch.version.cuda, torch.cuda.is_available(), torch.cuda.get_device_name(0))"
& '...\venv_dml\Scripts\python.exe' -c "import torch, torch_directml; d=torch_directml.device(); print(torch.__version__, torch_directml.device(), torch_directml.is_available(), torch_directml.device_name(0))"

# Benchmarks (dataclass configs, no argparse)
& 'C:\Program Files\Python310\python.exe' 'E:\__PROJECTS__\pytorch-vulkan\agent_space\baseline\run_benchmarks.py'

# Reference tensors and parity
$env:REF_MODE='reference'; & '...\venv312\Scripts\python.exe' reference_tensors.py
$env:REF_MODE='device'; $env:REF_BACKEND='cuda'; $env:REF_RUN='py312_cuda'; & '...\venv312\Scripts\python.exe' reference_tensors.py
$env:REF_MODE='device'; $env:REF_BACKEND='cpu'; $env:REF_RUN='py312_cpu'; & '...\venv312\Scripts\python.exe' reference_tensors.py
$env:REF_MODE='device'; $env:REF_BACKEND='dml'; $env:REF_RUN='py310_dml'; & '...\venv_dml\Scripts\python.exe' reference_tensors.py
& '...\venv312\Scripts\python.exe' check_parity.py
```

## 3. Benchmark setup

Scripts: `benchmark.py`, `run_benchmarks.py`, `reference_tensors.py`, `check_parity.py`.
`BenchConfig` (frozen dataclass): `seed=1337`, `warmup=5`, `repeats=20`, `dtype=float32`,
`add_shape=(4096,4096)`, `gemm_shape=(1024,1024,1024)`, `mlp_shape=(256,512,256,10)`.
Inputs are generated from a seeded CPU generator before transfer, so shapes/values match across backends;
`add` uses `torch.add(..., out=...)`, GEMM uses `torch.addmm(..., out=...)`, MLP is 3 Linear layers with ReLU
and MSE backward (forward+backward only, no optimizer step). Median of 20 timed iterations after 5 warmups.

Explicit sync: CUDA `torch.cuda.synchronize()`, DML `probe.cpu()` copy (torch-directml 0.2.5 has no
`synchronize` symbol), CPU no-op. CUDA peak VRAM uses `torch.cuda.reset_peak_memory_stats()` before each
timed call and `torch.cuda.max_memory_allocated()` after sync.

## 4. Measured results

All values are medians in milliseconds; 20 timed iterations, 5 warmups, float32.

| Run (interpreter / torch) | Backend | Startup (ms) | add (ms) | addmm (ms) | MLP fwd+bwd (ms) | Peak VRAM |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| `py312_cuda` (3.12.10 / 2.15.0+cu132) | CUDA | 0.1503 | 0.7023 | 0.9970 | 1.2719 | 214.42 MiB |
| `py312_cuda` (3.12.10 / 2.15.0+cu132) | CPU | 0.0150 | 6.9721 | 6.3849 | 1.5901 | n/a |
| `py310_cuda` (3.10.11 / 2.14.1+cu130) | CUDA | 0.1181 | 0.6898 | 0.9458 | 1.0260 | 222.55 MiB |
| `py310_cuda` (3.10.11 / 2.14.1+cu130) | CPU | 0.0229 | 6.4584 | 5.8186 | 1.2169 | n/a |
| `py310_dml` (3.10.11 / 2.4.1+cpu + DML) | DML | 0.2068 | 0.9007 | 0.8429 | 1.2353 | n/a (no DML VRAM API used) |
| `py310_dml` (3.10.11 / 2.4.1+cpu) | CPU | 0.0224 | 5.8329 | 4.6035 | 1.0590 | n/a |

Every backend completed with no op-level exceptions; raw per-iteration times are in `results/baseline_raw.json`.
Startup is "backend init + first synced 1-element tensor" in an already-imported torch process, not full process/import time.
CPU columns are not directly comparable across runs because torch versions differ (see table).

## 5. Cross-process reference check

`results/refs/cpu_reference.pt` was written once by `venv312` (Python 3.12.10, torch 2.15.0+cu132) with 6
inputs and 4 CPU expected outputs (`add`, `mm`, `addmm`, `mlp`). Each backend loaded those same inputs,
computed outputs on its device, copied them to CPU, and saved `results/refs/<run>_outputs.pt`.
`check_parity.py` ran under `venv312` with `atol=1e-5`, `rtol=1e-4`. Measured max absolute differences:

| Run | add | mm | addmm | mlp | `torch.allclose` |
| --- | ---: | ---: | ---: | ---: | --- |
| `py312_cuda` | 0.0 | 0.0 | 0.0 | 0.0 | true for all 4 ops |
| `py312_cpu` | 0.0 | 0.0 | 0.0 | 0.0 | true for all 4 ops |
| `py310_dml` | 0.0 | 1.1921e-07 | 1.1921e-07 | 1.6764e-07 | true for all 4 ops |

Full JSON: `results/refs/parity_report.json`. These are measured tiny-tensor results only; they do not
establish parity for larger shapes, other dtypes, or other operators.

## 6. Errors and workarounds

1. `pip install --dry-run --index-url .../cu130 torch==2.14.1` failed: the index's `typing_extensions` sdist had
   inconsistent metadata (`expected 'typing-extensions', metadata has 'typing_extensions'`), and the build then
   could not find `flit_core` on the PyTorch-only index. Fixed with `--extra-index-url https://pypi.org/simple`;
   the local `2.14.1+cu130` / `2.15.0+cu132` wheels still won because local versions sort higher.
2. `pip index versions torch-directml` returned `No matching distribution found`: all releases are dev
   prereleases and `pip index versions` hides prereleases. `pip download/install torch-directml` works; the
   PyPI JSON API shows latest `0.2.5.dev240914` pinning `torch==2.4.1`.
3. Running Python with cwd at the repo root imports the repo's `torch/` source instead of the wheel
   (`ModuleNotFoundError: No module named 'torch.version'`). All verification/benchmark commands used cwd =
   `agent_space\baseline`.
4. First `reference_tensors.py` run failed with `RuntimeError: The expanded size of the tensor (4) must match
   the existing size (2)` because the `addmm` bias was shape `(2,)`; corrected to `(4,)`. No numbers from the
   failed run were retained.
5. Python 3.10 cannot install torch 2.15.0 (`requires-python >=3.11`). Concrete check:
   `& 'C:\Program Files\Python310\python.exe' -m pip install --dry-run --no-cache-dir 'torch==2.15.0'`
   returned `ERROR: Could not find a version that satisfies the requirement torch==2.15.0 (from versions: ... 2.14.1)`
   and `ERROR: No matching distribution found for torch==2.15.0`. The 3.10 venv therefore holds 2.14.1+cu130
   as a fallback and is reported separately.

## 7. Files produced

```
agent_space/baseline/
  venv/                      Python 3.10 baseline (torch 2.14.1+cu130)
  venv312/                   Python 3.12 primary (torch 2.15.0+cu132)
  venv_dml/                  Python 3.10 + torch-directml (torch 2.4.1+cpu)
  benchmark.py               dataclass-configured benchmarks (no argparse)
  run_benchmarks.py          runs all three venvs, merges results
  reference_tensors.py       deterministic tiny tensors + per-device outputs
  check_parity.py            cross-process comparison
  results/baseline_raw.json  raw merged results (times, env, errors)
  results/_part_*.json       per-run raw parts
  results/refs/*.pt          reference and device output tensors (torch.save)
  results/refs/parity_report.json
```

Disk at end: `E:` 19.9 GB free, `C:` 29.6 GB free; venvs are 3.11 / 2.98 / 1.25 GiB (`venv312` / `venv` / `venv_dml`).
