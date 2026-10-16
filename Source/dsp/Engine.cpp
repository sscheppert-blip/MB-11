#include "Engine.h"

#include <algorithm>
#include <bit>
#include <cmath>

namespace mb11::dsp
{
namespace
{
    constexpr double twoPi = 6.283185307179586;

    // attack pulse: level and the knock left by the discharging trigger capacitor
    constexpr float pulseGain = 1.3f;
    constexpr float pulseKnock = 1.0f;
    constexpr double pulseKnockSeconds = 0.004;

    uint32_t hash (uint32_t value, uint32_t salt) noexcept
    {
        // splitmix32-style integer hash
        uint32_t x = value * 0x9e3779b1u + salt * 0x85ebca6bu + 0x27d4eb2fu;
        x ^= x >> 16; x *= 0x7feb352du;
        x ^= x >> 15; x *= 0x846ca68bu;
        x ^= x >> 16;
        return x;
    }

    float onePoleCoef (double seconds, double sampleRate) noexcept
    {
        return (float) std::exp (-1.0 / std::max (1.0e-6, seconds * sampleRate));
    }
}

//==============================================================================
namespace mapping
{
    float pitchHz (float pitch)          { return 10.0f * std::pow (50.0f, pitch); }
    float tuneOctaves (float tune)       { return 6.0f * std::pow (tune, 1.5f); }
    float decaySeconds (float decay)     { return 0.01f * std::pow (150.0f, decay); }
    float gateSeconds (float gate)       { return 0.0001f * std::pow (160.0f, gate); }
    float eqCutoffHz (float eq)          { return 20000.0f * std::pow (80.0f / 20000.0f, eq); }
    float comprThreshold (float compr)   { return 0.75f * compr * compr; }
    float lfoOctaves (float lfoInt)      { return 2.0f * lfoInt * lfoInt; }
}

//==============================================================================
void MetalNoise::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;
    const auto p = pattern;
    pattern = -1;
    setPattern (std::max (0, p));
}

void MetalNoise::setPattern (int value)
{
    value = std::clamp (value, 0, 255);

    if (value == pattern)
        return;

    pattern = value;

    if (value == 0)
        return;

    const auto v = (uint32_t) value;

    for (uint32_t i = 0; i < increment.size(); ++i)
    {
        const auto frac = (double) (hash (v, i) & 0xffffu) / 65535.0;
        increment[i] = 180.0 * std::pow (2.0, frac * 5.2) / sampleRate;
    }

    oscMask = (v ^ (hash (v, 9) >> 7)) & 0x3fu;

    if (std::popcount (oscMask) < 2)
        oscMask |= 0x3u << (v % 5);

    useLfsr = (v & 0x80u) != 0 || std::popcount (oscMask) < 3;
    lfsrTaps = 0x4000u | (hash (v, 7) & 0x3ffeu) | 1u;
    lfsr = 1u + (hash (v, 11) & 0x3fffu);
    lfsrIncrement = 1500.0 * std::pow (2.0, (double) (hash (v, 13) & 0xffffu) / 65535.0 * 3.5) / sampleRate;
}

float MetalNoise::white() noexcept
{
    whiteState ^= whiteState << 13;
    whiteState ^= whiteState >> 17;
    whiteState ^= whiteState << 5;
    return (float) whiteState * (2.0f / 4294967296.0f) - 1.0f;
}

void MetalNoise::stepLfsr() noexcept
{
    lfsr = (lfsr & 1u) != 0 ? (lfsr >> 1) ^ lfsrTaps : lfsr >> 1;

    if (lfsr == 0)
        lfsr = 1;
}

float MetalNoise::next() noexcept
{
    if (pattern <= 0)
        return white();

    uint32_t bit = 0;

    for (size_t i = 0; i < phase.size(); ++i)
    {
        phase[i] += increment[i];

        if (phase[i] >= 1.0)
            phase[i] -= 1.0;

        if ((oscMask >> i) & 1u)
            bit ^= phase[i] < 0.5 ? 1u : 0u;
    }

    if (useLfsr)
    {
        lfsrPhase += lfsrIncrement;

        while (lfsrPhase >= 1.0)
        {
            lfsrPhase -= 1.0;
            stepLfsr();
        }

        bit ^= lfsr & 1u;
    }

    return bit != 0 ? 0.8f : -0.8f;
}

