# exp_007 - Allocator and first memory operations

Date: 2026-11-25

## H1

A `c10::Allocator` over Vulkan buffers with size-class reuse can back `empty` and
`empty_strided` on `kaminari`, with honest strides and a device address as `DataPtr`.

**Falsifier:** allocation, tensor creation or view ops fail, or core files must change.

## H2

H2D/D2H `copy_` through host-visible staging buffers and `fill_`/`zero_` via
`vkCmdFillBuffer` produce values matching CPU reference tensors, and `item()` works.

**Falsifier:** any measured value differs from CPU reference at exact equality for fp32
fill/copy, or command submission hangs/TDRs.

## Measurement plan

- Smoke: create tensors, copy from CPU, copy back, compare with `torch.equal`.
- Fill with 1.0 and 0.0, copy back, compare.
- `_local_scalar_dense` via `.item()`.
- Record fallback count before/after unregistered ops.
- No add kernel yet: `add` is expected to use the counted CPU fallback.
