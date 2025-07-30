#include "includes.h"
#include "diagnostic.h"

// =======================
//   GLOBAL VARIABLES
// =======================
UIState uiState; // Central state object for the UI
Sequencer seq1; // Channel 1 for first sequencer
Sequencer seq2; // Channel 2 for second sequencer
LEDMatrix ledMatrix;

// --- MIDI & Clock ---
Adafruit_USBD_MIDI raw_usb_midi;
midi::SerialMIDI<Adafruit_USBD_MIDI> serial_usb_midi(raw_usb_midi);
midi::MidiInterface<midi::SerialMIDI<Adafruit_USBD_MIDI>> usb_midi(serial_usb_midi);
Adafruit_MPR121 touchSensor = Adafruit_MPR121();

// --- Constants needed for template parameters ---
constexpr float SAMPLE_RATE = 48000.0f;                                       // Compile-time constant for template parameters
constexpr size_t MAX_DELAY_SAMPLES = static_cast<size_t>(SAMPLE_RATE * 1.8f); 
// --- Audio & Synth ---
daisysp::Oscillator osc1A, osc1B, osc1C, osc2A, osc2B, osc2C, lfo1, lfo2;
daisysp::Svf highPass1, highPass2, delLowPass;
daisysp::Adsr env1, env2;
daisysp::DelayLine<float, MAX_DELAY_SAMPLES> del1;
float feedbackGain1 = 0.65f;
 float currentDelayOutputGain = 0.0f; // For smooth delay output fade

 float currentFeedbackGain = 0.0f; // For smooth delay feedback fade

daisysp::LadderFilter filt1, filt2;

 // Frequency slewing variables for slide functionality

 struct SlewParams {
     float currentFreq = 440.0f;
     float targetFreq = 440.0f;
 };

 // Two voices (1, 2) with three slots (A, B, C) each
 SlewParams freqSlew[2][3];

// Slew rate for frequency transitions (higher = faster transitions)
constexpr float FREQ_SLEW_RATE = 0.0005f; // Adjust this value to control slide speed

 VoiceState voiceState1;
 VoiceState voiceState2;

// =======================
//   OTHER CONSTANTS
// =======================
int MAX_HEIGHT = 1400;
int MIN_HEIGHT = 74;
int PIN_TOUCH_IRQ = 10;
uint8_t PICO_AUDIO_I2S_DATA_PIN = 15;
uint8_t PICO_AUDIO_I2S_CLOCK_PIN_BASE = 16;
int IRQ_PIN = 1;
int MAX_MIDI_NOTES = 16;
float INT16_MAX_AS_FLOAT = 32767.0f;
float INT16_MIN_AS_FLOAT = -32768.0f;
int NUM_AUDIO_BUFFERS = 3;
int SAMPLES_PER_BUFFER = 256;
float OSC_DETUNE_FACTOR = .0014f;
bool resetStepsLightsFlag = true;
float delayTarget = 48000.0f * .15f;
float currentDelay = 48000.0f * .15f;
float feedbackAmmount = 0.765f;
const float FEEDBACK_FADE_RATE = 0.0001f; // Adjust for desired fade speed


 float lfo1LEDWaveformValue = 0.0f; // Current LFO1 waveform value for smooth LED fade (-1.0 to 1.0)
 float lfo2LEDWaveformValue = 0.0f; // Current LFO2 waveform value for smooth LED fade (-1.0 to 1.0)


// =======================
//   FORWARD DECLARATIONS
// =======================
void fill_audio_buffer(audio_buffer_t *buffer);
void initOscillators();

void updateParametersForStep(uint8_t stepToUpdate);
void onStepCallback(uint32_t uClockCurrentStep);
void applyEnvelopeParameters(const  VoiceState &state, daisysp::Adsr &env, int voiceNum);
float calculateFilterFrequency(float filterValue);
void setupI2SAudio(audio_format_t *audioFormat, audio_i2s_config_t *i2sConfig);
void setup();
void setup1();
void loop();
void loop1();

// =======================
//     GLOBAL VARIABLES
// =======================
volatile bool GATE1 = false;
volatile bool GATE2 = false;
volatile uint8_t currentSequencerStep = 0;


 GateTimer gateTimer1;
 GateTimer gateTimer2;
// selectedStepForEdit is now defined in src/ui/ButtonManager.cpp
int raw_mm = 0;
int mm = 0;
uint8_t currentScale = 0;
float baseFreq = 110.0f;    // Hz
float filterfreq1 = 2000.f; // Renamed for Voice 1
float filterfreq2 = 2000.f; // Added for Voice 2
bool isClockRunning = true;
unsigned long previousMillis = 0;
// MIDI note tracking is now handled by MidiNoteManager in src/midi/MidiManager.h
audio_buffer_pool_t *producer_pool = nullptr;

