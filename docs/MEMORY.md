# MEMORY

Allocator design and fragmentation stats. Design only until Phase 1; no allocator exists
yet, so there are no fragmentation numbers.

## Planned design

- Caching allocator in the spirit of the CUDA one: size-class pools, block splitting
  and coalescing, a few large `vkAllocateMemory` calls.
- Device-local heap 0 (5.83 GiB, budget 5.08 GiB) for tensors; host heap 1 for staging;
  small device-local heap 2 noted but not assumed usable for large tensors.
- Compatible APIs planned: `memory_allocated`, `memory_reserved`,
  `max_memory_allocated`, `empty_cache`, `memory_stats`, with actionable OOM errors.
- Track read/write hazards per buffer to place minimal barriers for lazy submission.

## Measurements to record here

- Fragmentation ratio after a model training loop.
- Number of `vkAllocateMemory` calls vs number of allocations.
- Peak reserved vs allocated VRAM; OOM behavior at the 6 GB limit.