//==============================================================================
Unit Unit::fromSeed (uint32_t seed)
{
    auto spread = [seed] (uint32_t salt, float amount)
    {
        const auto h = hash (seed, salt);
        return 1.0f + amount * ((float) (h & 0xffffu) / 32767.5f - 1.0f);
    };

    Unit u;
    u.pitchScale  *= spread (1, 0.012f);
    u.tuneScale   *= spread (2, 0.04f);
    u.decayScale  *= spread (3, 0.05f);
    u.shaperDrive *= spread (4, 0.06f);
    u.asymmetry   *= spread (5, 0.4f);
    u.feedthrough *= spread (6, 0.2f);
    u.droop       *= spread (7, 0.25f);
    return u;
}

//==============================================================================
void Engine::prepare (double newSampleRate, int oversampling)
{
    sampleRate = newSampleRate;
    noiseSource.prepare (sampleRate);

    // white noise keeps its in-band level at any oversampling factor
    noiseGain = std::sqrt ((float) std::max (1, oversampling));

    chargeCoef    = 1.0f - onePoleCoef (0.0003, sampleRate);
    pitchCoef     = onePoleCoef (0.045, sampleRate);    // measured on hardware recordings: 40..50 ms
    attackEnvCoef = onePoleCoef (0.002, sampleRate);
    pulseHpCoef   = onePoleCoef (1.0 / (twoPi * 60.0), sampleRate);
    pulseClickCoef = onePoleCoef (pulseKnockSeconds, sampleRate);
    noiseLpCoef   = onePoleCoef (1.0 / (twoPi * 6000.0), sampleRate);
    couplingCoef  = onePoleCoef (1.0 / (twoPi * 9.0), sampleRate);
    dcCoef        = onePoleCoef (1.0 / (twoPi * 3.0), sampleRate);
    driftCoef     = onePoleCoef (1.5, sampleRate);
    smoothCoef    = 1.0f - onePoleCoef (0.005, sampleRate);

    setUnit (unit);
    setSettings (settings);
    eqG = eqGTarget;
    gain = gainTarget * velocity;
    reset();
}

void Engine::reset()
{
    phase = 0.0;
    pitchEnv = ampEnv = attackEnv = 0.0f;
    charging = false;
    gateRemaining = 0;
    pulseHpState = pulseHpPrev = pulseClick = 0.0f;
    noiseState = noiseState2 = 0.0f;
    couplingIn = couplingOut = 0.0f;
    eqState = 0.0f;
    dcIn = dcOut = 0.0f;
    lfoPhase = 0.0;
    lfoDone = true;
    lfoStarted = false;
}

void Engine::setUnit (const Unit& newUnit)
{
    unit = newUnit;

    const auto k = unit.shaperDrive;
    const auto a = unit.asymmetry;
    shaperOffset = std::tanh (a);
    shaperNorm = 1.0f / (std::tanh (k + a) - shaperOffset);

    // output op-amp: slightly asymmetric soft clipping
    outputOffset = std::tanh (0.08f);
    outputNorm = 1.0f / (std::tanh (1.4f + 0.08f) - outputOffset);
}

void Engine::setSettings (const Settings& s)
{
    settings = s;

    baseHz         = mapping::pitchHz (s.pitch);
    tuneOct        = mapping::tuneOctaves (s.tune);
    lfoOct         = mapping::lfoOctaves (s.lfoInt);
    comprThreshold = mapping::comprThreshold (s.compr);
    harmonicsMix   = std::pow (std::clamp (s.harmonics, 0.0f, 1.0f), 0.8f);
    gateSamples    = std::max (1, (int) std::lround (mapping::gateSeconds (s.gate) * sampleRate));
    decayCoef      = onePoleCoef (mapping::decaySeconds (s.decay) * unit.decayScale * decayDrift, sampleRate);

    const auto cutoff = std::min ((double) mapping::eqCutoffHz (s.eq), 0.45 * sampleRate);
    const auto g = std::tan (twoPi * 0.5 * cutoff / sampleRate);
    eqGTarget = (float) (g / (1.0 + g));

    gainTarget = std::pow (std::clamp (s.volume, 0.0f, 1.0f), 1.5f);

    noiseSource.setPattern (s.metNoise);
}

