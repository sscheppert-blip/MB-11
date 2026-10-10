#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Parameters.h"

namespace
{
    namespace id = mb11::params::id;

    constexpr int bassDrumNote = 36;   // C1
    constexpr int splitTopNote = 65;   // F3

    // MIDI implementation of the hardware (CC number -> parameter)
    struct ControllerMapping
    {
        int controller;
        const char* paramId;
    };

    constexpr ControllerMapping controllerMap[] =
    {
        { 100, id::tune }, { 101, id::pitch }, { 102, id::decay }, { 103, id::harmonics },
        { 104, id::pulse }, { 105, id::noise }, { 106, id::attack }, { 107, id::eq },
        { 113, id::compr }, { 114, id::gate }, { 115, id::metNoise }, { 117, id::volume },
        { 120, id::lfoSync }, { 121, id::lfoInt }, { 122, id::lfoSpeed }, { 123, id::lfoOneShot },
    };
}

juce::AudioProcessor::BusesProperties MB11AudioProcessor::createBuses()
{
    auto buses = BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true);

    // The CLAP wrapper would report the trigger input as an in-place partner of the
    // main output, so the CLAP version has no X Trig input.
    if (! clap_juce_extensions::clap_properties::building_clap)
        buses = buses.withInput ("X Trig In", juce::AudioChannelSet::mono(), false);

    return buses;
}

MB11AudioProcessor::MB11AudioProcessor()
    : AudioProcessor (createBuses()),
      state (*this, nullptr, "MB11", mb11::params::createLayout())
{
    auto bind = [this] (ParameterRefs& refs, const char* paramId)
    {
        refs.parameter = state.getParameter (paramId);
        refs.value = state.getRawParameterValue (paramId);
    };

    bind (tune, id::tune);           bind (pitch, id::pitch);         bind (decay, id::decay);
    bind (harmonics, id::harmonics); bind (pulse, id::pulse);         bind (noise, id::noise);
    bind (attack, id::attack);       bind (eq, id::eq);               bind (volume, id::volume);
    bind (gate, id::gate);           bind (compr, id::compr);         bind (metNoise, id::metNoise);
    bind (midiChannel, id::midiChannel); bind (splitMode, id::splitMode); bind (pitchMode, id::pitchMode);
    bind (lfoWave, id::lfoWave);     bind (lfoSpeed, id::lfoSpeed);   bind (lfoInt, id::lfoInt);
    bind (lfoSync, id::lfoSync);     bind (xTrigLevel, id::xTrigLevel); bind (xTrigOn, id::xTrigOn);
    bind (lfoOneShot, id::lfoOneShot);

    for (size_t i = 0; i < std::size (controllerMap); ++i)
    {
        const std::string_view paramId (controllerMap[i].paramId);
        auto kind = ControllerKind::byte;

        if (paramId == id::lfoSync || paramId == id::lfoOneShot)
            kind = ControllerKind::toggle;
        else if (paramId == id::lfoSpeed)
            kind = ControllerKind::lfoSpeed;

        controllers[i] = { controllerMap[i].controller, findRefs (controllerMap[i].paramId), kind };
    }
}

MB11AudioProcessor::ParameterRefs* MB11AudioProcessor::findRefs (const juce::String& paramId)
{
    for (auto* refs : { &tune, &pitch, &decay, &harmonics, &pulse, &noise, &attack, &eq, &volume, &gate, &compr,
                        &metNoise, &midiChannel, &splitMode, &pitchMode, &lfoWave, &lfoSpeed, &lfoInt, &lfoSync,
                        &xTrigLevel, &xTrigOn, &lfoOneShot })
        if (refs->parameter != nullptr && refs->parameter->paramID == paramId)
            return refs;

    return nullptr;
}

MB11AudioProcessor::~MB11AudioProcessor()
{
    cancelPendingUpdate();
}

