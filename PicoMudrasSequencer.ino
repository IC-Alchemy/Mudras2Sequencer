#include "includes.h"
#include "diagnostic.h"
#include "src/dsp/dsp.h"
#include "src/voice/Voice.h"
#include "src/utils/Debug.h"
#include "src/scales/scales.h"


// =======================
//   GLOBAL VARIABLES
// =======================
UIState uiState; // Central state object for the UI
Sequencer seq1(1); // Channel 1 for first sequencer
Sequencer seq2(2); // Channel 2 for second sequencer
Sequencer seq3(3); // Channel 3 for third sequencer
Sequencer seq4(4); // Channel 4 for fourth sequencer
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
// New Voice System
std::unique_ptr<VoiceManager> voiceManager;
uint8_t leadVoiceId = 0;
uint8_t bassVoiceId = 0;
uint8_t voice3Id = 0;
uint8_t voice4Id = 0;

// Global effects and delay (shared between voices)
daisysp::Svf delLowPass;
daisysp::DelayLine<float, MAX_DELAY_SAMPLES> del1;
float feedbackGain1 = 0.65f;
float currentDelayOutputGain = 0.0f; // For smooth delay output fade
OLEDDisplay display;
float currentFeedbackGain = 0.0f; // For smooth delay feedback fade

// Legacy voice states for compatibility
VoiceState voiceState1;
VoiceState voiceState2;
VoiceState voiceState3;
VoiceState voiceState4;

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
float OSC_DETUNE_FACTOR = .001f;
bool resetStepsLightsFlag = true;
float delayTarget = 48000.0f * .15f;
float currentDelay = 48000.0f * .15f;
float feedbackAmmount = 0.45f; // Safer initial feedback level
const float FEEDBACK_FADE_RATE = 0.001f; // Faster fade to prevent feedback buildup



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
    if (voiceManager) {
        voiceManager->setVoiceVolume(leadVoiceId, 0.0f);
        voiceManager->setVoiceVolume(bassVoiceId, 0.0f);
    }
}

void unmuteOscillators()
{
    if (voiceManager) {
        voiceManager->setVoiceVolume(leadVoiceId, 0.5f);
        voiceManager->setVoiceVolume(bassVoiceId, 0.5f);
    }
}
void onClockStart()
{
    Serial.println("[uClock] onClockStart()");
    usb_midi.sendRealTime(midi::Start);
    // Start all four sequencers so LEDs and audio advance for 3/4 as well
    seq1.start();
    seq2.start();
    seq3.start();
    seq4.start();
    isClockRunning = true;
    unmuteOscillators();
}

