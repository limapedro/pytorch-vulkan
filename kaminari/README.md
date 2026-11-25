# Kaminari backend

Self-contained PyTorch Vulkan backend. All code here is out-of-tree; no files under
`c10/`, `aten/`, `torch/` or `torchgen/` are modified.

## Layout

- `csrc/` - C++ runtime registration (pybind module, device guard, hooks).
- `torch_kaminari/` - Python package registered as the `torch.kaminari` device module.
- `tests/` - smoke tests using PyTorch's TestCase/run_tests.
- `CMakeLists.txt` - standalone build against an installed Torch.
- `setup.py` - editable install with the `torch.backends` autoload entry point.

## Status (PR #13, issue #9)

Device registration only. `torch.device("kaminari:0")` parses, `torch.kaminari` works,
and `import torch` alone registers the backend after `pip install -e kaminari`. There
is no allocator or kernel yet, so `torch.empty(..., device="kaminari")` is expected to
raise `NotImplementedError`. Vulkan runtime and first ops follow in issues #10/#11.
