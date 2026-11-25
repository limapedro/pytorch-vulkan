"""ATen op census over the benchmark models. No argparse by project convention."""

from collections import Counter
from dataclasses import dataclass
from pathlib import Path
import json
from typing import Callable

import torch
import torch.nn as nn
from torch.profiler import ProfilerActivity, profile
from torch.utils._python_dispatch import TorchDispatchMode

ROOT = Path(__file__).resolve().parent


@dataclass(frozen=True)
class Config:
    output: Path = ROOT / "results" / "op_census_raw.json"
    repeats: int = 2
    seed: int = 0
    tier0: tuple[str, ...] = (
        "empty", "zeros", "ones", "_to_copy", "copy_", "fill_", "zero_",
        "add", "sub", "mul", "div", "neg", "abs", "exp", "log", "sqrt",
        "sigmoid", "relu", "tanh", "pow", "maximum", "minimum", "where",
        "sum", "mean", "amax", "amin", "view", "reshape", "permute",
        "transpose", "squeeze", "unsqueeze", "expand", "contiguous",
        "clone", "select", "slice", "narrow", "index_select", "gather",
        "scatter_add", "index_add", "cat", "stack",
    )
    tier1: tuple[str, ...] = (
        "mm", "bmm", "addmm", "convolution", "softmax", "log_softmax",
        "native_layer_norm", "native_batch_norm", "embedding",
        "scaled_dot_product_attention", "nll_loss", "cross_entropy",
        "optim", "adam", "sgd",
    )


class CountingMode(TorchDispatchMode):
    def __init__(self) -> None:
        self.counts: Counter[str] = Counter()

    def __torch_dispatch__(self, func, types, args=(), kwargs=None):
        self.counts[str(func)] += 1
        return func(*args, **(kwargs or {}))


def mlp() -> tuple[nn.Module, tuple[torch.Tensor, ...]]:
    return (
        nn.Sequential(nn.Linear(256, 512), nn.ReLU(), nn.Linear(512, 256), nn.ReLU(), nn.Linear(256, 10)),
        (torch.randn(8, 256),),
    )


def small_cnn() -> tuple[nn.Module, tuple[torch.Tensor, ...]]:
    net = nn.Sequential(
        nn.Conv2d(3, 16, 3, padding=1), nn.ReLU(), nn.MaxPool2d(2),
        nn.Conv2d(16, 32, 3, padding=1), nn.ReLU(), nn.AdaptiveAvgPool2d(1),
        nn.Flatten(), nn.Linear(32, 10),
    )
    return net, (torch.randn(2, 3, 32, 32),)


def tiny_gpt() -> tuple[nn.Module, tuple[torch.Tensor, ...]]:
    class GPT(nn.Module):
        def __init__(self) -> None:
            super().__init__()
            self.emb = nn.Embedding(128, 64)
            self.pos = nn.Embedding(32, 64)
            self.blocks = nn.TransformerEncoderLayer(64, 4, 128, dropout=0.0)
            self.head = nn.Linear(64, 128)

        def forward(self, x: torch.Tensor) -> torch.Tensor:
            x = self.emb(x) + self.pos(torch.arange(x.shape[1]))
            return self.head(self.blocks(x))

    return GPT(), (torch.randint(0, 128, (2, 16)),)


def vit_tiny() -> tuple[nn.Module, tuple[torch.Tensor, ...]]:
    class ViT(nn.Module):
        def __init__(self) -> None:
            super().__init__()
            self.patch = nn.Conv2d(3, 64, 4, stride=4)
            self.cls = nn.Parameter(torch.zeros(1, 1, 64))
            self.blocks = nn.TransformerEncoderLayer(64, 4, 128, dropout=0.0)
            self.head = nn.Linear(64, 10)

        def forward(self, x: torch.Tensor) -> torch.Tensor:
            p = self.patch(x).flatten(2).transpose(1, 2)
            p = torch.cat([self.cls.expand(p.shape[0], -1, -1), p], dim=1)
            return self.head(self.blocks(p)[:, 0])

    return ViT(), (torch.randn(2, 3, 32, 32),)


