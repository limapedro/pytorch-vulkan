# BENCHMARKS

Current measured numbers on the dev machine. **No Kaminari numbers exist yet**: the
fork has not built. Every value below comes from a committed raw file; nothing is
estimated. New results must be added in the same PR as the code that produces them.

## Machine and method

- GPU: NVIDIA GeForce GTX 1660 SUPER (TU116, 6 GB), driver 621.01, Vulkan 1.4.357.
- CPU: i5-12400F, 32 GB RAM; Windows 11 (`10.0.26300`).
- Fork base commit: `1b6605b` (docs PR #8); upstream base `31a77fc` / v2.15.0.
- Method: seeded inputs generated once on CPU, 5 warmups, median of 20 iterations,
  float32. Explicit sync (CUDA `torch.cuda.synchronize`, DML `probe.cpu()` copy because
  torch-directml 0.2.5 has no `synchronize`, CPU no-op). CUDA peak VRAM via
  `reset_peak_memory_stats`/`max_memory_allocated` after sync. Startup is backend init
  plus one synced 1-element tensor *after* torch import, not full process startup.
- Shapes: `add` (4096,4096); `addmm` 1024x1024 @ 1024x1024; MLP 256->512->256->10 with
  ReLU and MSE backward (forward+backward, no optimizer step).
- Raw data: `benchmarks/kaminari/results/2026-11-25_baseline_raw.json` and
  `2026-11-25_parity_report.json`; full commands and negative results in
  `benchmarks/kaminari/REPORT.md`.

## Measured baselines (median ms)

| Environment | Backend | Startup (ms) | add (ms) | addmm (ms) | MLP fwd+bwd (ms) | Peak VRAM |
|---|---|---:|---:|---:|---:|---:|
| Python 3.12.10 / torch 2.15.0+cu132 | CUDA | 0.150 | 0.702 | 0.997 | 1.272 | 214.42 MiB |
| Python 3.12.10 / torch 2.15.0+cu132 | CPU | 0.015 | 6.972 | 6.385 | 1.590 | n/a |
| Python 3.10.11 / torch 2.14.1+cu130 | CUDA | 0.118 | 0.690 | 0.946 | 1.026 | 222.55 MiB |
| Python 3.10.11 / torch 2.14.1+cu130 | CPU | 0.023 | 6.458 | 5.819 | 1.217 | n/a |
| Python 3.10.11 / torch 2.4.1+cpu + DirectML 0.2.5.dev240914 | DML | 0.207 | 0.901 | 0.843 | 1.235 | not measured |
| Python 3.10.11 / torch 2.4.1+cpu | CPU | 0.022 | 5.833 | 4.604 | 1.059 | n/a |

CPU columns are not directly comparable across environments because torch versions
differ. DML numbers use a different torch version (2.4.1) and a different device
implementation (`privateuseone:0`); they are context, not a like-for-like target.

## Cross-process reference check

Reference tensors were generated once in the 2.15.0+cu132 venv and compared at
`atol=1e-5`, `rtol=1e-4` on four ops (`add`, `mm`, `addmm`, `mlp`).

| Run | add | mm | addmm | mlp | allclose |
|---|---:|---:|---:|---:|---|
| py312_cuda | 0.0 | 0.0 | 0.0 | 0.0 | true |
| py312_cpu | 0.0 | 0.0 | 0.0 | 0.0 | true |
| py310_dml | 0.0 | 1.1921e-07 | 1.1921e-07 | 1.6764e-07 | true |

This establishes serialized-reference parity for tiny tensors only, not for large
shapes, other dtypes or other operators.

## Kaminari

| Backend | Startup | add | mm | MLP step | Peak VRAM | Ratio vs CUDA | PR / commit |
|---|---|---|---|---|---|---|---|
| Kaminari (Vulkan) | not built | not built | not built | not built | not built | not measured | - |

## Targets and gates

- Bandwidth-bound and launch-bound work: aim to match CUDA on this GPU.
- GEMM/conv: report the real gap; cuBLAS/cuDNN are hand-tuned and TU116 has no Tensor
  Cores. Equality is not assumed and will not be faked.
- No unexplained performance regression >3% vs the last recorded number may merge.
- Peak bandwidth/FLOPS microbenchmarks are still to be measured; spec sheets are not
  used as denominators.
