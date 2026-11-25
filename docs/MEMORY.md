# MEMORY

Allocator design and fragmentation stats.

## v1 (implemented, exp_007)

- `REGISTER_ALLOCATOR(c10::DeviceType::PrivateUse1, ...)` installs a `c10::Allocator`.
  Each allocation is a Vulkan `VkBuffer` bound to its own device-local `VkDeviceMemory`;
  the `DataPtr` stores the buffer device address as the data pointer and an
  `Allocation*` as deleter context.
- A mutex-protected size-class pool rounds requests up to 4096 B and reuses freed
  buffers by class. This is intentionally simple: **no suballocation, splitting,
  coalescing or `empty_cache` yet**. One `vkAllocateMemory` per cached buffer, and freed
  buffers are retained for the life of the process.
- Host staging uses a separate host-visible/coherent buffer per copy, destroyed after
  the synchronous transfer.
- `copy_data` resolves device addresses back to allocations and issues a
  `vkCmdCopyBuffer` on the compute queue.
- Compatible APIs (`memory_allocated`, `empty_cache`, `memory_stats`) are **not** exposed
  yet; Phase 4 work.

## Measured behavior

Smoke tests (exp_007): H2D/D2H round-trip, `fill_`, `zero_`, `item()` and views all exact
on fp32 4-element tensors. 9/9 local tests pass. No allocator timings, peak-VRAM or
fragmentation numbers have been taken, so none are claimed.

## Planned v2 (Phase 4)

Few large `vkAllocateMemory` calls with block splitting and coalescing, stream-aware
reuse, fragmentation stats, OOM errors, and the CUDA-compatible memory API surface.
