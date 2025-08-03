#pragma once

#include "ModularVoice.h"
#include "../dsp/oscillator.h"
#include "../dsp/ladder.h"
#include "../dsp/svf.h"
#include "../dsp/adsr.h"
#include "../dsp/particle.h"
#include "../dsp/overdrive.h"
#include "../dsp/wavefolder.h"
#include "../dsp/tremolo.h"
#include "../dsp/vosim.h"
#include "../dsp/whitenoise.h"

/**
 * @brief Oscillator wrapper for modular voice system
 */
class ModularOscillator : public AudioSource {
private:
    daisysp::Oscillator osc;
    bool enabled = true;
    float frequency = 440.0f;
    float amplitude = 1.0f;
    
public:
    void init(float sampleRate) override {
        osc.Init(sampleRate);
        osc.SetWaveform(daisysp::Oscillator::WAVE_POLYBLEP_SAW);
        osc.SetFreq(frequency);
        osc.SetAmp(amplitude);
    }
    
    float process() override {
        if (!enabled) return 0.0f;
        return osc.Process();
    }
    
    void setParameter(const std::string& name, float value) override {
        if (name == "frequency") {
            frequency = value;
            osc.SetFreq(frequency);
        } else if (name == "amplitude") {
            amplitude = value;
            osc.SetAmp(amplitude);
        } else if (name == "waveform") {
            osc.SetWaveform(static_cast<uint8_t>(value));
        } else if (name == "pulsewidth") {
            osc.SetPw(value);
        }
    }
    
    bool getParameter(const std::string& name, float& value) const override {
        if (name == "frequency") {
            value = frequency;
            return true;
        } else if (name == "amplitude") {
            value = amplitude;
            return true;
        }
        return false;
    }
    
    void reset() override {
        osc.Reset();
    }
    
    bool isEnabled() const override { return enabled; }
    void setEnabled(bool en) override { enabled = en; }
    std::string getType() const override { return "oscillator"; }
    
    void setFrequency(float freq) override {
        frequency = freq;
        osc.SetFreq(frequency);
    }
    
    void setAmplitude(float amp) override {
        amplitude = amp;
        osc.SetAmp(amplitude);
    }
};

/**
 * @brief Particle synthesis wrapper for modular voice system
 */
class ModularParticle : public AudioSource {
private:
    daisysp::Particle particle;
    bool enabled = true;
    float frequency = 440.0f;
    float amplitude = 1.0f;
    
public:
    void init(float sampleRate) override {
        particle.Init(sampleRate);
        particle.SetFreq(frequency);
        particle.SetGain(amplitude);
    }
    
    float process() override {
        if (!enabled) return 0.0f;
        return particle.Process();
    }
    
    void setParameter(const std::string& name, float value) override {
        if (name == "frequency") {
            frequency = value;
            particle.SetFreq(frequency);
        } else if (name == "amplitude" || name == "gain") {
            amplitude = value;
            particle.SetGain(amplitude);
        } else if (name == "resonance") {
            particle.SetResonance(value);
        } else if (name == "density") {
            particle.SetDensity(value);
        } else if (name == "spread") {
            particle.SetSpread(value);
        } else if (name == "randomfreq") {
            particle.SetRandomFreq(value);
        }
    }
    
    bool getParameter(const std::string& name, float& value) const override {
        if (name == "frequency") {
            value = frequency;
            return true;
        } else if (name == "amplitude" || name == "gain") {
            value = amplitude;
            return true;
        }
        return false;
    }
    
    void reset() override {
        // Particle doesn't have a reset method, reinitialize
        particle.Init(48000.0f); // Use default sample rate
    }
    
    bool isEnabled() const override { return enabled; }
    void setEnabled(bool en) override { enabled = en; }
    std::string getType() const override { return "particle"; }
    
    void setFrequency(float freq) override {
        frequency = freq;
        particle.SetFreq(frequency);
    }
    
    void setAmplitude(float amp) override {
        amplitude = amp;
        particle.SetGain(amplitude);
    }
};

