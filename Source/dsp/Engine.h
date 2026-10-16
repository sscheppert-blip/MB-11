#pragma once

#include <array>
#include <cstdint>

// The MB-11 sound engine: a monophonic model of an analog bass drum circuit.
//
//   gate ──► pitch envelope ─┐                ┌─► pulse ─┐
//            LFO ────────────┼─► VCO ─► VCA ──┤          ├─► attack VCA
//                            │   (harmonics)  │  noise ──┘        │
//            amp envelope ───┘  (decay/compr) └───────► Σ ◄───────┘
//                                                        │
//                                         EQ low-pass ─► saturation ─► volume
//
// All parameters are normalised to 0..1 (hardware value / 255) unless noted.
namespace mb11::dsp
{
enum class LfoWave { sawUp, sawDown, sine, sineInv, tri, triInv, rect, rectInv };

struct Settings
{
    float tune      = 0.5f;
    float pitch     = 0.4f;
    float decay     = 0.4f;
    float harmonics = 0.0f;
    float pulse     = 0.5f;
    float noise     = 0.2f;
    float attack    = 0.5f;
    float eq        = 0.0f;
    float volume    = 1.0f;
    float gate      = 0.2f;
    float compr     = 0.0f;
    int   metNoise  = 0;        // 0 = white noise, 1..255 = metallic pattern

    LfoWave lfoWave = LfoWave::sine;
    float lfoBpm     = 120.0f;  // one waveform cycle per quarter note
    float lfoInt     = 0.0f;
    bool  lfoSync    = false;
    bool  lfoOneShot = false;
};

// Mapping of the normalised parameters to physical values.
namespace mapping
{
    float pitchHz (float pitch);             // 10 Hz .. 500 Hz, exponential
    float tuneOctaves (float tune);          // pitch envelope depth
    float decaySeconds (float decay);        // amp envelope time constant
    float gateSeconds (float gate);          // 0.1 ms .. 16 ms
    float eqCutoffHz (float eq);             // 0 = open
    float comprThreshold (float compr);
    float lfoOctaves (float lfoInt);
}

// Random or metallic noise. Values above zero build a signal from square-wave
// multitones and a short LFSR whose frequencies and taps are derived from the
// bit pattern of the value, so every value has its own character.
class MetalNoise
{
public:
    void prepare (double sampleRate);
    void setPattern (int value);
    float next() noexcept;

private:
    float white() noexcept;
    void stepLfsr() noexcept;

    double sampleRate = 44100.0;
    int pattern = -1;
    std::array<double, 6> phase {}, increment {};
    uint32_t oscMask = 0;
    bool useLfsr = false;
    uint32_t lfsr = 1, lfsrTaps = 0x6000;
    double lfsrPhase = 0.0, lfsrIncrement = 0.0;
    uint32_t whiteState = 0x9e3779b9u;
};

// Tolerances of one "unit": every instance of the plugin behaves like its
// own piece of hardware. The default is the nominal circuit.
struct Unit
{
    float pitchScale  = 1.0f;    // VCO scale trim
    float tuneScale   = 1.0f;    // pitch envelope depth
    float decayScale  = 1.0f;    // capacitor tolerance
    float shaperDrive = 1.55f;   // triangle-to-sine shaper
    float asymmetry   = 0.03f;   // shaper offset -> even harmonics
    float feedthrough = 0.06f;   // VCA control voltage bleeding into the audio
    float droop       = 0.02f;   // amplitude dependent pitch

    static Unit fromSeed (uint32_t seed);
};

class Engine
{
public:
    // sampleRate is the internal (oversampled) rate, oversampling the factor
    // between it and the output rate.
    void prepare (double sampleRate, int oversampling = 1);
    void reset();

    void setUnit (const Unit&);

    // Call once per block (or after a parameter change inside a block).
    void setSettings (const Settings&);

    // Starts a new hit at the next rendered sample. velocity 0..1,
    // keyRatio scales the base frequency (split mode 2).
    void trigger (float velocity, float keyRatio = 1.0f);

    // Writes numSamples mono samples to out.
    void render (float* out, int numSamples) noexcept;

    bool isSilent() const noexcept;

private:
    float lfoValue (double phase) const noexcept;
    float random() noexcept;    // -1..1
    float vco (double phase) const noexcept;
    void updateDrift() noexcept;

    double sampleRate = 44100.0;
    float noiseGain = 1.0f;
    Settings settings;
    Unit unit;
    MetalNoise noiseSource;

    // derived coefficients
    double baseHz = 50.0, tuneOct = 2.0, lfoOct = 0.0;
    float decayCoef = 0.999f, pitchCoef = 0.999f, attackEnvCoef = 0.99f, chargeCoef = 0.5f;
    float comprThreshold = 0.0f, pulseHpCoef = 0.99f, pulseClickCoef = 0.99f, couplingCoef = 0.999f, dcCoef = 0.9995f;
    float noiseLpCoef = 0.5f, driftCoef = 0.9999f;
    float shaperOffset = 0.0f, shaperNorm = 1.0f, outputOffset = 0.0f, outputNorm = 1.0f;
    float harmonicsMix = 0.0f;
    int gateSamples = 1;

    // smoothed values
    float eqG = 1.0f, eqGTarget = 1.0f;
    float gain = 1.0f, gainTarget = 1.0f;
    float smoothCoef = 0.01f;

    // voice state
    double phase = 0.0;
    float pitchEnv = 0.0f, ampEnv = 0.0f, attackEnv = 0.0f;
    bool charging = false;
    int gateRemaining = 0;
    float velocity = 1.0f, keyRatio = 1.0f, hitDrift = 1.0f, decayDrift = 1.0f;
    float pulseHpState = 0.0f, pulseHpPrev = 0.0f, pulseClick = 0.0f;
    float noiseState = 0.0f, noiseState2 = 0.0f;
    float couplingIn = 0.0f, couplingOut = 0.0f;
    float eqState = 0.0f;
    float dcIn = 0.0f, dcOut = 0.0f;

    // slow thermal drift
    float drift = 0.0f, driftTarget = 0.0f;
    int driftCountdown = 0;

    // LFO
    double lfoPhase = 0.0;
    bool lfoDone = true, lfoStarted = false;

    uint32_t rng = 0x2545f491u;
};
}
