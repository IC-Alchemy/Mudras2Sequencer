// Required includes for DSP functionality and math operations
#include "dsp.h"
#include "particle.h"
#include <math.h>

using namespace daisysp;

// Initialize particle parameters with given sample rate
void Particle::Init(float sample_rate)
{
    sample_rate_ = sample_rate;

    // Initialize default state
    sync_ = false;
    aux_  = 0.f;
    SetFreq(440.f);      // Set default frequency to A4 (440Hz)
    resonance_ = .9f;    // High default resonance
    density_   = .5f;    // Medium density of particles
    gain_      = 1.f;    // Unity gain
    spread_    = 1.f;    // Full frequency spread

    SetRandomFreq(sample_rate_ / 48.f); //48 is the default block size
    rand_phase_ = 0.f;   // Start with zero phase

    // Initialize filter parameters
    pre_gain_ = 0.0f;
    filter_.Init(sample_rate_);
    filter_.SetDrive(.7f);
}

// Process one sample of audio
float Particle::Process()
{
    // Generate random value and initialize output sample
    float u = rand() * kRandFrac;
    float s = 0.0f;

    // Process if random value is within density threshold or sync is active
    if(u <= density_ || sync_)
    {
        // Apply gain to random value if within density threshold
        s = u <= density_ ? u * gain_ : s;
        rand_phase_ += rand_freq_;

        // Check if it's time to update filter parameters
        if(rand_phase_ >= 1.f || sync_)
        {
            // Wrap phase back to [0,1] range
            rand_phase_ = rand_phase_ >= 1.f ? rand_phase_ - 1.f : rand_phase_;

            // Generate new random frequency
            const float u = 2.0f * rand() * kRandFrac - 1.0f;
            const float f
                = fmin(powf(2.f, kRatioFrac * spread_ * u) * frequency_, .25f);
            
            // Calculate pre-gain based on current parameters
            pre_gain_ = 0.5f / sqrtf(resonance_ * f * sqrtf(density_));
            
            // Update filter parameters
            filter_.SetFreq(f * sample_rate_);
            filter_.SetRes(resonance_);
        }
    }
    aux_ = s;

    // Process through filter and return bandpass output
    filter_.Process(pre_gain_ * s);
    return filter_.Band();
}

// Get the most recent noise sample before filtering
float Particle::GetNoise()
{
    return aux_;
}

// Set base frequency (normalized to sample rate)
void Particle::SetFreq(float freq)
{
    freq /= sample_rate_;
    frequency_ = fclamp(freq, 0.f, 1.f);
}

// Set filter resonance (0-1)
void Particle::SetResonance(float resonance)
{
    resonance_ = fclamp(resonance, 0.f, 1.f);
}

// Set frequency of random parameter updates
void Particle::SetRandomFreq(float freq)
{
    freq /= sample_rate_;
    rand_freq_ = fclamp(freq, 0.f, 1.f);
}

// Set particle density (scaled by 0.3 and clamped 0-1)
void Particle::SetDensity(float density)
{
    density_ = fclamp(density * .3f, 0.f, 1.f);
}

// Set output gain (0-1)
void Particle::SetGain(float gain)
{
    gain_ = fclamp(gain, 0.f, 1.f);
}

// Set frequency spread (minimum 0)
void Particle::SetSpread(float spread)
{
    spread_ = spread < 0.f ? 0.f : spread;
}

// Enable/disable sync mode
void Particle::SetSync(bool sync)
{
    sync_ = sync;
}