volatile bool touchFlag = false;

void touchInterrupt()
{
    touchFlag = true;
}

float delayTimeSmoothing(float currentDelay, float targetDelay, float slewRate)
{
    float difference = targetDelay - currentDelay;
    return currentDelay + (difference * slewRate);
}


// --- Clock Callbacks ---
void onSync24Callback(uint32_t tick)
{
    usb_midi.sendRealTime(midi::Clock);
}
void muteOscillators()
{

    osc1A.SetAmp(0.0f);
    osc1B.SetAmp(0.0f); 
    osc1C.SetAmp(0.0f);
    osc2A.SetAmp(0.0f);
    osc2B.SetAmp(0.0f);
    osc2C.SetAmp(0.0f);
}
void unmuteOscillators()
{

    osc1A.SetAmp(.5f);
    osc1B.SetAmp(.5f);
    osc1C.SetAmp(.5f);

    osc2A.SetAmp(.5f);
    osc2B.SetAmp(.5f);
    osc2C.SetAmp(1.f);
}
void onClockStart()
{
    Serial.println("[uClock] onClockStart()");
    usb_midi.sendRealTime(midi::Start);
    seq1.start();
    seq2.start();
    isClockRunning = true;
    unmuteOscillators();
}

void onClockStop()
{
    Serial.println("[uClock] onClockStop()");
    usb_midi.sendRealTime(midi::Stop);
    seq1.stop();
    seq2.stop();

    // Use MidiNoteManager for comprehensive cleanup
    midiNoteManager.onSequencerStop();
    

    // Legacy allNotesOff() call for sequencer state cleanup
muteOscillators();
    isClockRunning = false;
}

// =======================
//   FUNCTION DEFINITIONS
// =======================



// --- Audio Buffer Conversion ---
static inline int16_t convertSampleToInt16(float sample)
{
    float scaled = sample * INT16_MAX_AS_FLOAT;
    scaled = roundf(scaled);
    scaled = daisysp::fclamp(scaled, INT16_MIN_AS_FLOAT, INT16_MAX_AS_FLOAT);
    return static_cast<int16_t>(scaled);
}

// --- Oscillator and Envelope Initialization ---
void initOscillators()
{

    highPass1.Init(SAMPLE_RATE);
    highPass1.SetFreq(80.f); //  this removes mud from voice1
    highPass2.Init(SAMPLE_RATE);
    highPass2.SetFreq(140.f); // this removes lows and mud from voice2
    delLowPass.Init(SAMPLE_RATE);
    delLowPass.SetFreq(1340.f); //  delay Lowpass filter
    delLowPass.SetRes(0.19f);
    delLowPass.SetDrive(.9f);

    // voice 1
    osc1A.Init(SAMPLE_RATE);
    osc1B.Init(SAMPLE_RATE);
    osc1C.Init(SAMPLE_RATE);
    filt1.Init(SAMPLE_RATE);
    filt2.Init(SAMPLE_RATE);
    filt1.SetFreq(1000.f);
    filt1.SetRes(0.4f);
    filt1.SetInputDrive(1.1f);
    filt1.SetPassbandGain(0.23f);

    env1.Init(SAMPLE_RATE);
    env1.SetReleaseTime(.1f);
    env1.SetAttackTime(0.04f);
    env1.SetDecayTime(0.14f);
    env1.SetSustainLevel(0.5f);

    // voice 2

    osc2A.Init(SAMPLE_RATE);
    osc2B.Init(SAMPLE_RATE);

    env2.Init(SAMPLE_RATE);
    env2.SetAttackTime(0.015f);
    env2.SetDecayTime(0.1f);
    env2.SetSustainLevel(0.4f);
    env2.SetReleaseTime(0.1f);

    filt2.SetFreq(1000.f);
    filt2.SetRes(0.22f);
    filt2.SetInputDrive(2.f);
    filt2.SetPassbandGain(0.14f);
    lfo1.Init(SAMPLE_RATE);
    lfo1.SetWaveform(daisysp::Oscillator::WAVE_TRI);
    lfo1.SetFreq(0.5f);
    lfo1.SetAmp(0.5f);
    lfo2.Init(SAMPLE_RATE);
    lfo2.SetWaveform(daisysp::Oscillator::WAVE_TRI);
    lfo2.SetFreq(0.5f);
    lfo2.SetAmp(0.5f);
    osc1A.SetWaveform(daisysp::Oscillator::WAVE_POLYBLEP_SAW);
    osc1A.SetAmp(0.5f);
    osc1B.SetWaveform(daisysp::Oscillator::WAVE_POLYBLEP_SAW);
    osc1C.SetWaveform(daisysp::Oscillator::WAVE_POLYBLEP_SAW);
    osc2A.SetWaveform(daisysp::Oscillator::WAVE_POLYBLEP_SQUARE);
    osc2B.SetWaveform(daisysp::Oscillator::WAVE_POLYBLEP_SQUARE);
    osc2B.SetPw(0.35f);
    osc2A.SetPw(0.6f);
    osc2C.SetWaveform(daisysp::Oscillator::WAVE_POLYBLEP_SAW);
    osc2C.SetAmp(1.0f);

    del1.Init();

    const float delayMs1 = 500.f;
    size_t delaySamples1 = (size_t)(delayMs1 * SAMPLE_RATE * 0.001f);
    del1.SetDelay(delaySamples1);

    // Initialize frequency slewing variables using the structured approach
    float defaultFreq = 440.0f;
    for (int voice = 0; voice < 2; voice++) {
        for (int osc = 0; osc < 3; osc++) {
            freqSlew[voice][osc].currentFreq = defaultFreq;
            freqSlew[voice][osc].targetFreq = defaultFreq;
        }
    }
}

