#!/usr/bin/env python3
"""Generate distinct mono 48 kHz placeholder cab IRs (Tier A synthetic, swap-in safe)."""
from __future__ import annotations

import math
import struct
import wave
from pathlib import Path

SR = 48000
BIT_DEPTH = 24


def write_wav(path: Path, samples: list[float]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    peak = max((abs(s) for s in samples), default=1.0) or 1.0
    scaled = [int(round((s / peak) * 0.92 * ((1 << (BIT_DEPTH - 1)) - 1))) for s in samples]
    with wave.open(str(path), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(3)
        w.setframerate(SR)
        for v in scaled:
            w.writeframes(struct.pack("<i", v)[:3])


def exp_decay(n: int, tau: float) -> list[float]:
    return [math.exp(-i / tau) for i in range(n)]


def one_pole_lp(x: list[float], coef: float) -> list[float]:
    y = []
    state = 0.0
    for s in x:
        state = coef * state + (1.0 - coef) * s
        y.append(state)
    return y


def shaped_ir(
    length: int,
    *,
    decay_tau: float,
    lp_coef: float,
    pre_delay: int = 0,
    resonance_hz: float | None = None,
    resonance_q: float = 2.0,
    grit: float = 0.0,
) -> list[float]:
    body = [0.0] * pre_delay + exp_decay(length - pre_delay, decay_tau)
    if resonance_hz:
        w = 2.0 * math.pi * resonance_hz / SR
        r = math.exp(-w / (2.0 * resonance_q))
        b0 = 1.0 - r
        b1 = 0.0
        a1 = -r
        y = []
        z1 = 0.0
        for s in body:
            out = b0 * s + b1 * z1 - a1 * y[-1] if y else b0 * s
            z1 = s
            y.append(out)
        body = y
    body = one_pole_lp(body, lp_coef)
    if grit:
        body = [s + grit * math.tanh(3.5 * s) for s in body]
    return body


PROFILES: dict[str, dict] = {
    "ir_flat.wav": {
        "length": 2048,
        "decay_tau": 180.0,
        "lp_coef": 0.02,
        "pre_delay": 0,
        "resonance_hz": None,
        "grit": 0.0,
    },
    "ir_studio_ribbon.wav": {
        "length": 2048,
        "decay_tau": 420.0,
        "lp_coef": 0.14,
        "pre_delay": 6,
        "resonance_hz": 3200.0,
        "resonance_q": 1.2,
        "grit": 0.0,
    },
    "ir_vintage_4x12.wav": {
        "length": 2048,
        "decay_tau": 520.0,
        "lp_coef": 0.28,
        "pre_delay": 12,
        "resonance_hz": 2200.0,
        "resonance_q": 2.8,
        "grit": 0.12,
    },
    "ir_console_box.wav": {
        "length": 2048,
        "decay_tau": 260.0,
        "lp_coef": 0.42,
        "pre_delay": 18,
        "resonance_hz": 1400.0,
        "resonance_q": 3.5,
        "grit": 0.05,
    },
    "ir_old_radio.wav": {
        "length": 1536,
        "decay_tau": 200.0,
        "lp_coef": 0.62,
        "pre_delay": 24,
        "resonance_hz": 900.0,
        "resonance_q": 4.0,
        "grit": 0.18,
    },
    "ir_iron_core.wav": {
        "length": 2048,
        "decay_tau": 380.0,
        "lp_coef": 0.35,
        "pre_delay": 8,
        "resonance_hz": 2800.0,
        "resonance_q": 2.2,
        "grit": 0.28,
    },
}


def main() -> None:
    root = Path(__file__).resolve().parents[1] / "Resources" / "IRs"
    for name, cfg in PROFILES.items():
        samples = shaped_ir(
            cfg["length"],
            decay_tau=cfg["decay_tau"],
            lp_coef=cfg["lp_coef"],
            pre_delay=cfg.get("pre_delay", 0),
            resonance_hz=cfg.get("resonance_hz"),
            resonance_q=cfg.get("resonance_q", 2.0),
            grit=cfg.get("grit", 0.0),
        )
        write_wav(root / name, samples)
        print(f"wrote {root / name} ({len(samples)} taps)")


if __name__ == "__main__":
    main()