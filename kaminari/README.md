# Kaminari backend

Self-contained PyTorch Vulkan backend. All code here is out-of-tree; no files under
`c10/`, `aten/`, `torch/` or `torchgen/` are modified.

## Layout

- `csrc/` - C++ runtime registration (pybind module, device guard, hooks).
- `torch_kaminari/` - Python package registered as the `torch.kaminari` device module.
- `tests/` - smoke tests using PyTorch's TestCase/run_tests.
- `CMakeLists.txt` - standalone build against an installed Torch.
- `setup.py` - editable install with the `torch.backends` autoload entry point.

## Status (Phase 1, issues #9/#10)

Device registration, Vulkan runtime, allocator v1 and first memory ops.
`torch.empty(..., device="kaminari")`, H2D/D2H `copy_`, `fill_`/`zero_`, `item()`,
`view`/`as_strided`, a Vulkan `add` shader and a counted CPU fallback for everything else
all work. `_C._device_info()` returns live GPU limits and `_C._synchronize_vulkan()`
waits for the compute queue. Validation is available with `KAMINARI_VALIDATE=1`.
11/11 local tests pass.
