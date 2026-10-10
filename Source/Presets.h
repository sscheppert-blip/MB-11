#pragma once

#include <juce_core/juce_core.h>

#include <array>
#include <optional>
#include <vector>

// Preset memory of the MB-11: 10 user presets (Pr0..Pr9) followed by
// 100 writable factory presets (r00..r99), like the hardware.
namespace mb11::presets
{
    // One preset holds the 16 bytes of the hardware dump (in MIDI CC order)
    // plus the LFO wave.
    enum Field
    {
        tune, pitch, decay, harmonics, pulse, noise, attack, eq,
        compr, gate, metNoise, volume,
        lfoSync, lfoOneShot, lfoInt, lfoSpeed,   // lfoSpeed: 0 = Mid, else BPM - 39
        lfoWave,
        numFields
    };

    inline constexpr int numSysexFields = 16;
    inline constexpr int numUser = 10;
    inline constexpr int numFactory = 100;
    inline constexpr int numSlots = numUser + numFactory;

    using Data = std::array<uint8_t, numFields>;
    using SysexData = std::array<uint8_t, numSysexFields>;

    struct Preset
    {
        juce::String name;
        Data data {};
    };

    // "Pr0".."Pr9", "r00".."r99"
    juce::String slotLabel (int slot);

    // The factory bank (all 110 slots).
    std::vector<Preset> createFactoryBank();

    // SysEx: F0 31 7F 57 <preset> <16 x (MSB, LSB)> F7
    juce::MemoryBlock toSysex (const Data&, int slot);
    std::optional<SysexData> fromSysex (const void* bytes, size_t size);

    // The preset memory, shared by all instances through a file in the user's
    // application data folder (like the flash memory of the device).
    class Bank
    {
    public:
        Bank();

        const Preset& get (int slot) const;
        void store (int slot, const Data&);

        static juce::File getFile();

    private:
        void load();
        void save() const;

        std::vector<Preset> slots;
    };
}
