# exp_002 - Build findings (in progress)

Date: 2026-11-25

## H1 - not yet decided

The build has not reached CMake. Two setup failures were found and fixed in the harness.

## Attempt 1 - PowerShell stderr handling (negative)

`pip install -e . -v --no-build-isolation` under
`$ErrorActionPreference = "Stop"` stopped immediately with `NativeCommandError` on the
first stderr line ("Running command Checking if build backend supports build_editable").
`agent_space/build.log` contained only three lines.

**Fix:** invoke pip via `cmd /c` with `> log 2>&1` and set
`$ErrorActionPreference = "Continue"` around that call. Recorded in
`agent_space/build_fork.ps1` (git-ignored).

## Attempt 2 - missing submodules (negative)

With stderr captured, metadata generation failed with an ExceptionGroup of 54
`ConfigurationError`s: `Every pattern in "project.license-files" must match at least one
file: 'third_party/FP16/LICENSE' did not match any` (and 53 more).

Root cause: `git clone --filter=blob:none --no-checkout` left 36 submodules uninitialized;
the paths listed in `pyproject.toml:357+` live in those submodules. This is a
blobless-clone artifact, not a PyTorch bug.

**Fix in progress:** `git submodule update --init --depth 1 --jobs 4`.

## H2 - unverified

`USE_CUDA=0`, `USE_CUDNN=0`, `USE_DISTRIBUTED=0`, `BUILD_TESTING=0`, `MAX_JOBS=4`,
`CMAKE_BUILD_PARALLEL_LEVEL=4` are set in `agent_space/build_fork.ps1`. No CMake output
exists yet, so no claim is made about whether CUDA is fully disabled.

## H3 - observed without measurement

ccache/sccache is absent; the first build proceeds without it. H3 remains open until we
have an incremental rebuild time to compare.

## Environment corrections

- Python 3.12.10 installed from python.org because `pyproject.toml:350` requires >=3.11.
- Build venv at `agent_space/fork/venv` has all of `requirements-build.txt`
  (scikit-build-core 1.1.2, setuptools 81.0.0, cmake 4.4.5, ninja 1.13.2, etc.).
- VS DevShell (`Microsoft.VisualStudio.DevShell.dll`) supplies the MSVC environment.
