#include "AudioProcessors.h"
#include "../dsp/dsp.h"
#include <algorithm>
#include <cmath>

// ModularOscillator Implementation
void ModularOscillator::init(float sr)
{
    sampleRate = sr;
    oscillator.Init(sampleRate);
    oscillator.SetWaveform(daisysp::Oscillator::WAVE_SAW);
    oscillator.SetFreq(440.0f);
    oscillator.SetAmp(0.8f);
    enabled = true;
}

float ModularOscillator::process(float input)
{
    if (!enabled)
        return 0.0f;
    return oscillator.Process();
}

void ModularOscillator::setParameter(const std::string &param, float value)
{
    if (param == "frequency")
    {
        oscillator.SetFreq(value);
    }
    else if (param == "amplitude")
    {
        oscillator.SetAmp(value);
    }
    else if (param == "waveform")
    {
        int waveform = static_cast<int>(value);
        oscillator.SetWaveform(waveform);
    }
    else if (param == "pulsewidth")
    {
        // Clamp and set pulse width for square wave
        pulseWidth = std::max(0.01f, std::min(0.99f, value));
        oscillator.SetPw(pulseWidth);
    }
    else if (param == "detuning")
    {
        detuning = value;
        // Detuning is handled in the voice level
    }
}

bool ModularOscillator::getParameter(const std::string &param, float &value) const
{
    if (param == "frequency")
    {
        value = oscillator.GetFreq();
        return true;
    }
    else if (param == "amplitude")
    {
        value = oscillator.GetAmp();
        return true;
    }
    else if (param == "pulsewidth")
    {
        value = pulseWidth;
        return true;
    }
    else if (param == "detuning")
    {
        value = detuning;
        return true;
    }
    return false;
}

void ModularOscillator::reset()
{
    oscillator.Reset();
}

void ModularOscillator::setFrequency(float freq)
{
    oscillator.SetFreq(freq);
}

void ModularOscillator::setAmplitude(float amp)
{
    oscillator.SetAmp(amp);
}

// ModularParticle Implementation
void ModularParticle::init(float sr)
{
    sampleRate = sr;
    particle.Init(sampleRate);
    particle.SetFreq(440.0f);
    particle.SetResonance(0.5f);
    particle.SetDensity(0.5f);
    particle.SetGain(0.8f);
    particle.SetSpread(2.0f);
    enabled = true;
}

float ModularParticle::process(float input)
{
    if (!enabled)
        return 0.0f;
    return particle.Process();
}

void ModularParticle::setParameter(const std::string &param, float value)
{
    if (param == "frequency")
    {
        particle.SetFreq(value);
    }
    else if (param == "resonance")
    {
        particle.SetResonance(std::max(0.5f, std::min(0.98f, value)));
    }
    else if (param == "density")
    {
        particle.SetDensity(std::max(0.1f, std::min(1.0f, value)));
    }
    else if (param == "spread")
    {
        particle.SetSpread(std::max(0.0f, std::min(10.0f, value)));
    }
    else if (param == "gain")
    {
        particle.SetGain(std::max(0.0f, std::min(1.0f, value)));
    }
    else if (param == "randomfreq")
    {
        particle.SetRandomFreq(std::max(0.0f, std::min(10.0f, value)));
    }
    else if (param == "sync")
    {
        particle.SetSync(value > 0.5f);
    }
}

bool ModularParticle::getParameter(const std::string &param, float &value) const
{
    if (param == "frequency")
    {
        value = particle.GetFreq();
        return true;
    }
    else if (param == "resonance")
    {
        value = particle.GetResonance();
        return true;
    }
    else if (param == "density")
    {
        value = particle.GetDensity();
        return true;
    }
    else if (param == "gain" || param == "amplitude")
    {
        value = particle.GetGain();
        return true;
    }
    else if (param == "randomfreq")
    {
        value = particle.GetRandomFreq();
        return true;
    }
    else if (param == "spread")
    {
        value = particle.GetSpread();
        return true;
    }
    else if (param == "sync")
    {
        value = particle.GetSync() ? 1.0f : 0.0f;
        return true;
    }
    return false;
}

