/*
    RenderHarness.cpp — headless GUI render harness for WON-KNOBBER.

    Paints FaceplateView (front) + RearPanelView (rear) straight to PNG files with controlled state,
    with NO plugin host, NO window, and NO screen-coordinate clicking. Intended for visual-regression
    testing: generate baselines once, then diff future builds against them (e.g. ImageMagick `compare`).

    Usage:  WonKnobberRender [outputDir]      (default: ./render_out)

    It reuses the exact same gui/dsp source the plugin builds (minus the plugin wrapper), so what it
    renders is what the plugin draws. Components are snapshotted via Component::createComponentSnapshot,
    which renders offscreen through JUCE's software renderer — no display server needed.
*/
#include "gui/FaceplateView.h"
#include "gui/RearPanelView.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <iostream>

namespace
{
void writePNG(juce::Component& c, const juce::File& out)
{
    const auto img = c.createComponentSnapshot(c.getLocalBounds());
    out.deleteFile();
    if (auto os = out.createOutputStream())
    {
        juce::PNGImageFormat png;
        if (png.writeImageToStream(img, *os))
            std::cout << "  wrote " << out.getFileName() << "  (" << img.getWidth() << "x" << img.getHeight() << ")\n";
        else
            std::cout << "  FAILED writing " << out.getFileName() << "\n";
    }
}
} // namespace

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI guiInit; // brings up the GUI subsystem (fonts/images) without a window

    juce::File outDir = (argc > 1) ? juce::File(juce::CharPointer_UTF8(argv[1]))
                                   : juce::File::getCurrentWorkingDirectory().getChildFile("render_out");
    outDir.createDirectory();
    std::cout << "WON-KNOBBER render harness -> " << outDir.getFullPathName() << "\n";

    // ---- Rear service panel (cab/neural override + flip medallion) ----
    {
        RearPanelView rear;
        rear.setSize(960, 600);

        rear.setCabState("STUDIO_RIBBON", true);
        rear.setNeuralState("TAPE", true);
        writePNG(rear, outDir.getChildFile("rear_studio_tape_engaged.png"));

        rear.setCabState("FLAT", false);
        rear.setNeuralState("NONE", false);
        writePNG(rear, outDir.getChildFile("rear_flat_none_off.png"));

        rear.setCabState("VINTAGE_4X12", true);
        rear.setNeuralState("VALVE", true);
        rear.setBypassed(true);
        writePNG(rear, outDir.getChildFile("rear_bypassed.png"));
    }

    // ---- Front faceplate (hero knob + scopes + footer) ----
    {
        FaceplateView fp;
        fp.setSize(960, 600);
        fp.setPresetDisplayName("TAPE HEAD");

        fp.setVariant("diamond");
        fp.setDrive(0.5f);
        writePNG(fp, outDir.getChildFile("front_diamond_mid.png"));

        fp.setVariant("onyx");
        fp.setDrive(0.85f);
        writePNG(fp, outDir.getChildFile("front_onyx_high.png"));

        fp.setBypassed(true);
        writePNG(fp, outDir.getChildFile("front_bypassed.png"));
    }

    // ---- Front "voice active" — for Design's P0/P1 front pixel-check ----
    // A loaded voice + knob mid-travel, with the I/O meters + SIG LED driven (pushLevels lights
    // them; harmonics/transfer already track drive). Separate instance so the lit state doesn't
    // bleed into the clean/bypassed renders. Two pushes settle the meter ballistics pre-snapshot.
    {
        FaceplateView fp;
        fp.setSize(960, 600);
        fp.setPresetDisplayName("TAPE HEAD");
        fp.setVariant("diamond");
        fp.setDrive(0.5f);
        fp.pushLevels(0.72f, 0.66f, 0.80f, 0.74f, 0.2f);
        fp.pushLevels(0.72f, 0.66f, 0.80f, 0.74f, 0.033f);
        writePNG(fp, outDir.getChildFile("front_voice_active.png"));
    }

    std::cout << "Done.\n";
    return 0;
}
