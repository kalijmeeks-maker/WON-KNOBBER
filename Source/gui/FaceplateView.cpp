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

    // Bypass rocker (footer-left) + small About 'i' affordance (top-right corner).
    bypassRockerBounds = place(41, 531, 64, 49);
    aboutBtnBounds = place(930, 9, 20, 20);

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
            repaint();
        }
        return;
    }

    // 'i' affordance opens the About panel.
    if (aboutBtnBounds.contains(pos))
    {
        aboutVisible = true;
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

    // Recessed metal body (shares the transport-button palette).
    g.setColour(juce::Colour(0xff0f0f11).withAlpha(0.85f));
    g.fillRoundedRectangle(b, 4.0f);
    g.setColour(juce::Colour(0xff9aa0a3).withAlpha(0.35f));
    g.drawRoundedRectangle(b.reduced(0.5f), 4.0f, 0.7f);

    // Telltale dot: amber glow when BYPASSED (warns the signal is dry); dark when engaged.
    const float dotR = juce::jmin(b.getWidth(), b.getHeight()) * 0.20f;
    const juce::Point<float> dotC(b.getCentreX(), b.getY() + b.getHeight() * 0.34f);
    const auto dot = juce::Rectangle<float>(dotR * 2.0f, dotR * 2.0f).withCentre(dotC);

    if (bypassed)
    {
        g.setColour(juce::Colour(0xffffa726).withAlpha(0.30f));
        g.fillEllipse(dot.expanded(dotR * 0.9f));
        g.setColour(juce::Colour(0xffffb74d));
        g.fillEllipse(dot);
    }
    else
    {
        g.setColour(juce::Colour(0xff2a2c2e));
        g.fillEllipse(dot);
        g.setColour(juce::Colour(0xff000000).withAlpha(0.5f));
        g.drawEllipse(dot, 0.6f);
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

#ifdef JucePlugin_VersionString
    const juce::String ver = "v" JucePlugin_VersionString;
#else
    const juce::String ver = "v0.1.0";
#endif

    auto line = inner;
    g.setColour(juce::Colour(0xfff0ede4));
    g.setFont(juce::Font(juce::FontOptions(24.0f).withStyle("Bold")));
    g.drawText("WON-KNOBBER  " + ver, line.removeFromTop(34), juce::Justification::topLeft, false);

    g.setColour(juce::Colour(0xff9aa0a3));
    g.setFont(juce::Font(juce::FontOptions(13.0f)));
    g.drawText("Photoreal one-knob saturation", line.removeFromTop(22), juce::Justification::topLeft, false);

    // Credit block — verbatim per Design §3 (legally precise; hardcoded, NOT paraphrased).
    // Two-license picture: the plugin is PolyForm-NC, the Airwindows core is MIT — both appear.
    const juce::String copy = juce::String(juce::CharPointer_UTF8("\xc2\xa9"));      // ©
    const juce::String dash = juce::String(juce::CharPointer_UTF8("\xe2\x80\x94"));  // —
    const juce::String mid = juce::String(juce::CharPointer_UTF8("\xc2\xb7"));       // ·
    const juce::String tm = juce::String(juce::CharPointer_UTF8("\xe2\x84\xa2"));    // ™
    const juce::String arrow = juce::String(juce::CharPointer_UTF8("\xe2\x96\xb8")); // ▸

    juce::String credit;
    credit << "Saturation core derived from Airwindows " << dash << " " << copy
           << " 2018 Chris Johnson, used under the MIT licence.\n"
           << "Cabinet impulse responses under their respective licences (see notices). Neural models " << copy
           << " Kali Meeks.\n"
           << "WON KNOBBER " << copy << " 2026 Kali Meeks " << mid
           << " PolyForm Noncommercial 1.0.0. Built with JUCE 8. VST3" << tm << " Steinberg Media Technologies.";

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
    const int w = juce::jmin(680, getWidth() - 40);  // ~70% of the 960 face
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
    // Hardcoded static text (matches how drawAboutPanel builds its credit block). The MIT licence
    // below is copied VERBATIM from THIRD_PARTY_LICENSES.md; do not paraphrase. The trailing note
    // covers per-IR / per-model notices, which are appended as those assets ship.
    juce::String t;
    t << "Airwindows (saturation algorithms)\n"
      << "\n"
      << "The saturation transfer functions in Source/dsp/AirwindowsShapers.h\n"
      << "(Density3, Mojo, Spiral2 presence, PurestSaturation) are derived from Airwindows.\n"
      << "\n"
      << "Source: https://github.com/airwindows/airwindows\n"
      << "Copyright (c) 2018 Chris Johnson\n"
      << "License: MIT\n"
      << "\n"
      << "MIT License\n"
      << "\n"
      << "Copyright (c) 2018 Chris Johnson\n"
      << "\n"
      << "Permission is hereby granted, free of charge, to any person obtaining a copy\n"
      << "of this software and associated documentation files (the \"Software\"), to deal\n"
      << "in the Software without restriction, including without limitation the rights\n"
      << "to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\n"
      << "copies of the Software, and to permit persons to whom the Software is\n"
      << "furnished to do so, subject to the following conditions:\n"
      << "\n"
      << "The above copyright notice and this permission notice shall be included in all\n"
      << "copies or substantial portions of the Software.\n"
      << "\n"
      << "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
      << "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
      << "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n"
      << "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n"
      << "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n"
      << "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\n"
      << "SOFTWARE.\n"
      << "\n"
      << "Per-IR and per-model notices are added here as those assets ship.";
    return t;
}