def gpt2_small_style() -> tuple[nn.Module, tuple[torch.Tensor, ...]]:
    class GPT2(nn.Module):
        def __init__(self) -> None:
            super().__init__()
            self.emb = nn.Embedding(256, 96)
            self.pos = nn.Embedding(32, 96)
            layer = nn.TransformerEncoderLayer(96, 4, 192, dropout=0.0)
            self.blocks = nn.TransformerEncoder(layer, 4)
            self.head = nn.Linear(96, 256)

        def forward(self, x: torch.Tensor) -> torch.Tensor:
            x = self.emb(x) + self.pos(torch.arange(x.shape[1]))
            return self.head(self.blocks(x))

    return GPT2(), (torch.randint(0, 256, (2, 32)),)


def unet_style() -> tuple[nn.Module, tuple[torch.Tensor, ...]]:
    class UNet(nn.Module):
        def __init__(self) -> None:
            super().__init__()
            self.d1 = nn.Conv2d(1, 8, 3, padding=1)
            self.d2 = nn.Conv2d(8, 16, 3, padding=1)
            self.u1 = nn.ConvTranspose2d(16, 8, 2, stride=2)
            self.u2 = nn.ConvTranspose2d(16, 8, 2, stride=2)
            self.out = nn.Conv2d(8, 1, 1)

        def forward(self, x: torch.Tensor) -> torch.Tensor:
            a = torch.relu(self.d1(x))
            b = torch.relu(self.d2(torch.max_pool2d(a, 2)))
            c = torch.relu(self.u1(b) + a)
            return self.out(torch.relu(self.u2(b) + a) + c)

    return UNet(), (torch.randn(2, 1, 32, 32),)


MODELS: tuple[tuple[str, Callable[[], tuple[nn.Module, tuple[torch.Tensor, ...]]], bool], ...] = (
    ("MLP", mlp, True),
    ("small CNN", small_cnn, True),
    ("tiny GPT", tiny_gpt, True),
    ("ViT-tiny style", vit_tiny, True),
    ("GPT-2 small style", gpt2_small_style, False),
    ("UNet style", unet_style, True),
)


def normalize(name: str) -> str:
    key = str(name).split("(")[0]
    if key.startswith("aten::"):
        key = key[6:]
    if key.startswith("aten."):
        key = key[5:]
    return key.split(".")[0].split("::")[0]


def classify(name: str, cfg: Config) -> str:
    short = normalize(name)
    if any(short.startswith(p) for p in cfg.tier0):
        return "tier0"
    if any(short.startswith(p) for p in cfg.tier1):
        return "tier1"
    return "long_tail"


def run_one(name: str, factory: Callable, train: bool, cfg: Config) -> dict:
    torch.manual_seed(cfg.seed)
    model, inputs = factory()
    counts: Counter[str] = Counter()
    times: Counter[str] = Counter()
    for _ in range(cfg.repeats):
        model.zero_grad(set_to_none=True)
        mode = CountingMode()
        with mode, profile(activities=[ProfilerActivity.CPU]) as prof:
            out = model(*inputs)
            if train:
                if out.ndim == 2 and out.shape[1] >= 2:
                    target = torch.randint(0, out.shape[1], (out.shape[0],))
                    loss = torch.nn.functional.cross_entropy(out, target)
                else:
                    loss = out.float().mean()
                loss.backward()
        counts.update(mode.counts)
        for evt in prof.key_averages():
            times[normalize(evt.key)] += evt.self_cpu_time_total
    total = sum(counts.values())
    by_tier: Counter[str] = Counter()
    for op, n in counts.items():
        by_tier[classify(op, cfg)] += n
    total_time = sum(times.values()) or 1
    return {
        "model": name,
        "training": train,
        "total_dispatches": total,
        "distinct_ops": len(counts),
        "by_tier": dict(by_tier),
        "all_counts": dict(counts),
        "all_self_cpu_us": dict(times),
        "top_counts": counts.most_common(20),
        "top_self_cpu_us": times.most_common(20),
        "tier1_self_cpu_fraction": sum(
            v for k, v in times.items() if classify(k, cfg) == "tier1"
        ) / total_time,
    }


def main(cfg: Config | None = None) -> None:
    cfg = cfg or Config()
    torch.manual_seed(cfg.seed)
    result = {
        "torch_version": torch.__version__,
        "note": "CPU census; version may differ from fork base v2.15.0",
        "models": [run_one(n, f, t, cfg) for n, f, t in MODELS],
    }
    cfg.output.parent.mkdir(parents=True, exist_ok=True)
    cfg.output.write_text(json.dumps(result, indent=2), encoding="utf-8")


if __name__ == "__main__":
    main()
