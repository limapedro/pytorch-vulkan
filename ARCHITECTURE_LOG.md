# ARCHITECTURE_LOG

Every locked decision: what, why, evidence, date. A decision is locked only after
measurement or source evidence; "because it seems better" is not evidence. If a later
experiment contradicts one, amend the entry with a new dated line rather than silently
changing it.

| ID | Date | Decision | Why | Evidence | Status |
|---|---|---|---|---|---|
| D001 | 2026-11-25 | Branch from tag `v2.15.0`, not main. | Latest stable release (published 2026-10-28). | `gh api repos/pytorch/pytorch/releases`: v2.15.0 latest non-prerelease; `git log -1` = `31a77fc`. | locked |
| D002 | 2026-11-25 | Remote name `limapedro/pytorch-vulkan`, default branch `stable/v2.15.0`. | User corrected the repo name twice; default branch must equal the stable base so all PRs target it without force-pushing. | `gh repo view`, `git push -u origin stable/v2.15.0`. | locked |
| D003 | 2026-11-25 | Use `agent_space/` for all scratch files and generated dumps. | Repo `CLAUDE.md` requires it and the directory is git-ignored. | `CLAUDE.md` lines 21-23. | locked |
| D004 | 2026-11-25 | Follow the mission's explicit autonomous-GitHub authorization over the inherited `CLAUDE.md` AI policy; disclose AI authorship in GitHub bodies and commit messages. | Direct user instruction is more specific and later; conflict is recorded rather than hidden. | Mission text vs `CLAUDE.md` lines 5-19, 121-125. | locked (review if user changes instruction) |
| D005 | 2026-11-25 | No build-system or backend code before Phase 0 exit criteria are met. | Mission hard constraint. | Mission phases section. | locked |

| D006 | 2026-11-25 | Use an **out-of-tree PrivateUse1 extension** with ordinary strided tensors and a real `c10::Allocator`. Do not extend the in-tree `aten/src/ATen/native/vulkan` backend and do not add codegen dispatch entries. | The in-tree backend uses texture storage, faked strides/contiguity, materialized views, no autograd, one device and no events; it cannot be evolved into a general compute backend without rewriting its tensor representation. PrivateUse1 already has all needed core hooks, including the autograd fallback. | `aten/src/ATen/native/vulkan/VulkanOpaqueTensorImpl.h:6-9,28-40`; `ops/Factory.cpp:49-58`; `api/Tensor.h:102-105`; `VulkanGuardImpl.cpp:19-68`; `c10/core/DispatchKey.h:36-52`; `aten/src/ATen/core/VariableFallbackKernel.cpp:111-113`; `test/cpp_extensions/open_registration_extension/torch_openreg/` reference; `docs/source/accelerator/operators.md:15-31`; full report `agent_space/research_pytorch.md`. | locked |
| D007 | 2026-11-25 | Core PyTorch files stay untouched; backend and its build live in a self-contained `kaminari/` tree, registered at runtime. | `native_functions.yaml` has only the `is_vulkan_available()` schema for Vulkan and `torchgen/model.py:296-329` does not accept PrivateUse1/Vulkan as YAML dispatch keys, so codegen would require core edits. Runtime registration avoids them. | `aten/src/ATen/native/native_functions.yaml:4489`; `torchgen/model.py:296-329`; `git diff v2.15.0 -- ...` is empty (HEAD equals tag, so this is upstream v2.15.0 behavior, not something our fork changed). | locked |
| D008 | 2026-11-25 | Fork builds must use Python 3.12.10; system Python 3.10 is insufficient. | `pyproject.toml:350` declares `requires-python >=3.11`. | `python --version`; python.org installer with SHA-free size 26,964,224 B. | locked |

| D009 | 2026-11-25 | Keep separate baseline/fork venvs and compare via serialized reference tensors. | Measured: Python 3.10 cannot install torch 2.15.0; directml pins torch 2.4.1; 2.15.0+cu132 works on driver 621.01; cross-process references compare within 1.68e-7 for tiny tensors. Coexistence in one process was **not** directly tested because the fork has not built yet; this decision will be revisited if it becomes possible. | `agent_space/baseline/REPORT.md`, `benchmarks/kaminari/results/2026-11-25_parity_report.json`, exp_004. | locked after measurement, provisional on H1 |
| D010 | 2026-11-25 | Use the official `cu132` wheel (`torch 2.15.0+cu132`) as the version-matched CUDA baseline, not PyPI's CPU-only 2.15.0 wheel. | PyPI Windows wheel has no CUDA dependencies; the official index carries the CUDA build and it runs on this driver. | `pip install --dry-run torch==2.15.0` (126.7 MB, CPU) vs `cu132` index (2108.3 MB, `cuda_available=True`). | locked |

| D011 | 2026-11-25 | Keep `MAX_JOBS=2` for source builds on this host. | With 4 jobs, four large `torch/csrc/jit/passes/*.cpp` objects failed together with no diagnostic; with 2 jobs the build completed. | exp_002 attempts 4 and 5; `agent_space/build_fork.ps1`. | locked |

| D012 | 2026-11-25 | Validate the out-of-tree PrivateUse1 route before writing kernels. | Device string, module functions, guard, hooks and autoload all work with an empty core diff; the extension builds standalone against the built Torch. | exp_005; `kaminari/CMakeLists.txt`, `kaminari/setup.py`, `kaminari/csrc/`. | locked |

| D013 | 2026-11-25 | Query all Vulkan limits/features at runtime and enable BDA, timeline semaphores, 16-bit storage, fp16/int8 and descriptor indexing at device creation; do not bake in TU116 values. | Runtime query matched the independent `vulkaninfo` dump; other GPUs need different subgroup sizes and feature sets. | exp_006 raw JSON; `kaminari/csrc/Runtime.cpp`. | locked |

## Pending decisions

- Two-venv comparison strategy is locked as D009 but marked provisional on H1 (coexistence in one process was not directly tested).
- ccache/sccache installation (H3 in exp_002): not needed for a green build; revisit if incremental rebuild times justify it.
- Licensing: PyTorch v2.15.0's `pyproject.toml:356` uses a composite SPDX expression including BSD-3-Clause and MIT, not a bare BSD-3. The mission's simplification is noted; preserve all upstream license files.
