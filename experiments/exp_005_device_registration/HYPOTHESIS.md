# exp_005 - PrivateUse1 device registration skeleton

Date: 2026-11-25

## H1

An out-of-tree extension can register a `kaminari` device on `PrivateUse1` with zero
core PyTorch edits, such that `torch.device("kaminari:0")` parses and
`torch.kaminari.is_available()` / `device_count()` work after `import kaminari`.

**Falsifier:** registration requires editing `c10/`, `aten/`, `torch/` or codegen, or
device-string parsing fails after the documented registration calls.

## H2

The reference extension pattern (`torch_openreg`) is sufficient on Windows with the
built fork: rename backend, register device module, generate methods, pybind `_C` module
with `_init`/device-count helpers, guard impl, and hooks interface.

**Falsifier:** a required header or symbol is unavailable in the v2.15.0 fork, or the
standalone CMake build cannot link against the built Torch.

## Measurement plan

- Build the extension with standalone CMake against the fork venv.
- From a directory outside the source tree, verify device parsing, module functions,
  guard registration, and that no core PyTorch file was modified (`git diff`).
- Do not claim op parity or performance: no Vulkan kernels exist yet.