void ModularParticle::reset()
{
    particle.Reset();
}

void ModularParticle::setFrequency(float freq)
{
    particle.SetFreq(freq);
}

void ModularParticle::setAmplitude(float amp)
{
    particle.SetGain(amp);
}

// ModularNoise Implementation
void ModularNoise::init(float sr)
{
    sampleRate = sr;
    noise.Init();
    amplitude = 0.5f;
    enabled = true;
}

float ModularNoise::process(float input)
{
    if (!enabled)
        return 0.0f;
    return noise.Process() * amplitude;
}

void ModularNoise::setParameter(const std::string &param, float value)
{
    if (param == "amplitude" || param == "gain")
    {
        amplitude = std::max(0.0f, std::min(1.0f, value));
    }
}

bool ModularNoise::getParameter(const std::string &param, float &value) const
{
    if (param == "amplitude" || param == "gain")
    {
        value = amplitude;
        return true;
    }
    return false;
}

void ModularNoise::reset()
{
    // White noise doesn't need reset
}

void ModularNoise::setFrequency(float freq)
{
    // Noise doesn't have frequency
}

void ModularNoise::setAmplitude(float amp)
{
    amplitude = amp;
}

// ModularVosim Implementation
void ModularVosim::init(float sr)
{
    sampleRate = sr;
    vosim.Init(sampleRate);
    vosim.SetFreq(440.0f);
    vosim.SetForm1Freq(800.0f);
    vosim.SetForm2Freq(1200.0f);
    enabled = true;
}

float ModularVosim::process(float input)
{
    if (!enabled)
        return 0.0f;
    return vosim.Process();
}

void ModularVosim::setParameter(const std::string &param, float value)
{
    if (param == "frequency")
    {
        vosim.SetFreq(value);
    }
    else if (param == "form1_freq")
    {
        vosim.SetForm1Freq(value);
    }
    else if (param == "form2_freq")
    {
        vosim.SetForm2Freq(value);
    }
    else if (param == "amplitude")
    {
        amplitude = value;
    }
}

bool ModularVosim::getParameter(const std::string &param, float &value) const
{
    if (param == "frequency")
    {
        value = vosim.GetFreq();
        return true;
    }
    else if (param == "form1_freq")
    {
        value = vosim.GetForm1Freq();
        return true;
    }
    else if (param == "form2_freq")
    {
        value = vosim.GetForm2Freq();
        return true;
    }
    else if (param == "amplitude")
    {
        value = amplitude;
        return true;
    }
    return false;
}

void ModularVosim::reset()
{
    vosim.Reset();
}

void ModularVosim::setFrequency(float freq)
{
    vosim.SetFreq(freq);
}

void ModularVosim::setAmplitude(float amp)
{
    amplitude = amp;
}

// ModularLadderFilter Implementation
void ModularLadderFilter::init(float sr)
{
    sampleRate = sr;
    filter.Init(sampleRate);
    filter.SetFreq(1000.0f);
    filter.SetRes(0.5f);
    enabled = true;
}

float ModularLadderFilter::process(float input)
{
    if (!enabled)
        return input;
    return filter.Process(input);
}

void ModularLadderFilter::setParameter(const std::string &param, float value)
{
    if (param == "frequency" || param == "cutoff")
    {
        filter.SetFreq(std::max(20.0f, std::min(20000.0f, value)));
    }
    else if (param == "resonance")
    {
        filter.SetRes(std::max(0.0f, std::min(1.0f, value)));
    }
    else if (param == "drive")
    {
        filter.SetDrive(std::max(0.0f, std::min(1.0f, value)));
    }
}

bool ModularLadderFilter::getParameter(const std::string &param, float &value) const
{
    if (param == "frequency" || param == "cutoff")
    {
        value = filter.GetFreq();
        return true;
    }
    else if (param == "resonance")
    {
        value = filter.GetRes();
        return true;
    }
    return false;
}

void ModularLadderFilter::reset()
{
    filter.Reset();
}

