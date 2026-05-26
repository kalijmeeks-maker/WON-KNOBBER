# Resources

Asset tree consumed by WON-KNOBBER. Subfolders are tracked via `.gitkeep` and stay empty in
Git — **no binaries are committed**. Real assets are dropped in locally and embedded at build
time via JUCE `juce_add_binary_data` (wired in a later PR).

| Folder        | Asset category                         | Expected format                      | License           | Consumed by            |
|---------------|----------------------------------------|--------------------------------------|-------------------|------------------------|
| `Faceplates/` | Faceplate background art               | SVG (scalable)                       | per-asset         | `FaceplateView`        |
| `Knobs/`      | Knob filmstrips                        | PNG, 128 vertical frames             | per-asset         | `KnobLookAndFeel`      |
| `Meters/`     | VU meter art / reference               | PNG / SVG                            | per-asset         | `VUMeter`              |
| `Fonts/`      | Engraved + LED display fonts           | TTF/OTF                              | OFL / per-license | GUI text (`BinaryData`)|
| `IRs/`        | Impulse responses                      | WAV, mono/stereo, 48 kHz reference   | per-asset         | `Convolution`          |
| `Models/`     | Neural model weights                   | ONNX, fixed I/O shape                | per-asset         | `NeuralModel`          |

Keep a per-asset license note alongside any asset added here.
