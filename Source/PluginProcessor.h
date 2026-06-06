/*
    PluginProcessor.h — WonKnobberAudioProcessor: owns the `drive` parameter + DSP chain
    WON-KNOBBER · part of the core layer
*/
#pragma once

#include "Presets/PresetManager.h"
#include "WonKnobberState.h"
#include "dsp/Convolution.h"
#include "dsp/DryWet.h"
#include "dsp/NeuralModel.h"
#include "dsp/Saturation.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>

class WonKnobberAudioProcessor : public juce::AudioProcessor,
                                 private juce::AudioProcessorParameter::Listener
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
    ~WonKnobberAudioProcessor() override;

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

    // Host-facing bypass: returning the registered bypass param tells JUCE the host drives bypass via
    // automation (and manages bypassed latency). We still override processBlockBypassed so pluginval's
    // direct call is latency-correct (the default base impl asserts getLatencySamples()==0).
    juce::AudioProcessorParameter* getBypassParameter() const override;
    void processBlockBypassed(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioParameterFloat* getDriveParameter() const noexcept { return drive; }
    juce::AudioParameterFloat* getMixParameter() const noexcept { return mix; }

    // "Choose your stone" — persistent UI state (NOT an automatable audio param;
    // wrong shape for a host's automation lane). Round-tripped via
    // get/setStateInformation. Default = "diamond".
    juce::String getCurrentVariant() const { return currentVariant; }
    void setCurrentVariant(const juce::String& v) { currentVariant = v; }

    // Bypass state (rocker / host-recall sync). The host-facing AudioParameterBool (bypassParam) is the
    // authoritative value; bypassState is its lock-free audio-thread cache. These thin wrappers drive the
    // PARAM (setValueNotifyingHost on the write path) so the editor rocker also records host automation.
    // Persisted via get/setStateInformation (WonKnobberState.bypass). Default false (chain engaged).
    bool getBypassState() const noexcept
    {
        return bypassParam != nullptr ? bypassParam->get() : bypassState.load(std::memory_order_relaxed);
    }
    void setBypassState(bool shouldBypass)
    {
        if (bypassParam != nullptr)
            bypassParam->setValueNotifyingHost(shouldBypass ? 1.0f : 0.0f);
        else
            bypassState.store(shouldBypass, std::memory_order_relaxed);
    }

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

    // Rear-panel cab/neural override (message thread only). Each updates the live selection AND pushes it
    // to the DSP stage immediately (RT-safe via the stage's atomic gate / queued IR+model swap). loadedVoice
    // is deliberately NOT touched, so an override that diverges from the loaded voice lights the modified dot
    // (isDirty). Ids are machine ids: cab ∈ FLAT/STUDIO_RIBBON/VINTAGE_4X12/CONSOLE_BOX/OLD_RADIO/IRON_CORE,
    // neural ∈ NONE/TAPE/VALVE/TRANSISTOR/IRON.
    juce::String getCabIr() const noexcept { return currentCabIr; }
    juce::String getNeuralModel() const noexcept { return currentNeuralModel; }
    bool getCabEngage() const noexcept { return cabEngage; }
    bool getNeuralEngage() const noexcept { return neuralEngage; }
    void setCabIr(const juce::String& cabIrId);
    void setNeuralModel(const juce::String& modelId);
    void setCabEngage(bool shouldEngage);
    void setNeuralEngage(bool shouldEngage);

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

    // v1.1 user preset bank — disk-backed, message-thread only (NEVER call from processBlock).
    // Mirrors the factory API shape; persistence is the same WonKnobberState ValueTree XML written
    // under PresetManager::defaultDirectory(). Save snapshots the live voice; load applies a full state.
    int getNumUserPresets() const;
    juce::String getUserPresetName(int i) const;
    bool saveUserPreset(const juce::String& name); // snapshot live state -> "<name>.wknob"; true on success
    bool loadUserPreset(int i);                     // parse file -> applyState (resets slots + active=A)
    bool deleteUserPreset(int i);
    void refreshUserPresets();                        // rescan the bank dir (call before opening the menu)
    int findUserPresetIndex(const juce::String& name) const; // -1 if absent; used to re-select after save
    juce::File getUserPresetDirectory() const;        // for a "reveal in finder" affordance

    // Atomically read the latest per-block linear peaks across all 4 channels and
    // reset the audio-thread accumulators to 0. Called from the GUI timer (~30 Hz);
    // every block's peak is captured because reads of 0 between blocks just leave
    // the accumulator empty for the next merge. Lock-free.
    LevelSnapshot consumeMeterPeaks() noexcept;

private:
    juce::AudioParameterFloat* drive{nullptr}; // 0.0 - 1.0, default 0.5
    juce::String currentVariant{"diamond"};    // persisted in plugin state
    juce::AudioParameterFloat* mix{nullptr};   // 0.0 - 1.0, default 1.0 (full wet for backwards compat)

    // Host-facing bypass param (registered AFTER drive+mix => index 2 so drive=0/mix=1 automation lanes stay stable).
    // Authoritative bypass value; the host + editor write it, getBypassParameter() returns it.
    juce::AudioParameterBool* bypassParam{nullptr};
    // Audio-thread cache of bypassParam (lock-free read in processBlock). Kept in sync via parameterValueChanged.
    std::atomic<bool> bypassState{false}; // true bypass = dry passthrough

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

    PresetManager presetManager; // v1.1 user preset bank (disk-backed; message thread only)

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

    // AudioProcessorParameter::Listener — keeps bypassState (the audio-thread cache) in sync with bypassParam.
    // parameterValueChanged may fire on the audio thread in some wrappers; the body is a single lock-free
    // atomic store (no alloc/lock/IO), so it stays RT-safe. Only registered on bypassParam.
    void parameterValueChanged(int parameterIndex, float newValue) override;
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WonKnobberAudioProcessor)
};