// Long press detection is now handled by ButtonManager module
// isVoice2Mode is now managed by ButtonManager module
const uint8_t VOICE2_LED_OFFSET = 32;                        // Starting LED index for Voice 2
int currentThemeIndex = static_cast<int>(LEDTheme::DEFAULT); // Global variable for current theme
// Somewhere accessible by both the ISR and loop1(), e.g., globally or in a class
 uint32_t ppqnTicksPending = 0;

void onOutputPPQNCallback(uint32_t tick)
{
    // Increment the counter to signal a pending tick
    ppqnTicksPending++;
    // That's it! Keep the ISR minimal.
}


// =======================
//   HELPER FUNCTIONS FOR VOICE PARAMETER CALCULATIONS
// =======================

/**

 * @param state Voice state containing normalized parameter values (0.0-1.0)
 * @param env DaisySP ADSR envelope to configure
 * @param voiceNum Voice number (1 or 2) for voice-specific parameter ranges
 */
void applyEnvelopeParameters(const  VoiceState &state, daisysp::Adsr &env, int voiceNum)
{
    float attack, release;

    if (voiceNum == 1)
    {
        attack = daisysp::fmap(state.attack, 0.005f, 0.75f, daisysp::Mapping::LINEAR);
        release = daisysp::fmap(state.decay, 0.01f, .6f, daisysp::Mapping::LINEAR);
        env.SetDecayTime(.1f + (release*.75));
    }
    else
    {
        attack = daisysp::fmap(state.attack, 0.005f, 0.754f, daisysp::Mapping::LINEAR);
        release = daisysp::fmap(state.decay, 0.01f, .6f, daisysp::Mapping::LINEAR);
        env.SetDecayTime(.01f + (release * 0.25f));
    }

    env.SetAttackTime(attack);
    env.SetReleaseTime(release);
}

/**
 * Calculate filter cutoff frequency with exponential scaling.
 * Range: 100Hz to 5710Hz provides musical filter sweep from bass to presence
 * @param filterValue Normalized filter value (0.0-1.0) from sequencer
 * @return Filter frequency in Hz
 */
float calculateFilterFrequency(float filterValue)
{
    return daisysp::fmap(filterValue, 100.0f, 6710.0f, daisysp::Mapping::EXP);
}




// --- Update Parameters for Step Editing ---
void updateParametersForStep(uint8_t stepToUpdate) ///  This is the selected step for edit function
{
    if (stepToUpdate >= SEQUENCER_MAX_STEPS)
        return;

    Sequencer &activeSeq = uiState.isVoice2Mode ? seq2 : seq1;

    // Simple normalization of sensor value
    float normalized_mm_value = 0.0f;
    if (MAX_HEIGHT > 0)
    {
        normalized_mm_value = static_cast<float>(mm) / static_cast<float>(MAX_HEIGHT);
    }
    normalized_mm_value = std::max(0.0f, std::min(normalized_mm_value, 1.0f));

    bool parametersWereUpdated = false;
    const ParamButtonMapping* heldMapping = getHeldParameterButton(uiState);
    if (heldMapping)
    {
        // Use the helper function to do the scaling correctly for any parameter.
        float valueToSet = mapNormalizedValueToParamRange(heldMapping->paramId, normalized_mm_value);
        activeSeq.setStepParameterValue(heldMapping->paramId, stepToUpdate, valueToSet);
        parametersWereUpdated = true;

        // Send immediate MIDI CC for real-time parameter recording
        uint8_t voiceId = uiState.isVoice2Mode ? 1 : 0;
        midiNoteManager.updateParameterCC(voiceId, heldMapping->paramId, valueToSet);

        // Debug print if needed
        Serial.print("  -> Set ");
        Serial.print(CORE_PARAMETERS[static_cast<int>(heldMapping->paramId)].name);
        Serial.print(" to ");
        Serial.println(valueToSet);
    }

    // Provide immediate audio feedback when recording parameters to current step
    if (parametersWereUpdated)
    {
        updateActiveVoiceState(stepToUpdate, activeSeq);
    }
}

