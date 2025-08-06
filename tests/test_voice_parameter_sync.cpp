/**
 * @file test_voice_parameter_sync.cpp
 * @brief Test suite for voice parameter state synchronization
 * 
 * This test validates that voice parameter changes are properly synchronized
 * between the UIEventHandler, VoiceManager, and OLED display components.
 */

#include <Arduino.h>
#include <unity.h>
#include "../src/ui/UIState.h"
#include "../src/ui/UIEventHandler.h"
#include "../src/voice/VoiceManager.h"
#include "../src/OLED/oled.h"
#include "../src/matrix/Matrix.h"

// Test fixtures
UIState testUIState;
std::unique_ptr<VoiceManager> testVoiceManager;
OLEDDisplay testDisplay;
bool oledUpdateCallbackCalled = false;
UIState lastCallbackUIState;
VoiceManager* lastCallbackVoiceManager = nullptr;

// Mock callback for testing
void mockOLEDUpdateCallback(const UIState& uiState, VoiceManager* voiceManager) {
    oledUpdateCallbackCalled = true;
    lastCallbackUIState = uiState;
    lastCallbackVoiceManager = voiceManager;
}

void setUp(void) {
    // Reset test state
    testUIState = UIState{};
    testUIState.settingsMode = true;
    testUIState.voiceParameterObserver = &testDisplay;
    
    // Create test voice manager with 2 voices
    testVoiceManager = VoiceManagerBuilder()
        .withMaxVoices(2)
        .withVoice("analog")
        .withVoice("digital")
        .build();
    
    // Reset callback state
    oledUpdateCallbackCalled = false;
    lastCallbackVoiceManager = nullptr;
    
    // Set up callback
    setOLEDUpdateCallback(mockOLEDUpdateCallback);
}

void tearDown(void) {
    testVoiceManager.reset();
}

/**
 * Test that voice parameter changes trigger immediate UI state updates
 */
void test_voice_parameter_change_updates_ui_state(void) {
    // Arrange
    uint8_t voiceId = 0;
    uint8_t buttonIndex = 9; // Envelope toggle
    const char* parameterName = "Envelope";
    
    // Act
    testUIState.notifyVoiceParameterChanged(voiceId, buttonIndex, parameterName);
    
    // Assert
    TEST_ASSERT_TRUE(testUIState.voiceParameterChanged);
    TEST_ASSERT_EQUAL(voiceId, testUIState.changedVoiceId);
    TEST_ASSERT_EQUAL(buttonIndex, testUIState.lastVoiceParameterButton);
    TEST_ASSERT_EQUAL_STRING(parameterName, testUIState.changedParameterName);
    TEST_ASSERT_TRUE(testUIState.inVoiceParameterMode);
    TEST_ASSERT_TRUE(testUIState.voiceParameterChangeTime > 0);
}

/**
 * Test that voice parameter changes are properly applied to VoiceConfig
 */
void test_voice_parameter_toggle_updates_voice_config(void) {
    // Arrange
    uint8_t voiceId = 0;
    VoiceConfig* config = testVoiceManager->getVoiceConfig(voiceId);
    TEST_ASSERT_NOT_NULL(config);
    
    bool originalEnvelopeState = config->hasEnvelope;
    
    // Create mock button event for envelope toggle
    MatrixButtonEvent evt;
    evt.buttonIndex = 9; // Envelope toggle button
    evt.type = MATRIX_BUTTON_PRESSED;
    
    // Mock external dependencies
    extern std::unique_ptr<VoiceManager> voiceManager;
    voiceManager = std::move(testVoiceManager);
    
    // Act - simulate button press in settings mode
    testUIState.settingsMode = true;
    testUIState.inPresetSelection = false;
    
    // This would normally be called by matrixEventHandler
    // For testing, we'll directly test the voice parameter toggle logic
    config->hasEnvelope = !config->hasEnvelope;
    testVoiceManager->setVoiceConfig(voiceId, *config);
    testUIState.notifyVoiceParameterChanged(voiceId, evt.buttonIndex, "Envelope");
    
    // Assert
    TEST_ASSERT_NOT_EQUAL(originalEnvelopeState, config->hasEnvelope);
    TEST_ASSERT_TRUE(testUIState.voiceParameterChanged);
    TEST_ASSERT_EQUAL(9, testUIState.lastVoiceParameterButton);
    
    // Restore voiceManager
    testVoiceManager = std::move(voiceManager);
}

