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
    faceplate = juce::ImageCache::getFromMemory(BinaryData::faceplate_pro_960x600_png,
                                                BinaryData::faceplate_pro_960x600_pngSize);

    // Register all 7 gem variants for the "choose your stone" picker.
    // Diamond is added first so it's the default on a fresh load.
    auto reg = [this](const juce::String& name, const char* data, int size)
    { knobLnf.addVariant(name, juce::ImageCache::getFromMemory(data, size)); };
    reg("diamond", BinaryData::knob_diamond_256_png, BinaryData::knob_diamond_256_pngSize);
    reg("onyx", BinaryData::knob_onyx_256_png, BinaryData::knob_onyx_256_pngSize);
    reg("sapphire", BinaryData::knob_sapphire_256_png, BinaryData::knob_sapphire_256_pngSize);
    reg("emerald", BinaryData::knob_emerald_256_png, BinaryData::knob_emerald_256_pngSize);
    reg("ruby", BinaryData::knob_ruby_256_png, BinaryData::knob_ruby_256_pngSize);
    reg("amethyst", BinaryData::knob_amethyst_256_png, BinaryData::knob_amethyst_256_pngSize);
    reg("citrine", BinaryData::knob_citrine_256_png, BinaryData::knob_citrine_256_pngSize);

    driveKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    driveKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    driveKnob.setRange(0.0, 1.0, 0.0);
    driveKnob.setLookAndFeel(&knobLnf);

    addAndMakeVisible(transferCurve);
    addAndMakeVisible(harmonicBars);
    addAndMakeVisible(driveKnob);
    addAndMakeVisible(mixKnob);

    // Wire the gem chip: left-click cycles to the next stone, right-click pops the menu.
    gemChip.setLabel(knobLnf.getCurrentVariant());
    gemChip.onCycle = [this]
    {
        const auto& names = knobLnf.getVariantNames();
        if (names.isEmpty())
            return;
        const int idx = juce::jmax(0, names.indexOf(knobLnf.getCurrentVariant()));
        const juce::String next = names[(idx + 1) % names.size()];
        setVariant(next);
        if (onVariantPicked)
            onVariantPicked(next);
    };
    gemChip.onShowMenu = [this]
    {
        const auto& names = knobLnf.getVariantNames();
        const auto current = knobLnf.getCurrentVariant();
        juce::PopupMenu menu;
        for (int i = 0; i < names.size(); ++i)
            menu.addItem(i + 1, names[i].toUpperCase(), true, names[i] == current);
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&gemChip),
                           [this, names](int chosen)
                           {
                               if (chosen <= 0 || chosen > names.size())
                                   return;
                               const juce::String picked = names[chosen - 1];
                               setVariant(picked);
                               if (onVariantPicked)
                                   onVariantPicked(picked);
                           });
    };
    addAndMakeVisible(gemChip);

    addAndMakeVisible (ioMeter);
    addAndMakeVisible (statusLEDs);
    addAndMakeVisible (dbReadout);

    // bypassLed: Phase 2b — not drawn over the photoreal chassis yet.
}

FaceplateView::~FaceplateView()
{
    driveKnob.setLookAndFeel(nullptr);
}

void FaceplateView::paint(juce::Graphics& g)
{
    if (faceplate.isValid())
    {
        g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
        g.drawImage(faceplate, getLocalBounds().toFloat(), juce::RectanglePlacement::stretchToFit);
    }
    else
    {
        g.fillAll(juce::Colour(0xff141517));
    }

    // Phase 2b overlays for preset strip (engraved LED name + chevrons + A/B) and transport tray.
    // Drawn after chassis so text/buttons sit in the photoreal footer recesses.
    drawPresetStrip(g);
    drawTransportTray(g);
}

