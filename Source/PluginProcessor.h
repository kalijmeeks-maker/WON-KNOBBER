/*
    PluginProcessor.h — WonKnobberAudioProcessor: owns the `drive` parameter + DSP chain
    WON-KNOBBER · part of the core layer
*/
#pragma once

#include "WonKnobberState.h"
#include "dsp/Convolution.h"
#include "dsp/DryWet.h"
#include "dsp/NeuralModel.h"
#include "dsp/Saturation.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>

class WonKnobberAudioProcessor : public juce::AudioProcessor
{
public:
    // GUI peak-meter snapshot — linear magnitudes (0..~1), pre- and post-chain,
    // per stereo channel. Mono streams populate L only and leave R = 0.
    struct LevelSnapshot
    {
        float inL = 0.0f, inR = 0.0f;
        float outL = 0.0f, outR = 0.0f;
    };

    WonKnobberAudioProcessor();
    ~WonKnobberAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "WON-KNOBBER"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioParameterFloat* getDriveParameter() const noexcept { return drive; }
    juce::AudioParameterFloat* getMixParameter() const noexcept { return mix; }

    // "Choose your stone" — persistent UI state (NOT an automatable audio param;
    // wrong shape for a host's automation lane). Round-tripped via
    // get/setStateInformation. Default = "diamond".
    juce::String getCurrentVariant() const { return currentVariant; }
    void setCurrentVariant(const juce::String& v) { currentVariant = v; }

    // Bypass state (future rocker). Persisted via get/setStateInformation. Default false (chain engaged).
    bool getBypassState() const noexcept { return bypassState; }
    void setBypassState(bool shouldBypass) { bypassState = shouldBypass; }

    // Phase 2b: minimal factory preset API (embedded via BinaryData; 2 presets for this slice).
    int getNumFactoryPresets() const;
    juce::String getFactoryPresetName(int i) const;
    void loadFactoryPreset(int i); // parses xml, applyState (resets slots + active=A); snapshots the loaded voice

    // Modified-from-preset: true if the live state diverges from the last loaded voice in ANY of the six
    // identity fields (drive/mix/cabIr/neuralModel/cabEngage/neuralEngage). Floats compared with 1e-4 epsilon,
    // strings + bools compared exactly. Drives the footer "modified" ember dot.
    bool isDirty() const;
    // Re-apply the snapshot of the last loaded voice, discarding manual edits (clears the dirty flag).
    void revertToLoadedPreset();

    // Phase 2b: A/B compare slots (in-memory; saved in host state via slots in VT).
    char getActiveSlot() const;
    void setActiveSlot(
        char which); // 'A'/'B': save outgoing live to its slot, apply target slot to live, update active. Instant.
    void copySlot(char src, char dst);

    // Phase 2b: transport affordances wired to in-memory slots (no disk; file I/O later phase).
    void saveToActiveSlot();
    void loadFromActiveSlot();
    void undoLast();
    void randomizeParameters();

    // Atomically read the latest per-block linear peaks across all 4 channels and
    // reset the audio-thread accumulators to 0. Called from the GUI timer (~30 Hz);
    // every block's peak is captured because reads of 0 between blocks just leave
    // the accumulator empty for the next merge. Lock-free.
    LevelSnapshot consumeMeterPeaks() noexcept;

private:
    juce::AudioParameterFloat* drive{nullptr}; // 0.0 - 1.0, default 0.5
    juce::String currentVariant{"diamond"};    // persisted in plugin state
    juce::AudioParameterFloat* mix{nullptr};   // 0.0 - 1.0, default 1.0 (full wet for backwards compat)

    std::atomic<bool> bypassState{false}; // bypass rocker; persisted; read on audio thread (true bypass = dry passthrough)

    // Cab + neural slot selection (persisted UI state, like variant; NOT automatable params).
    // Defaults OFF so legacy sessions keep identical audio.
    juce::String currentCabIr{"FLAT"};
    juce::String currentNeuralModel{"NONE"};
    bool cabEngage{false};
    bool neuralEngage{false};
    WonKnobberState slotA;   // A/B slots for future transport; initialised to current; round-tripped via host state
    WonKnobberState slotB;
    char activeSlot{'A'}; // current compare slot; 'A' or 'B'; transient (not in state blob)

    // Snapshot of the voice last applied via loadFactoryPreset (or the ctor/state init). isDirty() compares the
    // live state against this to drive the "modified-from-preset" indicator. Transient (not in the state blob);
    // a host recall re-seats it via applyState so a freshly recalled session reads as un-modified.
    WonKnobberState loadedVoice;

    Saturation saturation;
    Convolution convolution;
    NeuralModel neuralModel;

    // RT-safe peak accumulators (audio thread compare-and-max, GUI thread exchanges
    // with 0). Linear magnitudes; the GUI does the dB conversion + ballistics.
    std::atomic<float> peakInL{0.0f}, peakInR{0.0f};
    std::atomic<float> peakOutL{0.0f}, peakOutR{0.0f};

    juce::AudioBuffer<float> dryBuffer; // pre-saturation scratch (sized in prepareToPlay)
    juce::SmoothedValue<float> mixSmooth{1.0f};
    DryWet dryWet;

    // Builds a WonKnobberState snapshot from live params + variant + bypass (for getState + slot init).
    WonKnobberState getCurrentState() const noexcept;

    // Single source apply (drive/variant/mix/bypass + init both slots + active=A). Called from set paths + ctor +
    // factory.
    void applyState(const WonKnobberState& st) noexcept;

    // Apply only to live params/variant (no touch to slots or active). Used by A/B switch/load-from-slot.
    void applyStateToParams(const WonKnobberState& st) noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WonKnobberAudioProcessor)
};
