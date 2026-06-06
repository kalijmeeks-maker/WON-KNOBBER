/*
    RearPanelView.cpp — see header. Paints live cab/neural state over the baked rose/amber rear plate
    and routes selection changes back to the processor (the rear-fold "override").
*/
#include "RearPanelView.h"

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
}

const juce::StringArray& RearPanelView::cabIds()
{
    static const juce::StringArray ids{"FLAT", "STUDIO_RIBBON", "VINTAGE_4X12", "CONSOLE_BOX", "OLD_RADIO", "IRON_CORE"};
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
    const juce::Colour capCol = on ? (bypassed ? juce::Colour(0xffa8632a) : juce::Colour(0xffFE9A00))
                                   : juce::Colour(0xff5a6066);
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