void FaceplateView::resized()
{
    const float sx = (float)getWidth() / (float)kRefW;
    const float sy = (float)getHeight() / (float)kRefH;

    // Map a 960x600 reference rect [x,y,w,h] onto the actual size.
    auto place = [sx, sy](int rx, int ry, int rw, int rh)
    {
        return juce::Rectangle<int>(juce::roundToInt((float)rx * sx), juce::roundToInt((float)ry * sy),
                                    juce::roundToInt((float)rw * sx), juce::roundToInt((float)rh * sy));
    };

    // Authoritative anchors from Claude Design (faceplate-pro-anchors.json, 960x600).
    transferCurve.setBounds(place(57, 168, 240, 240));          // transfer_panel
    harmonicBars.setBounds(place(705, 168, 240, 166));          // harmonics_panel
    driveKnob.setBounds(place(501 - 115, 289 - 115, 230, 230)); // hero_knob_well, centre (501,289)

    // GemChip — small persistent pill centred below the knob, above the dB readout.
    // Reference rect [438, 405, 126, 16] in 960x600 coords (sits in the gap between
    // the knob's bottom and db_readout's [421, 423, 161, 51]).
    gemChip.setBounds(place(438, 405, 126, 16));

    // Twin horizontal IN/OUT peak meters in the io_meters strip atop the chassis.
    ioMeter.setBounds(place(503, 31, 200, 73));

    // DRY/WET mix knob at the mix_knob anchor (bottom right of harmonics area).
    mixKnob.setBounds(place(758, 346, 134, 110));

    // POWER · SIG · CLIP dome LEDs at status_leds anchor centres (806,55) (850,55)
    // (894,55), 14 px each. Bounds span: left = POWER centre - 7, right = CLIP centre
    // + 7 → x=799 .. 901 (102 wide), y=48 .. 62 (14 tall). StatusLEDs derives the
    // three centres internally as fractions of its bounds.
    statusLEDs.setBounds(place(799, 48, 102, 14));

    // Drive -> dB readout in the recessed well below the hero knob.
    dbReadout.setBounds(place(421, 423, 161, 51));

    // Phase 2b: preset strip (center footer) + util_tray_transport (right). Anchors from
    // exports/faceplate-pro-anchors.json per PHASE2B_DESIGN_SPEC.md §3.
    presetStripBounds = place(121, 537, 518, 38);
    transportBounds = place(655, 531, 264, 48);

    // Sub-rects inside presetStrip for interactive elements + name LED.
    {
        const auto ps = presetStripBounds;
        const int margin = juce::jmax(4, ps.getHeight() / 8);
        const int chevW = juce::jmax(18, ps.getHeight() * 5 / 8);
        const int chevH = ps.getHeight() - margin;
        const int abW = juce::jmax(26, ps.getHeight() * 3 / 4);
        const int gap = 3;

        int x = ps.getX() + margin;
        chevLeftBounds = juce::Rectangle<int>(x, ps.getY() + (ps.getHeight() - chevH) / 2, chevW, chevH);
        x += chevW + gap;

        const int chevRightStartGuess = ps.getRight() - margin - (2 * abW + gap) - gap - chevW;
        const int nameW = juce::jmax(80, chevRightStartGuess - x);
        presetNameBounds = juce::Rectangle<int>(x, ps.getY() + margin / 2, nameW, ps.getHeight() - margin);

        // "Modified" ember dot (Design spec): ~6px against the preset name's right edge, vertically
        // centred, with a ~20px transparent hit-target centred on it (6px is too small to click).
        const int dotSize = juce::jmax(5, ps.getHeight() / 6);  // ~6px
        const int hit = juce::jmax(16, ps.getHeight() / 2);     // ~20px hit-target
        const int dotCx = presetNameBounds.getRight() - dotSize;
        const int dotCy = presetNameBounds.getCentreY();
        modifiedDotBounds = juce::Rectangle<int>(dotCx - hit / 2, dotCy - hit / 2, hit, hit);

        x += nameW + gap;

        chevRightBounds = juce::Rectangle<int>(x, ps.getY() + (ps.getHeight() - chevH) / 2, chevW, chevH);
        x += chevW + gap;

        aButtonBounds = juce::Rectangle<int>(x, ps.getY() + margin / 2, abW, ps.getHeight() - margin);
        x += abW + gap;
        bButtonBounds = juce::Rectangle<int>(x, ps.getY() + margin / 2, abW, ps.getHeight() - margin);
    }

    // Sub-rects inside transportBounds for 4 affordance buttons (S L U RND).
    {
        const auto tr = transportBounds;
        const int tmargin = juce::jmax(3, tr.getHeight() / 10);
        const int btnW = (tr.getWidth() - 5 * tmargin) / 4;
        const int btnH = tr.getHeight() - 2 * tmargin;
        const int ty = tr.getY() + tmargin;
        int tx = tr.getX() + tmargin;
        saveBtnBounds = juce::Rectangle<int>(tx, ty, btnW, btnH);
        tx += btnW + tmargin;
        loadBtnBounds = juce::Rectangle<int>(tx, ty, btnW, btnH);
        tx += btnW + tmargin;
        undoBtnBounds = juce::Rectangle<int>(tx, ty, btnW, btnH);
        tx += btnW + tmargin;
        randBtnBounds = juce::Rectangle<int>(tx, ty, btnW, btnH);
    }
}