void updateVoiceParameters(
    const  VoiceState &state,
    bool isVoice2,
    bool updateGate = false,
    volatile bool *gate = nullptr,
    volatile GateTimer *gateTimer = nullptr)
{
    // Handle gate timing and MIDI note events (sequencer playback mode only)
    if (updateGate && gate && gateTimer)
    {
        uint8_t voiceId = isVoice2 ? 1 : 0;

        if (state.gate)
        {
            // Calculate MIDI note to match audio synthesis approach
            uint8_t noteIndex = static_cast<uint8_t>(std::max(0.0f, std::min(state.note, 47.0f)));
            int midiNote = scale[currentScale][noteIndex] + 36 + static_cast<int>(state.octave);

            // Always restart the gate timer for gated steps to ensure proper timing
            gateTimer->start(state.gateLength);

            // Only send MIDI note-on when gate transitions from off to on
            if (!(*gate))
            {
                *gate = true;

                // Clamp MIDI note to valid range (0-127)
                int clampedMidiNote = std::max(0, std::min(midiNote, 127));

                // Use MidiNoteManager for proper note lifecycle management
                midiNoteManager.noteOn(voiceId, static_cast<int8_t>(clampedMidiNote),
                                     static_cast<uint8_t>(state.velocity * 127), 1, state.gateLength);
            }
            else
            {
                // Gate is already on - check if note changed and handle retrigger
                int8_t currentActiveNote = midiNoteManager.getActiveNote(voiceId);
                if (currentActiveNote != midiNote)
                {
                    // Note changed during gate - retrigger with new note
                    midiNoteManager.noteOn(voiceId, static_cast<int8_t>(midiNote),
                                         static_cast<uint8_t>(state.velocity * 127), 1, state.gateLength);
                }
                *gate = true;
            }

            // Update MidiNoteManager gate state
            midiNoteManager.setGateState(voiceId, true, state.gateLength);
        }
        else
        {
            // Step has no gate - turn off immediately
            gateTimer->stop();
            *gate = false;

            // Use MidiNoteManager for proper note-off handling
            midiNoteManager.setGateState(voiceId, false);
        }
    }

    // Update oscillator frequencies when gate is active or during editing
    if (!updateGate || (gate && *gate))
    {
        int noteIndex = state.note;
   int harmony = 7;

        if (!isVoice2)
        {
            // Voice 1 updates - use consistent base note of 48
            float freq = daisysp::mtof(scale[currentScale][noteIndex] + 48 + state.octave);
           float freqB = freq + (freq * OSC_DETUNE_FACTOR);
           float freqC = freq - (freq * OSC_DETUNE_FACTOR);

            if (state.slide)
            {
                // Slide mode: set target frequencies for smooth interpolation
                freqSlew[0][0].targetFreq = freq;  // Voice 1, Oscillator A
               freqSlew[0][1].targetFreq = freqB;  // Voice 1, Oscillator B
               freqSlew[0][2].targetFreq = freqC;  // Voice 1, Oscillator C
            }
            else
            {
                // Non-slide mode: set frequencies directly and update current values
                osc1A.SetFreq(freq);
             osc1B.SetFreq(freqB);
               osc1C.SetFreq(freqC);
                freqSlew[0][0].currentFreq = freq;  // Voice 1, Oscillator A
                freqSlew[0][1].currentFreq = freqB;  // Voice 1, Oscillator B
            freqSlew[0][2].currentFreq = freqC;  // Voice 1, Oscillator C
                freqSlew[0][0].targetFreq = freq;   // Voice 1, Oscillator A
                freqSlew[0][1].targetFreq = freqB;   // Voice 1, Oscillator B
                freqSlew[0][2].targetFreq = freqC;   // Voice 1, Oscillator C
            }
        }
        else
        {
            // Voice 2 updates - use base notes of 48 and 36
            float freq1 = daisysp::mtof(scale[currentScale][noteIndex] + 48 + state.octave);
            float freq2 = daisysp::mtof(scale[currentScale][noteIndex+harmony] + 48 + state.octave);
         float freq3 = daisysp::mtof(scale[currentScale][noteIndex] + 36 + state.octave);

            if (state.slide)
            {
                // Slide mode: set target frequencies for smooth interpolation
                freqSlew[1][0].targetFreq = freq1;  // Voice 2, Oscillator A
                freqSlew[1][1].targetFreq = freq2;  // Voice 2, Oscillator B
              freqSlew[1][2].targetFreq = freq3;  // Voice 2, Oscillator C
            }
            else
            {
                // Non-slide mode: set frequencies directly and update current values
                osc2A.SetFreq(freq1);
                osc2B.SetFreq(freq2);
                osc2C.SetFreq(freq3);
                freqSlew[1][0].currentFreq = freq1;  // Voice 2, Oscillator A
                freqSlew[1][1].currentFreq = freq2;  // Voice 2, Oscillator B
               freqSlew[1][2].currentFreq = freq3;  // Voice 2, Oscillator C
                freqSlew[1][0].targetFreq = freq1;   // Voice 2, Oscillator A
                freqSlew[1][1].targetFreq = freq2;   // Voice 2, Oscillator B
             freqSlew[1][2].targetFreq = freq3;   // Voice 2, Oscillator C
            }
        }
    }

    // Always update envelope parameters regardless of gate state
    daisysp::Adsr &env = isVoice2 ? env2 : env1;
    applyEnvelopeParameters(state, env, isVoice2 ? 2 : 1);

    // Always update filter frequency for smooth parameter changes
     float &filterFreq = isVoice2 ? filterfreq2 : filterfreq1;
    filterFreq = calculateFilterFrequency(state.filter);

    // Send MIDI CC messages for parameter changes
    uint8_t voiceId = isVoice2 ? 1 : 0;
    midiNoteManager.updateParameterCC(voiceId, ParamId::Filter, state.filter);
    midiNoteManager.updateParameterCC(voiceId, ParamId::Attack, state.attack);
    midiNoteManager.updateParameterCC(voiceId, ParamId::Decay, state.decay);
    midiNoteManager.updateParameterCC(voiceId, ParamId::Octave, state.octave);
}

