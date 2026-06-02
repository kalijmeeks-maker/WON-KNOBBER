/*
    GemChip.cpp — see header. Tone-on-tone metal pill matching the chassis voice.
*/
#include "GemChip.h"

void GemChip::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float r = b.getHeight() * 0.5f;

    // Subtle inlaid pill: dark fill, thin warm-grey border, light top-edge highlight.
    g.setColour (juce::Colour (0xff141517).withAlpha (0.85f));
    g.fillRoundedRectangle (b, r);
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.fillRoundedRectangle (b.withHeight (1.0f), r); // thin top-edge highlight
    g.setColour (juce::Colour (0xff9aa0a3).withAlpha (0.50f));
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), r, 1.0f);

    // Stone name — uppercase, wide tracking (faked via interspersed space), tone-on-tone.
    juce::String display = currentLabel.toUpperCase();
    juce::String spaced;
    for (int i = 0; i < display.length(); ++i)
    {
        spaced += display.substring (i, i + 1);
        if (i < display.length() - 1)
            spaced += " ";
    }
    g.setFont (juce::Font (juce::FontOptions ((float) std::round (b.getHeight() * 0.58f))
                              .withStyle ("Bold")));
    g.setColour (juce::Colour (0xffd6d8db));
    g.drawText (spaced, getLocalBounds(), juce::Justification::centred, false);
}

void GemChip::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu() || e.mods.isCtrlDown())
    {
        if (onShowMenu) onShowMenu();
    }
    else
    {
        if (onCycle) onCycle();
    }
}
