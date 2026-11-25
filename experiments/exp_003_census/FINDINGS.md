# exp_003 - ATen op census findings

Date: 2026-11-25. Raw data: `results/op_census_raw.json`.

## Method caveat

Executed with the system-installed `torch 2.11.0+cu126` on **CPU** (the fork has not
built yet), from outside the source tree so installed torch is not shadowed. Op counts
come from `TorchDispatchMode` (which adds PythonDispatchMode self time to the profiler);
self-time fractions below exclude `PythonDispatchMode` and `autograd` overhead. The fork
targets 2.15.0; re-run the census after the fork builds before treating op names as final.

## Per-model results

| Model | Mode | Dispatches | Distinct ops | Tier 0 / tier 1 / other (dispatch categories in script) | Tier 1 share of profiled self time, excl. instrumentation |
|---|---|---|---|---|---|
| MLP | train | 100 | 14 | 24 / 14 / 62 | 0.645 |
| small CNN | train | 86 | 21 | 22 / 16 / 48 | 0.876 |
| tiny GPT | train | 316 | 28 | 168 / 36 / 112 | 0.400 |
| ViT-tiny style | train | 328 | 32 | 170 / 36 / 122 | 0.576 |
| GPT-2 small style | infer | 352 | 16 | 272 / 20 / 60 | 0.403 |
| UNet style | train | 96 | 12 | 28 / 20 / 48 | 0.896 |

The script's initial tier classifier misparsed `aten.view.default` as `default`;
this was fixed by normalizing `aten::`/`aten.` prefixes before classification. The
first run also failed on a sequence-model loss shape and was fixed.

## Union of observed ops (39 distinct)

Highest counts: `view` 262, `detach` 186, `t` 172, `transpose` 68, `addmm` 62,
`add` 56, `mm` 54, `clone` 40, `select` 38, `sum` 32, `relu` 28, `_unsafe_view` 28,
`native_layer_norm` 24, `threshold_backward` 20, `convolution` 16,
`convolution_backward` 16. Full list is in the raw JSON (`all_counts`).

## H1 - narrowly confirmed

Counting movement/copy/elementwise/reduction primitives (view, detach, t, transpose,
add, clone, select, sum, relu, _unsafe_view, unsqueeze, squeeze, permute, expand,
ones_like, mean, div, slice, cat) gives 19 of 39 distinct ops (~49%), and a clear
majority of *dispatches* in the transformer models. The remaining ops are mostly
backward formulas of Tier 1 ops, so PyTorch decompositions can cover many of them.
Verdict: confirmed with caveat; the op set is small but not overwhelmingly Tier 0 by
distinct-name count. Must be re-run on 2.15.0.

## H2 - confirmed for conv models, not for transformers

Tier 1 self-time share excluding instrumentation: 0.645 (MLP), 0.876 (CNN), 0.576
(ViT), 0.896 (UNet), but only 0.400 (tiny GPT) and 0.403 (GPT-2 small inference).
Transformer time is spread across SDPA, layer norm, views and dispatch overhead.
Verdict: partially confirmed; hand-tuned GEMM and conv are priorities, but SDPA and
layer norm matter as much for transformer workloads.

## Decisions informed

- Tier 0 first: elementwise, movement-as-view, copy/cast, reductions, index.
- Tier 1 priorities by measured weight: mm/addmm, conv + conv_backward, then
  SDPA/layer_norm for transformers.
- `detach`, `t`, `_unsafe_view`, `select` must be zero-copy or the transformer models
  will allocate constantly.