/**
 * Apply immediate voice parameter updates during real-time parameter recording.
 * Called when user holds parameter buttons (16-21) to provide instant audio feedback.
 */
void updateActiveVoiceState(uint8_t stepIndex, Sequencer &activeSeq)
{
    uint8_t currentSequencerStep = activeSeq.getCurrentStep();

    // Only update currently playing step to avoid audio glitches
    if (stepIndex != currentSequencerStep)
    {
        return;
    }

     VoiceState *activeVoiceState = uiState.isVoice2Mode ? &voiceState2 : &voiceState1;

    // Update voice state with new step parameters + AS5600 encoder modifications
    activeSeq.playStepNow(stepIndex, activeVoiceState);
    applyAS5600BaseValues(activeVoiceState, uiState);

    // Update synth hardware for immediate audio feedback using the unified function
    updateVoiceParameters(*activeVoiceState, uiState.isVoice2Mode);

    Serial.print("Applied immediate voice updates for step ");
    Serial.println(stepIndex);
}

//  This gets called every 16th note
void onStepCallback(uint32_t uClockCurrentStep)
{
    currentSequencerStep = static_cast<uint8_t>(uClockCurrentStep); // Raw uClock step, sequencers handle their own modulo
    
    // 2. Advance sequencers and get their new state into local temporary variables.
    VoiceState tempState1, tempState2;
    seq1.advanceStep(uClockCurrentStep, mm, uiState, &tempState1, lfo1.Process() * globalLFOs.lfo1amp, lfo2.Process() * globalLFOs.lfo2amp);
    seq2.advanceStep(uClockCurrentStep, mm, uiState, &tempState2, lfo1.Process() * globalLFOs.lfo1amp, lfo2.Process() * globalLFOs.lfo2amp);

    // 3. Apply AS5600 base values to the voice states before using them
    // Create temporary UIState copies with appropriate voice modes for AS5600 application
    UIState tempUIState1 = uiState; tempUIState1.isVoice2Mode = false;
    UIState tempUIState2 = uiState; tempUIState2.isVoice2Mode = true;
    applyAS5600BaseValues(&tempState1, tempUIState1); // Voice 1
    applyAS5600BaseValues(&tempState2, tempUIState2); // Voice 2

    // Apply AS5600 base values to global delay effect parameters
    applyAS5600DelayValues();

    // Apply AS5600 base values to global LFO parameters
    applyAS5600LFOValues();

    // 4. Update synth hardware directly using the unified voice parameter function.
    updateVoiceParameters(tempState1, false, true, &GATE1, &gateTimer1);
    updateVoiceParameters(tempState2, true, true, &GATE2, &gateTimer2);

   
    memcpy((void *)&voiceState1, &tempState1, sizeof(VoiceState));
    memcpy((void *)&voiceState2, &tempState2, sizeof(VoiceState));
}

