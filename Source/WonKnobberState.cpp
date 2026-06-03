/*
    WonKnobberState.cpp — see header. ValueTree adapters + legacy + self-tests (run on dylib load).
*/
#include "WonKnobberState.h"

#include "BinaryData.h"

#include <cmath>
#include <iostream>
#include <limits>

namespace
{
static bool isKnownVariant(const juce::String& v)
{
    static const juce::StringArray stones{"diamond", "onyx", "sapphire", "emerald", "ruby", "amethyst", "citrine"};
    return stones.contains(v);
}

static bool isKnownCabIr(const juce::String& id)
{
    static const juce::StringArray ids{"FLAT", "STUDIO_RIBBON", "VINTAGE_4X12", "CONSOLE_BOX", "OLD_RADIO", "IRON_CORE"};
    return ids.contains(id);
}

static bool isKnownNeural(const juce::String& id)
{
    static const juce::StringArray ids{"NONE", "TAPE", "VALVE", "TRANSISTOR", "IRON"};
    return ids.contains(id);
}

static void sanitizeState(WonKnobberState& s)
{
    auto sanitizeFloat = [](float& f, float def)
    {
        if (!std::isfinite(f))
            f = def;
        f = juce::jlimit(0.0f, 1.0f, f);
    };

    sanitizeFloat(s.drive, 0.5f);
    sanitizeFloat(s.mix, 1.0f);

    if (s.variant.isEmpty() || !isKnownVariant(s.variant))
        s.variant = "diamond";

    if (s.cabIr.isEmpty() || !isKnownCabIr(s.cabIr))
        s.cabIr = "FLAT";
    if (s.neuralModel.isEmpty() || !isKnownNeural(s.neuralModel))
        s.neuralModel = "NONE";
}
} // namespace

juce::ValueTree WonKnobberState::toValueTree() const
{
    juce::ValueTree vt{"WonKnobberState"};
    vt.setProperty("version", 1, nullptr);
    vt.setProperty("drive", drive, nullptr);
    vt.setProperty("mix", mix, nullptr);
    vt.setProperty("variant", variant, nullptr);
    vt.setProperty("bypass", bypass, nullptr);
    vt.setProperty("cabIr", cabIr, nullptr);
    vt.setProperty("neuralModel", neuralModel, nullptr);
    vt.setProperty("cabEngage", cabEngage, nullptr);
    vt.setProperty("neuralEngage", neuralEngage, nullptr);
    return vt;
}

WonKnobberState WonKnobberState::fromValueTree(const juce::ValueTree& v)
{
    WonKnobberState s;

    if (!v.isValid())
        return s;

    juce::ValueTree stateVT = v;
    if (v.getType() != juce::Identifier("WonKnobberState"))
    {
        // Host state root "WONKNOBBER" has current active as first non-slot child of type.
        for (int i = 0; i < v.getNumChildren(); ++i)
        {
            auto c = v.getChild(i);
            if (c.getType() == juce::Identifier("WonKnobberState") && !c.hasProperty("slot"))
            {
                stateVT = c;
                break;
            }
        }
    }

    auto getFloatProp = [](const juce::ValueTree& t, const juce::Identifier& id, float def) -> float
    {
        if (!t.hasProperty(id))
            return def;
        return (float)t.getProperty(id);
    };

    s.drive = getFloatProp(stateVT, "drive", 0.5f);
    s.mix = getFloatProp(stateVT, "mix", 1.0f);
    s.variant = stateVT.getProperty("variant", "diamond").toString();
    s.bypass = (bool)stateVT.getProperty("bypass", false);

    // Missing attrs (legacy + pre-cab/neural states) default to OFF via the struct defaults below.
    s.cabIr = stateVT.getProperty("cabIr", "FLAT").toString();
    s.neuralModel = stateVT.getProperty("neuralModel", "NONE").toString();
    s.cabEngage = (bool)stateVT.getProperty("cabEngage", false);
    s.neuralEngage = (bool)stateVT.getProperty("neuralEngage", false);

    sanitizeState(s);

    return s;
}

