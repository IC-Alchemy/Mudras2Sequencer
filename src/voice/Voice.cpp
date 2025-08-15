#include "Voice.h"
#include "../dsp/dsp.h"
#include "Arduino.h"
#include <algorithm>
#include <cmath>

// Constants
static constexpr float FREQ_SLEW_RATE = 0.00035f; // Slide speed
static constexpr float BASE_FREQ =
    110.0f; // Base frequency for note calculations

// Static member initialization
float Voice::frequencyLookupTable[128];
bool Voice::lookupTableInitialized = false;

// Initialize frequency lookup table covering MIDI 0..127
void Voice::initFrequencyLookupTable()
{
  // Use daisysp::mtof once per MIDI note value
  for (int midi = 0; midi < 90; ++midi)
  {
    frequencyLookupTable[midi] = daisysp::mtof(static_cast<float>(midi));
  }
}

Voice::Voice(uint8_t id, const VoiceConfig &cfg)
    : voiceId(id), config(cfg), sampleRate(48000.0f), filterFrequency(1000.0f),
      gate(false)
{
  // Initialize frequency lookup table once (thread-safe since all voices share it)
  if (!lookupTableInitialized)
  {
    initFrequencyLookupTable();
    lookupTableInitialized = true;
  }

  // Initialize oscillators vector
  oscillators.resize(config.oscillatorCount);

  // Initialize frequency slewing
  for (int i = 0; i < 3; i++)
  {
    freqSlew[i].currentFreq = 440.0f;
    freqSlew[i].targetFreq = 440.0f;
  }

  // Initialize voice state with defaults
  state.note = 0.0f;
  state.velocity = 0.8f;
  state.filter = 0.37f;
  state.attack = 0.01f;
  state.decay = 0.1f;
  state.octave = 0;
  state.gate = false;
  state.slide = false;
  state.retrigger = false;
  state.gateLength = 27; // Default gate length
}

void Voice::init(float sr)
{
  sampleRate = sr;

  // Initialize oscillators
  for (size_t i = 0; i < oscillators.size() && i < config.oscillatorCount;
       i++)
  {
    oscillators[i].Init(sampleRate);
    oscillators[i].SetWaveform(config.oscWaveforms[i]);
    oscillators[i].SetAmp(config.oscAmplitudes[i]);

    // Set pulse width for square/pulse waves
    if (config.oscWaveforms[i] == daisysp::Oscillator::WAVE_POLYBLEP_SQUARE)
    {
      oscillators[i].SetPw(config.oscPulseWidth[i]);
    }
  }

  // Initialize noise generator
  noise_.Init();
  noise_.SetSeed(1);
  noise_.SetAmp(1.0f);

  // Initialize particle engine
  particle_.Init(sampleRate);
  particle_.SetFreq(220.f);
  particle_.SetResonance(config.particleResonance);
  particle_.SetDensity(config.particleDensity);
  particle_.SetGain(config.particleGain);
  particle_.SetSpread(config.particleSpread);
  particle_.SetSync(config.particleSync);

  // Initialize filter
  filter.Init(sampleRate);
  filter.SetFreq(filterFrequency);
  filter.SetRes(config.filterRes);
  filter.SetInputDrive(config.filterDrive);
  filter.SetPassbandGain(config.filterPassbandGain);
  filter.SetFilterMode(config.filterMode);
  // Initialize high-pass filter
  highPassFilter.Init(sampleRate);
  highPassFilter.SetFreq(config.highPassFreq);
  highPassFilter.SetRes(config.highPassRes);

  // Initialize envelope
  envelope.Init(sampleRate);
  envelope.SetAttackTime(config.defaultAttack);
  envelope.SetDecayTime(config.defaultDecay);
  envelope.SetSustainLevel(config.defaultSustain);
  envelope.SetReleaseTime(config.defaultRelease);

  // Initialize effects
  if (config.hasOverdrive)
  {
    overdrive.Init();
    overdrive.SetDrive(config.overdriveDrive);
  }

  if (config.hasWavefolder)
  {

    wavefolder.Init();
    wavefolder.SetGain(config.wavefolderGain);
    wavefolder.SetOffset(config.wavefolderOffset);
  }
}

