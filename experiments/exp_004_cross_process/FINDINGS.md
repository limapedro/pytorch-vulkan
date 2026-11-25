# exp_004 - Cross-process parity findings

Date: 2026-11-25. Raw data: `benchmarks/kaminari/results/2026-11-25_parity_report.json`
and the full baseline report at `benchmarks/kaminari/REPORT.md`.

## H1 - not directly falsified; decision locked provisionally

A single process containing both the CUDA wheel and the fork build was **not** tested:
the fork has not built yet. What *was* measured is that the required environments are
different: Python 3.10 cannot install torch 2.15.0, torch-directml pins torch 2.4.1,
and the fork requires Python >=3.11 with `USE_CUDA=0`. The two-venv strategy is locked
as D009 but marked provisional on H1; if a later build allows coexistence, revisit it.

A related gotcha was measured: running Python from the repo root imports the unbuilt
`torch/` source and fails with `ModuleNotFoundError: No module named 'torch.version'`.
All baseline commands therefore ran with cwd at `agent_space/baseline`.

## H2 - confirmed for tiny tensors

Reference tensors (6 inputs, 4 CPU expected outputs) were generated under
`venv312` (torch 2.15.0+cu132) and compared at `atol=1e-5`, `rtol=1e-4`:

| Run | add | mm | addmm | mlp | allclose |
|---|---:|---:|---:|---:|---|
| py312_cuda | 0.0 | 0.0 | 0.0 | 0.0 | true |
| py312_cpu | 0.0 | 0.0 | 0.0 | 0.0 | true |
| py310_dml | 0.0 | 1.1921e-07 | 1.1921e-07 | 1.6764e-07 | true |

Largest observed difference: 1.6764e-07, far below tolerance. This is a tiny-tensor
result only; larger shapes, other dtypes and other ops are not established.

## Consequence

The fork-side harness `compare.py` loads these serialized references and will compare
them to in-process CPU first, then to the Vulkan device once Phase 1 lands. No parity
claim about Kaminari is made here.