void MB11AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // The nonlinear circuit runs oversampled: 4x up to 48 kHz, 2x up to 96 kHz.
    const auto factorExponent = sampleRate <= 50000.0 ? 2 : (sampleRate <= 100000.0 ? 1 : 0);
    oversamplingFactor = 1 << factorExponent;
    maxChunk = juce::jmax (samplesPerBlock, 32);

    oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
        1, (size_t) factorExponent, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);
    oversampler->initProcessing ((size_t) maxChunk);
    setLatencySamples (juce::roundToInt (oversampler->getLatencyInSamples()));

    zeroBuffer.setSize (1, maxChunk);
    zeroBuffer.clear();
    oversampledBuffer.setSize (1, maxChunk * oversamplingFactor);

    appliedUnitSeed = unitSeed.load();
    engine.setUnit (mb11::dsp::Unit::fromSeed (appliedUnitSeed));
    engine.setSettings (readSettings());
    engine.prepare (sampleRate * oversamplingFactor, oversamplingFactor);
    midiOut.ensureSize (2048);
    xTrigEnvelope = 0.0f;
    xTrigArmed = true;
    xTrigHoldoff = 0;
    noteOffCountdown = -1;
}

void MB11AudioProcessor::releaseResources()
{
}

bool MB11AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.inputBuses.isEmpty())
        return true;

    const auto& in = layouts.getChannelSet (true, 0);
    return in.isDisabled() || in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

//==============================================================================
mb11::dsp::Settings MB11AudioProcessor::readSettings() const noexcept
{
    auto byte = [] (const ParameterRefs& p) { return p.load() / 255.0f; };

    mb11::dsp::Settings s;
    s.tune       = byte (tune);
    s.pitch      = byte (pitch);
    s.decay      = byte (decay);
    s.harmonics  = byte (harmonics);
    s.pulse      = byte (pulse);
    s.noise      = byte (noise);
    s.attack     = byte (attack);
    s.eq         = byte (eq);
    s.volume     = byte (volume);
    s.gate       = byte (gate);
    s.compr      = byte (compr);
    s.metNoise   = juce::roundToInt (metNoise.load());
    s.lfoWave    = (mb11::dsp::LfoWave) juce::jlimit (0, 7, juce::roundToInt (lfoWave.load()));
    s.lfoInt     = byte (lfoInt);
    s.lfoSync    = lfoSync.load() > 0.5f;
    s.lfoOneShot = lfoOneShot.load() > 0.5f;

    const auto speed = juce::roundToInt (lfoSpeed.load());
    s.lfoBpm = speed <= mb11::params::lfoSpeedSync ? (float) hostBpm : (float) speed;
    return s;
}

int MB11AudioProcessor::detectExternalTriggers (juce::AudioBuffer<float>& buffer, int numSamples)
{
    if (getBusCount (true) == 0 || xTrigOn.load() < 0.5f)
        return 0;

    const auto input = getBusBuffer (buffer, true, 0);
    const auto numChannels = juce::jmin (2, input.getNumChannels());

    if (numChannels == 0)
        return 0;

    // higher X Trig Level = more sensitive: -3 dB .. -54 dB
    const auto threshold = juce::Decibels::decibelsToGain (-3.0f - 51.0f * xTrigLevel.load() / 255.0f);
    const auto release = std::exp (-1.0f / (0.01f * (float) getSampleRate()));
    const auto holdoffSamples = juce::roundToInt (0.03 * getSampleRate());
    int count = 0;

    for (int i = 0; i < numSamples; ++i)
    {
        float x = 0.0f;

        for (int c = 0; c < numChannels; ++c)
            x = juce::jmax (x, std::abs (input.getSample (c, i)));

        xTrigEnvelope = juce::jmax (x, xTrigEnvelope * release);

        if (xTrigHoldoff > 0)
            --xTrigHoldoff;

        if (! xTrigArmed && xTrigHoldoff == 0 && xTrigEnvelope < 0.5f * threshold)
            xTrigArmed = true;

        if (xTrigArmed && x >= threshold && count < maxExternalTriggers)
        {
            externalTriggerPositions[(size_t) count] = i;
            externalTriggerVelocities[(size_t) count] = juce::jlimit (0.4f, 1.0f, 0.4f + 0.6f * (x - threshold) / juce::jmax (1.0e-3f, 1.0f - threshold));
            ++count;
            xTrigArmed = false;
            xTrigHoldoff = holdoffSamples;
        }
    }

    return count;
}

