# UPSTREAM

The only deliverable is the Vulkan backend. Core-PyTorch files must stay untouched;
every touchpoint (including "none") is listed here so rebasing is cheap.

Base: upstream tag `v2.15.0`. Fork default: `stable/v2.15.0`. Decision D007 in
`ARCHITECTURE_LOG.md`: runtime registration, no codegen or core edits.

## Core touchpoint inventory

| Touchpoint | Upstream file | Planned change | Status |
|---|---|---|---|
| Dispatch key / device registration | `c10/core/DispatchKey.h:36-52`, `c10/core/DeviceType.cpp:140-159` | none; use existing `PrivateUse1` and call `register_privateuse1_backend` from the extension | no core edit |
| Python device module | `torch/__init__.py:3347-3364`, `torch/utils/backend_registration.py:76,416` | none; extension calls `rename_privateuse1_backend`, `_register_device_module`, `generate_methods_for_privateuse1_backend` | no core edit |
| Op dispatch entries | `aten/src/ATen/native/native_functions.yaml` | none; extension registers `TORCH_LIBRARY_IMPL(aten, PrivateUse1, m)` | no core edit; verified that `torchgen/model.py:296-329` does not accept these keys |
| Autograd | `aten/src/ATen/core/VariableFallbackKernel.cpp:111-113` | none; existing `AutogradPrivateUse1` fallback is used | no core edit |
| Codegen | `torchgen/model.py`, `torchgen/gen.py` | none; `gen_backend_stubs.py` remains an unverified optional path | no core edit |
| Build-system hooks | `pyproject.toml`, `CMakeLists.txt`, `setup.py` | none; backend builds as a standalone CMake extension against installed Torch | no core edit (verify at Phase 1) |
| Python bindings | `torch/csrc/Module.cpp` | none; extension supplies its own pybind module | no core edit |
| Extension build hook | `kaminari/CMakeLists.txt`, `kaminari/setup.py` | standalone CMake/pybind11 build against installed Torch; `torch.backends` autoload entry point | self-contained tree |
| Python device module | `kaminari/torch_kaminari/` | registers `torch.kaminari`, generates tensor/storage methods | self-contained tree |
| Allocator and ATen ops | `kaminari/csrc/Allocator.cpp`, `VulkanMemory.cpp`, `Ops.cpp` | `REGISTER_ALLOCATOR` + `TORCH_LIBRARY_IMPL(aten/_, PrivateUse1, ...)` runtime registration | self-contained tree |

exp_007 confirmed the core diff remains empty after adding the allocator and first ops.

Validated by exp_005: `git diff --stat stable/v2.15.0 -- c10 aten torch torchgen CMakeLists.txt setup.py` is empty after adding the extension.

## Self-contained backend tree

All backend code, shaders, tuning data, tests and benchmarks for the backend live under
`kaminari/` (to be created in Phase 1) plus project-level docs/experiments. Nothing is
added to `aten/`, `c10/`, `torch/`, `torchgen/` or `CMakeLists.txt`.

## Rebase procedure

1. `git fetch upstream refs/tags/vX.Y.Z:refs/tags/vX.Y.Z`; branch from the tag.
2. Rebuild the fork and the extension against the new Torch headers.
3. Re-diff every file in the table above against `stable/v2.15.0`. Any change is a bug
   report, not a silent merge.
4. Run shader compile checks, OpInfo parity on lavapipe, and the local GPU benchmarks.
5. Update this table, `docs/PYTORCH.md` and `ARCHITECTURE_LOG.md` with the new base.

Never force-push `stable/v2.15.0`.
