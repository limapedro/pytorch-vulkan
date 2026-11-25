# OPS

Primitive op set, lowering/fusion model, and the measured op census.

## 1. tinygrad as a design input (attribution)

Studied tinygrad at commit `ab98ebd2472c97a09c55c20ce0b5290c79228043` and tag `v0.10.0`
(MIT, Copyright (c) 2024 the tiny corp; full license in the research clone). Design
ideas only; **no tinygrad code is copied**. Credit: "design ideas informed by tinygrad
(MIT, (c) 2024 the tiny corp)". If code is ever copied, preserve the MIT notice in a
third-party notices file.

Reality check that prevented a wrong doc: current tinygrad has no `ops.py`,
`engine/lower.py`, `engine/lazy.py`, `renderer/vulkan.py` or `runtime/ops_vulkan.py`.
The op enum lives in `tinygrad/uop/__init__.py`, scheduling in `engine/realize.py`,
the linearizer in `codegen/late/linearizer.py`; **tinygrad has no Vulkan backend at all**.
Its useful ideas:

- Small primitive set: unary/binary/ternary ALU, indexed `LOAD`/`STORE`, `REDUCE`,
  movement ops (`RESHAPE`, `PERMUTE`, `EXPAND`, `PAD`, `SHRINK`), `CAST`/`BITCAST`.
- Movement ops become indices in the consumer kernel, not separate kernels; a materialized
  `STAGE`/`COPY` is explicit.
- Fusion rules from v0.10.0 `engine/schedule.py`: "can only fuse contiguous", "max one
  reduceop per kernel", "fuse double reduces with no other child".
- `UnsafePad = {RECIP, LOG2, EXP2, IDIV}` records ops that cannot be computed over
  padded lanes because f(0) != 0 - important for predicated Vulkan kernels.
- Linearizer is a toposort with placement priorities; renderer is parameterized by
  buffer addressing and work-item mapping.
- Vulkan-specific parts (descriptor sets, push constants, subgroup ops, SPIR-V) are our
  design, not tinygrad's.

## 2. Mapping onto ATen

| tinygrad primitive | ATen surface | Decomposition evidence in this tree |
|---|---|---|
| unary ALU | `exp`, `log`, `sqrt`, `neg`, `reciprocal`, `relu`, `sigmoid` | `aten/src/ATen/native/UnaryOps.cpp`; `torch/_decomp/decompositions.py:295` (silu) |
| binary ALU + broadcast | `add`, `sub`, `mul`, `div`, `pow`, `where` | `BinaryOps.cpp`, `TensorIterator.cpp`; `native_functions.yaml:1399` (`broadcast_to`) |
| ternary | `where`, `addcmul` | `torch/_decomp/decompositions.py` |
| movement | `view`, `reshape`, `permute`, `expand`, `pad`, `narrow`, `slice`, `t` | `aten/src/ATen/native/TensorShape.cpp`; repeat/stack at `:1846,3402` |
| reduce | `sum`, `mean`, `amax`, `amin`, `prod` | `aten/src/ATen/native/ReduceOps.cpp`; softmax decomp `torch/_decomp/decompositions.py:1302` |
| index/load/store | `index_select`, `gather`, `scatter_add`, `index_put`, advanced indexing | `TensorAdvancedIndexing.cpp:2113,2231`; embedding decomp `:1369` |
| copy/cast | `to`, `_to_copy`, `clone`, `copy_` | `Copy.cpp`, `TensorConvert.cpp` |

High-level ops that PyTorch already reduces to primitives
(`native_functions.yaml`): `linear` (:3339), `matmul` (:3785), `layer_norm` (:3277),
`rms_norm` (:3302), convolutions (:1716-1759), `broadcast_to` (:1399), `chunk` (:1485).
**Do not write a Vulkan kernel per ATen op**: register primitives and let
`CompositeImplicitAutograd`/decompositions lower the rest.

## 3. Measured census

Source: `experiments/exp_003_census/` (`op_census_raw.json`), torch 2.11.0+cu126 on CPU;
re-run on the 2.15.0 fork after the build. `TorchDispatchMode` counts dispatches;
profiler self time excludes instrumentation overhead for the fractions below.