void MB11AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const auto numSamples = buffer.getNumSamples();

    if (auto* playHead = getPlayHead())
        if (const auto position = playHead->getPosition())
            if (const auto bpm = position->getBpm())
                hostBpm = juce::jlimit (20.0, 999.0, *bpm);

    if (const auto seed = unitSeed.load(); seed != appliedUnitSeed)
    {
        appliedUnitSeed = seed;
        engine.setUnit (mb11::dsp::Unit::fromSeed (seed));
    }

    engine.setSettings (readSettings());

    // only happens when a host exceeds the announced block size
    if (oversampledBuffer.getNumSamples() < numSamples * oversamplingFactor)
        oversampledBuffer.setSize (1, numSamples * oversamplingFactor, false, false, true);

    // The sidechain shares channels with the output, so read it before rendering.
    const auto numExternal = detectExternalTriggers (buffer, numSamples);

    auto output = getBusBuffer (buffer, false, 0);
    auto* mono = output.getWritePointer (0);
    auto* oversampled = oversampledBuffer.getWritePointer (0);
    int rendered = 0;

    auto renderUpTo = [&] (int end)
    {
        if (end > rendered)
        {
            engine.render (oversampled + rendered * oversamplingFactor, (end - rendered) * oversamplingFactor);
            rendered = end;
        }
    };

    midiOut.clear();

    if (uiTrigger.exchange (false))
    {
        engine.trigger (1.0f);
        ++activity.trigger;
    }

    int nextExternal = 0;

    auto handleExternalUpTo = [&] (int position)
    {
        const auto channel = juce::roundToInt (midiChannel.load());

        for (; nextExternal < numExternal && externalTriggerPositions[(size_t) nextExternal] <= position; ++nextExternal)
        {
            const auto pos = externalTriggerPositions[(size_t) nextExternal];
            const auto velocity = externalTriggerVelocities[(size_t) nextExternal];
            renderUpTo (pos);
            engine.trigger (velocity);
            ++activity.trigger;
            ++activity.xTrigger;

            // also acts as trigger-to-MIDI converter
            if (noteOffCountdown >= 0)
                midiOut.addEvent (juce::MidiMessage::noteOff (noteOffChannel, bassDrumNote), pos);

            midiOut.addEvent (juce::MidiMessage::noteOn (channel, bassDrumNote, velocity), pos);
            noteOffChannel = channel;
            noteOffCountdown = pos + juce::roundToInt (0.02 * getSampleRate());
        }
    };

    for (const auto metadata : midi)
    {
        const auto position = juce::jlimit (0, juce::jmax (0, numSamples - 1), metadata.samplePosition);
        handleExternalUpTo (position);
        renderUpTo (position);
        handleMidi (metadata.getMessage(), position);
    }

    handleExternalUpTo (numSamples);
    renderUpTo (numSamples);

    // back to the host rate
    for (int start = 0; start < numSamples; start += maxChunk)
    {
        const auto length = juce::jmin (maxChunk, numSamples - start);
        const auto zeros = juce::dsp::AudioBlock<float> (zeroBuffer).getSubBlock (0, (size_t) length);
        auto up = oversampler->processSamplesUp (zeros);
        juce::FloatVectorOperations::copy (up.getChannelPointer (0), oversampled + start * oversamplingFactor,
                                           length * oversamplingFactor);

        float* channels[] = { mono + start };
        juce::dsp::AudioBlock<float> down (channels, 1, (size_t) length);
        oversampler->processSamplesDown (down);
    }

    if (noteOffCountdown >= 0)
    {
        if (noteOffCountdown < numSamples)
        {
            midiOut.addEvent (juce::MidiMessage::noteOff (noteOffChannel, bassDrumNote), noteOffCountdown);
            noteOffCountdown = -1;
        }
        else
        {
            noteOffCountdown -= numSamples;
        }
    }

    for (int c = 1; c < output.getNumChannels(); ++c)
        output.copyFrom (c, 0, output, 0, 0, numSamples);

    // clear any input channels that are not part of the output
    for (int c = output.getNumChannels(); c < buffer.getNumChannels(); ++c)
        buffer.clear (c, 0, numSamples);

    midi.swapWith (midiOut);
}

