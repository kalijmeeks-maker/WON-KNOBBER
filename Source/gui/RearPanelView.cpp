/*
    RearPanelView.cpp — see header. Paints live cab/neural state over the baked rose/amber rear plate
    and routes selection changes back to the processor (the rear-fold "override").
*/
#include "RearPanelView.h"

#include "AboutContent.h"
#include "BinaryData.h"

namespace
{
// Hultog Engraved — the rear well/readout face (matches the front dB readout + preset strip).
// Typeface is built once (static) from BinaryData; the Font wraps it at the requested height each call.
juce::Font hultogFont(float height)
{
    static const juce::Typeface::Ptr tf =
        juce::Typeface::createSystemTypefaceFor(BinaryData::Hultog_Engraved_ttf, BinaryData::Hultog_Engraved_ttfSize);
    return juce::Font(juce::FontOptions(tf)).withHeight(height);
}
} // namespace

RearPanelView::RearPanelView()
{
    rearPlate = juce::ImageCache::getFromMemory(BinaryData::rearpanelbackground960x612_png,
                                                BinaryData::rearpanelbackground960x612_pngSize);

    // Allow keyboard focus so the About/Licences modal can field Esc (focus is grabbed only while
    // the modal is open and given back on close, so it never steals focus during normal use).
    setWantsKeyboardFocus(true);
}

const juce::StringArray& RearPanelView::cabIds()
{
    static const juce::StringArray ids{"FLAT",        "STUDIO_RIBBON", "VINTAGE_4X12",
                                       "CONSOLE_BOX", "OLD_RADIO",     "IRON_CORE"};
    return ids;
}

const juce::StringArray& RearPanelView::neuralIds()
{
    static const juce::StringArray ids{"NONE", "TAPE", "VALVE", "TRANSISTOR", "IRON"};
    return ids;
}

juce::String RearPanelView::cabDisplay(const juce::String& id)
{
    if (id == "STUDIO_RIBBON")
        return "STUDIO RIBBON";
    if (id == "VINTAGE_4X12")
        return "VINTAGE 4x12";
    if (id == "CONSOLE_BOX")
        return "CONSOLE BOX";
    if (id == "OLD_RADIO")
        return "OLD RADIO";
    if (id == "IRON_CORE")
        return "IRON CORE";
    return "FLAT";
}

juce::String RearPanelView::neuralDisplay(const juce::String& id)
{
    if (id == "TAPE")
        return "TAPE-1971";
    if (id == "VALVE")
        return "VALVE-CLASS A";
    if (id == "TRANSISTOR")
        return "TRANSISTOR-FET";
    if (id == "IRON")
        return "IRON-TRANSFORMER";
    return "NONE";
}

juce::String RearPanelView::cycle(const juce::StringArray& ids, const juce::String& current, int dir)
{
    const int n = ids.size();
    if (n == 0)
        return current;
    int idx = juce::jmax(0, ids.indexOf(current));
    idx = ((idx + dir) % n + n) % n;
    return ids[idx];
}

void RearPanelView::setCabState(const juce::String& id, bool engaged)
{
    if (id == cabIrId && engaged == cabEngaged)
        return;
    cabIrId = id;
    cabEngaged = engaged;
    repaint();
}

void RearPanelView::setNeuralState(const juce::String& id, bool engaged)
{
    if (id == neuralModelId && engaged == neuralEngaged)
        return;
    neuralModelId = id;
    neuralEngaged = engaged;
    repaint();
}

void RearPanelView::setBypassed(bool b)
{
    if (b == bypassed)
        return;
    bypassed = b;
    repaint();
}

void RearPanelView::openAboutModal(bool showLicences)
{
    aboutVisible = true;
    licencesVisible = showLicences;
    licencesScrollY = 0;
    repaint();
}