| Model | Mode | Dispatches | Distinct | Tier 1 self-time share |
|---|---|---|---|---|
| MLP | train | 100 | 14 | 0.645 |
| small CNN | train | 86 | 21 | 0.876 |
| tiny GPT | train | 316 | 28 | 0.400 |
| ViT-tiny style | train | 328 | 32 | 0.576 |
| GPT-2 small style | infer | 352 | 16 | 0.403 |
| UNet style | train | 96 | 12 | 0.896 |

Most frequent distinct ops: `view` (262), `detach` (186), `t` (172), `transpose` (68),
`addmm` (62), `add` (56), `mm` (54), `clone` (40), `select` (38), `sum` (32),
`relu` (28), `_unsafe_view` (28), `native_layer_norm` (24).

## 4. Tiers

Statuses and parity live in `parity/manifest.yaml` -> `docs/PARITY.md`. The table below
is the engineering plan; no kernel exists yet.

### Tier 0 - primitive set (correctness first)

| Op family | ATen overloads | Dtypes | Forward / backward | Kernel strategy | Bound |
|---|---|---|---|---|---|
| elementwise unary | `relu`, `neg`, `abs`, `exp`, `log`, `sqrt`, `sigmoid`, `tanh` | fp32 initially | `y=f(x)` / `dx=grad*f'(x)` | vectorized strided kernel, specialization constants | memory-bound |
| elementwise binary/ternary | `add`, `sub`, `mul`, `div`, `pow`, `where` (+ Scalar, in-place, out) | fp32 | broadcast + `alpha` / standard grads | one kernel per signature, broadcast strides in push constants | memory-bound |
| reduction | `sum`, `mean`, `amax`, `amin` | fp32 | partial subgroups then workgroup then second pass | subgroup reductions, 2-stage for large dims | memory-bound |
| movement | `view`, `reshape`, `t`, `transpose`, `permute`, `squeeze`, `unsqueeze`, `select`, `slice`, `expand`, `_unsafe_view`, `detach` | all | metadata only | zero-copy `as_strided`/aliasing, no kernel | launch-bound |
| copy/cast | `copy_`, `_to_copy`, `clone`, `empty`, `empty_strided`, `zeros`, `ones` | fp32 | identity/cast | `vkCmdCopyBuffer` or conversion kernel | memory-bound |
| index | `index_select`, `gather`, `scatter_add`, `index_add`, `embedding`(decomp) | fp32 | indexed load/store + atomic/segmented add | indexed kernel, predication for OOB | memory-bound |

Roofline expectation: Tier 0 should approach measured memory bandwidth (to be
measured, not taken from spec sheets). Views should be near-zero cost.

### Tier 1 - hot ops

| Op | Why (census) | Strategy | Bound |
|---|---|---|---|
| `mm`, `bmm`, `addmm` | 62+54 dispatches; 0.4-0.65 self-time share | register-blocked, shared-memory tiled, vec4, subgroup-aware, autotuned; no tensor cores on TU116 | compute-bound; real gap vs cuBLAS expected |
| `convolution`, `convolution_backward`, `_convolution`, `mkldnn_convolution` | 0.876 CNN, 0.896 UNet | implicit GEMM; Winograd only if measured to win | compute-bound |
| `_scaled_dot_product_flash_attention_for_cpu` + backward | transformers | tiled attention with online softmax | compute+memory |
| `native_layer_norm` + backward | 24 dispatches, GPT-2 | fused two-pass reduction | memory-bound |
| `embedding`, `embedding_dense_backward` | gather-heavy | indexed copy + segmented atomic add | memory-bound |
| `_log_softmax`, `nll_loss_forward/backward`, `cross_entropy` | classification heads | fused max/sum/exp | memory-bound |
| optimizer steps (`_foreach_*`, SGD/Adam/AdamW) | training | batched foreach kernels; fused paths likely CUDA-only | memory-bound |

### Tier 2 - long tail

Everything else, with a **loud, counted CPU fallback** so models run end to end early.
Fallback count per model is tracked in `docs/PARITY.md`; goal is zero.

## 5. Fusion research hypothesis

Elementwise fusion is a RESEARCH experiment, not an assumption. Views, strides and
in-place ops make it risky; the plan is to test a tiny lazy/JIT layer only after Tier 0
is correct, and to keep it optional if measurements do not justify it. Record the
experiment in `experiments/exp_NNN/` with HYPOTHESIS/FINDINGS before implementation.