void MB11AudioProcessor::handleMidi (const juce::MidiMessage& message, int)
{
    ++activity.midi;

    if (message.getChannel() != juce::roundToInt (midiChannel.load()))
        return;

    if (message.isNoteOn())
    {
        const auto note = message.getNoteNumber();
        const auto velocity = message.getFloatVelocity();

        if (splitMode.load() < 0.5f)
        {
            if (note == bassDrumNote)
            {
                engine.trigger (velocity);
                ++activity.trigger;
            }
        }
        else if (note >= bassDrumNote && note <= splitTopNote)
        {
            const auto semitones = (float) (note - bassDrumNote);
            const auto ratio = pitchMode.load() > 0.5f ? std::exp2 (semitones / 12.0f)
                                                              : 1.0f + semitones / 12.0f;
            engine.trigger (velocity, ratio);
            ++activity.trigger;
        }
    }
    else if (message.isController())
    {
        handleController (message.getControllerNumber(), message.getControllerValue());
    }
    else if (message.isProgramChange())
    {
        pendingProgram = message.getProgramChangeNumber();
        triggerAsyncUpdate();
    }
}

void MB11AudioProcessor::handleController (int controller, int value)
{
    for (const auto& mapping : controllers)
    {
        if (mapping.controller != controller || mapping.target == nullptr)
            continue;

        float plain;

        switch (mapping.kind)
        {
            case ControllerKind::toggle:
                plain = value >= 64 ? 1.0f : 0.0f;
                break;

            case ControllerKind::lfoSpeed:
                plain = (float) (mb11::params::lfoSpeedSync
                                 + juce::roundToInt (value * (mb11::params::lfoSpeedMax - mb11::params::lfoSpeedSync) / 127.0));
                break;

            case ControllerKind::byte:
            default:
                plain = (float) juce::roundToInt (value * 255.0 / 127.0);
                break;
        }

        // the engine follows at once, the parameter on the message thread
        mapping.target->pending = plain;
        triggerAsyncUpdate();
        engine.setSettings (readSettings());
        return;
    }
}

void MB11AudioProcessor::applyPendingControllerValues()
{
    for (const auto& mapping : controllers)
    {
        if (mapping.target == nullptr)
            continue;

        auto pendingValue = mapping.target->pending.load();

        if (pendingValue < 0.0f)
            continue;

        if (auto* parameter = mapping.target->parameter)
            parameter->setValueNotifyingHost (parameter->convertTo0to1 (pendingValue));

        // keep a newer value that arrived in the meantime
        mapping.target->pending.compare_exchange_strong (pendingValue, -1.0f);
    }
}

//==============================================================================
int MB11AudioProcessor::getParameterInt (const juce::String& paramId) const
{
    if (auto* value = state.getRawParameterValue (paramId))
        return juce::roundToInt (value->load());

    return 0;
}

void MB11AudioProcessor::setParameterInt (const juce::String& paramId, int value)
{
    if (auto* parameter = state.getParameter (paramId))
    {
        const auto normalised = parameter->convertTo0to1 ((float) value);

        if (std::abs (normalised - parameter->getValue()) < 1.0e-6f)
            return;

        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost (normalised);
        parameter->endChangeGesture();
    }
}

