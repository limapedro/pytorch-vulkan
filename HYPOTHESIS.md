# HYPOTHESIS

Every experiment in this project has a hypothesis recorded **before** work starts, at
`experiments/exp_NNN/HYPOTHESIS.md`, and a result at `experiments/exp_NNN/FINDINGS.md`.
This file is only the index. A hypothesis is a falsifiable claim with the measurement
that would settle it; it is not a plan or a wish.

| Experiment | Hypothesis | Status | Verdict |
|---|---|---|---|
| [exp_001](experiments/exp_001_recon/HYPOTHESIS.md) | Recon facts are sufficient to start Phase 0 without hardware changes; PrivateUse1 is the lowest-touch integration path. | done | confirmed (PrivateUse1 locked in D006) |
| [exp_002](experiments/exp_002_build/HYPOTHESIS.md) | A no-CUDA source build succeeds on Windows with Python 3.12 and MSVC/Ninja. | done | confirmed: build green, CUDA disabled |
| [exp_003](experiments/exp_003_census/HYPOTHESIS.md) | Tier 0 primitives cover most dispatches; Tier 1 ops dominate profiled self time. | done | partial (transformers spread time across SDPA/layernorm) |
| [exp_004](experiments/exp_004_cross_process/HYPOTHESIS.md) | CUDA and fork cannot coexist in one process; serialized references are necessary for parity. | done | H1 provisional (coexistence not directly tested), H2 confirmed for tiny tensors |
| [exp_005](experiments/exp_005_device_registration/HYPOTHESIS.md) | An out-of-tree PrivateUse1 extension can register `kaminari` with zero core edits and the reference pattern works on Windows. | done | confirmed: autoload, device parsing, guard/hooks, 3/3 smoke tests |
| [exp_006](experiments/exp_006_vulkan_runtime/HYPOTHESIS.md) | A Vulkan compute context can be created with required features and queried limits matching vulkaninfo. | done | confirmed: runtime dump matches all probed limits; 2/2 runtime tests |
| [exp_007](experiments/exp_007_memory_ops/HYPOTHESIS.md) | A c10::Allocator over Vulkan buffers can back empty/copy/fill/item with exact values. | done | confirmed: exact fp32 smoke, 4/4 memory tests, fallback counted |
| [exp_008](experiments/exp_008_add_kernel/HYPOTHESIS.md) | A GLSL BDA/push-constant shader can implement add.out and remove it from the fallback. | done | confirmed: exact fp32 parity, fallback delta 0, 11/11 local tests |

## Rules

1. Write the hypothesis before running the experiment.
2. Record the command and the raw evidence with it.
3. Write findings even when the result is negative.
4. If evidence contradicts a locked decision, update `ARCHITECTURE_LOG.md` and the docs first.