void RearPanelView::resized()
{
    const float sx = (float)getWidth() / (float)kRefW;
    const float sy = (float)getHeight() / (float)kRefH;
    auto place = [sx, sy](int rx, int ry, int rw, int rh)
    {
        return juce::Rectangle<int>(juce::roundToInt((float)rx * sx), juce::roundToInt((float)ry * sy),
                                    juce::roundToInt((float)rw * sx), juce::roundToInt((float)rh * sy));
    };

    // Authoritative rear anchors (docs/rear-panel-anchors.json, 960x612).
    cabEngageRockerBounds = place(59, 168, 64, 30);
    cabEngageLedBounds = place(135, 177, 12, 12);
    cabIrWellBounds = place(59, 233, 263, 48);
    neuralEngageRockerBounds = place(638, 168, 64, 30);
    neuralEngageLedBounds = place(714, 177, 12, 12);
    neuralModelWellBounds = place(638, 233, 263, 47);
    // Full flip_hero_well is the hit-target (FLIP label + SAT->CAB->NEURAL readout all read as one
    // control) — no dead rose-gold ring around the medallion.
    flipMedallionBounds = place(386, 170, 188, 188);

    // Baked ABOUT text link in the io_trim_strip [217,499,423,41]. The engraved "ABOUT" glyphs span
    // ~x497-536; this rect covers them with margin and stops ~19px short of "MANUAL" (~x569-620), which
    // stays an unwired baked placeholder (clicks on MANUAL must NOT open the modal).
    aboutLinkBounds = place(488, 499, 62, 41);

    const int chev = juce::jmax(20, cabIrWellBounds.getHeight());
    cabPrevBounds = cabIrWellBounds.withWidth(chev);
    cabNextBounds = cabIrWellBounds.withWidth(chev).withRightX(cabIrWellBounds.getRight());
    neuralPrevBounds = neuralModelWellBounds.withWidth(chev);
    neuralNextBounds = neuralModelWellBounds.withWidth(chev).withRightX(neuralModelWellBounds.getRight());
}

void RearPanelView::drawEngage(juce::Graphics& g, juce::Rectangle<int> rocker, juce::Rectangle<int> led, bool on) const
{
    // Rocker body (overdraws the baked rocker so the live cap position is authoritative).
    const auto rb = rocker.toFloat().reduced(1.0f);
    g.setColour(juce::Colour(0xff0f0f11).withAlpha(0.9f));
    g.fillRoundedRectangle(rb, 4.0f);
    g.setColour(juce::Colour(0xff9aa0a3).withAlpha(0.35f));
    g.drawRoundedRectangle(rb.reduced(0.5f), 4.0f, 0.7f);

    // Cap slides right + warms when ON; left + cool when OFF.
    auto cap = rb.reduced(2.0f).withWidth(rb.getWidth() * 0.5f - 2.0f);
    if (on)
        cap = cap.withX(rb.getCentreX());
    const juce::Colour capCol =
        on ? (bypassed ? juce::Colour(0xffa8632a) : juce::Colour(0xffFE9A00)) : juce::Colour(0xff5a6066);
    g.setColour(capCol);
    g.fillRoundedRectangle(cap, 3.0f);

    // Engage LED.
    const auto lb = led.toFloat();
    juce::Colour ledCol;
    if (!on)
        ledCol = juce::Colour(0xff2a2c2e); // dark
    else
        ledCol = bypassed ? juce::Colour(0xffa8632a) : juce::Colour(0xffFE9A00);
    if (on && !bypassed)
    {
        g.setColour(ledCol.withAlpha(0.35f));
        g.fillEllipse(lb.expanded(3.0f)); // glow halo
    }
    g.setColour(ledCol);
    g.fillEllipse(lb);
}

