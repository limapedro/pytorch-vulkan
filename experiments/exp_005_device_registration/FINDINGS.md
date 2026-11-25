# exp_005 - Device registration findings

Date: 2026-11-25. Result: **both hypotheses confirmed**.

## H1 - confirmed

An out-of-tree extension registers `kaminari` on `PrivateUse1` with **zero core edits**:
`git diff --stat stable/v2.15.0 -- c10 aten torch torchgen CMakeLists.txt setup.py` is
empty. After `import torch` alone (autoload via the `torch.backends` entry point),
`torch.device("kaminari:0")` parses, `torch._C._get_privateuse1_backend_name() ==
"kaminari"`, and `torch.kaminari.is_available()` / `device_count()` / `current_device()`
return `True / 1 / 0`. `torch.Tensor.kaminari` is generated.

## H2 - confirmed

The standalone extension builds against the built fork with CMake 4.3.3, MSVC 14.51,
Ninja, `find_package(Torch)` and pybind11 3.1.0. It registers:

- `C10_REGISTER_GUARD_IMPL(PrivateUse1, GuardImpl)` with one device and a default stream;
- `PrivateUse1HooksInterface` (isBuilt/isAvailable/hasPrimaryContext true, generator
  placeholder backed by a CPU generator);
- pybind `_C` helpers `_init`, `_get_device_count`, `_get_device`, `_set_device`,
  `_exchange_device`.

`kaminari/tests/test_smoke.py`: 3 tests, all passing.

## Negative results and fixes

1. `PYBIND11_MODULE(TORCH_EXTENSION_NAME, m)` failed with
   `dynamic module does not define module export function (PyInit__C)` until
   `TORCH_EXTENSION_NAME=_C` was added as a compile definition.
2. `Device::index()` returns `c10::DeviceIndex` in v2.15.0, not an optional; `.value_or(0)`
   failed to compile. Fixed to `device.index() == 0`.
3. Linking failed with unresolved `torch::utils::register_fork_handler_for_device_init`;
   fixed by linking `torch_python.lib` from `TORCH_INSTALL_PREFIX/lib`.
4. `expecttest` was missing in the fork venv; installed 0.3.0.
5. Adding `manual_seed_all` that called `torch.manual_seed` caused infinite recursion
   because `torch.manual_seed` dispatches to device modules. Replaced with a local seed
   placeholder (`torch.Generator().manual_seed`) until Phase 5 implements a real RNG.
6. The first package name collided with the outer `kaminari/` directory (namespace
   package had no `_autoload`), and CMake's multi-config generator nested the `.pyd`
   under `Release/`. Fixed by renaming the Python package to `torch_kaminari`, forcing
   the Ninja generator in `setup.py`, and using `DESTINATION .` for install.

## Expected absence

`torch.empty(2, device="kaminari")` raises `NotImplementedError ... 'kaminari' backend`
because no allocator or op kernels are registered yet. That is issue #11, not a failure
of this experiment.

## Artifacts

- `kaminari/` (CMakeLists.txt, setup.py, csrc/, torch_kaminari/, tests/).
- `agent_space/extension_build.log` (git-ignored).
