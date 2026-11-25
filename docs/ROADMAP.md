# ROADMAP

One milestone per phase (mirror of GitHub milestones 2-9). Exit criteria are hard gates:
a phase is not done until every criterion has evidence in a committed file.

| Phase | Milestone | Exit criteria | Current state |
|---|---|---|---|
| 0 - Recon and docs | #2 | Fork set up; host probed; no-CUDA fork builds on Windows; baseline venv works and baselines recorded; PYTORCH/OPS(with census)/VULKAN/BUILD written; integration approach locked with evidence. | recon done, build/baseline/docs in progress |
| 1 - Skeleton | #3 | Device registered; empty/copy H2D/D2H/fill/add/synchronize; allocator v1; first OpInfo parity green locally and on lavapipe CI. | not started |
| 2 - Tier 0 + fallback | #4 | Tier 0 coverage; counted loud CPU fallback; MLP inference end to end. | not started |
| 3 - Autograd/training | #5 | Forward+backward Tier 0/1, losses, SGD/Adam/AdamW incl. foreach/fused; MLP, CNN, tiny transformer loss curves match CPU/CUDA. | not started |
| 4 - Performance | #6 | Tier 1 kernels each with before/after experiment; lazy command buffers; allocator tuning. | not started |
| 5 - Dtypes/AMP | #7 | fp16, bf16 storage+conversion, autocast, GradScaler. | not started |
| 6 - Portability | #8 | Subgroup-agnostic kernels; autotune cache; per-vendor tables; lavapipe CI; standalone probe script. | not started |
| 7 - Hardening | #9 | Error handling/TDR safety; torch.save/load; streams/events; memory stats; rebase rehearsal. | not started |

Out of scope: `torch.compile`/Inductor, distributed, mobile, Vulkan graphics.