void RearPanelView::drawWell(juce::Graphics& g, juce::Rectangle<int> well, juce::Rectangle<int> prev,
                             juce::Rectangle<int> next, const juce::String& display) const
{
    // Recessed interior overdraws the baked default name so the live selection is authoritative.
    const auto wb = well.toFloat().reduced(1.0f);
    g.setColour(juce::Colour(0xff17120e));
    g.fillRoundedRectangle(wb, 4.0f);
    g.setColour(juce::Colour(0xff3a2f22).withAlpha(0.8f));
    g.drawRoundedRectangle(wb.reduced(0.5f), 4.0f, 0.8f);

    const juce::Colour amber = bypassed ? juce::Colour(0xffa8632a) : juce::Colour(0xffeaa24c);
    const juce::Colour chevCol = bypassed ? juce::Colour(0xff5a4a38) : juce::Colour(0xff9aa0a3);

    // Selection name centred between the chevrons (Hultog Engraved — matches the front readout/strip).
    g.setColour(amber);
    g.setFont(hultogFont((float)juce::jmin(23, well.getHeight() - 16)));
    auto textArea = well.withTrimmedLeft(prev.getWidth()).withTrimmedRight(next.getWidth());
    g.drawText(display, textArea, juce::Justification::centred, false);

    // Prev/next chevrons.
    g.setColour(chevCol);
    g.setFont(juce::Font(juce::FontOptions((float)juce::jmin(20, well.getHeight() - 18)).withStyle("Bold")));
    g.drawText(juce::String(juce::CharPointer_UTF8("\xe2\x80\xb9")), prev, juce::Justification::centred, false); // ‹
    g.drawText(juce::String(juce::CharPointer_UTF8("\xe2\x80\xba")), next, juce::Justification::centred, false); // ›
}

void RearPanelView::paint(juce::Graphics& g)
{
    if (rearPlate.isValid())
    {
        g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
        g.drawImage(rearPlate, getLocalBounds().toFloat(), juce::RectanglePlacement::stretchToFit);
    }
    else
    {
        g.fillAll(juce::Colour(0xff141114));
    }

    drawEngage(g, cabEngageRockerBounds, cabEngageLedBounds, cabEngaged);
    drawEngage(g, neuralEngageRockerBounds, neuralEngageLedBounds, neuralEngaged);
    drawWell(g, cabIrWellBounds, cabPrevBounds, cabNextBounds, cabDisplay(cabIrId));
    drawWell(g, neuralModelWellBounds, neuralPrevBounds, neuralNextBounds, neuralDisplay(neuralModelId));

    // §3 bypass wash (rear block of bypass-dimstate-tokens.json): cool offline veil reused from the front.
    if (bypassed)
    {
        g.setColour(juce::Colour(20, 30, 44).withAlpha(0.40f));
        g.fillRect(getLocalBounds());
    }
}

void RearPanelView::mouseDown(const juce::MouseEvent& e)
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
            giveAwayKeyboardFocus(); // release the focus grabbed when the modal opened
            repaint();
        }
        return;
    }

    // Baked ABOUT text link (right end of the io_trim_strip) opens the About card OVER the rear face.
    // MANUAL (to its right) is intentionally NOT hit-tested — it stays an unwired baked placeholder.
    if (aboutLinkBounds.contains(pos))
    {
        aboutVisible = true;
        licencesScrollY = 0;
        grabKeyboardFocus(); // so Esc reaches keyPressed while the modal is open
        repaint();
        return;
    }

    if (flipMedallionBounds.contains(pos))
    {
        if (onFlipToFront)
            onFlipToFront();
        return;
    }

    if (cabEngageRockerBounds.contains(pos))
    {
        if (onCabEngageToggled)
            onCabEngageToggled(!cabEngaged);
        return;
    }
    if (cabPrevBounds.contains(pos))
    {
        if (onCabIrChanged)
            onCabIrChanged(cycle(cabIds(), cabIrId, -1));
        return;
    }
    if (cabNextBounds.contains(pos))
    {
        if (onCabIrChanged)
            onCabIrChanged(cycle(cabIds(), cabIrId, +1));
        return;
    }

    if (neuralEngageRockerBounds.contains(pos))
    {
        if (onNeuralEngageToggled)
            onNeuralEngageToggled(!neuralEngaged);
        return;
    }
    if (neuralPrevBounds.contains(pos))
    {
        if (onNeuralModelChanged)
            onNeuralModelChanged(cycle(neuralIds(), neuralModelId, -1));
        return;
    }
    if (neuralNextBounds.contains(pos))
    {
        if (onNeuralModelChanged)
            onNeuralModelChanged(cycle(neuralIds(), neuralModelId, +1));
        return;
    }
}