//==============================================================================
// Phase 2b preset strip + transport (inline impl for smallest change; no new .h/.cpp per guidelines).

void FaceplateView::setPresetDisplayName(const juce::String& name)
{
    if (name != currentPresetName)
    {
        currentPresetName = name;
        repaint();
    }
}

void FaceplateView::setActiveSlot(char s)
{
    const char up = static_cast<char>(juce::CharacterFunctions::toUpperCase(static_cast<juce::juce_wchar>(s)));
    if (up != 'A' && up != 'B')
        return;
    if (up != activeSlot)
    {
        activeSlot = up;
        repaint();
    }
}

void FaceplateView::setModified(bool isModified)
{
    if (isModified != modified)
    {
        modified = isModified;
        repaint();
    }
}

void FaceplateView::mouseDown(const juce::MouseEvent& e)
{
    const auto pos = e.getPosition();

    // "Modified" ember dot — only clickable while lit; reverts the live state to the loaded preset.
    if (modified && modifiedDotBounds.contains(pos))
    {
        if (onRevertToPreset)
            onRevertToPreset();
        return;
    }

    if (chevLeftBounds.contains(pos))
    {
        cyclePreset(-1);
        return;
    }
    if (chevRightBounds.contains(pos))
    {
        cyclePreset(1);
        return;
    }
    if (aButtonBounds.contains(pos))
    {
        if (onActiveSlotSelected)
            onActiveSlotSelected('A');
        return;
    }
    if (bButtonBounds.contains(pos))
    {
        if (onActiveSlotSelected)
            onActiveSlotSelected('B');
        return;
    }

    // transport tray buttons (affordances; wired by editor to processor in-mem actions)
    if (saveBtnBounds.contains(pos))
    {
        if (onTransportAction)
            onTransportAction("save");
        return;
    }
    if (loadBtnBounds.contains(pos))
    {
        if (onTransportAction)
            onTransportAction("load");
        return;
    }
    if (undoBtnBounds.contains(pos))
    {
        if (onTransportAction)
            onTransportAction("undo");
        return;
    }
    if (randBtnBounds.contains(pos))
    {
        if (onTransportAction)
            onTransportAction("randomize");
        return;
    }
}

void FaceplateView::cyclePreset(int dir)
{
    const int n = juce::jmax(1, numFactoryPresets);
    currentPresetIndex = (currentPresetIndex + dir + n) % n;
    repaint();
    // Editor's onFactoryPresetSelected loads the preset and pushes the authoritative name back
    // via setPresetDisplayName, so names aren't hard-coded here.
    if (onFactoryPresetSelected)
        onFactoryPresetSelected(currentPresetIndex);
}

