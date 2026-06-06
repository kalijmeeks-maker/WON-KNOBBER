/*
    AboutContent.h — single source of truth for the ship-required About/Licences copy.

    The About credit block and the full third-party licence body are pure, stateless text:
    neither reads any instance state. They are lifted here as free functions so the FRONT
    (FaceplateView) and the REAR (RearPanelView) About/Licences modals render byte-identical
    legal text from ONE place — the MIT/Airwindows + RTNeural BSD-3 notices cannot drift
    between the two entry points.

    WON-KNOBBER · part of the gui layer
*/
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace wk::about
{
// Verbatim third-party licence body (Airwindows MIT + RTNeural BSD-3 + framework note +
// per-IR placeholder), copied from THIRD_PARTY_LICENSES.md. Do not paraphrase.
juce::String licencesBodyText();

// The <=4-line About-card credit block (Airwindows MIT / cabinet IRs / neural / PolyForm + VST3).
// Assembled from the verbatim glyph literals; do not paraphrase.
juce::String creditBlockText();
} // namespace wk::about
