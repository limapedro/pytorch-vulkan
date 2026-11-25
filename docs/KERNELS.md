# KERNELS

Kernel catalog. Empty until Phase 1; every entry must list tuning parameters and a
committed roofline result (ratio to measured peak, not spec sheet).

| Kernel | Op(s) | Strategy | Tuning parameters | Dtypes | Roofline result | PR |
|---|---|---|---|---|---|---|
| fill/zero | `fill_.Scalar`, `zero_` | `vkCmdFillBuffer` with float32 bit pattern | buffer offset alignment, fill width | fp32, int32 | not measured | - |
| add | `add.out`, `add.Tensor` | GLSL compute shader, BDA buffer refs in push constants, empty descriptor layout, 64-thread workgroup | workgroup size (64), alpha from push constants | fp32 | exact parity, no timing yet | - |

## Design constraints

- Subgroup-size agnostic; query `subgroupSize` at runtime (32 on TU116, varies elsewhere).
- Use specialization constants for tile sizes/dtypes to avoid shader permutations.
- 48 KiB shared memory and 1024 invocations/workgroup on TU116; tune per device.
- Split long dispatches to stay below the Windows TDR threshold (to be measured).
- On-disk pipeline cache to reduce startup; measure pipeline creation time separately.
