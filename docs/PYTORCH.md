# PYTORCH

How PyTorch works for Kaminari. Every path is from this fork at `stable/v2.15.0`
(identical to upstream tag `v2.15.0`, commit `31a77fc`). Line numbers are from that
checkout. Nothing here is generic folklore: verify against the tree after a rebase.

## 1. Dispatcher and dispatch keys

`c10/core/DispatchKey.h:13-30` describes the bit layout: low bits identify a backend,
upper bits layer functionality. `C10_FORALL_BACKEND_COMPONENTS` (`:36-52`) includes
`PrivateUse1`, `PrivateUse2`, `PrivateUse3`, then `Meta`. The header warns (`:26-34`)
to register a backend before Meta and to provide an `Autograd<Backend>` fallthrough.
`DispatchKeySet.h:768-769,856-857,877-878` defines the private-use autograd and
autocast sets. No source change is needed to obtain those keys.

A call like `torch.add(a, b)` goes through the dispatcher, which chooses a kernel by
the *highest-priority* key in the tensor's `DispatchKeySet`. Backend keys are
registered at runtime with `TORCH_LIBRARY_IMPL(aten, PrivateUse1, m)`; autograd for
private-use devices is already wired by
`aten/src/ATen/core/VariableFallbackKernel.cpp:111-113`:

```cpp
TORCH_LIBRARY_IMPL(_, AutogradPrivateUse1, m) { m.fallback(AUTOGRAD_FALLBACK); }
```

`__torch_dispatch__` is a tensor-subclass/mode hook
(`torch/csrc/autograd/python_variable.cpp`, `torch/csrc/PyInterpreter.cpp`), not the
backend registration API. We use `TORCH_LIBRARY_IMPL` for the C++ backend.

## 2. TensorImpl, StorageImpl, Allocator, DeviceGuard

- `c10/core/TensorImpl.h`: strides `:813-834`, contiguity `:871-886` and the
  overridable `is_contiguous_custom` `:1032`, storage access `:1089-1116`,
  allocator-driven resize `:2350-2382`.
- `c10/core/StorageImpl.h:391-410`: `DataPtr`, `SymInt size_bytes_`, `resizable_`,
  `Allocator*`. `SetStorageImplCreate` can override storage creation per device;
  the pattern is in `test/cpp_extensions/open_registration_extension.cpp:31-52`.
- `c10/core/Allocator.h:275-302`: `SetAllocator`, `GetAllocator`, `REGISTER_ALLOCATOR`.
  The CUDA caching allocator in this version is `c10/cuda/CUDACachingAllocator.cpp`
  (`NativeCachingAllocator` at `:4686`, `allocate` at `:5128-5154`); the older
  `aten/src/ATen/cuda/CachingAllocator.cu` path does not exist here.
- `c10/core/impl/DeviceGuardImplInterface.h:399` defines `C10_REGISTER_GUARD_IMPL`.
  The reference implementation is
  `test/cpp_extensions/open_registration_extension/torch_openreg/csrc/runtime/OpenRegGuard.cpp:6`.

A plain `TensorImpl` + `StorageImpl` whose `DataPtr` owns a Vulkan buffer gives us
honest strides, views and serialization. The in-tree Vulkan backend deliberately does
not do this (section 9).

## 3. Codegen, native_functions.yaml, structured kernels, TensorIterator

`aten/src/ATen/native/README.md:274-309` documents the `dispatch:` format; `:291-306`
says the default is `CompositeImplicitAutograd`. `torchgen/model.py:296-329` lists the
valid YAML `dispatch_keys` — **PrivateUse1 and Vulkan are not in it**. Therefore adding
a dispatch entry for our backend to `native_functions.yaml` would require editing
`torchgen/model.py` and codegen templates: a core touchpoint we avoid.

The zero-codegen route is runtime registration. `torchgen/gen_backend_stubs.py` can
generate a backend YAML into `<Backend>NativeFunctions.h` and `Register<Key>.cpp`
(`:482-506,619-631`), but no in-tree YAML uses PrivateUse1, so treat that path as
unverified until we try it.

