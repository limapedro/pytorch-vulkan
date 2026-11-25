# FINDINGS

Index of experimental results. Raw numbers live under `experiments/exp_NNN/` and
`benchmarks/results/`; nothing here may be claimed without a committed measurement.

| Experiment | Result summary | Raw evidence |
|---|---|---|
| [exp_001](experiments/exp_001_recon/FINDINGS.md) | Toolchain and GPU probed; fork created; issues were disabled and had to be enabled. Several assumptions corrected. | `agent_space/vulkaninfo.txt`, `agent_space/vulkaninfo.json` |
| [exp_002](experiments/exp_002_build/FINDINGS.md) | Build blocked first by PowerShell stderr handling, then by uninitialized submodules. Fixes in progress. | `agent_space/build.log` |
| [exp_003](experiments/exp_003_census/FINDINGS.md) | 39 distinct ATen ops across 6 models; Tier 1 dominates conv/MLP self time but not transformer time. | `experiments/exp_003_census/results/op_census_raw.json` |
| [exp_004](experiments/exp_004_cross_process/FINDINGS.md) | Serialized tiny-tensor parity within 1.68e-7; two-venv decision locked provisionally. | `benchmarks/kaminari/results/2026-11-25_parity_report.json` |

Negative results are first-class outputs and are kept here permanently.
