# HYPOTHESIS

Every experiment in this project has a hypothesis recorded **before** work starts, at
`experiments/exp_NNN/HYPOTHESIS.md`, and a result at `experiments/exp_NNN/FINDINGS.md`.
This file is only the index. A hypothesis is a falsifiable claim with the measurement
that would settle it; it is not a plan or a wish.

| Experiment | Hypothesis | Status | Verdict |
|---|---|---|---|
| [exp_001](experiments/exp_001_recon/HYPOTHESIS.md) | Recon facts are sufficient to start Phase 0 without hardware changes; PrivateUse1 is the lowest-touch integration path. | done | confirmed (PrivateUse1 locked in D006) |
| [exp_002](experiments/exp_002_build/HYPOTHESIS.md) | A no-CUDA source build succeeds on Windows with Python 3.12 and MSVC/Ninja. | running | setup failures found and fixed; build not reached |
| [exp_003](experiments/exp_003_census/HYPOTHESIS.md) | Tier 0 primitives cover most dispatches; Tier 1 ops dominate profiled self time. | done | partial (transformers spread time across SDPA/layernorm) |

## Rules

1. Write the hypothesis before running the experiment.
2. Record the command and the raw evidence with it.
3. Write findings even when the result is negative.
4. If evidence contradicts a locked decision, update `ARCHITECTURE_LOG.md` and the docs first.