/**
 * @brief White noise wrapper for modular voice system
 */
class ModularNoise : public AudioSource {
private:
    daisysp::WhiteNoise noise;
    bool enabled = true;
    float amplitude = 1.0f;
    
public:
    void init(float sampleRate) override {
        noise.Init();
        noise.SetAmp(amplitude);
    }
    
    float process() override {
        if (!enabled) return 0.0f;
        return noise.Process() * amplitude;
    }
    
    void setParameter(const std::string& name, float value) override {
        if (name == "amplitude" || name == "gain") {
            amplitude = value;
            noise.SetAmp(amplitude);
        }
    }
    
    bool getParameter(const std::string& name, float& value) const override {
        if (name == "amplitude" || name == "gain") {
            value = amplitude;
            return true;
        }
        return false;
    }
    
    void reset() override {
        // White noise doesn't need reset
    }
    
    bool isEnabled() const override { return enabled; }
    void setEnabled(bool en) override { enabled = en; }
    std::string getType() const override { return "noise"; }
    
    void setFrequency(float freq) override {
        // Noise doesn't have frequency
        (void)freq;
    }
    
    void setAmplitude(float amp) override {
        amplitude = amp;
        noise.SetAmp(amplitude);
    }
};

/**
 * @brief VOSIM wrapper for modular voice system
 */
class ModularVosim : public AudioSource {
private:
    daisysp::Vosim vosim;
    bool enabled = true;
    float frequency = 440.0f;
    float amplitude = 1.0f;
    
public:
    void init(float sampleRate) override {
        vosim.Init(sampleRate);
        vosim.SetFreq(frequency);
    }
    
    float process() override {
        if (!enabled) return 0.0f;
        return vosim.Process() * amplitude;
    }
    
    void setParameter(const std::string& name, float value) override {
        if (name == "frequency") {
            frequency = value;
            vosim.SetFreq(frequency);
        } else if (name == "amplitude" || name == "gain") {
            amplitude = value;
        } else if (name == "form1") {
            vosim.SetForm1Freq(value);
        } else if (name == "form2") {
            vosim.SetForm2Freq(value);
        } else if (name == "shape") {
            vosim.SetShape(value);
        }
    }
    
    bool getParameter(const std::string& name, float& value) const override {
        if (name == "frequency") {
            value = frequency;
            return true;
        } else if (name == "amplitude" || name == "gain") {
            value = amplitude;
            return true;
        }
        return false;
    }
    
    void reset() override {
        // VOSIM doesn't have a reset method
    }
    
    bool isEnabled() const override { return enabled; }
    void setEnabled(bool en) override { enabled = en; }
    std::string getType() const override { return "vosim"; }
    
    void setFrequency(float freq) override {
        frequency = freq;
        vosim.SetFreq(frequency);
    }
    
    void setAmplitude(float amp) override {
        amplitude = amp;
    }
};

/**
 * @brief Ladder filter wrapper for modular voice system
 */
class ModularLadderFilter : public AudioFilter {
private:
    daisysp::LadderFilter filter;
    bool enabled = true;
    float frequency = 1000.0f;
    float resonance = 0.4f;
    
public:
    void init(float sampleRate) override {
        filter.Init(sampleRate);
        filter.SetFreq(frequency);
        filter.SetRes(resonance);
    }
    
    float process(float input) override {
        if (!enabled) return input;
        return filter.Process(input);
    }
    
    void setParameter(const std::string& name, float value) override {
        if (name == "frequency" || name == "cutoff") {
            frequency = value;
            filter.SetFreq(frequency);
        } else if (name == "resonance") {
            resonance = value;
            filter.SetRes(resonance);
        } else if (name == "drive") {
            filter.SetInputDrive(value);
        } else if (name == "passband_gain") {
            filter.SetPassbandGain(value);
        }
    }
    
