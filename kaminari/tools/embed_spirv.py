"""Embed a compiled SPIR-V module as a C++ header. No argparse by project convention."""

import os
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class Config:
    spirv: Path = Path(os.environ.get("KAMINARI_SPV", "add.spv"))
    output: Path = Path(os.environ.get("KAMINARI_HEADER", "add_spirv.h"))
    symbol: str = "kAddShaderSpv"


def main(cfg: Config | None = None) -> None:
    cfg = cfg or Config()
    data = cfg.spirv.read_bytes()
    lines = [
        "// Generated from add.comp by tools/embed_spirv.py. Do not edit.",
        "#pragma once",
        "",
        f"constexpr unsigned char {cfg.symbol}[] = {{",
        ", ".join(f"0x{b:02x}" for b in data),
        "};",
        f"constexpr unsigned int {cfg.symbol}Size = sizeof({cfg.symbol});",
        "",
    ]
    cfg.output.parent.mkdir(parents=True, exist_ok=True)
    cfg.output.write_text("\n".join(lines), encoding="utf-8")


if __name__ == "__main__":
    main()