void FaceplateView::drawPresetStrip(juce::Graphics& g)
{
    if (presetStripBounds.isEmpty())
        return;

    // Draw only the live elements (chevrons, name LED box, A/B) — the chassis PNG already
    // provides the brushed-metal footer recess/engraving behind these coords.
    const float h = (float)presetStripBounds.getHeight();

    // Engraved/LED-style preset name display (recessed dark track + text in gui.md engravedText).
    if (!presetNameBounds.isEmpty())
    {
        const auto nb = presetNameBounds.toFloat();
        g.setColour(juce::Colour(0xff0a0a0c).withAlpha(0.88f));
        g.fillRoundedRectangle(nb, 2.0f);
        g.setColour(juce::Colour(0xff000000).withAlpha(0.55f));
        g.drawRoundedRectangle(nb.reduced(0.5f), 2.0f, 0.6f);

        g.setColour(juce::Colour(0xffc9c6be)); // -- engravedText
        const float fontH = juce::jmax(8.0f, h * 0.52f);
        g.setFont(juce::Font(juce::FontOptions(fontH).withStyle("Bold")));
        const juce::String disp = currentPresetName.toUpperCase();
        g.drawText(disp, presetNameBounds, juce::Justification::centred, false);
    }

    // "Modified-from-preset" ember dot (Design spec) — shown only when the live cab/neural identity
    // diverges from the loaded voice. ~6px, radial ember (highlight at ~40%/35%), amber glow bloom.
    if (modified && !modifiedDotBounds.isEmpty())
    {
        const float dotD = juce::jmax(5.0f, h * 0.16f); // ~6px
        const auto centre = modifiedDotBounds.getCentre().toFloat();
        const juce::Rectangle<float> dot(centre.x - dotD * 0.5f, centre.y - dotD * 0.5f, dotD, dotD);

        // Amber glow bloom (Design: 0 0 7px rgba(255,150,30,.8)).
        const float glowR = dotD * 1.9f;
        juce::ColourGradient glow(juce::Colour(0xffff961e).withAlpha(0.80f), centre.x, centre.y,
                                  juce::Colour(0x00ff961e), centre.x + glowR, centre.y, true);
        g.setGradientFill(glow);
        g.fillEllipse(centre.x - glowR, centre.y - glowR, glowR * 2.0f, glowR * 2.0f);

        // Radial ember: #ffd28a highlight at ~40%/35% → #ff8800 → #a35d00 rim.
        const juce::Point<float> hl(dot.getX() + dotD * 0.40f, dot.getY() + dotD * 0.35f);
        juce::ColourGradient ember(juce::Colour(0xffffd28a), hl.x, hl.y, juce::Colour(0xffa35d00),
                                   hl.x + dotD * 0.6f, hl.y, true);
        ember.addColour(0.5, juce::Colour(0xffff8800));
        g.setGradientFill(ember);
        g.fillEllipse(dot);
    }

    // Chevrons (‹ ›) — clickable, tone-on-tone.
    g.setColour(juce::Colour(0xffd6d8db).withAlpha(0.85f));
    const float chevFont = juce::jmax(10.0f, h * 0.78f);
    g.setFont(juce::Font(juce::FontOptions(chevFont)));
    if (!chevLeftBounds.isEmpty())
        g.drawText(juce::String(juce::CharPointer_UTF8("\xe2\x80\xb9")), chevLeftBounds, juce::Justification::centred,
                   false); // ‹
    if (!chevRightBounds.isEmpty())
        g.drawText(juce::String(juce::CharPointer_UTF8("\xe2\x80\xba")), chevRightBounds, juce::Justification::centred,
                   false); // ›

    // A / B two-state compare buttons (lit amber when active).
    auto drawAB = [&](const juce::Rectangle<int>& b, char lab, bool isLit)
    {
        const auto bf = b.toFloat().reduced(1.0f);
        const auto fill = isLit ? juce::Colour(0xffaa5500) : juce::Colour(0xff232527);
        g.setColour(fill);
        g.fillRoundedRectangle(bf, 2.0f);
        g.setColour(juce::Colour(0xff111111).withAlpha(0.65f));
        g.drawRoundedRectangle(bf.reduced(0.3f), 2.0f, 0.7f);
        g.setColour(isLit ? juce::Colour(0xffffe0a0) : juce::Colour(0xffc9c6be));
        const float abFont = juce::jmax(7.0f, (float)b.getHeight() * 0.58f);
        g.setFont(juce::Font(juce::FontOptions(abFont).withStyle("Bold")));
        g.drawText(juce::String::charToString(lab), b, juce::Justification::centred, false);
    };
    drawAB(aButtonBounds, 'A', activeSlot == 'A');
    drawAB(bButtonBounds, 'B', activeSlot == 'B');
}

void FaceplateView::drawTransportTray(juce::Graphics& g)
{
    if (transportBounds.isEmpty())
        return;

    auto drawTBtn = [&](const juce::Rectangle<int>& b, const juce::String& label)
    {
        if (b.isEmpty())
            return;
        const auto bf = b.toFloat().reduced(1.0f);
        // Recessed button on the metal tray.
        g.setColour(juce::Colour(0xff0f0f11).withAlpha(0.82f));
        g.fillRoundedRectangle(bf, 3.0f);
        g.setColour(juce::Colour(0xff9aa0a3).withAlpha(0.35f));
        g.drawRoundedRectangle(bf.reduced(0.4f), 3.0f, 0.6f);
        // Label (affordance symbol or letter; small to fit 4 across 264 ref).
        g.setColour(juce::Colour(0xffc9c6be));
        const float tf = juce::jmax(6.0f, (float)b.getHeight() * 0.42f);
        g.setFont(juce::Font(juce::FontOptions(tf).withStyle("Bold")));
        g.drawText(label, b, juce::Justification::centred, false);
    };

    // Labels chosen for recognisability in tiny space; full file I/O later.
    drawTBtn(saveBtnBounds, "S");                                                  // save (to active slot)
    drawTBtn(loadBtnBounds, "L");                                                  // load (from active slot)
    drawTBtn(undoBtnBounds, juce::String(juce::CharPointer_UTF8("\xe2\x86\xba"))); // ↺ undo
    drawTBtn(randBtnBounds, "R");                                                  // randomize (dice affordance)
}
