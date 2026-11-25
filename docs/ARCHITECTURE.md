# ARCHITECTURE

Layer map for the backend. Everything described here is **design**, not implemented
code, until Phase 1 lands; the implementation decision (extend in-tree Vulkan vs
PrivateUse1) is still being locked in `ARCHITECTURE_LOG.md`.

| Layer | Responsibility | Planned location (self-contained tree) | Upstream touchpoints |
|---|---|---|---|
| Runtime / device | Instance, physical device selection, queue families, command buffers, timeline semaphores, lazy flush at sync points | `torch_vulkan/runtime/` | device registration, Python module |
| Memory | Vulkan allocator, size-class pools, block split/coalesce, staging, memory stats | `torch_vulkan/memory/` | allocator registration hook |
| Kernel library | SPIR-V sources, specialization constants, pipeline cache, per-vendor tuning tables | `torch_vulkan/kernels/`, `torch_vulkan/data/` | shader compile build hook |
| Op dispatch | ATen op implementations, structured kernels, decompositions to Tier 0, counted CPU fallback | `torch_vulkan/ops/` | dispatch entries in native_functions.yaml, codegen |

## Invariants

1. All backend code lives under the self-contained backend tree; core touchpoints are
   listed in `docs/UPSTREAM.md`.
2. Tensor storage is plain strided device buffers, not textures.
3. Kernels query subgroup size and limits at runtime; tuning tables are data files.
4. Ops without a Vulkan kernel fall back to CPU with a loud, counted warning.
5. No Python API, autograd or other-backend semantic changes.

## Related docs

`docs/PYTORCH.md` (how PyTorch dispatches), `docs/VULKAN.md` (what Vulkan offers),
`docs/MEMORY.md` (allocator), `docs/KERNELS.md` (kernel catalog),
`docs/PARITY.md` (status source of truth).
