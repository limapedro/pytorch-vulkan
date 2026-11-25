# TESTING

## Policy

- Use `torch.testing._internal.common_utils.TestCase` / `run_tests` and `@parametrize`.
- Numerics of on-device implementations use `instantiate_device_type_tests` with
  device-generic tests, and OpInfo coverage is the parity source of truth.
- Every new op updates `parity/manifest.yaml`; regenerate `docs/PARITY.md` with
  `python parity/generate_parity_doc.py`.
- A CPU fallback is loud and counted; fallback count is recorded per model in PARITY.

## Planned suites

1. Device smoke: empty, copy H2D/D2H, fill, add, synchronize, allocator v1.
2. OpInfo parity: tier 0 first, then tier 1; cross-process reference tensors for CUDA
   because the fork is built without CUDA.
3. Model suites: MLP, CNN, ResNet-18/50, tiny GPT, ViT-tiny, GPT-2 small, UNet;
   inference and training step.
4. CI cheap checks: lint, docs checks, shader compile, op tests on lavapipe.
5. Full build nightly/manual; GPU performance runs local only.

## Commands (to be filled after the first build)

No test command has been run yet; do not record one until it has.
