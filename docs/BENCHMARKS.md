# BENCHMARKS

**No Kaminari numbers exist yet.** This file will not claim parity or speed until a
result is committed under `benchmarks/results/`. Baseline collection is in progress
(issue #6); fork numbers start in Phase 1.

## Methodology (fixed before measurements)

- Warmup, then median p50 and p95 of N iterations with explicit device sync.
- Same inputs across backends; serialized reference tensors for cross-process comparison.
- GPU timestamps where available plus wall-clock; record clocks/thermals when readable.
- Peak VRAM via allocator stats; startup time includes shader/pipeline creation.
- Peak bandwidth and FLOPS are measured with microbenchmarks, not spec sheets.

## Targets

- Bandwidth-bound and launch-bound work: aim to match CUDA.
- GEMM/conv: report the real gap; cuBLAS/cuDNN hand-tune at SASS level and TU116 has no
  Tensor Cores, so equality is unlikely and will not be faked.
- No unexplained perf regression >3% vs the last recorded number may merge.

## Raw data

`benchmarks/results/*.json` (commit hash, driver version, date, methodology in each file).

## Placeholder table

| Backend | Startup (ms) | add p50 (us) | mm p50 (us) | MLP step (ms) | Peak VRAM | Commit | Date |
|---|---|---|---|---|---|---|---|
| CUDA (official wheel) | not measured | not measured | not measured | not measured | not measured | - | - |
| torch-directml | not measured | not measured | not measured | not measured | not measured | - | - |
| CPU | not measured | not measured | not measured | not measured | - | - | - |
| Kaminari (fork) | not built | not built | not built | not built | not built | - | - |
