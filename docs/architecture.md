# Architecture

## Audio Path

    input buffer
      -> parameter smoothing (SmoothedValue per param, updated per block)
      -> DSP chain: Saturation -> Convolution -> (NeuralModel, optional)
      -> metering tap (peak/RMS for the VU meter, read by UI via atomics)
      -> output buffer

All stages share the `prepare/process/reset` contract and run inside
`juce::ScopedNoDenormals` on the audio thread. No allocation occurs in the path.

## GUI Hierarchy

    FaceplateView (root container, draws SVG faceplate background)
      |- Knob            (juce::Slider styled by KnobLookAndFeel, filmstrip render)
      |- VUMeter         (Component, 30Hz timer, amber->red gradient)
      |- BypassLED       (Component, setActive(bool) glow)

`FaceplateView` owns layout (`resized()`); children never position themselves.

## Threading Model

- Audio thread: prepareToPlay / processBlock / releaseResources — lock-free, no alloc.
- Message thread: editor, parameter edits, 30Hz meter timer, asset loading.
- GPU render thread: Skia paint (post-MVP), driven from the message thread.

Audio -> UI communication is via atomics / APVTS only.

## Asset Pipeline

- Knob: filmstrip PNG, 128 vertical frames, compiled into `BinaryData`.
- Faceplate: SVG, drawn as background.
- IR files: WAV (mono/stereo, 48 kHz reference) loaded by `Convolution`.
- Neural weights: ONNX, fixed I/O shape, loaded by `NeuralModel`.

Assets live under `Resources/` and are embedded via JUCE `juce_add_binary_data` (added when
real assets land). Nothing is read from disk on the audio thread.
