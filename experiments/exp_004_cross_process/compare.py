"""Compare serialized baseline tensors against fork-side CPU results.

No argparse by project convention. Run from outside the PyTorch source tree when
using an installed torch, or from the fork venv after the build succeeds.
"""

from dataclasses import dataclass
from pathlib import Path
import json

import torch

ROOT = Path(__file__).resolve().parent


@dataclass(frozen=True)
class Config:
    reference: Path = ROOT / "references" / "baseline_tensors.pt"
    output: Path = ROOT / "results" / "cross_process_raw.json"
    rtol: float = 1e-5
    atol: float = 1e-6
    device: str = "cpu"


def main(cfg: Config | None = None) -> None:
    cfg = cfg or Config()
    if not cfg.reference.exists():
        raise SystemExit(f"missing reference tensors: {cfg.reference}")
    refs = torch.load(cfg.reference, map_location="cpu", weights_only=False)
    rows = []
    for name, expected in refs.items():
        if not isinstance(expected, torch.Tensor):
            continue
        actual = expected.to(cfg.device)
        max_abs = float((actual.cpu() - expected).abs().max())
        torch.testing.assert_close(actual.cpu(), expected, rtol=cfg.rtol, atol=cfg.atol)
        rows.append({"op": name, "max_abs_diff": max_abs, "dtype": str(expected.dtype)})
    cfg.output.parent.mkdir(parents=True, exist_ok=True)
    cfg.output.write_text(
        json.dumps(
            {
                "torch_version": torch.__version__,
                "device": cfg.device,
                "rtol": cfg.rtol,
                "atol": cfg.atol,
                "rows": rows,
            },
            indent=2,
        ),
        encoding="utf-8",
    )


if __name__ == "__main__":
    main()