// ModularSvfFilter Implementation
void ModularSvfFilter::init(float sr)
{
    sampleRate = sr;
    filter.Init(sampleRate);
    filter.SetFreq(1000.0f);
    filter.SetRes(0.5f);
    filter.SetDrive(0.0f);
    enabled = true;
    filterMode = 0; // Low-pass by default
}

float ModularSvfFilter::process(float input)
{
    if (!enabled)
        return input;

    filter.Process(input);

    switch (filterMode)
    {
    case 0:
        return filter.Low(); // Low-pass
    case 1:
        return filter.High(); // High-pass
    case 2:
        return filter.Band(); // Band-pass
    case 3:
        return filter.Notch(); // Notch
    default:
        return filter.Low();
    }
}

void ModularSvfFilter::setParameter(const std::string &param, float value)
{
    if (param == "frequency" || param == "cutoff")
    {
        filter.SetFreq(std::max(20.0f, std::min(20000.0f, value)));
    }
    else if (param == "resonance")
    {
        filter.SetRes(std::max(0.0f, std::min(1.0f, value)));
    }
    else if (param == "drive")
    {
        filter.SetDrive(std::max(0.0f, std::min(1.0f, value)));
    }
    else if (param == "mode")
    {
        filterMode = static_cast<int>(std::max(0.0f, std::min(3.0f, value)));
    }
}

bool ModularSvfFilter::getParameter(const std::string &param, float &value) const
{
    if (param == "frequency" || param == "cutoff")
    {
        value = filter.GetFreq();
        return true;
    }
    else if (param == "resonance")
    {
        value = filter.GetRes();
        return true;
    }
    else if (param == "mode")
    {
        value = static_cast<float>(filterMode);
        return true;
    }
    return false;
}

void ModularSvfFilter::reset()
{
    filter.Reset();
}

// ModularOverdrive Implementation
void ModularOverdrive::init(float sr)
{
    sampleRate = sr;
    overdrive.Init();
    overdrive.SetDrive(0.5f);
    enabled = true;
    mix = 1.0f;
}

float ModularOverdrive::process(float input)
{
    if (!enabled)
        return input;

    float processed = overdrive.Process(input);
    return input * (1.0f - mix) + processed * mix;
}

void ModularOverdrive::setParameter(const std::string &param, float value)
{
    if (param == "drive" || param == "amount")
    {
        overdrive.SetDrive(std::max(0.0f, std::min(1.0f, value)));
    }
    else if (param == "mix")
    {
        mix = std::max(0.0f, std::min(1.0f, value));
    }
}

bool ModularOverdrive::getParameter(const std::string &param, float &value) const
{
    if (param == "drive" || param == "amount")
    {
        value = overdrive.GetDrive();
        return true;
    }
    else if (param == "mix")
    {
        value = mix;
        return true;
    }
    return false;
}

void ModularOverdrive::reset()
{
    // Overdrive doesn't need reset
}

// ModularWavefolder Implementation
void ModularWavefolder::init(float sr)
{
    sampleRate = sr;
    wavefolder.Init();
    wavefolder.SetGain(0.5f);
    wavefolder.SetOffset(0.0f);
    enabled = true;
    mix = 1.0f;
}

float ModularWavefolder::process(float input)
{
    if (!enabled)
        return input;

    float processed = wavefolder.Process(input);
    return input * (1.0f - mix) + processed * mix;
}

void ModularWavefolder::setParameter(const std::string &param, float value)
{
    if (param == "gain" || param == "amount")
    {
        wavefolder.SetGain(std::max(0.0f, std::min(1.0f, value)));
    }
    else if (param == "offset")
    {
        wavefolder.SetOffset(std::max(-1.0f, std::min(1.0f, value)));
    }
    else if (param == "mix")
    {
        mix = std::max(0.0f, std::min(1.0f, value));
    }
}