void Engine::trigger (float newVelocity, float newKeyRatio)
{
    // Like the analog circuit, a hit on a still ringing oscillator does not
    // reset its phase; only a quiet oscillator starts from zero.
    velocity = std::clamp (newVelocity, 0.0f, 1.0f);

    if (ampEnv < 0.03f)
    {
        phase = 0.0;
        gain = gainTarget * velocity;
    }

    keyRatio = newKeyRatio;
    gateRemaining = gateSamples;
    charging = true;
    attackEnv = 1.0f;
    pulseClick = 1.0f;

    // component noise: slight variation per hit
    hitDrift = 1.0f + 0.002f * random();
    decayDrift = 1.0f + 0.015f * random();
    decayCoef = onePoleCoef (mapping::decaySeconds (settings.decay) * unit.decayScale * decayDrift, sampleRate);

    if (settings.lfoSync || settings.lfoOneShot)
    {
        lfoPhase = 0.0;
        lfoDone = false;
    }

    lfoStarted = true;
}

float Engine::random() noexcept
{
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return (float) rng * (2.0f / 4294967296.0f) - 1.0f;
}

float Engine::lfoValue (double p) const noexcept
{
    const auto x = (float) p;

    switch (settings.lfoWave)
    {
        case LfoWave::sawUp:   return 2.0f * x - 1.0f;
        case LfoWave::sawDown: return 1.0f - 2.0f * x;
        case LfoWave::sine:    return (float) std::sin (twoPi * p);
        case LfoWave::sineInv: return (float) -std::sin (twoPi * p);
        case LfoWave::tri:     return 1.0f - 4.0f * std::abs (x - 0.5f);
        case LfoWave::triInv:  return 4.0f * std::abs (x - 0.5f) - 1.0f;
        case LfoWave::rect:    return x < 0.5f ? 1.0f : -1.0f;
        case LfoWave::rectInv: return x < 0.5f ? -1.0f : 1.0f;
    }

    return 0.0f;
}

// Triangle core with a tanh (OTA) shaper instead of an ideal sine: a slightly
// imperfect, asymmetric sine like the real circuit. Harmonics morphs towards a
// parabolic wave derived from the same core.
float Engine::vco (double p) const noexcept
{
    const auto x = (float) p;
    const auto tri = x < 0.25f ? 4.0f * x : (x < 0.75f ? 2.0f - 4.0f * x : 4.0f * x - 4.0f);
    const auto sine = (std::tanh (unit.shaperDrive * tri + unit.asymmetry) - shaperOffset) * shaperNorm;

    auto q = x - 0.25f;                       // parabola peak aligned with the sine peak
    q -= std::floor (q + 0.5f);
    const auto parabola = 0.79f * (1.0f - 12.0f * q * q);

    return sine + harmonicsMix * (parabola - sine);
}

// Slow random walk, like the temperature drift of the expo converter.
void Engine::updateDrift() noexcept
{
    if (--driftCountdown <= 0)
    {
        driftTarget = random();
        driftCountdown = (int) (sampleRate * (0.5 + 0.5 * (random() + 1.0f)));
    }

    drift = driftTarget + driftCoef * (drift - driftTarget);
}

bool Engine::isSilent() const noexcept
{
    return gateRemaining == 0 && ampEnv < 1.0e-5f && attackEnv < 1.0e-5f && pulseClick < 1.0e-5f
        && std::abs (eqState) < 1.0e-6f && std::abs (dcOut) < 1.0e-6f && std::abs (couplingOut) < 1.0e-6f;
}

