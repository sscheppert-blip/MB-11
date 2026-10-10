// Offline tests for the MB-11 sound engine and preset handling.

#include "Parameters.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "Presets.h"
#include "dsp/Engine.h"
#include "ui/FrontPanelModel.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string_view>
#include <vector>

namespace
{
    int failures = 0;

    void check (bool condition, const char* what)
    {
        std::printf ("%s  %s\n", condition ? "PASS" : "FAIL", what);

        if (! condition)
            ++failures;
    }

    mb11::dsp::Settings settingsFrom (const mb11::presets::Data& d)
    {
        using namespace mb11::presets;
        auto byte = [&d] (Field f) { return d[(size_t) f] / 255.0f; };

        mb11::dsp::Settings s;
        s.tune = byte (tune);       s.pitch = byte (pitch);     s.decay = byte (decay);
        s.harmonics = byte (harmonics); s.pulse = byte (pulse); s.noise = byte (noise);
        s.attack = byte (attack);   s.eq = byte (eq);           s.volume = byte (volume);
        s.gate = byte (gate);       s.compr = byte (compr);     s.metNoise = d[metNoise];
        s.lfoWave = (mb11::dsp::LfoWave) d[lfoWave];
        s.lfoInt = byte (lfoInt);
        s.lfoSync = d[lfoSync] != 0;
        s.lfoOneShot = d[lfoOneShot] != 0;
        s.lfoBpm = d[lfoSpeed] == 0 ? 120.0f : (float) (d[lfoSpeed] + 39);
        return s;
    }

    std::vector<float> renderHit (const mb11::dsp::Settings& s, double sampleRate, double seconds, float velocity = 1.0f)
    {
        mb11::dsp::Engine engine;
        engine.setSettings (s);
        engine.prepare (sampleRate);
        engine.trigger (velocity);

        std::vector<float> out ((size_t) (seconds * sampleRate));
        engine.render (out.data(), (int) out.size());
        return out;
    }

    float peak (const std::vector<float>& x, size_t from = 0, size_t to = SIZE_MAX)
    {
        float p = 0.0f;
        for (auto i = from; i < std::min (to, x.size()); ++i)
            p = std::max (p, std::abs (x[i]));
        return p;
    }

    double energy (const std::vector<float>& x, size_t from, size_t to)
    {
        double e = 0.0;
        for (auto i = from; i < std::min (to, x.size()); ++i)
            e += (double) x[i] * x[i];
        return e;
    }

    bool allFinite (const std::vector<float>& x)
    {
        for (auto v : x)
            if (! std::isfinite (v))
                return false;
        return true;
    }

    double measureFrequency (const std::vector<float>& x, double sampleRate, size_t from, size_t to)
    {
        int crossings = 0;
        size_t first = 0, last = 0;

        for (auto i = from + 1; i < std::min (to, x.size()); ++i)
        {
            if (x[i - 1] < 0.0f && x[i] >= 0.0f)
            {
                if (crossings == 0)
                    first = i;
                last = i;
                ++crossings;
            }
        }

        return crossings > 1 ? (crossings - 1) * sampleRate / (double) (last - first) : 0.0;
    }
}

// Writes one hit of each of the first factory presets to a 16-bit WAV file.
static int renderDemo (const char* path)
{
    const auto bank = mb11::presets::createFactoryBank();
    constexpr int sampleRate = 48000;
    constexpr int hitLength = sampleRate * 3 / 4;
    std::vector<float> out;

    for (int i = 0; i < 32; ++i)
    {
        const auto hit = renderHit (settingsFrom (bank[(size_t) (mb11::presets::numUser + i)].data), sampleRate, 0.75);
        out.insert (out.end(), hit.begin(), hit.begin() + hitLength);
    }

    juce::MemoryOutputStream wav;
    const auto dataSize = (uint32_t) out.size() * 2;
    wav.write ("RIFF", 4); wav.writeInt ((int) (36 + dataSize)); wav.write ("WAVE", 4);
    wav.write ("fmt ", 4); wav.writeInt (16); wav.writeShort (1); wav.writeShort (1);
    wav.writeInt (sampleRate); wav.writeInt (sampleRate * 2); wav.writeShort (2); wav.writeShort (16);
    wav.write ("data", 4); wav.writeInt ((int) dataSize);

    for (auto v : out)
        wav.writeShort ((short) juce::jlimit (-32767, 32767, (int) std::lround (v * 0.8f * 32767.0f)));

    const auto ok = juce::File (juce::String::fromUTF8 (path)).replaceWithData (wav.getData(), wav.getDataSize());
    std::printf ("%s %s\n", ok ? "wrote" : "could not write", path);
    return ok ? 0 : 1;
}

