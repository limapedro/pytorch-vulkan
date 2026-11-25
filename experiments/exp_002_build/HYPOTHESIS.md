# exp_002 - No-CUDA fork build on Windows

Date: 2026-11-25

## H1

PyTorch v2.15.0 can be built from source on this Windows host with CUDA disabled, using
Python 3.12.10 (the source requires >=3.11), MSVC 14.42/14.51, CMake 4.3.3 and Ninja,
via the repo-mandated `pip install -e . -v --no-build-isolation`.

**Falsifier:** the command fails with an unresolved compiler, generator, dependency or
codegen error that cannot be fixed without changing unrelated PyTorch code.

## H2

Setting `USE_CUDA=0`, `USE_DISTRIBUTED=0`, `BUILD_TESTING=0`, `USE_CUDNN=0` in the
environment is sufficient to keep the build lean and avoid the CUDA toolchain.

**Falsifier:** CMake still probes or requires CUDA, or an unrelated third-party library
dominates build time.

## H3

The build needs ccache/sccache to be practical for repeated iterations.

**Falsifier:** clean incremental rebuilds are fast enough without it; then skip the
installation and record the measurement.

## Measurement plan

- Record the exact command, environment variables, PATH additions, wall-clock time and
  build log location (`agent_space/build.log`, git-ignored).
- Record `python -c "import torch; print(torch.__version__, torch.cuda.is_available())"`
  from outside the source tree after install.
- Capture failure output verbatim before attempting a fix.
