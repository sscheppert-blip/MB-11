#include "Parameters.h"

namespace mb11::params
{
namespace
{
    constexpr int version = 1;

    std::unique_ptr<juce::AudioParameterInt> byteParam (const char* paramId, const char* name, int defaultValue)
    {
        return std::make_unique<juce::AudioParameterInt> (juce::ParameterID { paramId, version },
                                                          name, 0, 255, defaultValue);
    }

    std::unique_ptr<juce::AudioParameterChoice> choiceParam (const char* paramId, const char* name,
                                                             juce::StringArray choices, int defaultIndex)
    {
        return std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { paramId, version },
                                                             name, std::move (choices), defaultIndex);
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto sound = std::make_unique<juce::AudioProcessorParameterGroup> ("sound", "Sound", "|");
    sound->addChild (byteParam (id::tune,      "Tune",      150),
                     byteParam (id::pitch,     "Pitch",     105),
                     byteParam (id::decay,     "Decay",     110),
                     byteParam (id::harmonics, "Harmonics", 20),
                     byteParam (id::pulse,     "Pulse",     160),
                     byteParam (id::noise,     "Noise",     60),
                     byteParam (id::attack,    "Attack",    150),
                     byteParam (id::eq,        "EQ",        30),
                     byteParam (id::volume,    "Volume",    255),
                     byteParam (id::gate,      "Gate",      60),
                     byteParam (id::compr,     "Compr",     30),
                     byteParam (id::metNoise,  "MetNze",    0));

    auto master = std::make_unique<juce::AudioProcessorParameterGroup> ("master", "Master", "|");
    master->addChild (
        std::make_unique<juce::AudioParameterInt> (juce::ParameterID { id::midiChannel, version }, "Midi Ch", 1, 16, 1),
        choiceParam (id::splitMode, "Split Mode", { "SM1", "SM2" }, 0),
        choiceParam (id::pitchMode, "Pitch Mode", { "Lin", "SEM" }, 1),
        choiceParam (id::lfoWave,   "LFO Wave",   { "SuP", "Sdo", "Sin", "Si-", "tri", "tr-", "rCt", "rC-" }, 2),
        std::make_unique<juce::AudioParameterInt> (
            juce::ParameterID { id::lfoSpeed, version }, "LFO Speed", lfoSpeedSync, lfoSpeedMax, 120,
            juce::AudioParameterIntAttributes().withStringFromValueFunction ([] (int v, int)
            {
                return v <= lfoSpeedSync ? juce::String ("Mid") : juce::String (v);
            })),
        byteParam (id::lfoInt, "LFO Int", 0),
        choiceParam (id::lfoSync, "LFO Sync", { "FrE", "SYn" }, 0));

    auto second = std::make_unique<juce::AudioProcessorParameterGroup> ("second", "2nd Function", "|");
    second->addChild (byteParam (id::xTrigLevel, "X Trig Level", 150),
                      choiceParam (id::xTrigOn,    "X Trig",        { "oFF", "_on" }, 0),
                      choiceParam (id::lfoOneShot, "LFO One Shot",  { "oFF", "onE" }, 0));

    layout.add (std::move (sound), std::move (master), std::move (second));
    return layout;
}
}