void onClockStop()
{
    Serial.println("[uClock] onClockStop()");
    usb_midi.sendRealTime(midi::Stop);
    // Stop all four sequencers
    seq1.stop();
    seq2.stop();
    seq3.stop();
    seq4.stop();

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

// --- Voice System Initialization ---
void initOscillators()
{
    // Initialize global effects
    delLowPass.Init(SAMPLE_RATE);
    delLowPass.SetFreq(1340.f); //  delay Lowpass filter
    delLowPass.SetRes(0.19f);
    delLowPass.SetDrive(.9f);

    del1.Init();
    del1.Reset(); // Clear any garbage in delay buffer
    const float delayMs1 = 500.f;
    size_t delaySamples1 = (size_t)(delayMs1 * SAMPLE_RATE * 0.001f);
    del1.SetDelay(delaySamples1);

    // Initialize delay target to match initial delay
    delayTarget = static_cast<float>(delaySamples1);

    // Initialize Voice Manager with proper maxVoices parameter
    voiceManager = std::make_unique<VoiceManager>(8); // Max 8 voices instead of SAMPLE_RATE

    // Create voices 1-4 using presets from UIState defaults
    leadVoiceId = voiceManager->addVoice(VoicePresets::getPresetConfig(uiState.voice1PresetIndex));
    bassVoiceId = voiceManager->addVoice(VoicePresets::getPresetConfig(uiState.voice2PresetIndex));
    voice3Id    = voiceManager->addVoice(VoicePresets::getPresetConfig(uiState.voice3PresetIndex));
    voice4Id    = voiceManager->addVoice(VoicePresets::getPresetConfig(uiState.voice4PresetIndex));

    // Attach sequencers to voices
    voiceManager->attachSequencer(leadVoiceId, &seq1);
    voiceManager->attachSequencer(bassVoiceId, &seq2);
    voiceManager->attachSequencer(voice3Id, &seq3);
    voiceManager->attachSequencer(voice4Id, &seq4);

    // Register OLED display as observer for voice parameter changes
    // Note: This will be called after display.begin() in setup1()
    // The actual registration will happen in setup1() after display initialization
}

// Apply voice preset to the specified voice
void applyVoicePreset(uint8_t voiceNumber, uint8_t presetIndex) {
    if (presetIndex >= VoicePresets::getPresetCount()) {
        Serial.println("Invalid preset index");
        return;
    }

    VoiceConfig config = VoicePresets::getPresetConfig(presetIndex);
    uint8_t voiceId;
    switch (voiceNumber) {
        case 1: voiceId = leadVoiceId; break;
        case 2: voiceId = bassVoiceId; break;
        case 3: voiceId = voice3Id; break;
        case 4: voiceId = voice4Id; break;
        default: Serial.println("Invalid voice number"); return;
    }

    if (voiceManager->setVoiceConfig(voiceId, config)) {
        Serial.print("Applied preset '");
        Serial.print(VoicePresets::getPresetName(presetIndex));
        Serial.print("' to Voice ");
        Serial.println(voiceNumber);
    } else {
        Serial.println("Failed to apply voice preset");
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
        release = daisysp::fmap(state.decay, 0.001f, .6f, daisysp::Mapping::LINEAR);
        env.SetDecayTime(.01f + (release*.75));
    }
    else
    {
        attack = daisysp::fmap(state.attack, 0.005f, 0.754f, daisysp::Mapping::LINEAR);
        release = daisysp::fmap(state.decay, 0.001f, .6f, daisysp::Mapping::LINEAR);
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
    return daisysp::fmap(filterValue, 100.0f, 9710.0f, daisysp::Mapping::EXP);
}




// --- Update Parameters for Step Editing ---
void updateParametersForStep(uint8_t stepToUpdate) ///  This is the selected step for edit function
{
    if (stepToUpdate >= SEQUENCER_MAX_STEPS)
        return;

    Sequencer *seqPtr = (uiState.selectedVoiceIndex == 0) ? &seq1 :
                        (uiState.selectedVoiceIndex == 1) ? &seq2 :
                        (uiState.selectedVoiceIndex == 2) ? &seq3 : &seq4;
    Sequencer &activeSeq = *seqPtr;

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
        // GATE-CONTROLLED NOTE PROGRAMMING: Check gate restriction for Note parameter
        if (heldMapping->paramId == ParamId::Note)
        {
            float gateValue = activeSeq.getStepParameterValue(ParamId::Gate, stepToUpdate);
            if (gateValue <= 0.5f) // Gate is LOW (0.0)
            {
                // Skip Note parameter editing on steps with LOW gates
                // This protects steps from note frequency changes during programming/editing
                return;
            }
        }

        // Use the helper function to do the scaling correctly for any parameter.
        float valueToSet = mapNormalizedValueToParamRange(heldMapping->paramId, normalized_mm_value);
        activeSeq.setStepParameterValue(heldMapping->paramId, stepToUpdate, valueToSet);
        parametersWereUpdated = true;

        // Send immediate MIDI CC for real-time parameter recording (voices 1/2 only)
        uint8_t midiVoiceId = (uiState.selectedVoiceIndex == 0) ? 0 : (uiState.selectedVoiceIndex == 1) ? 1 : 255;
        if (midiVoiceId != 255)
        {
            midiNoteManager.updateParameterCC(midiVoiceId, heldMapping->paramId, valueToSet);
        }

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
            uint8_t noteIndex = static_cast<uint8_t>(std::max(0.0f, std::min(state.note, static_cast<float>(SCALE_STEPS - 1))));
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

    // OPTIMIZATION: Calculate voice ID once and consolidate all voice updates
    uint8_t voiceId = isVoice2 ? bassVoiceId : leadVoiceId;

    // GATE-CONTROLLED FREQUENCY UPDATES: Only update frequency when gate is HIGH
    // This prevents new frequencies from being sent when gate is LOW, allowing
    // current notes to continue playing or fade naturally
    if (!updateGate || (gate && *gate && state.gate))
    {
        // Calculate base frequency for the voice
        int noteIndex = state.note;
        float baseFreq = daisysp::mtof(scale[currentScale][noteIndex] + 36 + state.octave);

        // Update voice frequency through VoiceManager
        voiceManager->setVoiceFrequency(voiceId, baseFreq);

        // Handle slide/portamento
        voiceManager->setVoiceSlide(voiceId, state.slide);
    }

    // Update all voice parameters through VoiceManager in single call
    voiceManager->updateVoiceState(voiceId, state);

    // Send MIDI CC messages for parameter changes
    uint8_t midiVoiceId = isVoice2 ? 1 : 0;
}
// New helper to update a specific voice (1-4)
void updateVoiceParametersForVoice(
    const VoiceState &state,
    uint8_t voiceNumber,
    bool updateGate = false,
    volatile bool *gate = nullptr,
    volatile GateTimer *gateTimer = nullptr)
{
    // For voices 1 and 2, reuse existing gate/MIDI logic; for 3/4 skip gates
    bool isVoice2 = (voiceNumber == 2);

    if (updateGate && (voiceNumber == 1 || voiceNumber == 2))
    {
        updateVoiceParameters(state, isVoice2, updateGate, gate, gateTimer);
        return;
    }

    // Map to actual VoiceManager voice IDs
    uint8_t voiceId;
    switch (voiceNumber)
    {
        case 1: voiceId = leadVoiceId; break;
        case 2: voiceId = bassVoiceId; break;
        case 3: voiceId = voice3Id; break;
        case 4: voiceId = voice4Id; break;
        default: return; // Invalid
    }

    // Frequency updates only when gate HIGH (same policy)
    // Calculate base frequency for the voice
    int noteIndex = std::max(0, std::min(static_cast<int>(state.note), static_cast<int>(SCALE_STEPS - 1)));
    float baseFreq = daisysp::mtof(scale[currentScale][noteIndex] + 36 + state.octave);
    voiceManager->setVoiceFrequency(voiceId, baseFreq);
    voiceManager->setVoiceSlide(voiceId, state.slide);

    // Push full state to voice
    voiceManager->updateVoiceState(voiceId, state);

    // Send MIDI CC only for voices 1 and 2
    if (voiceNumber == 1 || voiceNumber == 2)
    {
        uint8_t midiVoiceId = (voiceNumber == 1) ? 0 : 1;
        midiNoteManager.updateParameterCC(midiVoiceId, ParamId::Filter, state.filter);
        midiNoteManager.updateParameterCC(midiVoiceId, ParamId::Attack, state.attack);
        midiNoteManager.updateParameterCC(midiVoiceId, ParamId::Decay, state.decay);
        midiNoteManager.updateParameterCC(midiVoiceId, ParamId::Octave, state.octave);
    }

    // Voice separation verified - distance sensor now voice-specific
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

    // Determine currently selected voice (1-4)
    uint8_t selectedIndex = uiState.selectedVoiceIndex; // 0..3
    uint8_t voiceNumber = selectedIndex + 1;            // 1..4

    // Pick the matching VoiceState for the selected voice
    VoiceState *activeVoiceState = nullptr;
    switch (voiceNumber)
    {
        case 1: activeVoiceState = &voiceState1; break;
        case 2: activeVoiceState = &voiceState2; break;
        case 3: activeVoiceState = &voiceState3; break;
        case 4: activeVoiceState = &voiceState4; break;
        default: return; // Invalid
    }

    // Update voice state with new step parameters + AS5600 encoder modifications
    activeSeq.playStepNow(stepIndex, activeVoiceState);

    // Apply AS5600 base values only for voices 1/2 (no mapping for 3/4 by design)
    if (voiceNumber == 1 || voiceNumber == 2)
    {
        applyAS5600BaseValues(activeVoiceState, (voiceNumber == 2) ? 1 : 0);
    }

    // Update synth hardware for immediate audio feedback using the per-voice function
    updateVoiceParametersForVoice(*activeVoiceState, voiceNumber);

    Serial.print("Applied immediate voice updates for step ");
    Serial.println(stepIndex);
}

//  This gets called every 16th note
void onStepCallback(uint32_t uClockCurrentStep)
{
    currentSequencerStep = static_cast<uint8_t>(uClockCurrentStep); // Raw uClock step, sequencers handle their own modulo



    // 2. Advance sequencers and get their new state into local temporary variables.
    // Extend to four voices; distance sensor assigned to currently selected voice only
    VoiceState tempState1, tempState2, tempState3, tempState4;

    // Route distance sensor to the currently selected voice (1..4); others disabled (-1)
    int v1Distance = (uiState.selectedVoiceIndex == 0) ? mm : -1;
    int v2Distance = (uiState.selectedVoiceIndex == 1) ? mm : -1;
    int v3Distance = (uiState.selectedVoiceIndex == 2) ? mm : -1;
    int v4Distance = (uiState.selectedVoiceIndex == 3) ? mm : -1;

    seq1.advanceStep(uClockCurrentStep, v1Distance, uiState, &tempState1);
    seq2.advanceStep(uClockCurrentStep, v2Distance, uiState, &tempState2);
    seq3.advanceStep(uClockCurrentStep, v3Distance, uiState, &tempState3);
    seq4.advanceStep(uClockCurrentStep, v4Distance, uiState, &tempState4);

    // 3. Apply AS5600 base values per voice (only velocity/filter/attack/decay are affected)
    applyAS5600BaseValues(&tempState1, 0);
    applyAS5600BaseValues(&tempState2, 1);
    // Voices 3/4 currently share no AS5600 mapping; leave as-is

    // Apply AS5600 base values to global delay effect parameters
    applyAS5600DelayValues();

    // 4. Update synth hardware (voices 1/2 with gates + MIDI; 3/4 audio only)
    updateVoiceParametersForVoice(tempState1, 1, true, &GATE1, &gateTimer1);
    updateVoiceParametersForVoice(tempState2, 2, true, &GATE2, &gateTimer2);
    updateVoiceParametersForVoice(tempState3, 3, false);
    updateVoiceParametersForVoice(tempState4, 4, false);

    // Store states
    voiceState1 = tempState1;
    voiceState2 = tempState2;
    voiceState3 = tempState3;
    voiceState4 = tempState4;
}

void fill_audio_buffer(audio_buffer_t *buffer)
{
    int N = buffer->max_sample_count;
    int16_t *out = reinterpret_cast<int16_t *>(buffer->buffer->bytes);
    float finalvoice;
    float output;

    // Determine the target gains based on delayOn state
    float targetDelayOutputGain = uiState.delayOn ? 1.0f : 0.0f;
    float targetFeedbackGain = uiState.delayOn ? feedbackAmmount : 0.0f;

    // Smooth parameters once per buffer to reduce CPU load
    currentFeedbackGain = delayTimeSmoothing(currentFeedbackGain, targetFeedbackGain, FEEDBACK_FADE_RATE);
    currentDelayOutputGain = delayTimeSmoothing(currentDelayOutputGain, targetDelayOutputGain, FEEDBACK_FADE_RATE);
    currentDelay = delayTimeSmoothing(currentDelay, delayTarget, 0.0001f);

    // Set delay time once per buffer
    del1.SetDelay(currentDelay);


    for (int i = 0; i < N; ++i)
    {
        // Process all voices efficiently (voice states are updated by sequencer callbacks)
        finalvoice = voiceManager->processAllVoices();

        // Process delay effect
        output = processDelayEffect(finalvoice);
        float softLimitedOutput = daisysp::SoftLimit(output);
        // Output to stereo channels
        out[2 * i + 0] = convertSampleToInt16(softLimitedOutput * 0.5f);
        out[2 * i + 1] = convertSampleToInt16(softLimitedOutput * 0.5f);
    }

    buffer->sample_count = N;
}

// --- Delay Effect Processing ---
float processDelayEffect(float inputSignal)
{
    // Read from delay line
    float delout = del1.Read();

    // Calculate feedback signal with proper gain staging
    float feedbackSignal = delout * currentFeedbackGain;

    // Apply low-pass filtering to feedback to prevent harsh artifacts
    delLowPass.Process(feedbackSignal);
    float filteredFeedback = delLowPass.Low();

    // Write to delay line: dry input + filtered feedback
    // Clamp feedback to prevent runaway
    del1.Write(inputSignal + (filteredFeedback*.75f));

    // Mix dry and wet signals
    return inputSignal + (delout * currentDelayOutputGain);
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

   // Initialize lightweight debug system
   Debug::begin(115200);
   Debug::setEnabled(true);
   Debug::setLevel(Debug::Level::Info);

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
         touchSensor.setThresholds(55, 33); // touch, release thresholds
        // Serial.println("MPR121 thresholds configured to 155/55");
    }

    initUIEventHandler(uiState);

    // Initialize OLED display
    display.begin();
    Serial.println("OLED display initialized");

    // Register OLED display as observer for voice parameter changes
    if (voiceManager) {
        display.setVoiceManager(voiceManager.get());

        // Use VoiceManager's callback system for parameter updates
        voiceManager->setVoiceUpdateCallback([](uint8_t voiceId, const VoiceState& state) {
            display.onVoiceParameterChanged(voiceId, state);
        });

        Serial.println("OLED display registered as voice parameter observer");
    } else {
        Serial.println("[ERROR] VoiceManager not initialized - cannot register OLED observer");
    }

    Matrix_init(&touchSensor);
    Serial.println("Matrix initialized");



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
        Sequencer* seqs[] = { &seq1, &seq2, &seq3, &seq4 };
        matrixEventHandler(evt, uiState, seqs, 4, midiNoteManager);
    });

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