Structured kernels (`torchgen/api/structured.py`, `torchgen/api/unboxing.py`) generate
Meta kernels automatically for ops declared with structured signatures
(`torchgen/model.py:320-322`). `TensorIterator` (`aten/src/ATen/native/TensorIterator.cpp`)
is the CPU/CUDA elementwise engine. The in-tree Vulkan backend does not use it; we will
not either at first. `REGISTER_PRIVATEUSE1_DISPATCH` exists in
`aten/src/ATen/native/DispatchStub.h:477-478`, but only a limited op set uses
`DECLARE_DISPATCH` stubs (`docs/source/accelerator/operators.md:174-186`).

## 4. CompositeImplicitAutograd vs backend-native

`aten/src/ATen/native/README.md:315-346`:

- `CompositeImplicitAutograd`: valid for all backends, implicitly differentiable
  because it calls differentiable ops. If we override it on PrivateUse1 we must keep a
  CompositeImplicitAutograd entry for other backends (`:377-380`).
- `CompositeExplicitAutograd`: inference kernel for all backends needing an explicit
  `derivatives.yaml` backward; registering is equivalent to registering on every
  backend (`:321-324`).

Examples: `linear` (`native_functions.yaml:3339`), `matmul` (`:3785`),
`broadcast_to` (`:1399`), `layer_norm` (`:3277`), convolutions (`:1716-1759`). Their
bodies reduce to smaller primitives: `aten/src/ATen/native/Linear.cpp:88`,
`LinearAlgebra.cpp:2046-2058,2228`, `torch/_decomp/decompositions.py:295,1302,1369`.
This is why a small primitive set plus PyTorch decompositions can cover many layers.

## 5. Autograd

`tools/autograd/derivatives.yaml` (not `aten/src/ATen/`) defines gradient formulas;
its header is `:1-38`, in-place defaults at `:54-57`, `add.Tensor` example at
`:229-236`. It contains no Vulkan/PrivateUse1 entries: generic formulas are inherited
through the `AutogradPrivateUse1` fallback. Custom backward kernels can be registered
on `AutogradPrivateUse1` from the extension; editing `derivatives.yaml` is a core
touchpoint to avoid. `output_differentiability` and `non_differentiable` matter for
index-like ops.

## 6. Views, strides, memory formats, in-place

With ordinary `StorageImpl` + `DataPtr`, zero-copy views come from generic
implementations. The reference backend implements the minimum honest set in
`test/cpp_extensions/open_registration_extension/torch_openreg/csrc/aten/native/Minimal.cpp`:
`empty.memory_format` (`:8-29`), `empty_strided` (`:32-53`, strides honored),
`as_strided` (`:55-63`), `resize_` (`:65-71`), `_reshape_alias` (`:73-79`),
`_copy_from` (`:81-129`), `_local_scalar_dense` (`:138-141`), `view` (`:160-163`).
The official required-op list is `docs/source/accelerator/operators.md:15-31` (12 rows
plus fallback). In-place semantics come from dispatcher alias annotations, not from
faked metadata.

## 7. RNG, streams/events, autocast/GradScaler, optimizers

- RNG: `PrivateUse1HooksInterface::getDefaultGenerator` / `getNewGenerator` in
  `aten/src/ATen/detail/PrivateUse1HooksInterface.h:33-45`; reference
  `torch_openreg/csrc/runtime/OpenRegGenerator.cpp:8-28`; Python `manual_seed` wiring
  via `torch.utils.generate_methods_for_privateuse1_backend`.
- Streams/events: implement `DeviceGuardImplInterface`; reference
  `torch_openreg/csrc/runtime/OpenRegStream.h/.cpp` and `OpenRegEvent.h`. The in-tree
  Vulkan guard rejects events (`VulkanGuardImpl.cpp:49-68`).
- Autocast: key `AutocastPrivateUse1` (`DispatchKey.h:345`; `DispatchKeySet.h:664,683`);
  reference registration `torch_openreg/csrc/amp/autocast_mode.cpp:19-36`. Python needs
  `torch.<backend>.get_amp_supported_dtype()` (`torch/amp/autocast_mode.py:250-268`).
  GradScaler is device-generic; no separate hook found.
- Optimizers: `torch/optim/adam.py:676-803` uses `torch._foreach_*`; `:803,877-878`
  uses `_fused_adam(w)_`. Selection at `:934-962`. Fused kernels are effectively CUDA
  only; without `_foreach_*` registrations these paths fall back to per-tensor ops (or
  error) unless a global CPU fallback is installed, as `torch_openreg` does.