void fill_audio_buffer(audio_buffer_t *buffer)
{
    int N = buffer->max_sample_count;
    int16_t *out = reinterpret_cast<int16_t *>(buffer->buffer->bytes);
    float finalvoice;
    float lfo1Output;
    float lfo2Output;
    float delout;
    float sigout;
    float slew = 0.0001f; // Slew rate for delay time smoothing
    float output;
    // Determine the target gains based on delayOn state

    float targetDelayOutputGain = uiState.delayOn ? 1.0f : 0.0f; // 1.0f for full output, 0.0f for no output
    float targetFeedbackGain = uiState.delayOn ? feedbackAmmount : 0.0f;
    for (int i = 0; i < N; ++i)
    {

        lfo1Output = lfo1.Process();
        lfo2Output = lfo2.Process();

        // Capture actual LFO output values for LED synchronization
        // Smooth the currentFeedbackGain towards its target
        currentFeedbackGain = delayTimeSmoothing(currentFeedbackGain, targetFeedbackGain, FEEDBACK_FADE_RATE);
        // Smooth the currentDelayOutputGain towards its target
        currentDelayOutputGain = delayTimeSmoothing(currentDelayOutputGain, targetDelayOutputGain, FEEDBACK_FADE_RATE);
        // Apply smoothing to delay time
        currentDelay = delayTimeSmoothing(currentDelay, delayTarget, slew); // Fixed: added slew argument
        // Sample every 64 samples to reduce overhead while maintaining visual accuracy
        if (i % 64 == 0)
        {
            lfo1LEDWaveformValue = lfo1Output;
            lfo2LEDWaveformValue = lfo2Output;
        }

        // Frequency slewing for slide functionality
        // Voice 1 frequency slewing
        if (voiceState1.slide)
        {
            // Exponential slewing for smooth frequency transitions
            freqSlew[0][0].currentFreq += (freqSlew[0][0].targetFreq - freqSlew[0][0].currentFreq) * FREQ_SLEW_RATE;
           freqSlew[0][1].currentFreq += (freqSlew[0][1].targetFreq - freqSlew[0][1].currentFreq) * FREQ_SLEW_RATE;
            freqSlew[0][2].currentFreq += (freqSlew[0][2].targetFreq - freqSlew[0][2].currentFreq) * FREQ_SLEW_RATE;
            osc1A.SetFreq(freqSlew[0][0].currentFreq);
            osc1B.SetFreq(freqSlew[0][1].currentFreq);
           osc1C.SetFreq(freqSlew[0][2].currentFreq);
        }

        // Voice 2 frequency slewing
        if (voiceState2.slide)
        {
            // Exponential slewing for smooth frequency transitions
            freqSlew[1][0].currentFreq += (freqSlew[1][0].targetFreq - freqSlew[1][0].currentFreq) * FREQ_SLEW_RATE;
            freqSlew[1][1].currentFreq += (freqSlew[1][1].targetFreq - freqSlew[1][1].currentFreq) * FREQ_SLEW_RATE;
           freqSlew[1][2].currentFreq += (freqSlew[1][2].targetFreq - freqSlew[1][2].currentFreq) * FREQ_SLEW_RATE;
            osc2A.SetFreq(freqSlew[1][0].currentFreq);
            osc2B.SetFreq(freqSlew[1][1].currentFreq);
          osc2C.SetFreq(freqSlew[1][2].currentFreq);
        }

        // Process voice 1
        // Check for envelope retrigger request
        if (voiceState1.retrigger)
        {
            env1.Retrigger(false);         // Retrigger envelope without hard reset
            voiceState1.retrigger = false; // Clear the flag
        }

        // Get envelope state using public methods of sequencer 1
        float current_filter_env_value1 = env1.Process(GATE1);
        filt1.SetFreq(filterfreq1 * current_filter_env_value1 );
       
        float voice1 = (osc1A.Process() + osc1B.Process() + osc1C.Process()) *.6f;


        float filtered_signal1 = filt1.Process(voice1);
                        highPass1.Process(filtered_signal1);

        float final_voice1 = highPass1.High() * current_filter_env_value1;


        // Process voice 2
        // Check for envelope retrigger request
        if (voiceState2.retrigger)
        {
            env2.Retrigger(false);         // Retrigger envelope without hard reset
            voiceState2.retrigger = false; // Clear the flag
        }

        // Get envelope state using public methods of sequencer 2
        float current_filter_env_value2 = env2.Process(GATE2);
        filt2.SetFreq((filterfreq2 * current_filter_env_value2) );

        float voice2 = (osc2A.Process() + osc2B.Process()+osc2C.Process() ) *.6f;
        float filtered_signal2 = filt2.Process(voice2) ; 

        highPass2.Process(filtered_signal2);


        float final_voice2 = highPass2.High() * current_filter_env_value2;

        finalvoice = (final_voice1 + final_voice2) * .5f;

        // Delay processing (always active, feedback controlled by currentFeedbackGain)
        del1.SetDelay(currentDelay);  // Set delay time (smoothed)
        delout = del1.Read();         
        sigout = (delout * currentDelayOutputGain) + finalvoice; // Mix dry input with current delay output (faded) for final audio output

        // Calculate feedback signal using the smoothed gain
        float feedbackSignal = delout * currentFeedbackGain; // Apply smoothed feedback gain here
        delLowPass.Process(feedbackSignal*.85f);                  // Filter the feedback signal

        // Write to the delay line: dry input + filtered feedback
        float signalToWrite = finalvoice + delLowPass.Low();
        del1.Write(signalToWrite); // Write to delay line

        output = sigout; // Final output is the mixed signal

        // Output mixing
        // Output mixing
        out[2 * i + 0] = convertSampleToInt16(output);
        out[2 * i + 1] = convertSampleToInt16(output);
    }

    buffer->sample_count = N;
}


