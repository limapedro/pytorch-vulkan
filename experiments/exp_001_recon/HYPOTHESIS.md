# exp_001 - Recon hypothesis

Date: 2026-11-25

## H1

The development machine has a usable Vulkan SDK, GLSL/SPIR-V toolchain, MSVC, CMake,
Ninja and a stable PyTorch release tag, so Phase 0 recon can be completed without
installing system software.

**Falsifier:** a required tool is absent and cannot be recovered from an already
installed Python package or SDK directory.

## H2

The fork can be hosted as `limapedro/pytorch-vulkan` with a default branch at the
latest stable tag, and the standard GitHub planning workflow (issues, milestones)
can be used on it.

**Falsifier:** the fork cannot be renamed, the default branch cannot be changed, or
issues stay disabled.

## H3 (deferred to the source study)

PrivateUse1 gives a smaller upstream touchpoint surface than extending the in-tree
Vulkan backend, because the in-tree backend is texture-based and inference-only.

**Falsifier:** source study shows the in-tree backend already provides the strided
buffer allocator and autograd hooks we need with fewer changes.

## Measurement plan

- `gh api repos/pytorch/pytorch/releases`, `gh repo view`, `git log -1`.
- `where.exe` / `Get-ChildItem` for each tool, plus `vulkaninfoSDK.exe --json -o`.
- Record every failure, not only successes.
