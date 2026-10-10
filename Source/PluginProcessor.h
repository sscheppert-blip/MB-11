#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <clap-juce-extensions/clap-juce-extensions.h>

#include "Presets.h"
#include "dsp/Engine.h"

class MB11AudioProcessor final : public juce::AudioProcessor,
                                 public clap_juce_extensions::clap_properties,
                                 private juce::AsyncUpdater
{
public:
    MB11AudioProcessor();
    ~MB11AudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return mb11::presets::numSlots; }
    int getCurrentProgram() override { return currentPreset.load(); }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    // Front panel interface (message thread)
    juce::AudioProcessorValueTreeState& getState() noexcept { return state; }

    int getParameterInt (const juce::String& paramId) const;
    void setParameterInt (const juce::String& paramId, int value);

    int getCurrentPreset() const noexcept { return currentPreset.load(); }
    const mb11::presets::Preset& getPreset (int slot) const { return bank.get (slot); }
    void loadPreset (int slot);
    void storePreset (int slot);
    bool isPresetEdited() const;

    juce::MemoryBlock exportSysex() const;
    bool importSysex (const juce::MemoryBlock&);

    void triggerFromUi() noexcept { uiTrigger = true; }

    struct Activity
    {
        std::atomic<uint32_t> trigger { 0 }, xTrigger { 0 }, midi { 0 };
    };

    const Activity& getActivity() const noexcept { return activity; }

private:
    static BusesProperties createBuses();
    void handleAsyncUpdate() override;

    mb11::dsp::Settings readSettings() const noexcept;
    mb11::presets::Data currentPresetData() const;
    void handleMidi (const juce::MidiMessage&, int samplePosition);
    void handleController (int controller, int value);
    void applyPendingControllerValues();
    int detectExternalTriggers (juce::AudioBuffer<float>&, int numSamples);

    juce::AudioProcessorValueTreeState state;
    mb11::presets::Bank bank;
    std::atomic<int> currentPreset { mb11::presets::numUser };
    std::atomic<int> pendingProgram { -1 };

    mb11::dsp::Engine engine;
    double hostBpm = 120.0;

    // every instance is its own unit with its own component tolerances
    std::atomic<uint32_t> unitSeed { (uint32_t) juce::Random::getSystemRandom().nextInt() };
    uint32_t appliedUnitSeed = 0;

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    juce::AudioBuffer<float> zeroBuffer, oversampledBuffer;
    int oversamplingFactor = 1;
    int maxChunk = 512;

    struct ParameterRefs
    {
        juce::RangedAudioParameter* parameter = nullptr;
        std::atomic<float>* value = nullptr;

        // Set by MIDI CCs on the audio thread, handed to the parameter on the
        // message thread (negative = nothing pending).
        std::atomic<float> pending { -1.0f };

        float load() const noexcept
        {
            const auto p = pending.load();
            return p >= 0.0f ? p : value->load();
        }
    };

    ParameterRefs* findRefs (const juce::String& paramId);

    ParameterRefs tune, pitch, decay, harmonics, pulse, noise, attack, eq, volume, gate, compr, metNoise,
                  midiChannel, splitMode, pitchMode, lfoWave, lfoSpeed, lfoInt, lfoSync,
                  xTrigLevel, xTrigOn, lfoOneShot;

    enum class ControllerKind { byte, toggle, lfoSpeed };

    struct Controller
    {
        int controller = -1;
        ParameterRefs* target = nullptr;
        ControllerKind kind = ControllerKind::byte;
    };

    std::array<Controller, 16> controllers;

    std::atomic<bool> uiTrigger { false };
    Activity activity;

    // external trigger input (sidechain)
    static constexpr int maxExternalTriggers = 16;
    std::array<int, maxExternalTriggers> externalTriggerPositions {};
    std::array<float, maxExternalTriggers> externalTriggerVelocities {};
    float xTrigEnvelope = 0.0f;
    bool xTrigArmed = true;
    int xTrigHoldoff = 0;

    juce::MidiBuffer midiOut;
    int noteOffCountdown = -1;
    int noteOffChannel = 1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MB11AudioProcessor)
};
