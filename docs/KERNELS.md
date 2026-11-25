# KERNELS

Kernel catalog. Empty until Phase 1; every entry must list tuning parameters and a
committed roofline result (ratio to measured peak, not spec sheet).

| Kernel | Op(s) | Strategy | Tuning parameters | Dtypes | Roofline result | PR |
|---|---|---|---|---|---|---|
| (none yet) | - | - | - | - | - | - |

## Design constraints

- Subgroup-size agnostic; query `subgroupSize` at runtime (32 on TU116, varies elsewhere).
- Use specialization constants for tile sizes/dtypes to avoid shader permutations.
- 48 KiB shared memory and 1024 invocations/workgroup on TU116; tune per device.
- Split long dispatches to stay below the Windows TDR threshold (to be measured).
- On-disk pipeline cache to reduce startup; measure pipeline creation time separately.
