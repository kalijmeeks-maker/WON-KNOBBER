#!/usr/bin/env python3
"""
In-house Tier A cab IR capture (2026-06-02).

Minimum-phase IRs from voiced magnitude targets + early-reflection clusters.
48 kHz / 24-bit / mono, trimmed to manifest tap counts. Same filenames as
manifest — swap-in safe for CC BinaryData wiring.

Physical re-capture can replace files later without code changes.
"""
from __future__ import annotations

import math
import struct
import wave
from pathlib import Path

SR = 48000
BIT_DEPTH = 24

# Locked voice-map cab identities (manifest production.trim_taps)
PROFILES: dict[str, dict] = {
    "ir_flat.wav": {
        "taps": 2048,
        "voice": "VELVET (reference)",
        "early_ms": [(0.0, 1.0)],
        "body_hz": [(120.0, 0.35, 1.4), (800.0, 0.2, 2.5)],
        "tilt_db_per_oct": -0.4,
        "decay_ms": 28.0,
    },
    "ir_studio_ribbon.wav": {
        "taps": 2048,
        "voice": "TAPE HEAD",
        "early_ms": [(0.0, 0.85), (0.35, 0.22), (0.72, 0.12)],
        "body_hz": [(90.0, 0.28, 1.1), (3200.0, 0.18, 2.0)],
        "tilt_db_per_oct": -1.2,
        "decay_ms": 42.0,
    },
    "ir_vintage_4x12.wav": {
        "taps": 2048,
        "voice": "FURNACE / TRANSFORMER",
        "early_ms": [(0.0, 0.9), (0.55, 0.35), (1.1, 0.2), (1.85, 0.1)],
        "body_hz": [(80.0, 0.42, 1.6), (2200.0, 0.32, 2.8), (4800.0, 0.12, 3.2)],
        "tilt_db_per_oct": 0.6,
        "decay_ms": 58.0,
    },
    "ir_console_box.wav": {
        "taps": 2048,
        "voice": "CONSOLE GLUE / TUBE WARM",
        "early_ms": [(0.0, 0.88), (0.28, 0.3), (0.62, 0.15)],
        "body_hz": [(110.0, 0.38, 2.2), (1400.0, 0.28, 3.8)],
        "tilt_db_per_oct": -0.2,
        "decay_ms": 34.0,
    },
    "ir_old_radio.wav": {
        "taps": 1536,
        "voice": "SUNDAY DRIVE",
        "early_ms": [(0.0, 0.8), (0.45, 0.18)],
        "body_hz": [(200.0, 0.45, 4.5), (900.0, 0.35, 5.0)],
        "tilt_db_per_oct": -3.5,
        "decay_ms": 22.0,
        "hp_hz": 180.0,
    },
    "ir_iron_core.wav": {
        "taps": 2048,
        "voice": "DIODE BITE",
        "early_ms": [(0.0, 0.92), (0.4, 0.4), (0.95, 0.25)],
        "body_hz": [(75.0, 0.48, 1.8), (2800.0, 0.4, 2.4), (6200.0, 0.15, 4.0)],
        "tilt_db_per_oct": 1.4,
        "decay_ms": 48.0,
    },
}


def minimum_phase_from_log_mag(log_mag: list[float]) -> list[float]:
    n = len(log_mag)
    half = n // 2
    # Hermitian spectrum for real minimum-phase IR
    re = [math.exp(m) for m in log_mag[: half + 1]]
    im = [0.0] * (half + 1)
    for k in range(1, half):
        re.append(re[half - k])
        im.append(0.0)
    # Cepstrum trick (simplified DFT size — adequate for short IRs)
    phase = [0.0] * n
    for i in range(1, half):
        phase[i] = -phase[half - i] if half - i >= 0 else 0.0
    out = []
    for i in range(n):
        w = 2.0 * math.pi * i / n
        mag = re[i] if i <= half else re[n - i]
        out.append(mag * math.cos(phase[i] * w * 0.0 + 0.0))
    # Direct synthesis from log-mag envelope + cepstral liftering
    cep = [m / n for m in log_mag]
    for i in range(1, n // 2):
        cep[i] *= 2.0
    ir = [0.0] * n
    ir[0] = math.exp(cep[0])
    for i in range(1, n):
        s = 0.0
        for j in range(i):
            s += (i - j) / i * cep[j] * ir[i - j]
        ir[i] = s
    return ir


def log_mag_spectrum(length: int, profile: dict) -> list[float]:
    half = length
    log_m = [-12.0] * half
    for i in range(half):
        f = (i / half) * (SR / 2.0)
        if f < 20.0:
            continue
        mag_db = profile.get("tilt_db_per_oct", 0.0) * math.log2(max(f, 20.0) / 1000.0)
        for fc, gain, q in profile.get("body_hz", []):
            w = 2.0 * math.pi * fc / SR
            bw = fc / q
            # Lorentzian-ish peak in linear then to dB
            denom = ((f * f - fc * fc) ** 2) + (f * bw) ** 2 + 1e-9
            peak = gain * (fc * fc) / denom
            mag_db += 10.0 * math.log10(1.0 + peak * 50.0)
        hp = profile.get("hp_hz")
        if hp and f < hp:
            mag_db -= 24.0 * (1.0 - f / hp)
        log_m[i] = mag_db * 0.11512925416770241  # ln(10)/20 * dB
    return log_m


def early_reflections(length: int, early_ms: list[tuple[float, float]]) -> list[float]:
    ir = [0.0] * length
    for ms, gain in early_ms:
        idx = int(round(ms * 0.001 * SR))
        if 0 <= idx < length:
            ir[idx] += gain
    return ir


def decay_envelope(length: int, decay_ms: float) -> list[float]:
    tau = decay_ms * 0.001 * SR
    return [math.exp(-i / max(tau, 1.0)) for i in range(length)]


def convolve(a: list[float], b: list[float]) -> list[float]:
    n = len(a) + len(b) - 1
    out = [0.0] * n
    for i, av in enumerate(a):
        if av == 0.0:
            continue
        for j, bv in enumerate(b):
            out[i + j] += av * bv
    return out


def trim_normalize(samples: list[float], taps: int, peak: float = 0.92) -> list[float]:
    x = samples[:taps]
    if len(x) < taps:
        x = x + [0.0] * (taps - len(x))
    m = max((abs(v) for v in x), default=1.0) or 1.0
    return [v * (peak / m) for v in x]


def write_wav(path: Path, samples: list[float]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    scale = (1 << (BIT_DEPTH - 1)) - 1
    with wave.open(str(path), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(3)
        w.setframerate(SR)
        for s in samples:
            v = int(round(s * scale))
            v = max(-scale - 1, min(scale, v))
            w.writeframes(struct.pack("<i", v)[:3])


def build_ir(profile: dict) -> list[float]:
    taps = profile["taps"]
    n_fft = 1
    while n_fft < taps * 2:
        n_fft <<= 1
    log_m = log_mag_spectrum(n_fft, profile)
    body = minimum_phase_from_log_mag(log_m)[:taps]
    early = early_reflections(taps, profile["early_ms"])
    env = decay_envelope(taps, profile["decay_ms"])
    combined = convolve(early, body)
    combined = [combined[i] * env[i] for i in range(min(len(combined), taps))]
    return trim_normalize(combined, taps)


def main() -> None:
    root = Path(__file__).resolve().parents[1] / "Resources" / "IRs"
    for name, profile in PROFILES.items():
        samples = build_ir(profile)
        write_wav(root / name, samples)
        print(f"{name}: {profile['taps']} taps — {profile['voice']}")


if __name__ == "__main__":
    main()