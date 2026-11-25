# DEVICES

GPU compatibility matrix and results. A row is only "supported" after OpInfo parity and
a benchmark result for that GPU are committed.

| GPU | Vendor | Driver | Vulkan API | subgroupSize | Status | Parity | Perf vs CUDA | Contributor / date |
|---|---|---|---|---|---|---|---|---|
| GTX 1660 SUPER 6 GB (TU116) | NVIDIA 0x10de | 621.01 | 1.4.357 | 32 | dev card, recon only | not measured | not measured | 2026-11-25 |
| AMD (TBD) | - | - | - | - | untested | - | - | - |
| Intel (TBD) | - | - | - | - | untested | - | - | - |
| Mesa lavapipe | Mesa | - | - | query via probe | CI target (planned Phase 6) | - | - | - |

Full extension/limit matrix for the dev card is in `docs/VULKAN.md`. The standalone
probe script (Phase 6) will let other users add rows here by sending back its JSON.