// Renders the editor to a PNG file.
static int snapshot (const char* path)
{
    MB11AudioProcessor processor;
    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
    editor->setSize (FrontPanel::designWidth * 2, FrontPanel::designHeight * 2);

    const auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
    juce::FileOutputStream stream (juce::File (juce::String::fromUTF8 (path)));
    stream.setPosition (0);
    stream.truncate();
    const auto ok = stream.openedOk() && juce::PNGImageFormat().writeImageToStream (image, stream);
    std::printf ("%s %s\n", ok ? "wrote" : "could not write", path);
    return ok ? 0 : 1;
}

static float renderProcessor (MB11AudioProcessor& processor, juce::MidiBuffer midi, int blocks = 20)
{
    juce::AudioBuffer<float> buffer (2, 512);
    float peakLevel = 0.0f;

    for (int i = 0; i < blocks; ++i)
    {
        buffer.clear();
        auto events = i == 0 ? midi : juce::MidiBuffer();
        processor.processBlock (buffer, events);
        peakLevel = std::max (peakLevel, buffer.getMagnitude (0, 0, buffer.getNumSamples()));
    }

    return peakLevel;
}

static void testProcessor()
{
    namespace id = mb11::params::id;

    MB11AudioProcessor processor;
    processor.setPlayConfigDetails (0, 2, 48000.0, 512);
    processor.prepareToPlay (48000.0, 512);

    auto noteOn = [] (int channel, int note)
    {
        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::noteOn (channel, note, (juce::uint8) 120), 10);
        return midi;
    };

    // lets the previous hit die away
    auto settle = [&processor] { renderProcessor (processor, {}, 1000); };

    check (renderProcessor (processor, noteOn (1, 36)) > 0.1f, "note C1 on channel 1 triggers");
    settle();
    check (renderProcessor (processor, {}, 50) < 1.0e-3f, "silence without notes");
    check (renderProcessor (processor, noteOn (1, 38)) < 1.0e-3f, "split mode 1 ignores other notes");
    check (renderProcessor (processor, noteOn (2, 36)) < 1.0e-3f, "other MIDI channels are ignored");

    processor.setParameterInt (id::splitMode, 1);
    check (renderProcessor (processor, noteOn (1, 48)) > 0.1f, "split mode 2 plays C1..F3");
    settle();
    check (renderProcessor (processor, noteOn (1, 70)) < 1.0e-3f, "split mode 2 ignores notes above F3");
    processor.setParameterInt (id::splitMode, 0);

    juce::MidiBuffer cc;
    cc.addEvent (juce::MidiMessage::controllerEvent (1, 101, 127), 0);
    cc.addEvent (juce::MidiMessage::controllerEvent (1, 120, 64), 1);
    cc.addEvent (juce::MidiMessage::controllerEvent (1, 122, 0), 2);
    renderProcessor (processor, cc, 1);
    juce::MessageManager::getInstance()->runDispatchLoopUntil (50);
    check (processor.getParameterInt (id::pitch) == 255
           && processor.getParameterInt (id::lfoSync) == 1
           && processor.getParameterInt (id::lfoSpeed) == mb11::params::lfoSpeedSync, "MIDI CCs set parameters");

    processor.loadPreset (mb11::presets::numUser + 1);
    check (processor.getCurrentPreset() == mb11::presets::numUser + 1 && ! processor.isPresetEdited(), "loading a preset");
    processor.setParameterInt (id::decay, processor.getParameterInt (id::decay) + 1);
    check (processor.isPresetEdited(), "editing a parameter sets Edit-On");
}