bool ModularWavefolder::getParameter(const std::string &param, float &value) const
{
    if (param == "gain" || param == "amount")
    {
        value = wavefolder.GetGain();
        return true;
    }
    else if (param == "offset")
    {
        value = wavefolder.GetOffset();
        return true;
    }
    else if (param == "mix")
    {
        value = mix;
        return true;
    }
    return false;
}

void ModularWavefolder::reset()
{
    // Wavefolder doesn't need reset
}

// ModularTremolo Implementation
void ModularTremolo::init(float sr)
{
    sampleRate = sr;
    tremolo.Init(sampleRate);
    tremolo.SetFreq(5.0f);
    tremolo.SetDepth(0.5f);
    tremolo.SetWaveform(daisysp::Tremolo::WAVE_SIN);
    enabled = true;
}

float ModularTremolo::process(float input)
{
    if (!enabled)
        return input;
    return tremolo.Process(input);
}

void ModularTremolo::setParameter(const std::string &param, float value)
{
    if (param == "frequency" || param == "rate")
    {
        tremolo.SetFreq(std::max(0.01f, std::min(4.0f, value)));
    }
    else if (param == "depth" || param == "amount")
    {
        tremolo.SetDepth(std::max(0.0f, std::min(1.0f, value)));
    }
    else if (param == "waveform")
    {
        int waveform = static_cast<int>(std::max(0.0f, std::min(3.0f, value)));
        tremolo.SetWaveform(waveform);
    }
}

bool ModularTremolo::getParameter(const std::string &param, float &value) const
{
    if (param == "frequency" || param == "rate")
    {
        value = tremolo.GetFreq();
        return true;
    }
    else if (param == "depth" || param == "amount")
    {
        value = tremolo.GetDepth();
        return true;
    }
    return false;
}

void ModularTremolo::reset()
{
    tremolo.Reset();
}

// ModularAdsr Implementation
void ModularAdsr::init(float sr)
{
    sampleRate = sr;
    adsr.Init(sampleRate);
    adsr.SetTime(daisysp::ADSR_SEG_ATTACK, 0.01f);
    adsr.SetTime(daisysp::ADSR_SEG_DECAY, 0.1f);
    adsr.SetTime(daisysp::ADSR_SEG_RELEASE, 0.2f);
    adsr.SetSustainLevel(0.7f);
    enabled = true;
    currentGate = false;
}

float ModularAdsr::process(float input)
{
    if (!enabled)
        return input;

    float envelope = adsr.Process(currentGate);
    return input * envelope;
}

void ModularAdsr::setParameter(const std::string &param, float value)
{
    if (param == "attack")
    {
        adsr.SetTime(daisysp::ADSR_SEG_ATTACK, std::max(0.001f, std::min(10.0f, value)));
    }
    else if (param == "decay")
    {
        adsr.SetTime(daisysp::ADSR_SEG_DECAY, std::max(0.001f, std::min(10.0f, value)));
    }
    else if (param == "sustain")
    {
        adsr.SetSustainLevel(std::max(0.0f, std::min(1.0f, value)));
    }
    else if (param == "release")
    {
        adsr.SetTime(daisysp::ADSR_SEG_RELEASE, std::max(0.001f, std::min(10.0f, value)));
    }
}

bool ModularAdsr::getParameter(const std::string &param, float &value) const
{
    if (param == "attack")
    {
        value = adsr.GetTime(daisysp::ADSR_SEG_ATTACK);
        return true;
    }
    else if (param == "decay")
    {
        value = adsr.GetTime(daisysp::ADSR_SEG_DECAY);
        return true;
    }
    else if (param == "sustain")
    {
        value = adsr.GetSustainLevel();
        return true;
    }
    else if (param == "release")
    {
        value = adsr.GetTime(daisysp::ADSR_SEG_RELEASE);
        return true;
    }
    return false;
}

void ModularAdsr::reset()
{
    adsr.Reset();
    currentGate = false;
}

void ModularAdsr::trigger(bool gate)
{
    currentGate = gate;
}

void ModularAdsr::retrigger()
{
    adsr.Retrigger(true);
}

bool ModularAdsr::isActive() const
{
    return adsr.IsRunning();
}