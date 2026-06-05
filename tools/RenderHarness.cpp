/*
    RenderHarness.cpp — headless GUI render harness for WON-KNOBBER (front faceplate).

    Paints FaceplateView straight to PNG with controlled state, with NO plugin host, NO window,
    and NO screen-coordinate clicking. Renders offscreen via Component::createComponentSnapshot
    (JUCE software renderer) — no display server needed. Build with -DWK_BUILD_RENDER_HARNESS=ON.

    Usage:  WonKnobberRender [outputDir]   (default: ./render_out)
*/
#include "gui/FaceplateView.h"

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
    juce::ScopedJuceInitialiser_GUI guiInit;

    juce::File outDir = (argc > 1) ? juce::File(juce::CharPointer_UTF8(argv[1]))
                                   : juce::File::getCurrentWorkingDirectory().getChildFile("render_out");
    outDir.createDirectory();
    std::cout << "WON-KNOBBER front render harness -> " << outDir.getFullPathName() << "\n";

    {
        FaceplateView fp;
        fp.setSize(960, 612);
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

    {
        FaceplateView fp;
        fp.setSize(960, 612);
        fp.setPresetDisplayName("TAPE HEAD");
        fp.setVariant("diamond");
        fp.setDrive(0.5f);
        fp.getMixKnob().setValue(1.0, juce::dontSendNotification);
        fp.pushLevels(0.72f, 0.66f, 0.80f, 0.74f, 0.2f);
        fp.pushLevels(0.72f, 0.66f, 0.80f, 0.74f, 0.033f);
        writePNG(fp, outDir.getChildFile("front_voice_active.png"));
    }

    std::cout << "Done.\n";
    return 0;
}