// --- Audio I2S Setup ---
void setupI2SAudio(audio_format_t *audioFormat, audio_i2s_config_t *i2sConfig)
{
    if (!audio_i2s_setup(audioFormat, i2sConfig))
    {
        g_errorState |= ERR_AUDIO;
        return;
    }
    if (!audio_i2s_connect(producer_pool))
    {
        g_errorState |= ERR_AUDIO;
        return;
    }
    audio_i2s_set_enabled(true);
    g_audioOK = true;
}

// --- Arduino Setup (Core0) ---
void setup()
{
    delay(100);
    initOscillators();
    static audio_format_t audioFormat = {
        .sample_freq = (uint32_t)SAMPLE_RATE,
        .format = AUDIO_BUFFER_FORMAT_PCM_S16,
        .channel_count = 2};
    static audio_buffer_format_t bufferFormat = {
        .format = &audioFormat,
        .sample_stride = 4};
    producer_pool = audio_new_producer_pool(&bufferFormat, NUM_AUDIO_BUFFERS, SAMPLES_PER_BUFFER);
    audio_i2s_config_t i2sConfig = {
        .data_pin = PICO_AUDIO_I2S_DATA_PIN,
        .clock_pin_base = PICO_AUDIO_I2S_CLOCK_PIN_BASE,
        .dma_channel = 0,
        .pio_sm = 0};
    setupI2SAudio(&audioFormat, &i2sConfig);
}

void setup1()
{
    delay(300);
    usb_midi.begin(MIDI_CHANNEL_OMNI);
    delay(100);

    Serial.begin(115200);

  Serial.print("[CORE1] Setup starting... ");

    randomSeed(analogRead(A0) + millis());
    ledMatrix.begin(200);
    setupLEDMatrixFeedback();
    initLEDController();

    if (!distanceSensor.begin())
    {
        Serial.println("[ERROR] Distance sensor initialization failed!");
    }
    else
    {
        Serial.println("Distance sensor initialized successfully");
    }

    // Initialize AS5600 magnetic encoder
    if (!as5600Sensor.begin())
    {
        Serial.println("[ERROR] AS5600 magnetic encoder initialization failed!");
    }
    else
    {
        Serial.println("AS5600 magnetic encoder initialized successfully");

        // Uncomment the line below to run smooth scaling validation test
        // as5600Sensor.validateSmoothScaling();
    }

    // Initialize AS5600 base values with proper defaults
    initAS5600BaseValues();

  if (!touchSensor.begin(0x5A)) {
    Serial.println("MPR121 not found, check wiring?");
    while (1);
  }
    else
    {
        Serial.println("MPR121 found and initialized");
        touchSensor.setAutoconfig(true);

        // Configure MPR121 touch thresholds.
        // Using the original, more conservative thresholds.
        // touchSensor.setThresholds(155, 55); // touch, release thresholds
        // Serial.println("MPR121 thresholds configured to 155/55");
    }

    initUIEventHandler(uiState);

    Matrix_init(&touchSensor);
    Serial.println("Matrix initialized");
    
    // Test touch sensor immediately
    uint16_t testTouch = touchSensor.touched();
    Serial.print("Initial touch test: 0x");
    if (testTouch < 0x100) Serial.print("0");
    if (testTouch < 0x10) Serial.print("0");
    Serial.println(testTouch, HEX);
    
    // Force a matrix scan to test the system
    Serial.println("Forcing initial matrix scan...");
    Matrix_scan();
    Matrix_printState();
    
    // Add one-time confirmation that matrix system is ready
    Serial.println("Button matrix system is now active - touch buttons to test");
    
    // Use a lambda to capture the context needed by the event handler
    Matrix_setEventHandler([](const MatrixButtonEvent &evt) {
        Serial.print("Matrix event: button ");
        Serial.print(evt.buttonIndex);
        Serial.print(evt.type == MATRIX_BUTTON_PRESSED ? " pressed" : " released");
        Serial.println();
        matrixEventHandler(evt, uiState, seq1, seq2, midiNoteManager);
    });
    // Rising edge handler not needed - all events handled by main matrixEventHandler

    uClock.init();
    uClock.setOnSync24(onSync24Callback);
    uClock.setOnClockStart(onClockStart);
    uClock.setOnClockStop(onClockStop);
    uClock.setOutputPPQN(uClock.PPQN_480);
    uClock.setOnStep(onStepCallback);
    uClock.setOnOutputPPQN(onOutputPPQNCallback);
    uClock.setTempo(90);
    uClock.start();
    uClock.setShuffle(true); 
    seq1.start();
    seq2.start();



    Serial.println("[CORE1] Setup complete!");
}

