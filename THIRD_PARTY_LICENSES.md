# Third-Party Licenses

WON-KNOBBER incorporates third-party code. Their licenses and copyright notices
are reproduced below as required.

---

## Airwindows (saturation algorithms)

The saturation transfer functions in `Source/dsp/AirwindowsShapers.h`
(Density3, Mojo, Spiral2 presence, PurestSaturation) are derived from Airwindows.

- Source: https://github.com/airwindows/airwindows
- Copyright (c) 2018 Chris Johnson
- License: MIT

```
MIT License

Copyright (c) 2018 Chris Johnson

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

---

## RTNeural (neural inference engine)

The neural character models (TAPE-1971, VALVE-CLASS A, TRANSISTOR-FET, IRON-TRANSFORMER)
run via RTNeural inference in `Source/dsp/NeuralModel.*`.

- Source: https://github.com/jatinchowdhury18/RTNeural
- Pinned: 1fb1f075a5d66e85bfc8f488c3f3626840cb3a1d (per WON_RTNeural_vs_ONNX_spike)
- Copyright (c) 2020, jatinchowdhury18
- License: BSD 3-Clause

```
BSD 3-Clause License

Copyright (c) 2020, jatinchowdhury18
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```

---

## WON-KNOBBER Cab IRs (in-house)

Six minimum-phase mono cab impulse responses (48 kHz, 24-bit WAV, 1536–2048 taps).

- Producer: Grok / WON Producer (Tier A, in-house, 2026-06-02)
- Method: synthesized from locked voice magnitude targets + early-reflection clusters (see docs/voice-cab-neural-map.md for per-voice assignment and docs/WON_IR_capture_log_2026-06-02.md for full log).
- Files (locked names, replaceable by real mic'd under same filename with no code changes):
  - ir_flat.wav — FLAT (CONSOLE GLUE anchor, 2048 taps)
  - ir_studio_ribbon.wav — STUDIO RIBBON (TAPE HEAD / VELVET, 2048 taps)
  - ir_vintage_4x12.wav — VINTAGE 4×12 (FURNACE / DIODE BITE, 2048 taps)
  - ir_console_box.wav — CONSOLE BOX (TUBE WARM, 2048 taps)
  - ir_old_radio.wav — OLD RADIO (SUNDAY DRIVE, 1536 taps)
  - ir_iron_core.wav — IRON CORE (TRANSFORMER, 2048 taps)
- Regenerate: `python3 Scripts/generate_cab_ir_captures.py`
- Licence / compatibility: In-house. Explicitly "PolyForm Noncommercial plugin" (Resources/IRs/LICENSE_IRS.txt). Fully redistributable under the project's PolyForm-NC terms; no external third-party retail IR licence conflicts. No ship blockers.

(Closes the last 1.0 licence item for the six cab IRs. Notices also appear in the shipped About/Licences modal.)

---

## nlohmann/json (JSON parser)

Compiled into the shipped binary: RTNeural's model loader parses the embedded neural model
JSON via nlohmann/json (`Source/dsp/NeuralModel.cpp`).

- Source: https://github.com/nlohmann/json
- Version: 3.11.1
- Copyright (c) 2013-2022 Niels Lohmann
- License: MIT

```
MIT License

Copyright (c) 2013-2022 Niels Lohmann

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

---

## VST3 SDK (plug-in format)

The VST3 plug-in interfaces are provided by Steinberg and bundled via JUCE.

- Source: https://github.com/steinbergmedia/vst3sdk
- Copyright (c) Steinberg Media Technologies GmbH
- License: MIT (SDK interfaces). The MIT terms are identical to the text above.
- "VST" is a registered trademark of Steinberg Media Technologies GmbH.

---

## JUCE (framework)

Built with the JUCE framework, pulled via CMake FetchContent at tag `8.0.13`.

- Source: https://juce.com · https://github.com/juce-framework/JUCE
- JUCE 8 is dual-licensed: AGPLv3 **or** a JUCE commercial/personal licence.

> OWNER ACTION (pre-1.0, legal-basis only — not a code item): confirm the JUCE licence
> basis under which WON KNOBBER ships as PolyForm Noncommercial (AGPLv3 compliance vs a
> held JUCE personal/commercial licence). Stated here so the basis is explicit before tagging.