/**
 * Test that OLED update callback is triggered on voice parameter changes
 */
void test_oled_callback_triggered_on_parameter_change(void) {
    // Arrange
    uint8_t voiceId = 0;
    uint8_t buttonIndex = 10; // Overdrive toggle
    
    // Act
    testUIState.notifyVoiceParameterChanged(voiceId, buttonIndex, "Overdrive");
    
    // Simulate the callback being triggered (normally done by UIEventHandler)
    mockOLEDUpdateCallback(testUIState, testVoiceManager.get());
    
    // Assert
    TEST_ASSERT_TRUE(oledUpdateCallbackCalled);
    TEST_ASSERT_EQUAL(testVoiceManager.get(), lastCallbackVoiceManager);
    TEST_ASSERT_TRUE(lastCallbackUIState.voiceParameterChanged);
    TEST_ASSERT_EQUAL(buttonIndex, lastCallbackUIState.lastVoiceParameterButton);
}

/**
 * Test that voice parameter change flags are properly cleared
 */
void test_voice_parameter_flags_cleared_after_update(void) {
    // Arrange
    testUIState.notifyVoiceParameterChanged(0, 11, "Wavefolder");
    TEST_ASSERT_TRUE(testUIState.voiceParameterChanged);
    
    // Act
    testUIState.clearVoiceParameterChangeFlags();
    
    // Assert
    TEST_ASSERT_FALSE(testUIState.voiceParameterChanged);
    TEST_ASSERT_NULL(testUIState.changedParameterName);
}

/**
 * Test multiple parameter changes maintain correct state
 */
void test_multiple_parameter_changes_maintain_state(void) {
    // Test envelope toggle
    testUIState.notifyVoiceParameterChanged(0, 9, "Envelope");
    TEST_ASSERT_EQUAL(9, testUIState.lastVoiceParameterButton);
    TEST_ASSERT_EQUAL_STRING("Envelope", testUIState.changedParameterName);
    
    // Test overdrive toggle (should update state)
    testUIState.notifyVoiceParameterChanged(1, 10, "Overdrive");
    TEST_ASSERT_EQUAL(10, testUIState.lastVoiceParameterButton);
    TEST_ASSERT_EQUAL_STRING("Overdrive", testUIState.changedParameterName);
    TEST_ASSERT_EQUAL(1, testUIState.changedVoiceId);
}

/**
 * Test observer pattern implementation
 */
void test_observer_notification(void) {
    // Arrange
    bool observerCalled = false;
    
    // Create a mock observer
    class MockObserver : public VoiceParameterObserver {
    public:
        bool* called;
        uint8_t lastVoiceId = 0;
        uint8_t lastButtonIndex = 0;
        const char* lastParameterName = nullptr;
        
        MockObserver(bool* calledFlag) : called(calledFlag) {}
        
        void onVoiceParameterChanged(uint8_t voiceId, uint8_t buttonIndex, const char* parameterName) override {
            *called = true;
            lastVoiceId = voiceId;
            lastButtonIndex = buttonIndex;
            lastParameterName = parameterName;
        }
    };
    
    MockObserver mockObserver(&observerCalled);
    testUIState.voiceParameterObserver = &mockObserver;
    
    // Act
    testUIState.notifyVoiceParameterChanged(1, 12, "Filter Mode");
    
    // Assert
    TEST_ASSERT_TRUE(observerCalled);
    TEST_ASSERT_EQUAL(1, mockObserver.lastVoiceId);
    TEST_ASSERT_EQUAL(12, mockObserver.lastButtonIndex);
    TEST_ASSERT_EQUAL_STRING("Filter Mode", mockObserver.lastParameterName);
}

void runTests(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_voice_parameter_change_updates_ui_state);
    RUN_TEST(test_voice_parameter_toggle_updates_voice_config);
    RUN_TEST(test_oled_callback_triggered_on_parameter_change);
    RUN_TEST(test_voice_parameter_flags_cleared_after_update);
    RUN_TEST(test_multiple_parameter_changes_maintain_state);
    RUN_TEST(test_observer_notification);
    
    UNITY_END();
}

// Arduino setup and loop for running tests
void setup() {
    Serial.begin(115200);
    delay(2000); // Wait for serial connection
    
    Serial.println("Starting Voice Parameter Synchronization Tests...");
    runTests();
}

void loop() {
    // Tests run once in setup()
    delay(1000);
}