    bool getParameter(const std::string& name, float& value) const override {
        if (name == "frequency" || name == "cutoff") {
            value = frequency;
            return true;
        } else if (name == "resonance") {
            value = resonance;
            return true;
        }
        return false;
    }
    
    void reset() override {
        filter.Reset();
    }
    
    bool isEnabled() const override { return enabled; }
    void setEnabled(bool en) override { enabled = en; }
    std::string getType() const override { return "ladder_filter"; }
    
    void setFrequency(float freq) override {
        frequency = freq;
        filter.SetFreq(frequency);
    }
    
    void setResonance(float res) override {
        resonance = res;
        filter.SetRes(resonance);
    }
};

/**
 * @brief SVF filter wrapper for modular voice system
 */
class ModularSvfFilter : public AudioFilter {
private:
    daisysp::Svf filter;
    bool enabled = true;
    float frequency = 1000.0f;
    float resonance = 0.4f;
    int filterMode = 0; // 0=lowpass, 1=highpass, 2=bandpass, 3=notch, 4=peak
    
public:
    void init(float sampleRate) override {
        filter.Init(sampleRate);
        filter.SetFreq(frequency);
        filter.SetRes(resonance);
    }
    
    float process(float input) override {
        if (!enabled) return input;
        filter.Process(input);
        
        switch (filterMode) {
            case 0: return filter.Low();
            case 1: return filter.High();
            case 2: return filter.Band();
            case 3: return filter.Notch();
            case 4: return filter.Peak();
            default: return filter.Low();
        }
    }
    
    void setParameter(const std::string& name, float value) override {
        if (name == "frequency" || name == "cutoff") {
            frequency = value;
            filter.SetFreq(frequency);
        } else if (name == "resonance") {
            resonance = value;
            filter.SetRes(resonance);
        } else if (name == "drive") {
            filter.SetDrive(value);
        } else if (name == "mode") {
            filterMode = static_cast<int>(value);
        }
    }
    
    bool getParameter(const std::string& name, float& value) const override {
        if (name == "frequency" || name == "cutoff") {
            value = frequency;
            return true;
        } else if (name == "resonance") {
            value = resonance;
            return true;
        } else if (name == "mode") {
            value = static_cast<float>(filterMode);
            return true;
        }
        return false;
    }
    
    void reset() override {
        // SVF doesn't have a reset method, reinitialize
        filter.Init(48000.0f);
    }
    
    bool isEnabled() const override { return enabled; }
    void setEnabled(bool en) override { enabled = en; }
    std::string getType() const override { return "svf_filter"; }
    
    void setFrequency(float freq) override {
        frequency = freq;
        filter.SetFreq(frequency);
    }
    
    void setResonance(float res) override {
        resonance = res;
        filter.SetRes(resonance);
    }
};

/**
 * @brief Overdrive effect wrapper for modular voice system
 */
class ModularOverdrive : public AudioEffect {
private:
    daisysp::Overdrive overdrive;
    bool enabled = true;
    float mix = 1.0f;
    
public:
    void init(float sampleRate) override {
        (void)sampleRate; // Overdrive doesn't need sample rate
        overdrive.Init();
    }
    
    float process(float input) override {
        if (!enabled) return input;
        float processed = overdrive.Process(input);
        return input * (1.0f - mix) + processed * mix;
    }
    
    void setParameter(const std::string& name, float value) override {
        if (name == "drive") {
            overdrive.SetDrive(value);
        } else if (name == "mix") {
            mix = value;
        }
    }
    
    bool getParameter(const std::string& name, float& value) const override {
        if (name == "mix") {
            value = mix;
            return true;
        }
        return false;
    }
    
    void reset() override {
        // Overdrive doesn't need reset
    }
    
    bool isEnabled() const override { return enabled; }
    void setEnabled(bool en) override { enabled = en; }
    std::string getType() const override { return "overdrive"; }
    
    void setMix(float mixLevel) override {
        mix = mixLevel;
    }
};

/**
 * @brief Wavefolder effect wrapper for modular voice system
 */
