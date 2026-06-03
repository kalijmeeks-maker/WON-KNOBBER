# WON-KNOBBER — IR capture log (2026-06-02)

**Author:** Grok / WON Producer  
**Tier:** A (in-house, PolyForm-NC safe)  
**Branch:** `feat/cab-neural-assets-manifest` · **PR:** [#40](https://github.com/kalijmeeks-maker/WON-KNOBBER/pull/40)

## Method

Minimum-phase IRs synthesized from voiced magnitude targets + early-reflection
clusters aligned to the locked 8-voice cab identities. Not third-party retail IRs.

Regenerate: `python3 Scripts/generate_cab_ir_captures.py`

## Delivered files

| File | Taps | Voice anchor |
|------|------|----------------|
| `ir_flat.wav` | 2048 | VELVET reference |
| `ir_studio_ribbon.wav` | 2048 | TAPE HEAD |
| `ir_vintage_4x12.wav` | 2048 | FURNACE / TRANSFORMER |
| `ir_console_box.wav` | 2048 | CONSOLE GLUE / TUBE WARM |
| `ir_old_radio.wav` | 1536 | SUNDAY DRIVE |
| `ir_iron_core.wav` | 2048 | DIODE BITE |

Format: mono, 48 kHz, 24-bit WAV. Filenames unchanged for CC BinaryData swap-in.

## Optional upgrade

Mic'd cab captures may replace any file under the same name before release; no
code or manifest symbol changes required.