// Distance sensor functionality moved to src/sensors/DistanceSensor.h/.cpp

// --- Audio Loop (Core0) ---
void loop()
{
    audio_buffer_t *buf = take_audio_buffer(producer_pool, true);
    if (buf)
    {
        fill_audio_buffer(buf);
        give_audio_buffer(producer_pool, buf);
    }
}

// --- Optimized LED and UI Update Loop (Core1) ---
void loop1()
{
    static bool firstRun = true;
    if (firstRun) {
        Serial.println("loop1 started - matrix scanning active");
        firstRun = false;
    }
    
    usb_midi.read();

    unsigned long currentMillis = millis();

    // Check if any parameter buttons are held for prioritized sensor updates
    bool parameterRecordingActive = isAnyParameterButtonHeld(uiState);

    // Update sensor once per loop iteration (avoid multiple calls)
    distanceSensor.update();
    // Update global mm variable for backward compatibility
    int rawValue = distanceSensor.getRawValue();
    if (rawValue >= MIN_HEIGHT && rawValue <= MAX_HEIGHT)
    {
        mm = rawValue - MIN_HEIGHT;
    }
    else
    {
        mm = 0; // Invalid reading
    }

    // Update AS5600 sensor and base values
    as5600Sensor.update();
    updateAS5600BaseValues(uiState);

    // Process all pending PPQN ticks
    static uint16_t globalTickCounter = 0; // Global tick counter for MidiNoteManager

    while (ppqnTicksPending > 0)
    {
        // Decrement the counter *before* processing the tick
        ppqnTicksPending--;
        globalTickCounter++;

        // Update MidiNoteManager timing - this handles all MIDI note-off timing
        midiNoteManager.updateTiming(globalTickCounter);

        // Process sequencer note duration timing
        seq1.tickNoteDuration(&voiceState1);
        seq2.tickNoteDuration(&voiceState2);

        // Process gate timers - now synchronized with MidiNoteManager
        gateTimer1.tick();

        if (gateTimer1.isExpired() && GATE1)
        {
            GATE1 = false;
        }

        // Safety mechanism: Force gate off if it's been active too long (more than 2 seconds worth of ticks)
        if (GATE1 && gateTimer1.totalTicksProcessed > 960) // 960 ticks = 2 seconds at 480 PPQN, 90 BPM
        {
            GATE1 = false;
            gateTimer1.stop();
            // Force emergency note-off through MidiNoteManager
            midiNoteManager.setGateState(0, false);
        }

        gateTimer2.tick();
        if (gateTimer2.isExpired() && GATE2)
        {
            GATE2 = false;
        }

        // Safety mechanism: Force gate off if it's been active too long (more than 2 seconds worth of ticks)
        if (GATE2 && gateTimer2.totalTicksProcessed > 960) // 960 ticks = 2 seconds at 480 PPQN, 90 BPM
        {
            GATE2 = false;
            gateTimer2.stop();
            // Force emergency note-off through MidiNoteManager
            midiNoteManager.setGateState(1, false);
        }
    }



    uint16_t currtouched = touchSensor.touched();

  

    // Always scan the matrix regardless of timing - this is critical for debugging
    Matrix_scan();
 
        updateStepLEDs(ledMatrix, seq1, seq2, uiState, mm);

        // LFO LED pulse states are updated directly in fill_audio_buffer()

        updateControlLEDs(ledMatrix, uiState);
        ledMatrix.show();

        if (uiState.selectedStepForEdit != -1)
        {
            updateParametersForStep(uiState.selectedStepForEdit);
        }
    }