//==============================================================================
// About/Licences modal — self-contained on the rear so the ship-required MIT/Airwindows +
// RTNeural notices have an entry point while the front (and its modal) is hidden. Mirrors the
// front geometry/behaviour exactly; legal copy comes from wk::about (single source of truth).

void RearPanelView::paintOverChildren(juce::Graphics& g)
{
    // About modal sits above everything (including the bypass wash) so it dims the rear face.
    if (aboutVisible)
        drawAboutPanel(g);

    // Full third-party licences scroll sits above the About card.
    if (licencesVisible)
        drawLicencesPanel(g);
}

bool RearPanelView::keyPressed(const juce::KeyPress& key)
{
    // Esc steps Licences -> About -> close (matches the Design spec). Only acts while the modal is open.
    if (key == juce::KeyPress::escapeKey)
    {
        if (licencesVisible)
        {
            licencesVisible = false;
            repaint();
            return true;
        }
        if (aboutVisible)
        {
            aboutVisible = false;
            giveAwayKeyboardFocus();
            repaint();
            return true;
        }
    }
    return juce::Component::keyPressed(key);
}

void RearPanelView::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
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

juce::Rectangle<int> RearPanelView::computeAboutPanelBounds() const
{
    const int w = juce::jmin(580, getWidth() - 60); // ~60% of the 960 face, per Design §3
    const int h = juce::jmin(360, getHeight() - 90);
    return juce::Rectangle<int>(0, 0, juce::jmax(120, w), juce::jmax(120, h)).withCentre(getLocalBounds().getCentre());
}

juce::Rectangle<int> RearPanelView::computeLicencesLinkBounds() const
{
    // Mirrors the bottom strip used by drawAboutPanel: inner = panel.reduced(24), and the link
    // occupies the bottom 20px of that inner rect (only vertical removeFrom* mutate `line`, so the
    // horizontal extent stays equal to inner's). Recomputed deterministically — never paint-dependent.
    const auto inner = computeAboutPanelBounds().reduced(24);
    return juce::Rectangle<int>(inner.getX(), inner.getBottom() - 20, inner.getWidth(), 20);
}

juce::Rectangle<int> RearPanelView::computeLicencesPanelBounds() const
{
    // Wider + taller than the About card so the full licence text has room before it scrolls.
    const int w = juce::jmin(680, getWidth() - 40); // ~70% of the 960 face
    const int h = juce::jmin(480, getHeight() - 50);
    return juce::Rectangle<int>(0, 0, juce::jmax(140, w), juce::jmax(160, h)).withCentre(getLocalBounds().getCentre());
}

void RearPanelView::drawAboutPanel(juce::Graphics& g)
{
    // Scrim behind the modal (dims the rear face).
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

    // Credit block — verbatim, single-sourced in wk::about (legally precise, NOT paraphrased).
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

    // Single amber "View full licences" link → opens the full licences scroll. Hit-target computed
    // deterministically via computeLicencesLinkBounds() (same geometry).
    g.setColour(juce::Colour(0xffffb74d));
    g.setFont(juce::Font(juce::FontOptions(12.0f).withStyle("Bold")));
    g.drawText("View full licences " + arrow, computeLicencesLinkBounds(), juce::Justification::bottomLeft, false);
}

void RearPanelView::drawLicencesPanel(juce::Graphics& g)
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

    // Full third-party licence body, verbatim from wk::about (single source of truth).
    const juce::String licenceBody = wk::about::licencesBodyText();

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
