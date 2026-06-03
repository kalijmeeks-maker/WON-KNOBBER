#!/usr/bin/env python3
"""Emit RTNeural json_parser-compatible identity-ish stubs (2 in / 2 out, feedforward)."""
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1] / "Resources" / "Models"


def dense_layer(out_size: int, in_size: int, activation: str = "tanh") -> dict:
    # RTNeural json_parser: weights[0][out_i][in_j], weights[1] = bias vector
    kernel = [[0.02 * (i - j) for j in range(in_size)] for i in range(out_size)]
    bias = [0.0] * out_size
    return {
        "type": "dense",
        "activation": activation,
        "shape": [None, None, out_size],
        "weights": [kernel, bias],
    }


def build_model(hidden: int) -> dict:
    return {
        "in_shape": [None, None, 2],
        "layers": [
            dense_layer(hidden, 2),
            dense_layer(2, hidden),
        ],
    }


def main() -> None:
    specs = {
        "model_tape.json": 8,
        "model_valve.json": 8,
        "model_transistor.json": 8,
        "model_iron.json": 12,
    }
    for name, hidden in specs.items():
        path = ROOT / name
        path.write_text(json.dumps(build_model(hidden), indent=2) + "\n")
        print(f"wrote {path} (hidden={hidden})")


if __name__ == "__main__":
    main()