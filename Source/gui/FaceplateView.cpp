/*
    FaceplateView.cpp — draws the embedded photoreal PRO chassis (960x612) and seats the
    hero filmstrip knob in the SATURATION well. Controls are placed in 960x612 reference
    coords and scaled to the actual size. Live instruments (transfer / harmonics / meters)
    are Phase 2.
*/
#include "FaceplateView.h"

#include "AboutContent.h"
#include "BinaryData.h"

FaceplateView::FaceplateView()
{
    faceplate = juce::ImageCache::getFromMemory(BinaryData::faceplate_pro_960x612_png,
                                                BinaryData::faceplate_pro_960x612_pngSize);

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

    addAndMakeVisible(ioMeter);
    addAndMakeVisible(statusLEDs);
    addAndMakeVisible(dbReadout);

    // Allow keyboard focus so the About/Licences modal can field Esc (focus is grabbed only while the modal is
    // open and given back on close, so it never steals focus during normal use). Mirrors RearPanelView.
    setWantsKeyboardFocus(true);

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
    // The footer bay is painted FIRST so the row's controls (and the lit rocker drawn later in
    // paintOverChildren) all read as seated inside one continuous machined trough.
    drawFooterBay(g);
    drawPresetStrip(g);
    drawTransportTray(g);

    // §3.4 bypass panel wash — cool blue-grey "offline" cue beneath the live layers (which render
    // on top at their own dim levels). Approximates the multiply wash from bypass-dimstate-tokens.json.
    if (bypassed)
    {
        g.setColour(juce::Colour(20, 30, 44).withAlpha(0.40f));
        g.fillRect(getLocalBounds());
    }
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

    // Authoritative anchors from Claude Design (faceplate-pro-anchors.json, 960x612 plate-truth, 2026-06-05).
    transferCurve.setBounds(place(76, 240, 184, 184));          // transfer_screen
    harmonicBars.setBounds(place(695, 186, 224, 128));          // harmonics_bars
    driveKnob.setBounds(place(480 - 104, 287 - 104, 208, 208)); // hero_knob_well, centre (480,287), ~208px

    // GemChip — small persistent pill centred below the knob, above the dB readout
    // (sits in the gap between the knob's bottom and the db_readout well [402,457,157,52]).
    gemChip.setBounds(place(417, 432, 126, 16));

    // Twin horizontal IN/OUT peak meters in the io_meters strip atop the chassis.
    ioMeter.setBounds(place(549, 40, 176, 42));

    // DRY/WET mix knob at the mix_knob anchor (bottom right of harmonics area).
    mixKnob.setBounds(place(726, 359, 134, 110));

    // POWER · SIG · CLIP dome LEDs. Plate-truth bezels: power [812,49,21], sig [858,49,21],
    // clip [897,49,21]; bounds span those three. StatusLEDs derives the centres internally.
    statusLEDs.setBounds(place(812, 49, 106, 21));

    // Drive -> dB readout in the recessed well below the hero knob.
    dbReadout.setBounds(place(402, 457, 157, 52));

    // Phase 2b: preset strip (center footer) + util_tray_transport (right). Anchors from
    // exports/faceplate-pro-anchors.json per PHASE2B_DESIGN_SPEC.md §3.
    presetStripBounds = place(133, 531, 494, 38);
    transportBounds = place(643, 526, 264, 48);

    // foot_bay: one continuous machined trough behind the whole transport row (rocker + preset
    // LCD + A/B + util buttons all seat inside it). Plate-truth rect [37,512,886,80]; the +12px
    // 612 extension gives the footer breathing room below.
    footerBayBounds = place(37, 512, 886, 80);

    // Bypass rocker (footer-left) + small About 'i' affordance (top-right corner).
    // 64x49 centred on the baked BYPASS dome at ref (59,567): x=59-32=27, y=567-24.5≈542.
    bypassRockerBounds = place(27, 542, 64, 49);
    aboutBtnBounds = place(930, 9, 20, 20);

    // PROVISIONAL flip-to-rear hit-target in the free bottom-right corner (right of the transport tray).
    // Final iced rose-gold corner affordance + exact placement land with Design's flip-spec.html.
    flipBtnBounds = place(916, 533, 42, 60);

    // Sub-rects inside presetStrip for interactive elements + name LED.
    {
        // Baked preset-strip wells measured on faceplate_pro_960x612.png (Design-confirmed, ref
        // coords): prev chevron (141,575), preset-name trough [165,556,369,38], next chevron
        // (559,575), A well (593,575), B well (617,575). Seat each overlay on its baked well (all
        // centred at y=575) instead of subdividing presetStripBounds — the old subdivision sat at
        // y≈550 (strip top 531) and floated the whole row ~25px above the baked wells.
        const int wy = 575; // baked well centre-y (ref)
        const int chevW = 22, chevH = 30;
        chevLeftBounds = place(141 - chevW / 2, wy - chevH / 2, chevW, chevH);
        presetNameBounds = place(165, 556, 369, 38);
        chevRightBounds = place(559 - chevW / 2, wy - chevH / 2, chevW, chevH);

        const int abW = 24, abH = 30;
        aButtonBounds = place(593 - abW / 2, wy - abH / 2, abW, abH);
        bButtonBounds = place(617 - abW / 2, wy - abH / 2, abW, abH);

        // "Modified" ember dot: ~6px against the preset name's right edge, vertically centred,
        // with an ~18px transparent hit-target (6px is too small to click).
        const int dotSize = 6;
        const int hit = 18;
        const int dotCx = presetNameBounds.getRight() - dotSize;
        const int dotCy = presetNameBounds.getCentreY();
        modifiedDotBounds = juce::Rectangle<int>(dotCx - hit / 2, dotCy - hit / 2, hit, hit);
    }

    // Sub-rects for the 4 transport affordance buttons (S L ↺ R).
    // The baked plate has 4 round "sphere" wells inside the footer trough.
    // Make the C++ hit-areas + label rects square (to match round wells) and evenly spaced
    // within the transport area. Precise centers will come from Design's measurement of the
    // baked wells; for now this aligns the labels/hit-targets much better than the old
    // wide rectangular subdivision and stops the visual offset.
    {
        // Baked transport domes measured from faceplate_pro_960x612.png (connected-components on
        // the footer trough): four round wells centred at ref-x {684,731,794,841}, ref-y ≈ 575.
        // The bake is NOT evenly spaced (2+3 grouped), so seat each label on its own dome centre
        // rather than subdividing transportBounds — that is what made the old squares float.
        const int domeCY = 575;
        const int domeR = 21; // ~42px square seats on the ~40px round well
        const int domeCX[4] = { 684, 731, 794, 841 };
        saveBtnBounds = place(domeCX[0] - domeR, domeCY - domeR, 2 * domeR, 2 * domeR);
        loadBtnBounds = place(domeCX[1] - domeR, domeCY - domeR, 2 * domeR, 2 * domeR);
        undoBtnBounds = place(domeCX[2] - domeR, domeCY - domeR, 2 * domeR, 2 * domeR);
        randBtnBounds = place(domeCX[3] - domeR, domeCY - domeR, 2 * domeR, 2 * domeR);
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

void FaceplateView::setBypassed(bool b)
{
    if (bypassed == b)
        return;
    bypassed = b;
    // §3.4 per-element de-energize: each lit/processing layer dims; meters keep moving (grey).
    statusLEDs.setBypassed(b);
    ioMeter.setBypassed(b);
    transferCurve.setBypassed(b);
    harmonicBars.setBypassed(b);
    dbReadout.setBypassed(b);
    knobLnf.setBypassed(b);
    driveKnob.repaint();
    mixKnob.setBypassed(b);
    repaint();
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

    // Licences scroll sits ON TOP of the About card — handled first so it captures clicks while open.
    // Its close affordance OR a click outside its panel dismisses it back to the About card.
    if (licencesVisible)
    {
        const auto panel = computeLicencesPanelBounds();
        const auto closeBox = juce::Rectangle<int>(panel.getRight() - 30, panel.getY() + 10, 20, 20);
        if (closeBox.contains(pos) || !panel.contains(pos))
        {
            licencesVisible = false;
            repaint();
        }
        return;
    }

    // About modal is top-most + captures all clicks while open. Geometry is recomputed
    // deterministically (same source as drawAboutPanel) so hit-testing never depends on a paint.
    if (aboutVisible)
    {
        const auto panel = computeAboutPanelBounds();
        const auto closeBox = juce::Rectangle<int>(panel.getRight() - 26, panel.getY() + 8, 18, 18);

        // "View full licences ▸" link → open the scrollable full-text licences modal.
        if (computeLicencesLinkBounds().contains(pos))
        {
            licencesVisible = true;
            licencesScrollY = 0;
            repaint();
            return;
        }

        if (closeBox.contains(pos) || !panel.contains(pos))
        {
            aboutVisible = false;
            setModalCaptureActive(false);  // restore child interaction
            giveAwayKeyboardFocus();       // release the focus grabbed when the modal opened
            repaint();
        }
        return;
    }

    // 'i' affordance opens the About panel. Make it truly modal: capture clicks (children stop intercepting,
    // so further clicks route to this mouseDown's modal hit-test) and grab focus so Esc reaches keyPressed.
    if (aboutBtnBounds.contains(pos))
    {
        aboutVisible = true;
        licencesScrollY = 0;
        setModalCaptureActive(true);
        grabKeyboardFocus();
        repaint();
        return;
    }

    // Bypass rocker toggles bypass; editor pushes the new state back via setBypassed().
    if (bypassRockerBounds.contains(pos))
    {
        if (onBypassToggled)
            onBypassToggled(!bypassed);
        return;
    }

    // Provisional flip-to-rear affordance (bottom-right corner) → swap to the service panel.
    if (flipBtnBounds.contains(pos))
    {
        if (onFlipToRear)
            onFlipToRear();
        return;
    }

    // "Modified" ember dot — only clickable while lit; reverts the live state to the loaded preset.
    if (modified && modifiedDotBounds.contains(pos))
    {
        if (onRevertToPreset)
            onRevertToPreset();
        return;
    }

    // Preset name LED click (menu), but only if not hitting the chevrons or modified dot inside the strip.
    if (presetNameBounds.contains(pos) && !chevLeftBounds.contains(pos) && !chevRightBounds.contains(pos) &&
        !modifiedDotBounds.contains(pos))
    {
        if (onPresetMenuRequested)
            onPresetMenuRequested();
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

bool FaceplateView::keyPressed(const juce::KeyPress& key)
{
    // Esc steps Licences -> About -> close (mirrors RearPanelView). Only acts while the modal is open.
    if (key == juce::KeyPress::escapeKey)
    {
        if (licencesVisible)
        {
            // Return to the still-open About card; capture + focus must STAY active.
            licencesVisible = false;
            repaint();
            return true;
        }
        if (aboutVisible)
        {
            aboutVisible = false;
            setModalCaptureActive(false); // restore child interaction
            giveAwayKeyboardFocus();
            repaint();
            return true;
        }
    }
    return juce::Component::keyPressed(key);
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

void FaceplateView::drawFooterBay(juce::Graphics& g)
{
    if (footerBayBounds.isEmpty())
        return;

    // One continuous machined trough for the whole transport row. Drawn before the per-control
    // faces (preset LCD, A/B slots, util buttons, rocker cap) so they read as seated inside it —
    // mirrors how the rear panel sits every control in a single well instead of three islands.
    //
    // Material recipe (CSS shadows translated to juce::Graphics):
    //   border-radius: 10px
    //   background: linear-gradient(rgba(0,0,0,.44) -> rgba(0,0,0,.20))   [top -> bottom]
    //   inset 0 0 0 1px rgba(0,0,0,.62)                                    [inner 1px stroke]
    //   inset top lip: 1px rgba(1,1,1,.05)                                 [inner top edge]
    //   inset 0 2px 9px rgba(0,0,0,.55)                                    [soft top inset shadow]
    const float s = (float)getHeight() / (float)kRefH; // vertical scale (matches place()'s sy)
    const auto bay = footerBayBounds.toFloat();
    const float radius = 10.0f * s; // 10px corner radius, scaled

    // Body: vertical dark gradient (top a touch darker than the bottom) → reads as a sunken trough.
    juce::ColourGradient body(juce::Colour::fromFloatRGBA(0.0f, 0.0f, 0.0f, 0.44f), bay.getX(), bay.getY(),
                              juce::Colour::fromFloatRGBA(0.0f, 0.0f, 0.0f, 0.20f), bay.getX(), bay.getBottom(), false);
    g.setGradientFill(body);
    g.fillRoundedRectangle(bay, radius);

    // Soft top inset shadow: a short vertical gradient band hugging the inside of the top edge
    // (emulates `inset 0 2px 9px` — ~2px offset / ~9px blur). Clipped to the rounded bay so it
    // never bleeds past the corners.
    {
        juce::Graphics::ScopedSaveState clip(g);
        juce::Path bayPath;
        bayPath.addRoundedRectangle(bay, radius);
        g.reduceClipRegion(bayPath);

        const float bandY = bay.getY() + 2.0f * s;      // ~2px inset offset
        const float bandH = juce::jmax(3.0f, 9.0f * s); // ~9px blur emulated as band height
        juce::ColourGradient inset(juce::Colour::fromFloatRGBA(0.0f, 0.0f, 0.0f, 0.55f), bay.getX(), bandY,
                                   juce::Colour::fromFloatRGBA(0.0f, 0.0f, 0.0f, 0.0f), bay.getX(), bandY + bandH,
                                   false);
        g.setGradientFill(inset);
        g.fillRect(bay.getX(), bandY, bay.getWidth(), bandH);
    }

    // Inner 1px stroke (the machined edge that catches the trough wall).
    g.setColour(juce::Colour::fromFloatRGBA(0.0f, 0.0f, 0.0f, 0.62f));
    g.drawRoundedRectangle(bay.reduced(0.5f), radius, 1.0f);

    // Top lip highlight: a faint 1px bright line just inside the top edge — the bevel where the
    // plate meets the recess catches light. Inset horizontally by the radius so it tracks the
    // straight run of the top edge, not the corners.
    g.setColour(juce::Colour::fromFloatRGBA(1.0f, 1.0f, 1.0f, 0.05f));
    const float lipY = bay.getY() + 1.0f;
    g.drawLine(bay.getX() + radius, lipY, bay.getRight() - radius, lipY, 1.0f);
}

void FaceplateView::drawPresetStrip(juce::Graphics& g)
{
    if (presetStripBounds.isEmpty())
        return;

    // Draw only the live CONTROL FACES (chevrons, name LED box, A/B slots). The unifying recess is
    // now drawFooterBay()'s single trough (plus the chassis PNG engraving) — this routine adds NO
    // strip-wide background fill, so the controls seat inside the one bay (no nested recess).
    const float h = (float)presetStripBounds.getHeight();

    // Engraved/LED-style preset name display. The baked plate supplies the recessed name trough;
    // C++ draws ONLY the live engraved text + caret (no body fill/border, which would float a dark
    // box over the baked well — the "pasted-on" bug).
    if (!presetNameBounds.isEmpty())
    {
        g.setColour(juce::Colour(0xffc9c6be)); // -- engravedText
        const float fontH = juce::jmax(8.0f, h * 0.52f);
        g.setFont(juce::Font(juce::FontOptions(fontH).withStyle("Bold")));
        const juce::String disp = currentPresetName.toUpperCase();
        g.drawText(disp, presetNameBounds, juce::Justification::centred, false);

        // Tiny ▾ caret to the right of the name to signal "click for menu" (per spec).
        if (!presetNameBounds.isEmpty())
        {
            const float caretH = fontH * 0.6f;
            const auto caretR =
                presetNameBounds.toFloat().withLeft(presetNameBounds.getRight() - caretH * 1.2f).withWidth(caretH);
            g.setColour(juce::Colour(0xffc9c6be).withAlpha(0.7f));
            g.setFont(juce::Font(juce::FontOptions(caretH * 0.9f)));
            g.drawText(juce::String(juce::CharPointer_UTF8("\xe2\x96\xbe")), caretR, juce::Justification::centred,
                       false);
        }
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
        juce::ColourGradient ember(juce::Colour(0xffffd28a), hl.x, hl.y, juce::Colour(0xffa35d00), hl.x + dotD * 0.6f,
                                   hl.y, true);
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

    // A / B two-state compare buttons.
    // The baked plate supplies the visual wells/squares for A/B (and the preset name LED area).
    // C++ draws only the letters (no more floating button bodies). Lit state via brighter text.
    auto drawAB = [&](const juce::Rectangle<int>& b, char lab, bool isLit)
    {
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

    // The baked plate provides the round "sphere" wells for the 4 transport buttons (S L ↺ R)
    // inside the continuous machined footer trough (drawn by drawFooterBay).
    // C++ must NOT draw its own button bodies (no more floating dark rounded rects over the baked wells).
    // We only draw the labels, centered on the baked wells.
    // The individual *BtnBounds (set in resized()) must be aligned to the baked well centers/sizes.
    auto drawTBtn = [&](const juce::Rectangle<int>& b, const juce::String& label)
    {
        if (b.isEmpty())
            return;
        // No body fill or stroke — the baked plate supplies the round well visual.
        // Label only (small bold engraved style to read on the dark well).
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

//==============================================================================
// While the front About/Licences modal is open, the modal is only drawn (in paintOverChildren) — clicks would
// otherwise still hit the live children below it. Toggle interception off on every interactive child so clicks
// fall through to FaceplateView::mouseDown (whose modal hit-test handles close/click-outside/View-licences),
// and restore it on close. The MixKnob's inner slider is a grandchild, so it must be suppressed explicitly too.
void FaceplateView::setModalCaptureActive(bool active)
{
    const bool clickable = !active;
    driveKnob.setInterceptsMouseClicks(clickable, clickable);
    mixKnob.setInterceptsMouseClicks(clickable, clickable);
    mixKnob.getSlider().setInterceptsMouseClicks(clickable, clickable);
    gemChip.setInterceptsMouseClicks(clickable, clickable);
    // Non-interactive scopes/meters today, but future-proof + harmless (they never consume clicks anyway).
    transferCurve.setInterceptsMouseClicks(clickable, clickable);
    harmonicBars.setInterceptsMouseClicks(clickable, clickable);
    ioMeter.setInterceptsMouseClicks(clickable, clickable);
    statusLEDs.setInterceptsMouseClicks(clickable, clickable);
    dbReadout.setInterceptsMouseClicks(clickable, clickable);
}

//==============================================================================
// §6 P1 bypass dim-state + §6 P2 About panel. Drawn above all child components via
// paintOverChildren so the dim veil actually dims the live scopes/meters/knob.

void FaceplateView::paintOverChildren(juce::Graphics& g)
{
    // §3.4 dim-state is now per-element (children dim themselves; the panel wash is in paint()).
    // Here we only cool the hero stone — a filmstrip can't be desaturated in place, so a cool
    // overlay over the knob well reads it as a cold, lifeless stone.
    if (bypassed)
    {
        g.setColour(juce::Colour(20, 30, 44).withAlpha(0.45f));
        g.fillEllipse(driveKnob.getBounds().toFloat());
    }

    // Rocker stays crisp + lit on top — it's the control to re-energize.
    drawBypassRocker(g);
    drawAboutButton(g);
    drawFlipButton(g);

    // About modal sits above everything (including the dim veil).
    if (aboutVisible)
        drawAboutPanel(g);

    // Full third-party licences scroll sits above the About card.
    if (licencesVisible)
        drawLicencesPanel(g);
}

void FaceplateView::drawBypassRocker(juce::Graphics& g)
{
    if (bypassRockerBounds.isEmpty())
        return;

    const auto b = bypassRockerBounds.toFloat().reduced(2.0f);

    // The baked plate supplies the rocker dome/well. C++ draws ONLY the live overlays (reflective
    // top-lip glint + the lit/dark telltale dot + the state-keyed BYPASS label). The cap body fill
    // and border were removed: drawn over the baked well they read as a dark shape floating above
    // it (the "pasted-on" bug). Centred on the baked dome via bypassRockerBounds.
    // 1px top-lip glint where the recess catches light (pure reflective highlight, not a control face).
    g.setColour(juce::Colour::fromFloatRGBA(1.0f, 1.0f, 1.0f, 0.06f));
    g.drawLine(b.getX() + 4.0f, b.getY() + 1.0f, b.getRight() - 4.0f, b.getY() + 1.0f, 1.0f);

    // Telltale: a small amber light seated ON the baked dome (rocker centre) when BYPASSED. When
    // engaged we draw NOTHING — the baked dome IS the engaged visual; a second filled ellipse over
    // it produced the doubled-dome "snowman" floating bug (it was offset to y=getY()+h*0.34).
    if (bypassed)
    {
        const float dotR = juce::jmin(b.getWidth(), b.getHeight()) * 0.14f;
        const auto dot = juce::Rectangle<float>(dotR * 2.0f, dotR * 2.0f)
                             .withCentre({ b.getCentreX(), b.getCentreY() });
        g.setColour(juce::Colour(0xffffa726).withAlpha(0.30f));
        g.fillEllipse(dot.expanded(dotR * 0.9f));
        g.setColour(juce::Colour(0xffffb74d));
        g.fillEllipse(dot);
    }

    g.setColour(bypassed ? juce::Colour(0xffffd9a0) : juce::Colour(0xff9aa0a3));
    const float f = juce::jmax(7.0f, b.getHeight() * 0.20f);
    g.setFont(juce::Font(juce::FontOptions(f).withStyle("Bold")));
    auto labelArea = bypassRockerBounds;
    g.drawText("BYPASS", labelArea.removeFromBottom(juce::roundToInt(b.getHeight() * 0.34f)),
               juce::Justification::centred, false);
}

void FaceplateView::drawAboutButton(juce::Graphics& g)
{
    if (aboutBtnBounds.isEmpty())
        return;

    const auto b = aboutBtnBounds.toFloat().reduced(1.0f);
    g.setColour(juce::Colour(0xff0d0e10).withAlpha(0.55f));
    g.fillEllipse(b);
    g.setColour(juce::Colour(0xff9aa0a3).withAlpha(aboutVisible ? 0.9f : 0.5f));
    g.drawEllipse(b.reduced(0.5f), 1.0f);
    g.setColour(juce::Colour(0xffc9c6be).withAlpha(aboutVisible ? 1.0f : 0.7f));
    g.setFont(juce::Font(juce::FontOptions(b.getHeight() * 0.72f).withStyle("Bold Italic")));
    g.drawText("i", aboutBtnBounds, juce::Justification::centred, false);
}

void FaceplateView::drawFlipButton(juce::Graphics& g)
{
    // PROVISIONAL placement [916,533,42,60], pending Design's flip-spec.html. C++ draws ONLY the live
    // overlays (⟳ glyph + "FLIP" label). The pad body fill + hairline border were removed: drawn with
    // no measured baked well they floated a dark shape in the corner (the "pasted-on" bug). Once
    // flip-spec.html lands, seat these overlays on the baked corner affordance's measured centre.
    if (flipBtnBounds.isEmpty())
        return;

    auto b = flipBtnBounds.toFloat().reduced(3.0f);

    const juce::Colour amber = bypassed ? juce::Colour(0xffa8632a) : juce::Colour(0xffe9b06a);
    g.setColour(amber);
    auto glyph = b.removeFromTop(b.getHeight() * 0.62f);
    g.setFont(juce::Font(juce::FontOptions(glyph.getHeight() * 0.9f).withStyle("Bold")));
    g.drawText(juce::String(juce::CharPointer_UTF8("\xe2\x86\xbb")), glyph, juce::Justification::centred, false); // ⟳
    g.setFont(juce::Font(juce::FontOptions(juce::jmax(7.0f, b.getHeight() * 0.62f)).withStyle("Bold")));
    g.drawText("FLIP", b, juce::Justification::centred, false);
}

juce::Rectangle<int> FaceplateView::computeAboutPanelBounds() const
{
    const int w = juce::jmin(580, getWidth() - 60); // ~60% of the 960 face, per Design §3
    const int h = juce::jmin(360, getHeight() - 90);
    return juce::Rectangle<int>(0, 0, juce::jmax(120, w), juce::jmax(120, h)).withCentre(getLocalBounds().getCentre());
}

void FaceplateView::drawAboutPanel(juce::Graphics& g)
{
    // Scrim behind the modal.
    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.fillRect(getLocalBounds());

    const auto panel = computeAboutPanelBounds();
    const auto pf = panel.toFloat();
    g.setColour(juce::Colour(0xff17181b));
    g.fillRoundedRectangle(pf, 8.0f);
    g.setColour(juce::Colour(0xffaa5500).withAlpha(0.6f)); // amber accent edge
    g.drawRoundedRectangle(pf.reduced(0.5f), 8.0f, 1.2f);

    // Close box (top-right) — geometry mirrors mouseDown's hit-test exactly.
    const auto closeBox = juce::Rectangle<int>(panel.getRight() - 26, panel.getY() + 8, 18, 18);
    g.setColour(juce::Colour(0xff9aa0a3));
    g.setFont(juce::Font(juce::FontOptions(16.0f).withStyle("Bold")));
    g.drawText(juce::String(juce::CharPointer_UTF8("\xc3\x97")), closeBox, juce::Justification::centred, false); // ×

    auto inner = panel.reduced(24);

    const juce::String ver = wk::about::versionString();

    auto line = inner;
    g.setColour(juce::Colour(0xfff0ede4));
    g.setFont(juce::Font(juce::FontOptions(24.0f).withStyle("Bold")));
    g.drawText("WON-KNOBBER  " + ver, line.removeFromTop(34), juce::Justification::topLeft, false);

    g.setColour(juce::Colour(0xff9aa0a3));
    g.setFont(juce::Font(juce::FontOptions(13.0f)));
    g.drawText("Photoreal one-knob saturation", line.removeFromTop(22), juce::Justification::topLeft, false);

    // Credit block — verbatim per Design §3, single-sourced in wk::about (legally precise, NOT
    // paraphrased). Two-license picture: the plugin is PolyForm-NC, the Airwindows core is MIT.
    const juce::String arrow = juce::String(juce::CharPointer_UTF8("\xe2\x96\xb8")); // ▸
    const juce::String credit = wk::about::creditBlockText();

    // Hairline rule above the credit block (per §3 layout).
    line.removeFromTop(8);
    g.setColour(juce::Colour(0xff3a3d3e));
    g.drawLine((float)line.getX(), (float)line.getY(), (float)line.getRight(), (float)line.getY(), 1.0f);
    line.removeFromTop(10);

    g.setColour(juce::Colour(0xffc9c6be));
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.drawMultiLineText(credit, line.getX(), line.getY() + 12, line.getWidth(), juce::Justification::topLeft);

    // Single amber "View full licences" link → opens the full THIRD_PARTY_LICENSES.md scroll.
    // The hit-target is computed deterministically via computeLicencesLinkBounds() (same geometry).
    g.setColour(juce::Colour(0xffffb74d));
    g.setFont(juce::Font(juce::FontOptions(12.0f).withStyle("Bold")));
    g.drawText("View full licences " + arrow, computeLicencesLinkBounds(), juce::Justification::bottomLeft, false);
}

juce::Rectangle<int> FaceplateView::computeLicencesLinkBounds() const
{
    // Mirrors the bottom strip used by drawAboutPanel: inner = panel.reduced(24), and the link
    // occupies the bottom 20px of that inner rect (only vertical removeFrom* mutate `line`, so the
    // horizontal extent stays equal to inner's). Recomputed deterministically — never paint-dependent.
    const auto inner = computeAboutPanelBounds().reduced(24);
    return juce::Rectangle<int>(inner.getX(), inner.getBottom() - 20, inner.getWidth(), 20);
}

juce::Rectangle<int> FaceplateView::computeLicencesPanelBounds() const
{
    // Wider + taller than the About card so the full MIT text has room before it scrolls.
    const int w = juce::jmin(680, getWidth() - 40); // ~70% of the 960 face
    const int h = juce::jmin(480, getHeight() - 50);
    return juce::Rectangle<int>(0, 0, juce::jmax(140, w), juce::jmax(160, h)).withCentre(getLocalBounds().getCentre());
}

void FaceplateView::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (licencesVisible)
    {
        // Scroll the licence body; deltaY > 0 means wheel-up (content moves down → reduce offset).
        const int step = juce::roundToInt(wheel.deltaY * -120.0f);
        licencesScrollY = juce::jlimit(0, licencesMaxScrollY, licencesScrollY + step);
        repaint();
        return;
    }

    juce::Component::mouseWheelMove(e, wheel);
}

void FaceplateView::drawLicencesPanel(juce::Graphics& g)
{
    // Scrim behind the licences modal (a touch darker than the About scrim since it layers on top).
    g.setColour(juce::Colours::black.withAlpha(0.72f));
    g.fillRect(getLocalBounds());

    const auto panel = computeLicencesPanelBounds();
    const auto pf = panel.toFloat();
    g.setColour(juce::Colour(0xff131417));
    g.fillRoundedRectangle(pf, 8.0f);
    g.setColour(juce::Colour(0xffaa5500).withAlpha(0.6f)); // amber accent edge
    g.drawRoundedRectangle(pf.reduced(0.5f), 8.0f, 1.2f);

    // Title.
    auto header = panel.reduced(24, 18);
    auto titleRow = header.removeFromTop(28);
    g.setColour(juce::Colour(0xfff0ede4));
    g.setFont(juce::Font(juce::FontOptions(20.0f).withStyle("Bold")));
    g.drawText("Third-party licences", titleRow, juce::Justification::topLeft, false);

    // Close box (top-right) — geometry mirrors mouseDown's hit-test exactly.
    const auto closeBox = juce::Rectangle<int>(panel.getRight() - 30, panel.getY() + 10, 20, 20);
    g.setColour(juce::Colour(0xff9aa0a3));
    g.setFont(juce::Font(juce::FontOptions(18.0f).withStyle("Bold")));
    g.drawText(juce::String(juce::CharPointer_UTF8("\xc3\x97")), closeBox, juce::Justification::centred, false); // ×

    // Hairline rule under the title.
    header.removeFromTop(8);
    g.setColour(juce::Colour(0xff3a3d3e));
    g.drawLine((float)header.getX(), (float)header.getY(), (float)header.getRight(), (float)header.getY(), 1.0f);
    header.removeFromTop(10);

    // Scrollable body region: clip to it, then draw the (offset) full licence text.
    const auto body = header;
    const float bodyFont = 12.0f;
    const float lineH = bodyFont * 1.4f;

    // Full Airwindows MIT licence, verbatim from THIRD_PARTY_LICENSES.md, plus the per-asset note.
    const juce::String licenceBody = getLicencesBodyText();

    // Word-wrap the body to the body width so we can measure its true rendered height for clamping.
    juce::GlyphArrangement glyphs;
    glyphs.addJustifiedText(juce::Font(juce::FontOptions(bodyFont)), licenceBody, (float)body.getX(),
                            (float)body.getY() + bodyFont, (float)body.getWidth(), juce::Justification::topLeft);
    const float textBottom = glyphs.getBoundingBox(0, -1, true).getBottom();
    const int contentH = juce::jmax(0, juce::roundToInt(textBottom - (float)body.getY()) + juce::roundToInt(lineH));

    licencesMaxScrollY = juce::jmax(0, contentH - body.getHeight());
    licencesScrollY = juce::jlimit(0, licencesMaxScrollY, licencesScrollY);

    {
        juce::Graphics::ScopedSaveState clip(g);
        g.reduceClipRegion(body);
        g.setColour(juce::Colour(0xffc9c6be));
        g.setFont(juce::Font(juce::FontOptions(bodyFont)));
        g.drawMultiLineText(licenceBody, body.getX(), body.getY() + juce::roundToInt(bodyFont) - licencesScrollY,
                            body.getWidth(), juce::Justification::topLeft);
    }

    // Scroll affordance: a slim track + thumb on the right edge when the content overflows.
    if (licencesMaxScrollY > 0)
    {
        const int trackW = 4;
        const auto track = juce::Rectangle<int>(body.getRight() - trackW, body.getY(), trackW, body.getHeight());
        g.setColour(juce::Colour(0xff2a2c2e));
        g.fillRoundedRectangle(track.toFloat(), 2.0f);

        const float frac = (float)body.getHeight() / (float)(body.getHeight() + licencesMaxScrollY);
        const int thumbH = juce::jmax(24, juce::roundToInt((float)body.getHeight() * frac));
        const float scrolled = (float)licencesScrollY / (float)licencesMaxScrollY;
        const int thumbY = body.getY() + juce::roundToInt(scrolled * (float)(body.getHeight() - thumbH));
        g.setColour(juce::Colour(0xff9aa0a3).withAlpha(0.85f));
        g.fillRoundedRectangle(juce::Rectangle<int>(track.getX(), thumbY, trackW, thumbH).toFloat(), 2.0f);
    }
}

juce::String FaceplateView::getLicencesBodyText() const
{
    // Single-sourced verbatim third-party licence body (Airwindows MIT + RTNeural BSD-3 + framework
    // note + per-IR placeholder). Thin forwarder so the rear modal renders byte-identical legal text.
    return wk::about::licencesBodyText();
}
