#include "FrontPanelModel.h"

#include "../Parameters.h"
#include "../PluginProcessor.h"

namespace
{
    namespace id = mb11::params::id;

    constexpr double timeoutMs = 10000.0;
    constexpr double messageMs = 1500.0;
    constexpr double blinkPeriodMs = 500.0;

    // Down steps through this order, Up the other way round.
    constexpr const char* soundIds[] =
    {
        id::tune, id::pitch, id::decay, id::harmonics, id::pulse, id::noise, id::attack, id::eq,
        id::volume, id::gate, id::compr, id::metNoise
    };

    constexpr const char* masterIds[] =
    {
        id::midiChannel, id::splitMode, id::pitchMode, id::lfoWave, id::lfoSpeed, id::lfoInt, id::lfoSync, nullptr
    };

    constexpr const char* secondIds[] =
    {
        id::xTrigLevel, id::xTrigOn, nullptr, nullptr, id::lfoOneShot
    };
}

FrontPanelModel::FrontPanelModel (MB11AudioProcessor& p)
    : processor (p)
{
}

const char* FrontPanelModel::getSoundParamId (int index)  { return soundIds[index]; }
const char* FrontPanelModel::getMasterParamId (int index) { return masterIds[index]; }
const char* FrontPanelModel::getSecondParamId (int index) { return secondIds[index]; }

const char* FrontPanelModel::currentParamId() const
{
    switch (mode)
    {
        case Mode::sound:  return soundIds[item];
        case Mode::master: return masterIds[item];
        case Mode::second: return secondIds[item];
        case Mode::preset:
        case Mode::store:  break;
    }

    return nullptr;
}

//==============================================================================
void FrontPanelModel::setMode (Mode newMode, int newItem)
{
    mode = newMode;
    item = newItem;
    potCaught = false;
}

void FrontPanelModel::touch()
{
    lastInput = now;
}

void FrontPanelModel::enterTimedMode (Mode newMode)
{
    if (mode == Mode::preset || mode == Mode::sound)
    {
        returnMode = mode;
        returnItem = item;
    }

    setMode (newMode, 0);
    touch();
}

void FrontPanelModel::leaveTimedMode()
{
    setMode (returnMode, returnItem);
}

void FrontPanelModel::down()
{
    touch();

    switch (mode)
    {
        case Mode::preset: setMode (Mode::sound, 0); break;
        case Mode::sound:  item + 1 < numSoundItems ? setMode (Mode::sound, item + 1) : setMode (Mode::preset, 0); break;
        case Mode::master: setMode (Mode::master, (item + 1) % numMasterItems); break;
        case Mode::second: setMode (Mode::second, (item + 1) % numSecondItems); break;
        case Mode::store:  leaveTimedMode(); break;
    }
}

void FrontPanelModel::up()
{
    touch();

    switch (mode)
    {
        case Mode::preset: setMode (Mode::sound, numSoundItems - 1); break;
        case Mode::sound:  item > 0 ? setMode (Mode::sound, item - 1) : setMode (Mode::preset, 0); break;
        case Mode::master: setMode (Mode::master, (item + numMasterItems - 1) % numMasterItems); break;
        case Mode::second: setMode (Mode::second, (item + numSecondItems - 1) % numSecondItems); break;
        case Mode::store:  leaveTimedMode(); break;
    }
}

void FrontPanelModel::upDown()
{
    if (mode == Mode::master || mode == Mode::store)
        leaveTimedMode();
    else
        enterTimedMode (Mode::master);
}

void FrontPanelModel::upPlay()
{
    if (mode == Mode::second)
        leaveTimedMode();
    else
        enterTimedMode (Mode::second);
}

void FrontPanelModel::play()
{
    processor.triggerFromUi();
}

//==============================================================================
void FrontPanelModel::changeCurrentValue (int steps)
{
    const auto* paramId = currentParamId();

    if (paramId == nullptr)
        return;

    auto* parameter = processor.getState().getParameter (paramId);

    if (parameter == nullptr)
        return;

    const auto& range = parameter->getNormalisableRange();
    const auto value = processor.getParameterInt (paramId) + steps;
    processor.setParameterInt (paramId, juce::jlimit ((int) range.start, (int) range.end, value));
}

void FrontPanelModel::encoderTurn (int steps)
{
    if (steps == 0)
        return;

    touch();
    potCaught = false;

    switch (mode)
    {
        case Mode::preset:
            processor.loadPreset (juce::jlimit (0, mb11::presets::numSlots - 1, processor.getCurrentPreset() + steps));
            break;

        case Mode::store:
            storeSlot = juce::jlimit (0, mb11::presets::numSlots - 1, storeSlot + steps);
            break;

        case Mode::sound:
        case Mode::master:
        case Mode::second:
            changeCurrentValue (steps);
            break;
    }
}

