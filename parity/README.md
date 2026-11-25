# Parity manifest

`manifest.yaml` is the machine-readable source of truth for `docs/PARITY.md`.

- Add or update an op/layer row here, never in the generated doc.
- Statuses: `roadmap`, `in progress`, `forward done`, `forward+backward done`, `optimized`.
- A null result means **not measured**, not "pass".
- Regenerate: `python parity/generate_parity_doc.py` (PyYAML required; @dataclass config,
  no argparse).
- Parity tests use PyTorch OpInfo and device-generic tests, with cross-process reference
  tensors for CUDA because the fork is built without CUDA.
