#include "Presets.h"

namespace mb11::presets
{
namespace
{
    constexpr uint8_t sysexManufacturer = 0x31;
    constexpr uint8_t sysexDumpCommand  = 0x7f;
    constexpr uint8_t sysexProductCode  = 0x57;

    constexpr uint8_t speed120 = 120 - 39;

    struct FactorySound
    {
        const char* name;
        // tune pitch decay harm pulse noise attack eq compr gate metnze volume
        std::array<uint8_t, 12> sound;
        // lfo: sync oneShot int speed wave
        std::array<uint8_t, 5> lfo { 0, 0, 0, speed120, 2 };
    };

    const FactorySound factorySounds[] =
    {
        { "909 Classic",    { 150, 105, 110,  20, 160,  60, 150,  30,  30,  60,   0, 255 } },
        { "808 Boom",       {  40,  85, 170,   0,  60,   0,  80,  60,   0,  40,   0, 255 } },
        { "Techno Punch",   { 170, 110,  95,  70, 200,  40, 180,  20,  60,  90,   0, 255 } },
        { "Hard Pauke",     { 120, 120, 120, 200, 150,  30, 140,  40,  40,  70,   0, 255 } },
        { "Sub Drop",       {  90,  60, 230,   0,  40,   0,  50, 120,   0,  30,   0, 255 } },
        { "Click Kick",     { 200, 130,  75,  40, 230,  60, 220,   0, 160,  20,   0, 255 } },
        { "Clap Attack",    { 130, 100, 100,  30,  80, 220, 200,   0,  30, 200,   0, 255 } },
        { "Metal Kick",     { 140, 105, 105,  60, 120, 180, 170,  10,  40, 140,  77, 255 } },
        { "Gabber",         { 230, 125, 120, 255, 255,  80, 230,   0,  20, 100,   0, 240 } },
        { "Deep House",     {  70,  95, 130,  10,  90,  20,  90,  90,  50,  50,   0, 255 } },
        { "Minimal Tick",   { 110, 140,  70,  30, 200,  40, 160,  20, 170,  30,   0, 255 } },
        { "Bass Synth",     {  15, 100, 220, 120,  30,   0,  40, 100,   0,  40,   0, 255 } },
        { "Wobble Sync",    { 100,  90, 200,  50, 100,   0, 100,  50,   0,  50,   0, 255 }, { 1, 0, 120, 128 - 39, 2 } },
        { "Laser Drop",     {  60, 110, 180,  40, 120,  20, 120,  30,  20,  40,   0, 255 }, { 1, 1, 200,  80 - 39, 1 } },
        { "Tom Low",        {  60, 140, 120,  30,  90,  50, 100,  50,  30,  80,   0, 255 } },
        { "Tom High",       {  60, 175, 100,  30,  90,  50, 100,  40,  30,  80,   0, 255 } },
        { "Distant Thud",   {  80,  90, 120,   0, 100,  30, 120, 200,  40, 120,   0, 255 } },
        { "Trance Kick",    { 160, 115, 115,  50, 180,  50, 170,  25,  70,  70,   0, 255 } },
        { "Schranz",        { 210, 120, 140, 220, 220, 120, 220,  10,  90, 160, 201, 240 } },
        { "808 Glide",      {  30,  80, 235,  20,  50,   0,  60,  70,   0,  40,   0, 255 }, { 1, 1,  90,  60 - 39, 1 } },
        { "Electro",        { 180, 100, 150,  90, 140,  90, 150,  30,  20, 110,  13, 255 } },
        { "Industrial",     { 190,  95, 120, 180, 160, 255, 240,   0,  60, 230, 145, 235 } },
        { "Soft Thump",     {  50, 100,  90,   0,  40,   0,  50, 150,  40,  60,   0, 255 } },
        { "Drum & Bass",    { 190, 125,  80,  60, 190,  70, 190,  15, 110,  50,   0, 255 } },
        { "Kick Snare",     { 120, 150,  90,  40, 120, 240, 230,   0,  50, 240,   0, 245 } },
        { "Rim Shot",       { 100, 230,  35, 100, 230, 120, 200,   0, 150,  40,  99, 240 } },
        { "Sub Hum",        {  20,  75, 250,  30,  20,   0,  30, 120,   0,  30,   0, 255 }, { 0, 0,  40, speed120, 4 } },
        { "Hardstyle",      { 220, 118, 160, 240, 200,  60, 210,  20,  10, 150,   0, 235 } },
        { "Lo-Fi Box",      {  90, 110,  75, 150, 120, 150, 140, 140,  80, 120,  33, 255 } },
        { "Zap",            { 255, 140,  70,  30, 150,   0, 120,   0,  60,  20,   0, 255 } },
        { "Kaleido 1",      { 140, 108, 100,  70, 140, 200, 190,   5,  50, 180, 171, 250 } },
        { "Kaleido 2",      { 140, 108, 100,  70, 140, 230, 190,   5,  50, 210, 234, 250 } },
    };

    constexpr int numFactorySounds = (int) std::size (factorySounds);

    Data toData (const FactorySound& f)
    {
        // The sound bytes are in Field order (tune .. volume).
        Data d {};
        for (int i = 0; i < 12; ++i)
            d[(size_t) i] = f.sound[(size_t) i];

        d[lfoSync]    = f.lfo[0];
        d[lfoOneShot] = f.lfo[1];
        d[lfoInt]     = f.lfo[2];
        d[lfoSpeed]   = f.lfo[3];
        d[lfoWave]    = f.lfo[4];
        return d;
    }