void Voice::setConfig(const VoiceConfig &cfg)
{
  config = cfg;

  // Resize oscillators if needed
  if (oscillators.size() != config.oscillatorCount)
  {
    oscillators.resize(config.oscillatorCount);
    // Initialize new oscillators
    for (size_t i = 0; i < oscillators.size(); i++)
    {
      oscillators[i].Init(sampleRate);
    }
  }

  // Update all components with new configuration
  init(sampleRate);
}

// Injected scale-data setters (defined out-of-line)
void Voice::setScaleTable(const int (*table)[48], size_t scaleCount)
{
  scaleTable = table;
  scaleTableCount = scaleCount;
}

void Voice::setCurrentScalePointer(const uint8_t* ptr)
{
  currentScalePtr = ptr;
}

float Voice::process()
{
  if (!config.enabled)
  {
    return 0.0f;
  }


  // Handle envelope retrigger
  if (state.retrigger)
  {
    envelope.Retrigger(false);
    state.retrigger = false;
  }

  // Process envelope
 float envelopeValue = config.hasEnvelope ? envelope.Process(gate) : 1.0f;
  //float envelopeValue =  envelope.Process(gate);

  // Update filter frequency with envelope modulation
  filter.SetFreq(100.f + (filterFrequency * envelopeValue) +
                 (filterFrequency * .1f));

  // Process frequency slewing for slide functionality
  if (state.slide)
  {
    for (size_t i = 0; i < oscillators.size() && i < 3; i++)
    {
      processFrequencySlew(i, freqSlew[i].targetFreq);
      oscillators[i].SetFreq(freqSlew[i].currentFreq);
    }
  }

  // Mix voice signal (supports oscillator mix, ring-mod, noise, or particle engine)
  float mixedOscillators = 0.0f;

  if (config.useParticleEngine)
  {
  float dynamicDensity = config.particleDensity * state.velocity * envelopeValue;
particle_.SetDensity(dynamicDensity);
    mixedOscillators = particle_.Process();

  }
  else if (config.oscillatorCount == 0)
  {
    // Special case for percussion voices (no oscillators, only noise)
    mixedOscillators = noise_.Process();
  }

  else if (config.hasDalek)
  {
    // Ring Modulation across oscillators
    mixedOscillators = 1.f;
    for (size_t i = 0; i < oscillators.size(); i++)
    {
      mixedOscillators *= oscillators[i].Process();
    }
    mixedOscillators *= 2.f;
  }
  else
  {
    for (size_t i = 0; i < oscillators.size(); i++)
    {
      mixedOscillators += oscillators[i].Process();
    }
  }

  // Apply effects chain
  processEffectsChain(mixedOscillators);
  mixedOscillators *= (.25f + (state.velocity));
  // Apply filter
  float filteredSignal = filter.Process(mixedOscillators);

  // Apply high-pass filter
  highPassFilter.Process(filteredSignal);
  float highPassedSignal = highPassFilter.High();

  // Apply envelope to final output
  float finalOutput = highPassedSignal * (envelopeValue) * config.outputLevel;



  return finalOutput;
  }

  void Voice::updateParameters(const VoiceState &newState)
  {
    state = newState;

    // Update gate state to sync with sequencer
    gate = state.gate;

    // Apply envelope parameters
    applyEnvelopeParameters();

    // Calculate and set filter frequency
    filterFrequency =
        daisysp::fmap(state.filter, 150.0f, 9710.0f, daisysp::Mapping::EXP);

    // Update oscillator frequencies
    updateOscillatorFrequencies();
  }

  void Voice::setSequencer(std::unique_ptr<Sequencer> seq)
  {
    // Extract raw pointer from unique_ptr and take ownership externally
    sequencer = seq.release();
  }

  void Voice::setSequencer(Sequencer * seq)
  {
    // Store raw pointer directly (no ownership transfer)
    sequencer = seq;
  }

  void Voice::processEffectsChain(float &signal)
  {

    if (config.hasOverdrive)
    {
      signal = overdrive.Process(signal) * config.overdriveGain;
    }

    if (config.hasWavefolder)
    {
      signal = wavefolder.Process(signal);
      signal *= config.wavefolderGain;
    }
  }

  void Voice::updateOscillatorFrequencies()
  {
    // GATE-CONTROLLED FREQUENCY UPDATES: Only update frequencies when gate is HIGH
    if (!state.gate)
    {
      return; // Skip frequency updates when gate is LOW
    }

    // Particle engine path uses a single center frequency following the base note
    if (config.useParticleEngine)
    {
      float baseFreq = calculateNoteFrequency(state.note, state.octave, config.harmony[0]);
      particle_.SetFreq(baseFreq);
      // Keep particle params in sync with config (in case edited live)
      particle_.SetResonance(config.particleResonance);
      particle_.SetDensity(config.particleDensity);
      particle_.SetGain(config.particleGain);
      particle_.SetSpread(config.particleSpread);
      particle_.SetSync(config.particleSync);
      return;
    }

    // Calculate base frequency once and cache it (used when harmony offset is 0)
    const float baseFreq = calculateNoteFrequency(state.note, state.octave, 0);

    // Limit oscillator loop to max 3
    const size_t oscCount = std::min(static_cast<size_t>(3), oscillators.size());

    for (size_t i = 0; i < oscCount; i++)
    {
      // Calculate frequency for this oscillator using harmony interval
      float harmonyFreq;
      const int harmonyInterval = config.harmony[i];

      if (harmonyInterval == 0)
      {
        harmonyFreq = baseFreq;
      }
      else
      {
        harmonyFreq = calculateNoteFrequency(state.note, state.octave, harmonyInterval);
      }

      // Apply TripleSaw-style percentage detuning relative to harmony frequency
      // fmaf(a, b, c) computes a*b + c using a single FPU instruction when available
      const float targetFreq = std::fmaf(0.05f * config.oscDetuning[i], harmonyFreq, harmonyFreq);

      if (state.slide)
      {
        // Set target for slewing
        freqSlew[i].targetFreq = targetFreq;
      }
      else
      {
        // Set frequency directly
        oscillators[i].SetFreq(targetFreq);
        freqSlew[i].currentFreq = targetFreq;
        freqSlew[i].targetFreq = targetFreq;
      }
    }
  }

  void Voice::applyEnvelopeParameters()
  {
    // Map normalized parameters to appropriate ranges
    float attack =
        daisysp::fmap(state.attack, 0.005f, 0.75f, daisysp::Mapping::LINEAR);
    float decay =
        daisysp::fmap(state.decay, 0.002f, 0.6f, daisysp::Mapping::LINEAR);
    float release = decay; // Use decay for release in this implementation

    envelope.SetAttackTime(attack);
    envelope.SetDecayTime(0.01f + (release * 0.22f));
    envelope.SetReleaseTime(release);
  }

  float Voice::calculateNoteFrequency(float note, int8_t octaveOffset,
                                      int harmony)
  {
    // Clamp note to valid range with ARM-friendly integer operations
    const int noteIndex = std::max(0, std::min(static_cast<int>(note), 47));

    // Apply harmony interval, ensuring we stay within scale bounds
    const int harmonyNoteIndex = std::max(0, std::min(noteIndex + harmony, 47));

    // Resolve scale step to semitone offset using injected scale table if available
    int scaleSemitone = harmonyNoteIndex; // chromatic fallback: 0..47 maps to 0..47
    if (scaleTable && scaleTableCount > 0 && currentScalePtr)
    {
      const uint8_t idx = *currentScalePtr;
      if (idx < scaleTableCount)
      {
        scaleSemitone = scaleTable[idx][harmonyNoteIndex];
      }
    }

    // Base MIDI mapping: center around 48 as before (C3-ish) then add octave offset in semitones
    int midiNote = scaleSemitone + 48 + static_cast<int>(octaveOffset);
    // Clamp to table range 0..127
    midiNote = std::max(0, std::min(midiNote, 127));

    // Fast lookup
    return frequencyLookupTable[midiNote];
  }

  void Voice::processFrequencySlew(uint8_t oscIndex, float targetFreq)
  {
    if (oscIndex >= 3)
      return;

    // Exponential slewing for smooth frequency transitions
    const float delta = freqSlew[oscIndex].targetFreq - freqSlew[oscIndex].currentFreq;
    freqSlew[oscIndex].currentFreq = std::fmaf(delta, FREQ_SLEW_RATE, freqSlew[oscIndex].currentFreq);
  }

  void Voice::setFrequency(float frequency)
  {
    // GATE-CONTROLLED FREQUENCY UPDATES: Only update oscillator frequencies when gate is HIGH
    // This maintains consistency with updateOscillatorFrequencies() and the gate-controlled architecture
    if (!state.gate)
    {
      return; // Skip frequency updates when gate is LOW
    }

    // Set the base frequency for all oscillators with TripleSaw-style percentage detuning
    for (uint8_t i = 0; i < config.oscillatorCount && i < oscillators.size() && i < 3; i++)
    {
      float targetFreq;

      // TripleSaw-style detuning: each oscillator gets detuned by up to 5% of base frequency
      // config.oscDetuning[i] is treated as a multiplier for the 5% detuning amount
      // Positive values detune up, negative values detune down
      targetFreq = frequency + (0.05f * frequency * config.oscDetuning[i]);

      if (state.slide)
      {
        // Set target for slewing (slide functionality)
        freqSlew[i].targetFreq = targetFreq;
      }
      else
      {
        // Set frequency directly
        oscillators[i].SetFreq(targetFreq);
        freqSlew[i].currentFreq = targetFreq;
        freqSlew[i].targetFreq = targetFreq;
      }
    }
  }

  void Voice::setSlideTime(float slideTime)
  {
    // Note: VoiceSlewParams doesn't have slideTime member
    // This could be implemented by storing slideTime as a class member
    // For now, this is a placeholder implementation
    (void)slideTime; // Suppress unused parameter warning
  }

  // Voice Presets Implementation
  namespace VoicePresets
  {
    VoiceConfig getAnalogVoice()
    {
      VoiceConfig config;
      config.oscillatorCount = 3;
      config.oscWaveforms[0] = daisysp::Oscillator::WAVE_POLYBLEP_SAW;
      config.oscWaveforms[1] = daisysp::Oscillator::WAVE_POLYBLEP_SAW;
      config.oscWaveforms[2] = daisysp::Oscillator::WAVE_POLYBLEP_SAW;
      config.oscAmplitudes[0] = .33f;
      config.oscAmplitudes[1] = .33f;
      config.oscAmplitudes[2] = .33f;
      config.oscDetuning[0] = 0.0f;
      config.oscDetuning[1] = 0.047;   // Slight detune
      config.oscDetuning[2] = -0.044f; // Slight detune opposite`
      config.harmony[0] = 0;          // Root note
      config.harmony[1] = 0;          // Unison (no harmony)
      config.harmony[2] = 0;          // Unison (no harmony)

      config.filterRes = 0.43f;
      config.filterDrive = 2.1f;
      config.filterMode = daisysp::LadderFilter::FilterMode::LP24;
      config.filterPassbandGain = 0.23f;
      config.highPassFreq = 140.0f;

      config.hasOverdrive = false;
      config.hasWavefolder = false;
      config.overdriveGain = 0.3f;
      config.overdriveDrive = 0.25f;
      config.wavefolderGain = 2.5f;
      config.wavefolderOffset = 1.0f;

      config.defaultAttack = 0.04f;
      config.defaultDecay = 0.14f;
      config.defaultSustain = 0.5f;
      config.defaultRelease = 0.1f;
      return config;
    }

    VoiceConfig getDigitalVoice()
    {
      VoiceConfig config;
      config.oscillatorCount = 1;
      config.oscWaveforms[0] = daisysp::Oscillator::WAVE_POLYBLEP_SAW;

      config.oscAmplitudes[0] = 1.f;
      config.oscAmplitudes[1] = 0.35f;
      config.oscAmplitudes[2] = .36f;
      config.oscPulseWidth[0] = 0.69f;
      config.oscDetuning[0] = 0.0f; // Fixed duplicate assignment
      config.oscDetuning[1] = 0.0f; // Fixed duplicate assignment
      config.oscDetuning[2] = 0.0f;
      config.harmony[0] = 0;  // Root note
      config.harmony[1] = 11; // PERFECT 5TH
      config.harmony[2] = 0; // Octave
      config.filterRes = 0.42f;
      config.filterDrive = 3.0f;
      config.filterPassbandGain = 0.24f;
      config.highPassFreq = 170.0f;
      config.highPassRes = 0.5f;
      config.filterMode =
          daisysp::LadderFilter::FilterMode::LP24; // Low-pass filter

      config.hasOverdrive = false;
      config.hasWavefolder = false;
      config.overdriveGain = 0.3f;
      config.overdriveDrive = 0.21f;
      config.wavefolderGain = 1.0f;
      config.defaultAttack = 0.015f;
      config.defaultDecay = 0.1f;
      config.defaultSustain = 0.5f;
      config.defaultRelease = 0.1f;

      return config;
    }

    VoiceConfig getBassVoice()
    {
      VoiceConfig config;
      config.oscillatorCount = 2;
      config.oscWaveforms[0] = daisysp::Oscillator::WAVE_POLYBLEP_SAW;
      config.oscWaveforms[1] = daisysp::Oscillator::WAVE_POLYBLEP_TRI;
      config.oscAmplitudes[0] = .25f;
      config.oscAmplitudes[1] = 1.f;
      config.oscDetuning[0] = 0.0f;
      config.oscDetuning[1] = 0.0f;
      config.oscDetuning[2] = 0.0f;
      config.harmony[0] = 7; // Root note
      config.harmony[1] = 0; // Unison (bass typically monophonic)
      config.harmony[2] = 0; // Unison
      config.highPassRes = 0.45f;
      config.filterRes = 0.33f;
      config.filterDrive = 2.9f;
      config.filterPassbandGain = 0.22f;
      config.highPassFreq = 75.0f; // Lower for bass
      config.filterMode = daisysp::LadderFilter::FilterMode::LP24;
      config.hasWavefolder = false;
      config.hasOverdrive = false;
      config.overdriveGain = 0.15f;
      config.overdriveDrive = 0.15f; // Subtle overdrive

      config.defaultAttack = 0.01f;
      config.defaultDecay = 0.3f;
      config.defaultSustain = 0.5f;
      config.defaultRelease = 0.2f;

      return config;
    }

    VoiceConfig getLeadVoice()
    {
      VoiceConfig config;
      config.oscillatorCount = 3;
      config.oscWaveforms[0] = daisysp::Oscillator::WAVE_POLYBLEP_SAW;
      config.oscWaveforms[1] = daisysp::Oscillator::WAVE_POLYBLEP_SAW;
      config.oscWaveforms[2] = daisysp::Oscillator::WAVE_POLYBLEP_SAW;
      config.oscAmplitudes[0] = .35f;
      config.oscAmplitudes[1] = .34f;
      config.oscAmplitudes[2] = 0.34f;
      config.oscDetuning[0] = 0.0f;
      config.oscDetuning[1] = 0.02f;
      config.oscDetuning[2] = -0.225f;
      config.harmony[0] = 0; // Root note
      config.harmony[1] = 0; // Unison (lead typically monophonic)
      config.harmony[2] = 0; // Unison

      config.filterRes = 0.23f;
      config.filterDrive = 2.8f;
      config.filterPassbandGain = 0.33f;
      config.highPassFreq = 120.0f;
      config.filterMode = daisysp::LadderFilter::FilterMode::LP24;
      config.hasOverdrive = false;
      config.hasWavefolder = false;
      config.overdriveGain = 0.2f;
      config.overdriveDrive = 0.25f;
      config.wavefolderGain = 1.0f;

      config.defaultAttack = 0.02f;
      config.defaultDecay = 0.2f;
      config.defaultSustain = 0.2f;
      config.defaultRelease = 0.15f;

      return config;
    }

    VoiceConfig getPadVoice()
    {
      VoiceConfig config;
      config.oscillatorCount = 3;
      config.oscWaveforms[0] = daisysp::Oscillator::WAVE_POLYBLEP_SAW;
      config.oscWaveforms[1] = daisysp::Oscillator::WAVE_POLYBLEP_SAW;
      config.oscWaveforms[2] = daisysp::Oscillator::WAVE_POLYBLEP_SAW;
      config.oscAmplitudes[0] = 0.33f;
      config.oscAmplitudes[1] = 0.32f;
      config.oscAmplitudes[2] = 0.32f;
      config.harmony[0] = 0; // Root note
      config.harmony[1] = 4; // Perfect Fifth
      config.harmony[2] = 9; // Major Third One octave up

      config.filterRes = 0.3f;
      config.filterDrive = 2.8f;
      config.filterPassbandGain = 0.23f;
      config.highPassFreq = 160.0f;
      config.filterMode =
          daisysp::LadderFilter::FilterMode::LP12; // Band-pass for percussive sound

      config.hasOverdrive = false;
      config.hasWavefolder = false;

      config.defaultAttack = 0.5f; // Slow attack for pad
      config.defaultDecay = 0.8f;
      config.defaultSustain = 0.5f;
      config.defaultRelease = .5f; // Long release
      config.outputLevel = 1.f;    // Lower level for pad
      // config.outputLevel = 0.6f; // conLower level for pad

      return config;
    }

    VoiceConfig getPercussionVoice()
    {
      VoiceConfig config;
      config.oscillatorCount = 0; // No oscillators, only noise
      config.oscWaveforms[0] =
          VoiceConfig::WAVE_NOISE; // Use noise for percussive texture
      config.oscWaveforms[1] =
          VoiceConfig::WAVE_NOISE;    // Use noise for percussive texture
      config.oscAmplitudes[0] = 1.0f; // Full amplitude for noise
      config.oscAmplitudes[1] = 0.0f;
      config.oscAmplitudes[2] = 0.0f;
      config.oscDetuning[0] = 0.0f;
      config.oscDetuning[1] = 0.0f;
      config.oscDetuning[2] = 0.0f;
      config.harmony[0] = 0; // Root note
      config.harmony[1] = 0; // Percussion typically monophonic
      config.harmony[2] = 0; // Percussion typically monophonic
      config.filterMode =
          daisysp::LadderFilter::FilterMode::LP24; // Band-pass for percussive sound
      config.filterRes = 0.49f;                    // High resonance
      config.filterDrive = 3.0f;
      config.filterPassbandGain = 0.33f;
      config.highPassFreq = 222.0f; // High-pass for percussion

      config.hasOverdrive = true;
      config.hasWavefolder = true;
      config.overdriveGain = 0.25f;
      config.overdriveDrive = 0.3f;
      config.wavefolderGain = 3.0f;

      config.defaultAttack = 0.001f; // Very fast attack
      config.defaultDecay = 0.05f;   // Short decay
      config.defaultSustain = 0.0f;  // Low sustain
      config.defaultRelease = 0.1f;  // Short release

      return config;
    }

    VoiceConfig getParticleVoice()
    {
      VoiceConfig config;
      // Use particle engine instead of oscillators
      config.useParticleEngine = true;
      config.oscillatorCount = 0; // Ignored when useParticleEngine is true
      // Particle defaults (tweak as desired)
      config.particleResonance = 0.42f;
      config.particleDensity   = .9f;
      config.particleGain      = 0.8f;
      config.particleSpread    = 2.0f;
      config.particleSync      = false;

      // Filtering/envelope for particle timbre
      config.filterRes = 0.3f;
      config.filterDrive = 3.0f;
      config.filterPassbandGain = 0.23f;
      config.highPassFreq = 120.0f;
      config.filterMode = daisysp::LadderFilter::FilterMode::BP12;

      config.hasOverdrive = true;
      config.hasWavefolder = true;

      config.defaultAttack = 0.01f;
      config.defaultDecay = 0.18f;
      config.defaultSustain = 0.f;
      config.defaultRelease = 0.12f;
      config.outputLevel = 1.f;
      return config;
    }

    // Preset name mapping for settings menu
    static const char *VOICE_PRESET_NAMES[] = {"Analog", "Digital", "Bass",
                                               "Lead", "Pad", "Percussion", "Particle"};

    static const uint8_t VOICE_PRESET_COUNT = 7;

    const char *getPresetName(uint8_t presetIndex)
    {
      if (presetIndex < VOICE_PRESET_COUNT)
      {
        return VOICE_PRESET_NAMES[presetIndex];
      }
      return "Unknown";
    }

    VoiceConfig getPresetConfig(uint8_t presetIndex)
    {
      switch (presetIndex)
      {
      case 0:
        return getAnalogVoice();
      case 1:
        return getDigitalVoice();
      case 2:
        return getBassVoice();
      case 3:
        return getLeadVoice();
      case 4:
        return getPadVoice();
      case 5:
        return getPercussionVoice();
      case 6:
        return getParticleVoice();
      default:
        return getAnalogVoice(); // Default fallback
      }
    }

    uint8_t getPresetCount() { return VOICE_PRESET_COUNT; }
  } // namespace VoicePresets