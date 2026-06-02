/*
    DbReadout.cpp — see header. Orbitron-rendered amber-glow text in a recessed well.
*/
#include "DbReadout.h"

#include "BinaryData.h"

DbReadout::DbReadout()
{
    setInterceptsMouseClicks (false, false);

    orbitron = juce::Typeface::createSystemTypefaceFor (
        BinaryData::Orbitron_ttf, BinaryData::Orbitron_ttfSize);
}

void DbReadout::setDrive (float newDrive) noexcept
{
    newDrive = juce::jlimit (0.0f, 1.0f, newDrive);
    if (std::abs (newDrive - drive) > 1.0e-4f)
    {
        drive = newDrive;
        repaint();
    }
}

void DbReadout::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    if (bounds.isEmpty()) return;

    // Recessed well backdrop — dark interior with hairline top shadow + bottom highlight.
    const float cornerR = juce::jmin (4.0f, bounds.getHeight() * 0.12f);
    const auto well = bounds.reduced (1.0f);

    g.setColour (juce::Colour (0xff0a0a0c).withAlpha (0.93f));
    g.fillRoundedRectangle (well, cornerR);

    g.setColour (juce::Colour (0xff000000).withAlpha (0.55f));
    g.drawRoundedRectangle (well, cornerR, 0.7f);

    // Build the display string per spec: leading "+" for >= 0, one decimal, " dB".
    const float driveDB = drive * kSpanDB + kOffsetDB;
    juce::String text;
    if (driveDB >= 0.0f) text << "+";
    text << juce::String (driveDB, 1) << " dB";

    if (orbitron == nullptr) return; // safety: fall through silently if the font failed to load

    // Font sized to ~60% of the well's height. Tabular-nums via Orbitron's monospace
    // digit width keeps "+24.0 dB" and "-12.3 dB" aligned identically.
    const float fontHeight = bounds.getHeight() * 0.60f;
    const juce::Font ledFont (juce::FontOptions (orbitron).withHeight (fontHeight));

    // Outer glow halo — same amber but soft, drawn as a few stacked semitransparent
    // passes at slight offsets. Cheap and gives the LED-in-a-well bloom.
    g.setFont (ledFont);
    const auto haloColour = juce::Colour (0xffff8800);
    for (float offset : { 3.0f, 2.0f, 1.0f })
    {
        g.setColour (haloColour.withAlpha (0.10f));
        g.drawText (text,
                    bounds.translated (offset, 0.0f),
                    juce::Justification::centred, false);
        g.drawText (text,
                    bounds.translated (-offset, 0.0f),
                    juce::Justification::centred, false);
        g.drawText (text,
                    bounds.translated (0.0f, offset),
                    juce::Justification::centred, false);
        g.drawText (text,
                    bounds.translated (0.0f, -offset),
                    juce::Justification::centred, false);
    }

    // Primary glyphs.
    g.setColour (haloColour);
    g.drawText (text, bounds, juce::Justification::centred, false);
}
