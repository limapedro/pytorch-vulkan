# exp_008 - Vulkan add kernel

Date: 2026-11-25

## H1

A single GLSL compute shader using BDA buffer references and push constants can implement
`add.out` (including scalar alpha) on Vulkan with no per-op descriptor allocation, and
its output matches CPU reference exactly for fp32 contiguous tensors.

**Falsifier:** shader compilation, pipeline creation or push constants fail, or output
differs from CPU reference.

## H2

The kernel can replace the CPU fallback for `add`, reducing the counted fallback count
to zero for a simple two-tensor add, without changing core PyTorch.

**Falsifier:** dispatcher still selects the fallback, or a core file must change.

## Measurement plan

- Compile `shaders/add.comp` with glslc to SPIR-V at extension build time.
- Register `add.out` and `add.Tensor` on PrivateUse1.
- Compare `a + b` and `a + 2*b` against CPU tensors with `assertEqual`.
- Record fallback delta before/after; no latency claims without timed runs.
