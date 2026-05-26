# GUI

Dark-mode only. All custom drawing goes through a `juce::LookAndFeel_V4` subclass.

## LookAndFeel Pattern

`KnobLookAndFeel : juce::LookAndFeel_V4` overrides `drawRotarySlider`. The editor sets it on
the slider; no widget hard-codes its own paint logic.

## Filmstrip Convention

- One tall PNG containing **128 frames** stacked vertically.
- `frameHeight = imageHeight / 128`.
- `frameIndex = round(sliderProportion * 127)`.
- Blit the source rect `(0, frameIndex*frameHeight, frameWidth, frameHeight)` into the knob bounds.

## Faceplate

SVG loaded from `BinaryData`, drawn as the background of `FaceplateView`, scaled to fit.

## Fonts

Engraved/LED display fonts loaded from `BinaryData` via `juce::Typeface::createSystemTypefaceFor`.

## Color Tokens (dark-mode)

    background     #1B1B1E  (deep charcoal)
    panel          #2A2D2E  (gunmetal)
    accentAmber    #FFB14E
    accentTeal     #2FBFB0
    meterLow       #FFC24B  (amber)
    meterHigh      #E2493B  (red)
    engravedText   #C9C6BE