static void testFrontPanel()
{
    using Mode = FrontPanelModel::Mode;
    namespace id = mb11::params::id;

    MB11AudioProcessor processor;
    FrontPanelModel model (processor);
    model.tick (1000.0);

    model.down();
    check (model.getMode() == Mode::sound && model.getItem() == 0 && model.getSoundLed() == 0, "Down from preset selects Tune");

    for (int i = 0; i < 8; ++i)
        model.down();
    check (model.getItem() == 8 && model.getSoundLed() == 0 && model.isSoundLedBlinking(), "past EQ: first LED blinks (Volume)");

    for (int i = 0; i < 4; ++i)
        model.down();
    check (model.getMode() == Mode::preset, "past MetNze back to preset selection");

    model.up();
    check (model.getMode() == Mode::sound && model.getItem() == 11, "Up from preset starts at MetNze");

    model.up(); model.up(); model.up(); model.up();
    check (model.getItem() == 7, "Up from Volume jumps to EQ");

    const auto eq = processor.getParameterInt (id::eq);
    model.encoderTurn (5);
    check (processor.getParameterInt (id::eq) == eq + 5 && model.getDisplayText() == juce::String (eq + 5).paddedLeft ('0', 3),
           "encoder changes the value");

    // pot pickup: moving below the value does nothing, crossing it catches
    processor.setParameterInt (id::eq, 200);
    model.potMoved (0.1f);
    model.potMoved (0.2f);
    check (processor.getParameterInt (id::eq) == 200 && ! model.isPotCaught(), "pot does not jump before pickup");
    model.potMoved (0.9f);
    check (model.isPotCaught() && processor.getParameterInt (id::eq) == 230, "pot picks up the value");

    model.encoderClick();
    check (! processor.isPresetEdited(), "encoder click reloads the preset");

    model.upDown();
    check (model.getMode() == Mode::master && model.getDisplayText() == "001", "Up+Down opens master (Midi Ch)");
    model.down();
    check (model.getDisplayText() == "SM1", "Split Mode shows SM1");
    model.tick (1000.0 + 10500.0);
    check (model.getMode() == Mode::sound && model.getItem() == 7, "master returns after 10 s");

    model.upPlay();
    check (model.getMode() == Mode::second && model.getDisplayText() == "150", "Up+Play opens 2nd function");
    model.down(); model.down();
    check (model.getDisplayText() == "dMP", "Sys Dmp item");
    model.upPlay();

    model.down(); model.down(); model.down(); model.down(); model.down();
    check (model.getMode() == Mode::preset && model.getDisplayText() == "r00", "preset display r00");
    model.encoderTurn (3);
    check (processor.getCurrentPreset() == mb11::presets::numUser + 3 && model.getDisplayText() == "r03", "encoder selects presets");
    model.encoderTurn (-100);
    check (model.getDisplayText() == "Pr0", "user presets Pr0..Pr9 come first");
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    if (argc == 3 && std::string_view (argv[1]) == "--render")
        return renderDemo (argv[2]);

    if (argc == 3 && std::string_view (argv[1]) == "--snapshot")
        return snapshot (argv[2]);

    if (argc == 2 && std::string_view (argv[1]) == "--bench")
    {
        MB11AudioProcessor processor;
        processor.setPlayConfigDetails (0, 2, 48000.0, 512);
        processor.prepareToPlay (48000.0, 512);
        juce::AudioBuffer<float> buffer (2, 512);
        const auto blocks = 48000 * 20 / 512;
        const auto start = juce::Time::getMillisecondCounterHiRes();

        for (int i = 0; i < blocks; ++i)
        {
            juce::MidiBuffer midi;
            if (i % 24 == 0)
                midi.addEvent (juce::MidiMessage::noteOn (1, 36, (juce::uint8) 110), 0);
            processor.processBlock (buffer, midi);
        }

        const auto ms = juce::Time::getMillisecondCounterHiRes() - start;
        std::printf ("20 s audio in %.0f ms (%.2f %% of one core), latency %d samples\n",
                     ms, ms / 200.0, processor.getLatencySamples());
        return 0;
    }

    testProcessor();
    testFrontPanel();

    const auto bank = mb11::presets::createFactoryBank();

    // every factory preset renders a sane signal at common sample rates
    for (auto sampleRate : { 44100.0, 48000.0, 96000.0 })
    {
        bool finite = true, audible = true, bounded = true;

        for (const auto& preset : bank)
        {
            const auto out = renderHit (settingsFrom (preset.data), sampleRate, 1.0);
            finite &= allFinite (out);
            audible &= peak (out) > 0.05f;
            bounded &= peak (out) < 1.6f;
        }

        char text[128];
        std::snprintf (text, sizeof (text), "all presets finite/audible/bounded at %.0f Hz", sampleRate);
        check (finite && audible && bounded, text);
    }

    // the base pitch follows the mapping when there is no pitch envelope
    {
        mb11::dsp::Settings s;
        s.tune = 0.0f; s.harmonics = 0.0f; s.pulse = 0.0f; s.noise = 0.0f; s.attack = 0.0f;
        s.eq = 0.0f; s.compr = 0.0f; s.decay = 0.9f; s.pitch = 100.0f / 255.0f;

        const auto sampleRate = 48000.0;
        const auto out = renderHit (s, sampleRate, 1.0);
        const auto expected = mb11::dsp::mapping::pitchHz (s.pitch);
        const auto measured = measureFrequency (out, sampleRate, 4800, 48000);
        check (std::abs (measured - expected) / expected < 0.035, "base pitch matches mapping");
    }

    // tune adds a downward pitch sweep
    {
        mb11::dsp::Settings s;
        s.tune = 0.8f; s.pulse = 0.0f; s.noise = 0.0f; s.attack = 0.0f; s.decay = 0.9f;
        const auto sampleRate = 48000.0;
        const auto out = renderHit (s, sampleRate, 1.0);
        const auto early = measureFrequency (out, sampleRate, 0, 960);
        const auto late = measureFrequency (out, sampleRate, 24000, 48000);
        check (early > 2.0 * late, "tune creates a pitch sweep");
    }

    // decay: the sound dies away
    {
        mb11::dsp::Settings s;
        s.decay = 60.0f / 255.0f;
        const auto out = renderHit (s, 44100.0, 3.0);
        check (peak (out, 88200) < 1.0e-3f, "short decay is silent after 2 s");
    }

    // compr shortens the tail
    {
        mb11::dsp::Settings s;
        s.decay = 0.7f;
        s.compr = 0.0f;
        const auto open = renderHit (s, 44100.0, 1.5);
        s.compr = 0.8f;
        const auto compressed = renderHit (s, 44100.0, 1.5);
        check (energy (compressed, 13230, 44100) < 0.3 * energy (open, 13230, 44100), "compr shortens the tail");
    }

    // EQ removes the noise burst
    {
        mb11::dsp::Settings s;
        s.noise = 1.0f; s.attack = 1.0f; s.pulse = 0.0f; s.gate = 0.9f; s.tune = 0.0f; s.pitch = 0.2f;
        s.eq = 0.0f;
        const auto open = renderHit (s, 48000.0, 0.02);
        s.eq = 1.0f;
        const auto closed = renderHit (s, 48000.0, 0.02);

        auto highEnergy = [] (const std::vector<float>& x)
        {
            double e = 0.0;
            for (size_t i = 1; i < x.size(); ++i)
                e += (double) (x[i] - x[i - 1]) * (x[i] - x[i - 1]);
            return e;
        };

        check (highEnergy (closed) < 0.05 * highEnergy (open), "EQ filters the attack noise");
    }

    // pulse adds a clearly audible click at the default attack and gate
    {
        mb11::dsp::Settings s;
        s.noise = 0.0f; s.attack = 150.0f / 255.0f; s.gate = 60.0f / 255.0f;
        s.pulse = 0.0f;
        const auto without = renderHit (s, 48000.0, 0.01);
        s.pulse = 1.0f;
        const auto with = renderHit (s, 48000.0, 0.01);

        double click = 0.0;
        for (size_t i = 0; i < with.size(); ++i)
            click += (double) (with[i] - without[i]) * (with[i] - without[i]);

        check (click > 0.04 * energy (without, 0, without.size()), "pulse adds an audible click");
    }

    // velocity scales the level
    {
        mb11::dsp::Settings s;
        const auto loud = renderHit (s, 44100.0, 0.3, 1.0f);
        const auto soft = renderHit (s, 44100.0, 0.3, 0.5f);
        check (std::abs (peak (soft) / peak (loud) - 0.5f) < 0.05f, "velocity scales the level");
    }

    // metal noise patterns differ from each other and from white noise
    {
        mb11::dsp::MetalNoise a, b;
        a.prepare (48000.0); b.prepare (48000.0);
        a.setPattern (17); b.setPattern (18);
        int differences = 0;
        for (int i = 0; i < 4800; ++i)
            differences += std::abs (a.next() - b.next()) > 0.1f ? 1 : 0;
        check (differences > 100, "metal noise patterns differ per value");
    }

    // SysEx round trip
    {
        const auto& data = bank[12].data;
        const auto block = mb11::presets::toSysex (data, 12);
        const auto parsed = mb11::presets::fromSysex (block.getData(), block.getSize());
        bool same = parsed.has_value() && block.getSize() == 38;

        for (int i = 0; same && i < mb11::presets::numSysexFields; ++i)
            same = (*parsed)[(size_t) i] == data[(size_t) i];

        check (same, "SysEx dump round trip");
        check (! mb11::presets::fromSysex ("\xf0\x00\xf7", 3).has_value(), "SysEx rejects foreign data");
    }

    check (mb11::presets::slotLabel (0) == "Pr0" && mb11::presets::slotLabel (10) == "r00"
           && mb11::presets::slotLabel (109) == "r99", "preset slot labels");

    std::printf ("\n%s (%d failure%s)\n", failures == 0 ? "ALL PASSED" : "FAILED", failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