void FrontPanelModel::encoderClick()
{
    touch();

    switch (mode)
    {
        case Mode::preset:
        case Mode::sound:
            // Load: discards the edits of the current preset
            processor.loadPreset (processor.getCurrentPreset());
            break;

        case Mode::master:
            if (item == masterStoreItem)
            {
                storeSlot = processor.getCurrentPreset();
                setMode (Mode::store, 0);
            }
            break;

        case Mode::store:
            processor.storePreset (storeSlot);
            returnMode = Mode::preset;
            returnItem = 0;
            leaveTimedMode();
            break;

        case Mode::second:
            if (item == secondDumpItem && onSysexDump != nullptr)
                onSysexDump();
            else if (item == secondLoadItem && onSysexLoad != nullptr)
                onSysexLoad();
            break;
    }
}

void FrontPanelModel::potMoved (float newPosition)
{
    newPosition = juce::jlimit (0.0f, 1.0f, newPosition);
    const auto oldPosition = potPosition;
    potPosition = newPosition;
    touch();

    const auto* paramId = currentParamId();
    auto* parameter = paramId != nullptr ? processor.getState().getParameter (paramId) : nullptr;

    if (parameter == nullptr)
    {
        if (mode == Mode::preset)
        {
            // no pickup needed for the preset selection
            processor.loadPreset (juce::roundToInt (newPosition * (mb11::presets::numSlots - 1)));
        }
        else if (mode == Mode::store)
        {
            storeSlot = juce::roundToInt (newPosition * (mb11::presets::numSlots - 1));
        }

        return;
    }

    const auto current = parameter->getValue();

    if (! potCaught)
    {
        const auto lo = juce::jmin (oldPosition, newPosition) - 0.004f;
        const auto hi = juce::jmax (oldPosition, newPosition) + 0.004f;
        potCaught = current >= lo && current <= hi;
    }

    if (potCaught)
        processor.setParameterInt (paramId, juce::roundToInt (parameter->convertFrom0to1 (newPosition)));
}

//==============================================================================
void FrontPanelModel::tick (double nowMs)
{
    now = nowMs;

    if (lastInput == 0.0)
        lastInput = now;

    const auto timed = mode == Mode::master || mode == Mode::second || mode == Mode::store;

    if (timed && now - lastInput > timeoutMs)
        leaveTimedMode();
}

void FrontPanelModel::showMessage (const juce::String& text)
{
    message = text;
    messageUntil = now + messageMs;
}

juce::String FrontPanelModel::formatValue (const char* paramId) const
{
    const auto value = processor.getParameterInt (paramId);
    const std::string_view idView (paramId);

    if (idView == id::lfoSpeed)
        return value <= mb11::params::lfoSpeedSync ? juce::String ("Mid") : juce::String (value).paddedLeft ('0', 3);

    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (processor.getState().getParameter (paramId)))
        return choice->getCurrentChoiceName();

    return juce::String (value).paddedLeft ('0', 3);
}

juce::String FrontPanelModel::getDisplayText() const
{
    if (now < messageUntil)
        return message;

    switch (mode)
    {
        case Mode::preset: return mb11::presets::slotLabel (processor.getCurrentPreset());
        case Mode::store:  return mb11::presets::slotLabel (storeSlot);
        case Mode::sound:  return formatValue (soundIds[item]);

        case Mode::master:
            return item == masterStoreItem ? juce::String ("Sto") : formatValue (masterIds[item]);

        case Mode::second:
            if (item == secondDumpItem) return "dMP";
            if (item == secondLoadItem) return "_Ld";
            return formatValue (secondIds[item]);
    }

    return {};
}

bool FrontPanelModel::isDisplayVisible() const
{
    if (mode != Mode::store)
        return true;

    return std::fmod (now, blinkPeriodMs) < blinkPeriodMs * 0.6;
}

bool FrontPanelModel::isEditOn() const
{
    return processor.isPresetEdited();
}

int FrontPanelModel::getSoundLed() const noexcept
{
    if (mode != Mode::sound)
        return -1;

    return item < 8 ? item : item - 8;
}

bool FrontPanelModel::isSoundLedBlinking() const noexcept
{
    return mode == Mode::sound && item >= 8;
}

int FrontPanelModel::getMasterLed() const noexcept
{
    switch (mode)
    {
        case Mode::master: return item;
        case Mode::store:  return masterStoreItem;
        case Mode::second: return item;
        case Mode::preset:
        case Mode::sound:  break;
    }

    return -1;
}

bool FrontPanelModel::isMasterLedBlinking() const noexcept
{
    return mode == Mode::second || mode == Mode::store;
}