void Engine::render (float* out, int numSamples) noexcept
{
    const auto lfoIncrement = settings.lfoBpm / 60.0 / sampleRate;
    const auto nyquistLimit = 0.45 * sampleRate;
    const auto pulseLevel = settings.pulse * pulseGain;
    const auto noiseLevel = settings.noise * 0.9f * noiseGain;
    const auto attackGain = settings.attack * 1.2f;
    const auto vcaNorm = 1.0f / std::tanh (1.1f);
    const auto baseFrequency = baseHz * unit.pitchScale;
    const auto tuneDepth = tuneOct * unit.tuneScale;

    for (int n = 0; n < numSamples; ++n)
    {
        updateDrift();

        // LFO
        float lfo = 0.0f;

        if (lfoOct > 0.0)
        {
            if (settings.lfoOneShot)
            {
                if (lfoStarted)
                    lfo = lfoValue (lfoDone ? 1.0 : lfoPhase);

                if (! lfoDone && (lfoPhase += lfoIncrement) >= 1.0)
                    lfoDone = true;
            }
            else
            {
                lfo = lfoValue (lfoPhase);

                if ((lfoPhase += lfoIncrement) >= 1.0)
                    lfoPhase -= 1.0;
            }
        }

        // trigger gate and capacitor envelopes
        const bool gateOn = gateRemaining > 0;

        if (gateOn)
        {
            --gateRemaining;
            pitchEnv = 1.0f;
            attackEnv = 1.0f;
        }
        else
        {
            pitchEnv *= pitchCoef;
            attackEnv *= attackEnvCoef;
        }

        if (charging)
        {
            ampEnv += (1.0f - ampEnv) * chargeCoef;

            if (ampEnv > 0.995f && ! gateOn)
                charging = false;
        }
        else
        {
            ampEnv *= decayCoef;
        }

        // Compr: a threshold subtracted from the envelope makes the tail fade out early.
        const auto shapedEnv = std::max (0.0f, ampEnv - comprThreshold) / (1.0f - comprThreshold);

        // VCO with a slightly non-ideal expo converter, amplitude dependent
        // pitch (louder = a little higher) and thermal drift
        const auto octaves = tuneDepth * pitchEnv + lfoOct * lfo;
        const auto expo = std::exp2 (octaves * (1.0 + 0.012 * octaves));
        const auto freq = std::clamp (baseFrequency * keyRatio * hitDrift * expo
                                          * (1.0 + unit.droop * ampEnv) * (1.0 + 0.0015 * drift),
                                      1.0, nyquistLimit);

        phase += freq / sampleRate;
        phase -= std::floor (phase);

        // OTA VCA: soft saturation and a little control voltage feedthrough,
        // which the coupling capacitor turns into a low thump
        const auto vcaIn = vco (phase) * shapedEnv + unit.feedthrough * shapedEnv;
        const auto body = std::tanh (1.1f * vcaIn) * vcaNorm;

        // attack section: rectangular pulse for the gate time plus noise
        const auto pulseRaw = gateOn ? 1.0f : 0.0f;
        pulseHpState = pulseHpCoef * (pulseHpState + pulseRaw - pulseHpPrev);
        pulseHpPrev = pulseRaw;

        // the discharge of the trigger capacitor adds a short knock after the edge
        const auto pulseSig = pulseHpState + pulseKnock * pulseClick;
        pulseClick *= pulseClickCoef;

        // the noise transistor is band limited (two poles, as dark as on hardware recordings)
        const auto noiseIn = noiseSource.next();
        noiseState = noiseIn + noiseLpCoef * (noiseState - noiseIn);
        noiseState2 = noiseState + noiseLpCoef * (noiseState2 - noiseState);
        const auto noiseSig = noiseState2 * attackEnv;
        const auto attackSig = attackGain * (pulseLevel * pulseSig + noiseLevel * noiseSig);

        auto x = 0.9f * body + 0.8f * attackSig;

        // coupling capacitor between the stages
        const auto coupled = x - couplingIn + couplingCoef * couplingOut;
        couplingIn = x;
        couplingOut = coupled;
        x = coupled;

        // EQ: smooth one-pole low-pass (TPT)
        eqG += (eqGTarget - eqG) * smoothCoef;
        const auto v = (x - eqState) * eqG;
        x = v + eqState;
        eqState = x + v;

        // output stage: asymmetric soft clipping, DC blocker, volume
        x = (std::tanh (1.4f * x + 0.08f) - outputOffset) * outputNorm;

        const auto dc = x - dcIn + dcCoef * dcOut;
        dcIn = x;
        dcOut = dc;

        // velocity is smoothed with the volume so a retrigger never clicks
        gain += (gainTarget * velocity - gain) * smoothCoef;
        out[n] = dc * gain;
    }

    if (std::abs (eqState) < 1.0e-12f)      eqState = 0.0f;
    if (std::abs (dcOut) < 1.0e-12f)        dcOut = 0.0f;
    if (std::abs (couplingOut) < 1.0e-12f)  couplingOut = 0.0f;
    if (std::abs (pulseHpState) < 1.0e-12f) pulseHpState = 0.0f;
    if (pulseClick < 1.0e-12f)              pulseClick = 0.0f;
    if (std::abs (noiseState) < 1.0e-12f)   noiseState = 0.0f;
    if (std::abs (noiseState2) < 1.0e-12f)  noiseState2 = 0.0f;
}
}