// --- LED, Display and UI Update Loop (Core1) ---
void loop1()
{

    usb_midi.read();
    pollUIHeldButtons(uiState, seq1, seq2);

    unsigned long currentMillis = millis();

    // Check if any parameter buttons are held 
    bool parameterRecordingActive = isAnyParameterButtonHeld(uiState);



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

      

        gateTimer2.tick();
        if (gateTimer2.isExpired() && GATE2)
        {
            GATE2 = false;
        }

       
    }


    static unsigned long lastLEDUpdate = 0;
    static unsigned long lastControlUpdate = 0;

    const unsigned long LED_UPDATE_INTERVAL = 10; // 30ms interval
    const unsigned long CONTROL_UPDATE_INTERVAL = 2; // 2ms interval
    uint16_t currtouched = touchSensor.touched();




//  Button, LIDAR, and AS5600 polling occur here slowed way down to 1000 times per second.
if ((currentMillis - lastControlUpdate >= CONTROL_UPDATE_INTERVAL)){

    
    lastControlUpdate = currentMillis;
    Matrix_scan();


    as5600Sensor.update();
    updateAS5600BaseValues(uiState);


    distanceSensor.update();
    int rawValue = distanceSensor.getRawValue();
    if (rawValue >= MIN_HEIGHT && rawValue <= MAX_HEIGHT)
    {
        mm = rawValue - MIN_HEIGHT;
    }
    else
    {
        mm = 0; // Invalid reading
    }


}


    // Check for voice switch trigger and handle immediate OLED update
    if (uiState.voiceSwitchTriggered) {
        uiState.voiceSwitchTriggered = false;  // Clear the flag
        display.onVoiceSwitched(uiState, voiceManager.get());
        //Serial.println("Voice switch OLED update triggered");
    }

    // Update LEDs only every 20ms
    if (currentMillis - lastLEDUpdate >= LED_UPDATE_INTERVAL) {
     lastLEDUpdate = currentMillis;
        updateStepLEDs(ledMatrix, seq1, seq2, seq3, seq4, uiState, mm);
        display.update(uiState, seq1, seq2, seq3, seq4, voiceManager.get());

        updateControlLEDs(ledMatrix, uiState);
        ledMatrix.show();
    }
        if (uiState.selectedStepForEdit != -1)
        {
            updateParametersForStep(uiState.selectedStepForEdit);
        }
    }

