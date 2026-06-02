/*
    FaceplateView.cpp — draws the embedded photoreal PRO chassis (960x600) and seats the
    hero filmstrip knob in the SATURATION well. Controls are placed in 960x600 reference
    coords and scaled to the actual size. Live instruments (transfer / harmonics / meters)
    are Phase 2.
*/
#include "FaceplateView.h"

#include "BinaryData.h"

FaceplateView::FaceplateView()
{
    faceplate = juce::ImageCache::getFromMemory (
        BinaryData::faceplate_pro_960x600_png, BinaryData::faceplate_pro_960x600_pngSize);

    // Register all 7 gem variants for the "choose your stone" picker.
    // Diamond is added first so it's the default on a fresh load.
    auto reg = [this] (const juce::String& name, const char* data, int size)
    {
        knobLnf.addVariant (name, juce::ImageCache::getFromMemory (data, size));
    };
    reg ("diamond",  BinaryData::knob_diamond_256_png,  BinaryData::knob_diamond_256_pngSize);
    reg ("onyx",     BinaryData::knob_onyx_256_png,     BinaryData::knob_onyx_256_pngSize);
    reg ("sapphire", BinaryData::knob_sapphire_256_png, BinaryData::knob_sapphire_256_pngSize);
    reg ("emerald",  BinaryData::knob_emerald_256_png,  BinaryData::knob_emerald_256_pngSize);
    reg ("ruby",     BinaryData::knob_ruby_256_png,     BinaryData::knob_ruby_256_pngSize);
    reg ("amethyst", BinaryData::knob_amethyst_256_png, BinaryData::knob_amethyst_256_pngSize);
    reg ("citrine",  BinaryData::knob_citrine_256_png,  BinaryData::knob_citrine_256_pngSize);

    driveKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    driveKnob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    driveKnob.setRange (0.0, 1.0, 0.0);
    driveKnob.setLookAndFeel (&knobLnf);

    addAndMakeVisible (transferCurve);
    addAndMakeVisible (harmonicBars);
    addAndMakeVisible (driveKnob);

    // Wire the gem chip: left-click cycles to the next stone, right-click pops the menu.
    gemChip.setLabel (knobLnf.getCurrentVariant());
    gemChip.onCycle = [this]
    {
        const auto& names = knobLnf.getVariantNames();
        if (names.isEmpty()) return;
        const int idx = juce::jmax (0, names.indexOf (knobLnf.getCurrentVariant()));
        const juce::String next = names[(idx + 1) % names.size()];
        setVariant (next);
        if (onVariantPicked) onVariantPicked (next);
    };
    gemChip.onShowMenu = [this]
    {
        const auto& names = knobLnf.getVariantNames();
        const auto current = knobLnf.getCurrentVariant();
        juce::PopupMenu menu;
        for (int i = 0; i < names.size(); ++i)
            menu.addItem (i + 1, names[i].toUpperCase(), true, names[i] == current);
        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&gemChip),
                            [this, names] (int chosen)
                            {
                                if (chosen <= 0 || chosen > names.size()) return;
                                const juce::String picked = names[chosen - 1];
                                setVariant (picked);
                                if (onVariantPicked) onVariantPicked (picked);
                            });
    };
    addAndMakeVisible (gemChip);

    addAndMakeVisible (ioMeter);

    // bypassLed: Phase 2b — not drawn over the photoreal chassis yet.
}

FaceplateView::~FaceplateView()
{
    driveKnob.setLookAndFeel (nullptr);
}

void FaceplateView::paint (juce::Graphics& g)
{
    if (faceplate.isValid())
    {
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.drawImage (faceplate, getLocalBounds().toFloat(), juce::RectanglePlacement::stretchToFit);
    }
    else
    {
        g.fillAll (juce::Colour (0xff141517));
    }
}

void FaceplateView::resized()
{
    const float sx = (float) getWidth()  / (float) kRefW;
    const float sy = (float) getHeight() / (float) kRefH;

    // Map a 960x600 reference rect [x,y,w,h] onto the actual size.
    auto place = [sx, sy] (int rx, int ry, int rw, int rh)
    {
        return juce::Rectangle<int> (juce::roundToInt ((float) rx * sx),
                                     juce::roundToInt ((float) ry * sy),
                                     juce::roundToInt ((float) rw * sx),
                                     juce::roundToInt ((float) rh * sy));
    };

    // Authoritative anchors from Claude Design (faceplate-pro-anchors.json, 960x600).
    transferCurve.setBounds (place (57, 168, 240, 240));   // transfer_panel
    harmonicBars.setBounds  (place (705, 168, 240, 166));  // harmonics_panel
    driveKnob.setBounds     (place (501 - 115, 289 - 115, 230, 230)); // hero_knob_well, centre (501,289)

    // GemChip — small persistent pill centred below the knob, above the dB readout.
    // Reference rect [438, 405, 126, 16] in 960x600 coords (sits in the gap between
    // the knob's bottom and db_readout's [421, 423, 161, 51]).
    gemChip.setBounds (place (438, 405, 126, 16));

    // Twin horizontal IN/OUT peak meters in the io_meters strip atop the chassis.
    ioMeter.setBounds (place (503, 31, 200, 73));
}