class ModularWavefolder : public AudioEffect {
private:
    daisysp::Wavefolder wavefolder;
    bool enabled = true;
    float mix = 1.0f;
    
public:
    void init(float sampleRate) override {
        (void)sampleRate; // Wavefolder doesn't need sample rate
        wavefolder.Init();
    }
    
    float process(float input) override {
        if (!enabled) return input;
        float processed = wavefolder.Process(input);
        return input * (1.0f - mix) + processed * mix;
    }
    
    void setParameter(const std::string& name, float value) override {
        if (name == "gain") {
            wavefolder.SetGain(value);
        } else if (name == "offset") {
            wavefolder.SetOffset(value);
        } else if (name == "mix") {
            mix = value;
        }
    }
    
    bool getParameter(const std::string& name, float& value) const override {
        if (name == "mix") {
            value = mix;
            return true;
        }
        return false;
    }
    
    void reset() override {
        // Wavefolder doesn't need reset
    }
    
    bool isEnabled() const override { return enabled; }
    void setEnabled(bool en) override { enabled = en; }
    std::string getType() const override { return "wavefolder"; }
    
    void setMix(float mixLevel) override {
        mix = mixLevel;
    }
};

/**
 * @brief Tremolo effect wrapper for modular voice system
 */
class ModularTremolo : public AudioEffect {
private:
    daisysp::Tremolo tremolo;
    bool enabled = true;
    float mix = 1.0f;
    
public:
    void init(float sampleRate) override {
        tremolo.Init(sampleRate);
    }
    
    float process(float input) override {
        if (!enabled) return input;
        float processed = tremolo.Process(input);
        return input * (1.0f - mix) + processed * mix;
    }
    
    void setParameter(const std::string& name, float value) override {
        if (name == "freq" || name == "rate") {
            tremolo.SetFreq(value);
        } else if (name == "depth") {
            tremolo.SetDepth(value);
        } else if (name == "waveform") {
            tremolo.SetWaveform(static_cast<int>(value));
        } else if (name == "mix") {
            mix = value;
        }
    }
    
    bool getParameter(const std::string& name, float& value) const override {
        if (name == "mix") {
            value = mix;
            return true;
        }
        return false;
    }
    
    void reset() override {
        // Tremolo doesn't need reset
    }
    
    bool isEnabled() const override { return enabled; }
    void setEnabled(bool en) override { enabled = en; }
    std::string getType() const override { return "tremolo"; }
    
    void setMix(float mixLevel) override {
        mix = mixLevel;
    }
};

/**
 * @brief ADSR envelope wrapper for modular voice system
 */
class ModularAdsr : public AudioEnvelope {
private:
    daisysp::Adsr adsr;
    bool enabled = true;
    bool gateState = false;
    
public:
    void init(float sampleRate) override {
        adsr.Init(sampleRate);
    }
    
    float process(float input) override {
        if (!enabled) return input;
        float envelope = adsr.Process(gateState);
        return input * envelope;
    }
    
    void setParameter(const std::string& name, float value) override {
        if (name == "attack") {
            adsr.SetAttackTime(value);
        } else if (name == "decay") {
            adsr.SetDecayTime(value);
        } else if (name == "sustain") {
            adsr.SetSustainLevel(value);
        } else if (name == "release") {
            adsr.SetReleaseTime(value);
        }
    }
    
    bool getParameter(const std::string& name, float& value) const override {
        // ADSR doesn't provide getters, return false
        (void)name;
        (void)value;
        return false;
    }
    
    void reset() override {
        adsr.Reset();
    }
    
    bool isEnabled() const override { return enabled; }
    void setEnabled(bool en) override { enabled = en; }
    std::string getType() const override { return "adsr"; }
    
    void trigger(bool gate) override {
        gateState = gate;
    }
    
    bool isActive() const override {
        return gateState; // Simplified - ADSR doesn't provide isActive
    }
    
    void retrigger() override {
        adsr.Retrigger(false);
    }
};