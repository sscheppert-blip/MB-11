#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// Parameter IDs and layout of the MB-11.
// Sound parameters use the hardware's internal resolution (0..255).
namespace mb11::params
{
    namespace id
    {
        // Sound parameters
        inline constexpr auto tune        = "tune";
        inline constexpr auto pitch       = "pitch";
        inline constexpr auto decay       = "decay";
        inline constexpr auto harmonics   = "harmonics";
        inline constexpr auto pulse       = "pulse";
        inline constexpr auto noise       = "noise";
        inline constexpr auto attack      = "attack";
        inline constexpr auto eq          = "eq";
        inline constexpr auto volume      = "volume";
        inline constexpr auto gate        = "gate";
        inline constexpr auto compr       = "compr";
        inline constexpr auto metNoise    = "metnze";

        // Master parameters
        inline constexpr auto midiChannel = "midich";
        inline constexpr auto splitMode   = "splitmode";
        inline constexpr auto pitchMode   = "pitchmode";
        inline constexpr auto lfoWave     = "lfowave";
        inline constexpr auto lfoSpeed    = "lfospeed";
        inline constexpr auto lfoInt      = "lfoint";
        inline constexpr auto lfoSync     = "lfosync";

        // 2nd function
        inline constexpr auto xTrigLevel  = "xtriglevel";
        inline constexpr auto xTrigOn     = "xtrigon";
        inline constexpr auto lfoOneShot  = "lfooneshot";
    }

    // LFO speed: lowest value means "Mid" (host/MIDI clock sync), then 40..290 BPM.
    inline constexpr int lfoSpeedSync = 39;
    inline constexpr int lfoSpeedMin  = 40;
    inline constexpr int lfoSpeedMax  = 290;

    // Parameter of each preset field, in mb11::presets::Field order.
    inline constexpr const char* presetFieldIds[] =
    {
        id::tune, id::pitch, id::decay, id::harmonics, id::pulse, id::noise, id::attack, id::eq,
        id::compr, id::gate, id::metNoise, id::volume,
        id::lfoSync, id::lfoOneShot, id::lfoInt, id::lfoSpeed, id::lfoWave
    };

    // Preset bytes store the LFO speed as BPM - 39 (0 = Mid).
    inline int presetByteToValue (int field, int byte)
    {
        return std::string_view (presetFieldIds[field]) == id::lfoSpeed ? byte + lfoSpeedSync : byte;
    }

    inline int valueToPresetByte (int field, int value)
    {
        return std::string_view (presetFieldIds[field]) == id::lfoSpeed ? value - lfoSpeedSync : value;
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
}
