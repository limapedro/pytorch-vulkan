# exp_004 - Cross-process parity strategy

Date: 2026-11-25

## H1

The official CUDA wheel and the no-CUDA fork cannot be imported in one process, so
parity must be checked by serializing CUDA/CPU reference tensors from the baseline venv
and comparing them to fork results (and to in-process CPU) in a separate process.

**Falsifier:** a single process can load both the CUDA wheel and the fork build with
correct CUDA availability, which would make cross-process comparison unnecessary.

## H2

Serialized fp32 reference tensors compared with `torch.testing.assert_close` at
rtol=1e-5 / atol=1e-6 give stable cross-process parity for Tier 0 ops, with differences
from the in-process CPU result no larger than normal CPU/CUDA numerical variance.

**Falsifier:** repeated runs show differences above tolerance that are not explained by
numerical variance, making serialized comparison unusable.

## Measurement plan

- Baseline venv saves fixed-input tensors and outputs with `torch.save` to
  `agent_space/baseline/results/`.
- Fork process loads those tensors, runs the same ops on CPU and (once built) on the
  Vulkan device, and reports max absolute difference per op.
- Record whether both backends could coexist in one process (H1) with the exact command
  and error.
- Lock the two-venv strategy in `ARCHITECTURE_LOG.md` only after this measurement.