mb11::presets::Data MB11AudioProcessor::currentPresetData() const
{
    mb11::presets::Data data {};

    for (int field = 0; field < mb11::presets::numFields; ++field)
    {
        const auto value = getParameterInt (mb11::params::presetFieldIds[field]);
        data[(size_t) field] = (uint8_t) juce::jlimit (0, 255, mb11::params::valueToPresetByte (field, value));
    }

    return data;
}

void MB11AudioProcessor::loadPreset (int slot)
{
    slot = juce::jlimit (0, mb11::presets::numSlots - 1, slot);
    const auto& preset = bank.get (slot);

    for (int field = 0; field < mb11::presets::numFields; ++field)
    {
        if (auto* parameter = state.getParameter (mb11::params::presetFieldIds[field]))
        {
            const auto value = mb11::params::presetByteToValue (field, preset.data[(size_t) field]);
            parameter->setValueNotifyingHost (parameter->convertTo0to1 ((float) value));
        }
    }

    currentPreset = slot;
    updateHostDisplay (ChangeDetails().withProgramChanged (true));
}

void MB11AudioProcessor::storePreset (int slot)
{
    slot = juce::jlimit (0, mb11::presets::numSlots - 1, slot);
    bank.store (slot, currentPresetData());
    currentPreset = slot;
    updateHostDisplay (ChangeDetails().withProgramChanged (true));
}

bool MB11AudioProcessor::isPresetEdited() const
{
    return currentPresetData() != bank.get (currentPreset).data;
}

juce::MemoryBlock MB11AudioProcessor::exportSysex() const
{
    return mb11::presets::toSysex (currentPresetData(), currentPreset);
}

bool MB11AudioProcessor::importSysex (const juce::MemoryBlock& block)
{
    const auto dump = mb11::presets::fromSysex (block.getData(), block.getSize());

    if (! dump.has_value())
        return false;

    for (int field = 0; field < mb11::presets::numSysexFields; ++field)
        setParameterInt (mb11::params::presetFieldIds[field],
                         mb11::params::presetByteToValue (field, (*dump)[(size_t) field]));

    // like the hardware, a received dump is stored into the current preset
    storePreset (currentPreset);
    return true;
}

void MB11AudioProcessor::setCurrentProgram (int index)
{
    if (juce::isPositiveAndBelow (index, mb11::presets::numSlots))
        loadPreset (index);
}

const juce::String MB11AudioProcessor::getProgramName (int index)
{
    return mb11::presets::slotLabel (index) + " " + bank.get (index).name;
}

void MB11AudioProcessor::handleAsyncUpdate()
{
    applyPendingControllerValues();

    const auto program = pendingProgram.exchange (-1);

    if (juce::isPositiveAndBelow (program, mb11::presets::numSlots))
        loadPreset (program);
}

//==============================================================================
juce::AudioProcessorEditor* MB11AudioProcessor::createEditor()
{
    return new MB11AudioProcessorEditor (*this);
}

void MB11AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto snapshot = state.copyState();
    snapshot.setProperty ("preset", currentPreset.load(), nullptr);
    snapshot.setProperty ("unit", (juce::int64) unitSeed.load(), nullptr);

    if (auto xml = snapshot.createXml())
        copyXmlToBinary (*xml, destData);
}

void MB11AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (state.state.getType()))
        {
            const auto tree = juce::ValueTree::fromXml (*xml);
            currentPreset = juce::jlimit (0, mb11::presets::numSlots - 1,
                                          (int) tree.getProperty ("preset", mb11::presets::numUser));

            // keep the "serial number" of the unit the project was made with
            if (tree.hasProperty ("unit"))
                unitSeed = (uint32_t) (juce::int64) tree.getProperty ("unit");

            state.replaceState (tree);

            // Tells hosts that all parameter values changed (CLAP: params rescan).
            updateHostDisplay (ChangeDetails().withProgramChanged (true));
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MB11AudioProcessor();
}
