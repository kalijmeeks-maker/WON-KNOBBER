# DSP

Three processing stages, each real-time safe and sharing the
`prepare(double, int) / process(AudioBuffer<float>&) / reset()` contract.

## Saturation (MVP)

Soft-clip via `tanh`-style waveshaping driven by the smoothed `drive` parameter.
MVP ships the stub interface only; the transfer function is added in a dedicated `feat:` PR.
Drive maps 0.0–1.0 to an internal gain range; output is compensated to keep perceived level.

## Convolution

Wraps `juce::dsp::Convolution`. `loadIR(const juce::File&)` loads an impulse response on the
message thread and hands it to the processor safely. Used for cabinet/hardware coloration.

- IR format: WAV, mono or stereo, **48 kHz** reference (resampled if the host differs).

## Neural Inference (placeholder)

`NeuralModel` is a placeholder for a learned analog model (NEBULA-style character).
`loadModel(const juce::File&)` + `process(...)`; inference engine integration is TODO.

- Model format: ONNX, fixed input/output tensor shape (block-size agnostic via internal buffering).

## Real-Time Safety Rules

- No allocation/locks/I/O in `process`.
- Parameter changes only via `SmoothedValue`.
- IR/model loading happens off the audio thread; swap in via lock-free handoff.
