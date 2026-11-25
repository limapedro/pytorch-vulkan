# exp_003 - ATen op census on benchmark models

Date: 2026-11-25

## H1

A small Tier 0 primitive set (elementwise unary/binary/ternary, reductions, views,
copy/cast and index) covers the large majority of **distinct ATen ops** exercised by the
benchmark models, even though high-level layers (matmul, conv, softmax, norm) dominate
time. That would justify building Tier 0 first and letting PyTorch's decompositions
do the heavy lifting.

**Falsifier:** fewer than half of distinct dispatched ops fall into the Tier 0 primitive
categories, or the long tail includes ops that decompositions cannot reduce.

## H2

A handful of Tier 1 ops (mm/addmm/bmm, conv, softmax, layer_norm, embedding, SDPA,
cross-entropy, optimizer steps) account for most measured CPU self time on these
models, so hand-tuning them is where performance work should go.

**Falsifier:** Tier 1 ops account for less than half of profiled self time.

## Measurement plan

- Run MLP, small CNN, tiny GPT, ViT-tiny-style, GPT-2-small-style and UNet-style models
  on CPU with fixed seeds and small shapes (6 GB VRAM constraint is irrelevant on CPU but
  kept realistic).
- Count ops with `TorchDispatchMode`; measure self time with `torch.profiler`.
- Record forward and backward separately, dtypes and overload names.
- Note the torch version used; a mismatch with the fork base of 2.15.0 is a caveat,
  not a reason to discard the census.
