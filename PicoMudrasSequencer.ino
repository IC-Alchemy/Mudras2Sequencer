#include "includes.h"
#include "diagnostic.h"
#include "src/voice/Voice.h"

// =======================
//   GLOBAL VARIABLES
// =======================
UIState uiState; // Central state object for the UI
Sequencer seq1(1); // Channel 1 for first sequencer
Sequencer seq2(2); // Channel 2 for second sequencer
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

// Global effects and delay (shared between voices)
daisysp::Oscillator lfo1, lfo2;
daisysp::Svf delLowPass;
daisysp::DelayLine<float, MAX_DELAY_SAMPLES> del1;
float feedbackGain1 = 0.65f;
float currentDelayOutputGain = 0.0f; // For smooth delay output fade
OLEDDisplay display;
float currentFeedbackGain = 0.0f; // For smooth delay feedback fade

// Legacy voice states for compatibility
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
float OSC_DETUNE_FACTOR = .001f;
bool resetStepsLightsFlag = true;
float delayTarget = 48000.0f * .15f;
float currentDelay = 48000.0f * .15f;
float feedbackAmmount = 0.45f; // Safer initial feedback level
const float FEEDBACK_FADE_RATE = 0.001f; // Faster fade to prevent feedback buildup


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

// --- Voice System Initialization ---
void initOscillators()
{
    // Initialize global effects
    delLowPass.Init(SAMPLE_RATE);
    delLowPass.SetFreq(1340.f); //  delay Lowpass filter
    delLowPass.SetRes(0.19f);
    delLowPass.SetDrive(.9f);

    lfo1.Init(SAMPLE_RATE);
    lfo1.SetWaveform(daisysp::Oscillator::WAVE_TRI);
    lfo1.SetFreq(0.5f);
    lfo1.SetAmp(0.5f);
    
    lfo2.Init(SAMPLE_RATE);
    lfo2.SetWaveform(daisysp::Oscillator::WAVE_TRI);
    lfo2.SetFreq(0.5f);
    lfo2.SetAmp(0.5f);

    del1.Init();
    del1.Reset(); // Clear any garbage in delay buffer
    const float delayMs1 = 500.f;
    size_t delaySamples1 = (size_t)(delayMs1 * SAMPLE_RATE * 0.001f);
    del1.SetDelay(delaySamples1);
    
    // Initialize delay target to match initial delay
    delayTarget = static_cast<float>(delaySamples1);

    // Initialize Voice Manager
    voiceManager = std::make_unique<VoiceManager>(SAMPLE_RATE);
    
    // Create Lead Voice (Voice 1 replacement)
    leadVoiceId = voiceManager->addVoice(VoicePresets::getAnalogVoice());
    
    // Create Bass Voice (Voice 2 replacement)
    bassVoiceId = voiceManager->addVoice(VoicePresets::getDigitalVoice());
    
    // Attach sequencers to voices
    voiceManager->attachSequencer(leadVoiceId, &seq1);
    voiceManager->attachSequencer(bassVoiceId, &seq2);
    
    Serial.println("Voice system initialized with Lead and Bass voices");
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

    // Update voice parameters using the new voice system
    if (!updateGate || (gate && *gate))
    {
        uint8_t voiceId = isVoice2 ? bassVoiceId : leadVoiceId;
        
        // Calculate base frequency for the voice
        int noteIndex = state.note;
        float baseFreq = daisysp::mtof(scale[currentScale][noteIndex] + 48 + state.octave);
        
        // Update voice frequency through VoiceManager
        voiceManager->setVoiceFrequency(voiceId, baseFreq);
        
        // Handle slide/portamento
        voiceManager->setVoiceSlide(voiceId, state.slide);
    }

    // Update voice parameters through VoiceManager
    uint8_t voiceId = isVoice2 ? bassVoiceId : leadVoiceId;
    voiceManager->updateVoiceState(voiceId, state);

    // Send MIDI CC messages for parameter changes
    uint8_t midiVoiceId = isVoice2 ? 1 : 0;
    midiNoteManager.updateParameterCC(midiVoiceId, ParamId::Filter, state.filter);
    midiNoteManager.updateParameterCC(midiVoiceId, ParamId::Attack, state.attack);
    midiNoteManager.updateParameterCC(midiVoiceId, ParamId::Decay, state.decay);
    midiNoteManager.updateParameterCC(midiVoiceId, ParamId::Octave, state.octave);
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
    float targetDelayOutputGain = uiState.delayOn ? 1.0f : 0.0f;
    float targetFeedbackGain = uiState.delayOn ? feedbackAmmount : 0.0f;
    
    // Smooth parameters once per buffer to reduce CPU load
    currentFeedbackGain = delayTimeSmoothing(currentFeedbackGain, targetFeedbackGain, FEEDBACK_FADE_RATE);
    currentDelayOutputGain = delayTimeSmoothing(currentDelayOutputGain, targetDelayOutputGain, FEEDBACK_FADE_RATE);
    currentDelay = delayTimeSmoothing(currentDelay, delayTarget, slew);
    
    // Set delay time once per buffer
    del1.SetDelay(currentDelay);
    
    for (int i = 0; i < N; ++i)
    {
        lfo1Output = lfo1.Process();
        lfo2Output = lfo2.Process();
        
        // Sample every 64 samples to reduce overhead while maintaining visual accuracy
        if (i % 64 == 0)
        {
            lfo1LEDWaveformValue = lfo1Output;
            lfo2LEDWaveformValue = lfo2Output;
        }

        // Process all voices efficiently (voice states are updated by sequencer callbacks)
        finalvoice = voiceManager->processAllVoices();

        // Read from delay line
        delout = del1.Read();
        
        // Calculate feedback signal with proper gain staging
        float feedbackSignal = delout * currentFeedbackGain;
        
        // Apply low-pass filtering to feedback to prevent harsh artifacts
        delLowPass.Process(feedbackSignal);
        float filteredFeedback = delLowPass.Low();
        
        // Write to delay line: dry input + filtered feedback
        // Clamp feedback to prevent runaway
        filteredFeedback = std::max(-1.0f, std::min(filteredFeedback, .9f));
        del1.Write(finalvoice + filteredFeedback);
        
        // Mix dry and wet signals
        output = finalvoice + (delout * currentDelayOutputGain);
        
     

        // Output to stereo channels
        out[2 * i + 0] = convertSampleToInt16(output*.75f);
        out[2 * i + 1] = convertSampleToInt16(output*.75f);
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
         touchSensor.setThresholds(55, 33); // touch, release thresholds
        // Serial.println("MPR121 thresholds configured to 155/55");
    }

    initUIEventHandler(uiState);

    // Initialize OLED display
    display.begin();
    Serial.println("OLED display initialized");

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
   
    usb_midi.read();
    // Trigger immediate long-press resets for Randomize buttons
    pollUIHeldButtons(uiState, seq1, seq2);

    unsigned long currentMillis = millis();

    // Check if any parameter buttons are held for prioritized sensor updates
    bool parameterRecordingActive = isAnyParameterButtonHeld(uiState);

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


    static unsigned long lastLEDUpdate = 0;
    const unsigned long LED_UPDATE_INTERVAL = 2; // 20ms interval

    uint16_t currtouched = touchSensor.touched();
    
    Matrix_scan();

    // Update LEDs only every 20ms
    if (currentMillis - lastLEDUpdate >= LED_UPDATE_INTERVAL) {
        lastLEDUpdate = currentMillis;
        
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

        updateStepLEDs(ledMatrix, seq1, seq2, uiState, mm);
        display.update(uiState, seq1, seq2);
        // LFO LED pulse states are updated directly in fill_audio_buffer()

        updateControlLEDs(ledMatrix, uiState);  
        ledMatrix.show();
    }
        if (uiState.selectedStepForEdit != -1)
        {
            updateParametersForStep(uiState.selectedStepForEdit);
        }
    }