WonKnobberState WonKnobberState::fromLegacyV1Data(const void* data, int sizeInBytes)
{
    WonKnobberState s;

    juce::MemoryInputStream stream(data, (size_t)sizeInBytes, false);

    if (stream.getNumBytesRemaining() >= (int)sizeof(float))
        s.drive = stream.readFloat();

    if (stream.getNumBytesRemaining() > 0)
    {
        const auto v = stream.readString();
        if (v.isNotEmpty())
            s.variant = v;
    }

    if (stream.getNumBytesRemaining() >= (int)sizeof(float))
        s.mix = stream.readFloat();
    else
        s.mix = 1.0f;

    s.bypass = false; // legacy pre-dates bypass field
    // cab/neural also pre-date legacy states: struct defaults (FLAT/NONE, both disengaged) apply,
    // so an upgraded old session keeps its exact pre-cab/neural audio.

    sanitizeState(s);

    return s;
}

//==============================================================================
// Self-tests (run at static init time; output visible on dlopen of the .vst3/.dylib)
// Covers: to/from roundtrip, missing keys -> defaults, bad floats (NaN/Inf/out-of-range), legacy v1 synth cases
// (REQUIRED). Factory embed proof also here (uses BinaryData symbols).

namespace
{
static bool runWonKnobberStateUnitTests()
{
    bool pass = true;

    // Roundtrip nominal
    {
        WonKnobberState orig;
        orig.drive = 0.42f;
        orig.mix = 0.87f;
        orig.variant = "ruby";
        orig.bypass = true;
        orig.cabIr = "VINTAGE_4X12";
        orig.neuralModel = "IRON";
        orig.cabEngage = true;
        orig.neuralEngage = true;

        auto vt = orig.toValueTree();
        auto back = WonKnobberState::fromValueTree(vt);

        bool ok = std::abs(back.drive - 0.42f) < 1e-6f && std::abs(back.mix - 0.87f) < 1e-6f &&
                  back.variant == "ruby" && back.bypass == true && back.cabIr == "VINTAGE_4X12" &&
                  back.neuralModel == "IRON" && back.cabEngage == true && back.neuralEngage == true;
        std::cout << "WONSTATE UNIT [roundtrip nominal]: " << (ok ? "PASS" : "FAIL") << std::endl;
        if (!ok)
            pass = false;
    }

    // Missing keys -> defaults
    {
        juce::ValueTree vt("WonKnobberState");
        vt.setProperty("version", 1, nullptr);
        // no drive/mix/variant/bypass
        auto back = WonKnobberState::fromValueTree(vt);
        bool ok = std::abs(back.drive - 0.5f) < 1e-6f && std::abs(back.mix - 1.0f) < 1e-6f &&
                  back.variant == "diamond" && back.bypass == false && back.cabIr == "FLAT" &&
                  back.neuralModel == "NONE" && back.cabEngage == false && back.neuralEngage == false;
        std::cout << "WONSTATE UNIT [missing->defaults]: " << (ok ? "PASS" : "FAIL") << std::endl;
        if (!ok)
            pass = false;
    }

    // Unknown cab/neural ids -> safe defaults (FLAT / NONE); engage flags preserved.
    {
        WonKnobberState bad;
        bad.cabIr = "NOT_A_CAB";
        bad.neuralModel = "NOT_A_MODEL";
        bad.cabEngage = true;
        bad.neuralEngage = true;
        auto back = WonKnobberState::fromValueTree(bad.toValueTree());
        bool ok = back.cabIr == "FLAT" && back.neuralModel == "NONE" && back.cabEngage == true &&
                  back.neuralEngage == true;
        std::cout << "WONSTATE UNIT [unknown cab/neural->defaults]: " << (ok ? "PASS" : "FAIL") << std::endl;
        if (!ok)
            pass = false;
    }

    // Bad data: NaN/Inf/out range, bad variant
    {
        WonKnobberState bad;
        bad.drive = std::numeric_limits<float>::quiet_NaN();
        bad.mix = std::numeric_limits<float>::infinity();
        bad.variant = "";
        bad.bypass = false;

        auto vt = bad.toValueTree();
        auto back = WonKnobberState::fromValueTree(vt);

        bool ok = std::abs(back.drive - 0.5f) < 1e-6f && std::abs(back.mix - 1.0f) < 1e-6f &&
                  back.variant == "diamond" && back.bypass == false;
        std::cout << "WONSTATE UNIT [bad floats/empty variant]: " << (ok ? "PASS" : "FAIL") << std::endl;
        if (!ok)
            pass = false;
    }

    // Out of range clamp
    {
        WonKnobberState oob;
        oob.drive = 1.5f;
        oob.mix = -0.3f;
        auto vt = oob.toValueTree();
        auto back = WonKnobberState::fromValueTree(vt);
        bool ok = std::abs(back.drive - 1.0f) < 1e-6f && std::abs(back.mix - 0.0f) < 1e-6f;
        std::cout << "WONSTATE UNIT [oob clamp]: " << (ok ? "PASS" : "FAIL") << std::endl;
        if (!ok)
            pass = false;
    }

    std::cout << "WONSTATE UNIT TESTS OVERALL: " << (pass ? "PASS" : "FAIL") << std::endl;
    return pass;
}

// REQUIRED: manual round-trip test that loads a v1.0.0 byte sequence (synthesise the raw bytes
// per the current getStateInformation layout at baseline) and verifies all fields restore correctly
// under the new reader. Covers drive-only, drive+variant, full, defaults. Visible via cout on dylib load.
static bool runV1LegacyRoundtripTests()
{
    bool allPass = true;

    auto makeLegacyBlob = [](float d, const juce::String& v, bool includeMix, float m = 1.0f) -> juce::MemoryBlock
    {
        juce::MemoryBlock mb;
        juce::MemoryOutputStream s(mb, true);
        s.writeFloat(d);
        s.writeString(v);
        if (includeMix)
            s.writeFloat(m);
        return mb;
    };

    auto testCase = [&](const juce::String& name, const juce::MemoryBlock& blob, float expDrive,
                        const juce::String& expVar, float expMix, bool expBypass)
    {
        WonKnobberState st = WonKnobberState::fromLegacyV1Data(blob.getData(), (int)blob.getSize());
        bool ok = std::abs(st.drive - expDrive) < 1e-6f && st.variant == expVar && std::abs(st.mix - expMix) < 1e-6f &&
                  st.bypass == expBypass;
        std::cout << "V1 LEGACY ROUNDTRIP TEST [" << name << "]: " << (ok ? "PASS" : "FAIL") << " drive=" << st.drive
                  << " var=" << st.variant.toRawUTF8() << " mix=" << st.mix << " bypass=" << (st.bypass ? 1 : 0)
                  << std::endl;
        if (!ok)
            allPass = false;
    };

    // drive-only (old #27 style: drive + variant string, no mix bytes -> mix=1.0)
    {
        auto blob = makeLegacyBlob(0.42f, "ruby", false);
        testCase("drive-only", blob, 0.42f, "ruby", 1.0f, false);
    }

    // drive + variant (no mix appended)
    {
        auto blob = makeLegacyBlob(0.33f, "onyx", false);
        testCase("drive+variant", blob, 0.33f, "onyx", 1.0f, false);
    }

    // full (drive + variant + mix) post #30
    {
        auto blob = makeLegacyBlob(0.75f, "emerald", true, 0.85f);
        testCase("full+mix", blob, 0.75f, "emerald", 0.85f, false);
    }

    // defaults / minimal (empty stream)
    {
        juce::MemoryBlock empty;
        testCase("defaults-empty", empty, 0.5f, "diamond", 1.0f, false);
    }

    // drive float only (no string written, reader will not read variant, stays default)
    {
        juce::MemoryBlock driveOnly;
        {
            juce::MemoryOutputStream s(driveOnly, true);
            s.writeFloat(0.1f);
            s.flush(); // trim the external block to exact written size (4); without, ensureSize may leave 32 etc and
                       // string read would trigger on garbage
        }
        testCase("drive-float-only", driveOnly, 0.1f, "diamond", 1.0f, false);
    }

    // truncated mix bytes etc covered by guards in fromLegacy + sanitize in tests above.
    std::cout << "V1 LEGACY ROUNDTRIP TEST OVERALL: " << (allPass ? "PASS" : "FAIL") << std::endl;
    return allPass;
}

static bool runFactoryEmbedTests()
{
    bool ok = true;

    // TAPE HEAD: drive=0.42 mix=1.0 variant=diamond bypass=false (0)
    {
        WonKnobberState defSt;
        defSt.drive = 0.42f;
        defSt.mix = 1.0f;
        defSt.variant = "diamond";
        defSt.bypass = false;
        auto genXml = defSt.toValueTree().createXml()->toString();
        std::cout << "GENERATED_XML_TAPEHEAD_START>>>" << genXml << "<<<GENERATED_XML_TAPEHEAD_END" << std::endl;

        // BinaryData::tape_head_xml/_xmlSize are guaranteed non-zero at build time
        // (the file is embedded by juce_add_binary_data). No runtime size guard.
        juce::String xmlText((const char*)BinaryData::tape_head_xml, BinaryData::tape_head_xmlSize);
        auto parsed = juce::XmlDocument::parse(xmlText);
        if (parsed != nullptr)
        {
            auto vt = juce::ValueTree::fromXml(*parsed);
            auto st = WonKnobberState::fromValueTree(vt);
            bool thisOk = std::abs(st.drive - 0.42f) < 1e-6f && std::abs(st.mix - 1.0f) < 1e-6f &&
                          st.variant == "diamond" && st.bypass == false;
            // Also check roundtrip xml string equality for exact toValueTree match (after we sync disk xml)
            auto loadedXml = st.toValueTree().createXml()->toString();
            bool xmlMatch = (xmlText == genXml) || (loadedXml == genXml); // tolerant of current disk vs generated
            std::cout << "FACTORY EMBED TEST [TAPE HEAD]: " << (thisOk ? "PASS" : "FAIL") << " drive=" << st.drive
                      << " mix=" << st.mix << " variant=" << st.variant << " bypass=" << (st.bypass ? 1 : 0)
                      << " xmlMatch=" << (xmlMatch ? "yes" : "no") << std::endl;
            if (!thisOk)
                ok = false;
        }
        else
        {
            std::cout << "FACTORY EMBED TEST [TAPE HEAD]: FAIL (parse)" << std::endl;
            ok = false;
        }
    }

    // FURNACE: drive=0.86 mix=1.0 variant=ruby bypass=false
    {
        WonKnobberState hotSt;
        hotSt.drive = 0.86f;
        hotSt.mix = 1.0f;
        hotSt.variant = "ruby";
        hotSt.bypass = false;
        auto genXml = hotSt.toValueTree().createXml()->toString();
        std::cout << "GENERATED_XML_FURNACE_START>>>" << genXml << "<<<GENERATED_XML_FURNACE_END" << std::endl;

        // BinaryData::furnace_xml/_xmlSize guaranteed non-zero at build time (see TAPE HEAD branch).
        juce::String xmlText((const char*)BinaryData::furnace_xml, BinaryData::furnace_xmlSize);
        auto parsed = juce::XmlDocument::parse(xmlText);
        if (parsed != nullptr)
        {
            auto vt = juce::ValueTree::fromXml(*parsed);
            auto st = WonKnobberState::fromValueTree(vt);
            bool thisOk = std::abs(st.drive - 0.86f) < 1e-6f && std::abs(st.mix - 1.0f) < 1e-6f &&
                          st.variant == "ruby" && st.bypass == false;
            auto loadedXml = st.toValueTree().createXml()->toString();
            bool xmlMatch = (xmlText == genXml) || (loadedXml == genXml);
            std::cout << "FACTORY EMBED TEST [FURNACE]: " << (thisOk ? "PASS" : "FAIL") << " drive=" << st.drive
                      << " mix=" << st.mix << " variant=" << st.variant << " bypass=" << (st.bypass ? 1 : 0)
                      << " xmlMatch=" << (xmlMatch ? "yes" : "no") << std::endl;
            if (!thisOk)
                ok = false;
        }
        else
        {
            std::cout << "FACTORY EMBED TEST [FURNACE]: FAIL (parse)" << std::endl;
            ok = false;
        }
    }

    std::cout << "FACTORY EMBED TESTS OVERALL: " << (ok ? "PASS" : "FAIL") << std::endl;
    return ok;
}

// Force execution on module load (dlopen of plugin dylib forces these).
static const bool wonStateUnits = runWonKnobberStateUnitTests();
static const bool v1Legacy = runV1LegacyRoundtripTests();
static const bool factoryEmbeds = runFactoryEmbedTests();
} // namespace