## 8. torch.save/load and map_location

`torch/serialization.py:602-651` `_validate_device` requires a registered device module
with `_utils._get_device_index`, `device_count` and `is_available` for private-use
backends; otherwise `torch.load` demands `map_location="cpu"` (`:618-624`). Tag 23 is
registered for privateuse1 at `:680-684`, and `:662-669` maps the renamed device via
`torch._C._get_privateuse1_backend_name()`. A normal `StorageImpl` is required so
storage tagging works.

## 9. Out-of-tree registration and the in-tree Vulkan backend

Registration surface for our backend: `c10::register_privateuse1_backend(name)`
(`c10/core/DeviceType.cpp:140-159`), `torch.utils.rename_privateuse1_backend`
(`torch/utils/backend_registration.py:76`), `torch._register_device_module`
(`torch/__init__.py:3347-3364`), `generate_methods_for_privateuse1_backend(...)`
(`torch/utils/backend_registration.py:416`), `PrivateUse1HooksInterface`,
`REGISTER_ALLOCATOR`, `C10_REGISTER_GUARD_IMPL`, and `TORCH_LIBRARY_IMPL`. Reference:
`test/cpp_extensions/open_registration_extension/torch_openreg/`.

The in-tree Vulkan backend (`aten/src/ATen/native/vulkan/`) is instructive but not
reusable as-is:

| Aspect | Evidence | Consequence |
|---|---|---|
| Global `api::Context`, one queue, one device | `api/Context.h:31-38,53-77` | no multi-GPU or async streams |
| VMA allocator, not `c10::Allocator` | `api/Allocator.h:4-6,44-45` | no PyTorch storage semantics |
| Texture storage (`TEXTURE_3D`, channels-packed) | `api/Tensor.h:102-105`, `ops/Convert.h:19-43` | not a plain strided buffer |
| Faked strides / `is_contiguous()==true` | `VulkanOpaqueTensorImpl.h:6-9,28-40` | views are not honest |
| `empty_strided` ignores requested strides | `ops/Factory.cpp:49-58` | breaks strided kernels |
| Views materialized (`expand`->`repeat`, `slice`/`permute` copy) | `ops/Expand.cpp:67`, `Slice.cpp:12-79`, `Permute.cpp:101-112` | no zero-copy views |
| No autograd, no events, narrow dtypes | `VulkanGuardImpl.cpp:49-68`, `api/Types.h:14-29` | inference/mobile only |
| Vendor shader dispatch + prepack/quantized contexts | `api/ShaderRegistry.h:23-66`, `ops/Register.cpp:99-368` | mobile deployment oriented |

**Decision (D006/D007 in `ARCHITECTURE_LOG.md`):** fresh PrivateUse1 extension, plain
strided buffers, runtime registration, zero core edits.

## 10. Windows build quirks

- PyTorch 2.15 builds with scikit-build-core via PEP 517 (`setup.py:1-12`); repo policy
  mandates `pip install -e . -v --no-build-isolation`.
- `pyproject.toml:60-79` forces the Ninja generator on win32 and pins `CC`/`CXX=cl`;
  CMake 4.3.3 supports `Visual Studio 18 2026` and `Ninja`.
- `requires-python = ">=3.11"` (`pyproject.toml:350`); Python 3.12.10 was installed.
- MSVC flags: `/Zc:__cplusplus` (`CMakeLists.txt:765`),
  `/D_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR` (`:777-778`), C++20 (`:155-168`).
- `USE_CUDA=0` via `cmake/EnvVarForwarding.cmake:1-7`; `USE_VULKAN` options at
  `CMakeLists.txt:448-459,1236-1248`; `glslc` located via `VULKAN_SDK` in
  `cmake/VulkanCodegen.cmake:19-37`.
- Blobless clones must initialize the 36 submodules before build, otherwise
  `project.license-files` patterns in `pyproject.toml` fail metadata generation.

## 11. Licensing note

`pyproject.toml:352-356` uses a composite SPDX expression (BSD-3-Clause among others,
MIT third-party components). The mission's "PyTorch is BSD-3" shorthand is not the
whole story; retain every upstream `LICENSE` file and add attribution when borrowing
tinygrad ideas (see `docs/OPS.md`).