    // Variations of the hand-made sounds fill the remaining factory slots.
    Data mutate (Data d, uint32_t seed)
    {
        auto rnd = [&seed]
        {
            seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
            return seed;
        };

        for (auto field : { tune, pitch, decay, harmonics, pulse, noise, attack, eq, compr, gate })
        {
            const auto delta = (int) (rnd() % 49) - 24;
            d[(size_t) field] = (uint8_t) juce::jlimit (0, 255, (int) d[(size_t) field] + delta);
        }

        if (d[metNoise] > 0)
            d[metNoise] = (uint8_t) (1 + rnd() % 255);

        return d;
    }
}

juce::String slotLabel (int slot)
{
    if (slot < numUser)
        return "Pr" + juce::String (slot);

    return "r" + juce::String (slot - numUser).paddedLeft ('0', 2);
}

std::vector<Preset> createFactoryBank()
{
    std::vector<Preset> bank ((size_t) numSlots);

    for (int i = 0; i < numFactory; ++i)
    {
        auto& slot = bank[(size_t) (numUser + i)];
        const auto& base = factorySounds[i % numFactorySounds];

        if (i < numFactorySounds)
        {
            slot.name = base.name;
            slot.data = toData (base);
        }
        else
        {
            slot.name = juce::String (base.name) + " " + juce::String (i / numFactorySounds + 1);
            slot.data = mutate (toData (base), 0x1234567u + (uint32_t) i * 7919u);
        }
    }

    for (int i = 0; i < numUser; ++i)
        bank[(size_t) i] = bank[(size_t) (numUser + i)];

    return bank;
}

//==============================================================================
juce::MemoryBlock toSysex (const Data& data, int slot)
{
    juce::MemoryOutputStream out;
    out.writeByte ((char) 0xf0);
    out.writeByte ((char) sysexManufacturer);
    out.writeByte ((char) sysexDumpCommand);
    out.writeByte ((char) sysexProductCode);
    out.writeByte ((char) (slot & 0x7f));

    for (int i = 0; i < numSysexFields; ++i)
    {
        const auto byte = data[(size_t) i];
        out.writeByte ((char) ((byte >> 7) & 1));
        out.writeByte ((char) (byte & 0x7f));
    }

    out.writeByte ((char) 0xf7);
    return out.getMemoryBlock();
}

std::optional<SysexData> fromSysex (const void* bytes, size_t size)
{
    const auto* b = static_cast<const uint8_t*> (bytes);
    constexpr size_t expectedSize = 5 + 2 * numSysexFields + 1;

    if (size < expectedSize || b[0] != 0xf0 || b[1] != sysexManufacturer
        || b[2] != sysexDumpCommand || b[3] != sysexProductCode || b[expectedSize - 1] != 0xf7)
        return std::nullopt;

    SysexData d {};

    for (int i = 0; i < numSysexFields; ++i)
    {
        const auto msb = b[5 + 2 * i];
        const auto lsb = b[6 + 2 * i];

        if (msb > 1 || lsb > 0x7f)
            return std::nullopt;

        d[(size_t) i] = (uint8_t) ((msb << 7) | lsb);
    }

    return d;
}

//==============================================================================
Bank::Bank()
    : slots (createFactoryBank())
{
    load();
}

const Preset& Bank::get (int slot) const
{
    return slots[(size_t) juce::jlimit (0, numSlots - 1, slot)];
}

void Bank::store (int slot, const Data& data)
{
    if (! juce::isPositiveAndBelow (slot, numSlots))
        return;

    slots[(size_t) slot].data = data;
    save();
}

juce::File Bank::getFile()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("MB-11")
        .getChildFile ("PresetBank.xml");
}

void Bank::load()
{
    const auto xml = juce::XmlDocument::parse (getFile());

    if (xml == nullptr || ! xml->hasTagName ("MB11Bank"))
        return;

    for (auto* e : xml->getChildWithTagNameIterator ("Preset"))
    {
        const auto slot = e->getIntAttribute ("slot", -1);

        if (! juce::isPositiveAndBelow (slot, numSlots))
            continue;

        const auto values = juce::StringArray::fromTokens (e->getStringAttribute ("data"), " ", "");

        if (values.size() != numFields)
            continue;

        auto& target = slots[(size_t) slot];

        for (int i = 0; i < numFields; ++i)
            target.data[(size_t) i] = (uint8_t) juce::jlimit (0, 255, values[i].getIntValue());

        target.name = e->getStringAttribute ("name", target.name);
    }
}

void Bank::save() const
{
    juce::XmlElement xml ("MB11Bank");
    xml.setAttribute ("version", 1);

    for (int slot = 0; slot < numSlots; ++slot)
    {
        const auto& preset = slots[(size_t) slot];
        juce::StringArray values;

        for (auto v : preset.data)
            values.add (juce::String (v));

        auto* e = xml.createNewChildElement ("Preset");
        e->setAttribute ("slot", slot);
        e->setAttribute ("name", preset.name);
        e->setAttribute ("data", values.joinIntoString (" "));
    }

    const auto file = getFile();
    file.getParentDirectory().createDirectory();
    xml.writeTo (file);
}
}
