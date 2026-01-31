#include <Arduino.h>
#include "Config.h"
#include "Hardware.h"
#include "AudioEngine.h"
#include "Sequencer.h"
#include "UI.h"

// =============================================================================
// AUDIO AT AUDIO RATE (AudioTools + Maximilian)
// =============================================================================
// play() is called once per stereo sample by maximilian.copy(), not once per
// loop(). Oscillators run at 32 kHz; I2S gives hardware-timed DAC output.
// Buttons/UI are handled in loop() - no GPIO in the audio callback.
// =============================================================================

// Modules
Hardware hardware;
AudioEngine audioEngine;
Sequencer sequencer(audioEngine);
SynthUI ui(sequencer, audioEngine, hardware);

// Application State
volatile Mode currentMode = MODE_LAUNCHPAD;

// =============================================================================
// INPUT HANDLING - Runs on Core 1
// =============================================================================
void handleInput() {
    hardware.scanButtons();
    
    // Cooldown timers for function keys
    static uint32_t lastModePress = 0;
    static uint32_t lastOctavePress = 0;
    const uint32_t FUNCTION_KEY_COOLDOWN_MS = 200;  // 200ms cooldown for function keys
    uint32_t now = millis();
    
    // Mode Switching with cooldown
    if (hardware.isModeJustPressed() && (now - lastModePress >= FUNCTION_KEY_COOLDOWN_MS)) {
        currentMode = (Mode)((currentMode + 1) % 4); // 4 modes now
        audioEngine.killAll();
        // Force Display Update
        ui.draw(currentMode); // Immediate feedback (Issue #21/23)
        lastModePress = now;
        return;
    }
    
    // Octave / Function with cooldown
    if (hardware.isOctaveJustPressed() && (now - lastOctavePress >= FUNCTION_KEY_COOLDOWN_MS)) {
        if (currentMode == MODE_LAUNCHPAD) {
            int oct = sequencer.getCurrentOctave();
            oct = (oct + 1) % 7;
            if (oct == 0) oct = 2;
            sequencer.setCurrentOctave(oct);
        } else if (currentMode == MODE_SEQUENCER) {
            int trk = sequencer.getCurrentTrack();
            sequencer.setCurrentTrack((trk + 1) % 4);
        }
        lastOctavePress = now;
    }
    
    // Matrix Handling
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            // Check for Press
            if (hardware.isPadJustPressed(r, c)) {
                int padIndex = r * 4 + c;
                
                if (currentMode == MODE_LAUNCHPAD) {
                    // Play Note
                    int note = 36 + padIndex + (sequencer.getCurrentOctave() * 12);
                    Instrument inst = sequencer.getInstrument(sequencer.getCurrentTrack());
                    audioEngine.noteOn(note, inst);
                    
                } else if (currentMode == MODE_SEQUENCER) {
                    // Toggle Step
                    sequencer.toggleStep(sequencer.getCurrentTrack(), padIndex);
                    
                } else if (currentMode == MODE_SETTINGS) {
                    // Menu Navigation with Cooldown to prevent rapid-fire
                    static uint32_t lastMenuAction = 0;
                    const uint32_t MENU_COOLDOWN_MS = 150;  // 150ms between menu actions
                    uint32_t now = millis();
                    
                    // 0: Up, 1: Down, 3: Select
                    if (padIndex == 0 && (now - lastMenuAction >= MENU_COOLDOWN_MS)) {
                        ui.menuCursor--;
                        if (ui.menuCursor < 0) {
                            ui.menuCursor = MENU_ITEM_COUNT - 1;
                            ui.menuScroll = max(0, ui.menuCursor - 3);
                        } else if (ui.menuCursor < ui.menuScroll) {
                            ui.menuScroll = ui.menuCursor;
                        }
                        lastMenuAction = now;
                    } else if (padIndex == 1 && (now - lastMenuAction >= MENU_COOLDOWN_MS)) { 
                        ui.menuCursor++;
                        if (ui.menuCursor >= MENU_ITEM_COUNT) {
                            ui.menuCursor = 0;
                            ui.menuScroll = 0;
                        } else if (ui.menuCursor >= ui.menuScroll + 4) {
                            ui.menuScroll = ui.menuCursor - 3;
                        }
                        lastMenuAction = now;
                    } else if (padIndex == 2 && (now - lastMenuAction >= MENU_COOLDOWN_MS)) { 
                        // Select / Action
                        int item = ui.menuCursor;
                        if (item == MENU_INSTRUMENT) {
                            int trk = sequencer.getCurrentTrack();
                            Instrument inst = sequencer.getInstrument(trk);
                            inst = (Instrument)((inst + 1) % INST_COUNT);
                            sequencer.setInstrument(trk, inst);
                        } else if (item == MENU_BPM) {
                             int b = sequencer.getBPM();
                             b += 20; if (b > 180) b = 60;
                             sequencer.setBPM(b);
                        } else if (item == MENU_PLAY_PAUSE) {
                             sequencer.togglePlay();
                        } else if (item == MENU_CLEAR_TRACK) {
                             sequencer.clearTrack(sequencer.getCurrentTrack());
                             ui.menuCursor = 0;
                             ui.menuScroll = 0;
                        } else if (item == MENU_VOLUME) {
                             // Cycle + 20
                             int v = audioEngine.getVolume() + 20;
                             if (v > 100) v = 0;
                             audioEngine.setVolume(v);
                         } else if (item == MENU_BRIGHTNESS) {
                             // Cycle + 50ish (20%)
                             int b = hardware.getBrightness();
                             b += 51; // 20% of 255
                             if (b > 255) b = 0;
                             hardware.setBrightness(b);
                         }
                         lastMenuAction = now;
                    } 
                    
                    // Fine Adjustments (Pad 3: -, Pad 4: +) with cooldown
                    const uint32_t FINE_ADJUST_COOLDOWN_MS = 100;
                    if (padIndex == 3 && (now - lastMenuAction >= FINE_ADJUST_COOLDOWN_MS)) { // Decrease
                        if (ui.menuCursor == MENU_BPM) sequencer.setBPM(max(60, sequencer.getBPM() - 5));
                        else if (ui.menuCursor == MENU_VOLUME) audioEngine.setVolume(audioEngine.getVolume() - 5);
                        else if (ui.menuCursor == MENU_BRIGHTNESS) hardware.setBrightness(hardware.getBrightness() - 13); // ~5%
                        lastMenuAction = now;
                    } else if (padIndex == 4 && (now - lastMenuAction >= FINE_ADJUST_COOLDOWN_MS)) { // Increase
                        if (ui.menuCursor == MENU_BPM) sequencer.setBPM(min(180, sequencer.getBPM() + 5));
                        else if (ui.menuCursor == MENU_VOLUME) audioEngine.setVolume(audioEngine.getVolume() + 5);
                        else if (ui.menuCursor == MENU_BRIGHTNESS) hardware.setBrightness(hardware.getBrightness() + 13);
                        lastMenuAction = now;
                    }
                    
                } else if (currentMode == MODE_NOTE_EDITOR) {
                    // Note Editor Menu Navigation with Cooldown
                    static uint32_t lastNoteMenuAction = 0;
                    const uint32_t NOTE_MENU_COOLDOWN_MS = 150;
                    uint32_t now = millis();
                    
                    // 0: Up, 1: Down, 2: Select
                    if (padIndex == 0 && (now - lastNoteMenuAction >= NOTE_MENU_COOLDOWN_MS)) {
                        ui.noteMenuCursor--;
                        if (ui.noteMenuCursor < 0) {
                            ui.noteMenuCursor = NOTE_MENU_ITEM_COUNT - 1;
                            ui.noteMenuScroll = max(0, ui.noteMenuCursor - 3);
                        } else if (ui.noteMenuCursor < ui.noteMenuScroll) {
                            ui.noteMenuScroll = ui.noteMenuCursor;
                        }
                        lastNoteMenuAction = now;
                    } else if (padIndex == 1 && (now - lastNoteMenuAction >= NOTE_MENU_COOLDOWN_MS)) { 
                        ui.noteMenuCursor++;
                        if (ui.noteMenuCursor >= NOTE_MENU_ITEM_COUNT) {
                            ui.noteMenuCursor = 0;
                            ui.noteMenuScroll = 0;
                        } else if (ui.noteMenuCursor >= ui.noteMenuScroll + 4) {
                            ui.noteMenuScroll = ui.noteMenuCursor - 3;
                        }
                        lastNoteMenuAction = now;
                    } else if (padIndex == 2 && (now - lastNoteMenuAction >= NOTE_MENU_COOLDOWN_MS)) { 
                        // Select / Action
                        int item = ui.noteMenuCursor;
                        if (item == NOTE_MENU_SWING) {
                            // Cycle through preset values: 0, 25, 50, 75, 100
                            int s = sequencer.getSwing();
                            if (s == 0) s = 25;
                            else if (s == 25) s = 50;
                            else if (s == 50) s = 75;
                            else if (s == 75) s = 100;
                            else s = 0;
                            sequencer.setSwing(s);
                        } else if (item == NOTE_MENU_GATE) {
                            // Cycle through preset values: 0.25, 0.5, 0.75, 1.0
                            float g = sequencer.getGate();
                            if (g < 0.3f) g = 0.5f;
                            else if (g < 0.6f) g = 0.75f;
                            else if (g < 0.9f) g = 1.0f;
                            else g = 0.25f;
                            sequencer.setGate(g);
                        } else if (item == NOTE_MENU_FILTER) {
                            // Cycle through preset values: 0.25, 0.5, 0.75, 1.0
                            float f = audioEngine.getFilterCutoff();
                            if (f < 0.3f) f = 0.5f;
                            else if (f < 0.6f) f = 0.75f;
                            else if (f < 0.9f) f = 1.0f;
                            else f = 0.25f;
                            audioEngine.setFilterCutoff(f);
                        }
                        lastNoteMenuAction = now;
                    }
                    
                    // Fine Adjustments (Pad 3: -, Pad 4: +) with cooldown
                    const uint32_t NOTE_FINE_ADJUST_COOLDOWN_MS = 100;
                    if (padIndex == 3 && (now - lastNoteMenuAction >= NOTE_FINE_ADJUST_COOLDOWN_MS)) { // Decrease
                        if (ui.noteMenuCursor == NOTE_MENU_SWING) sequencer.setSwing(max(0, sequencer.getSwing() - 5));
                        else if (ui.noteMenuCursor == NOTE_MENU_GATE) sequencer.setGate(max(0.0f, sequencer.getGate() - 0.05f));
                        else if (ui.noteMenuCursor == NOTE_MENU_FILTER) audioEngine.setFilterCutoff(max(0.0f, audioEngine.getFilterCutoff() - 0.05f));
                        lastNoteMenuAction = now;
                    } else if (padIndex == 4 && (now - lastNoteMenuAction >= NOTE_FINE_ADJUST_COOLDOWN_MS)) { // Increase
                        if (ui.noteMenuCursor == NOTE_MENU_SWING) sequencer.setSwing(min(100, sequencer.getSwing() + 5));
                        else if (ui.noteMenuCursor == NOTE_MENU_GATE) sequencer.setGate(min(1.0f, sequencer.getGate() + 0.05f));
                        else if (ui.noteMenuCursor == NOTE_MENU_FILTER) audioEngine.setFilterCutoff(min(1.0f, audioEngine.getFilterCutoff() + 0.05f));
                        lastNoteMenuAction = now;
                    }
                }
            } // End Press Check

            // Check for Release
            if (hardware.isPadJustReleased(r, c)) {
                if (currentMode == MODE_LAUNCHPAD) {
                    int padIndex = r * 4 + c;
                    int note = 36 + padIndex + (sequencer.getCurrentOctave() * 12);
                    audioEngine.noteOff(note); // Stop note
                }
            }
        }
    }
}

// =============================================================================
// AUDIO TASK - Runs on Core 0 (dedicated audio core)
// =============================================================================
TaskHandle_t audioTaskHandle = NULL;

void audioTask(void* parameter) {
    Serial.println("[Audio Task] Running on Core 0");
    
    while (true) {
        // Continuously call audio engine - this is the ONLY thing on Core 0
        audioEngine.copy();
        
        // Yield to allow watchdog and other critical tasks
        taskYIELD();
    }
}

// =============================================================================
// SETUP - Runs on Core 1
// =============================================================================
void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("Booting Modular Synth...");
    Serial.println("[Main] AudioTools + Maximilian (dual-core)");
    
    hardware.init();
    ui.init();
    audioEngine.init();
    sequencer.init();
    
    ui.draw(currentMode);
    
    // Create audio task on Core 0 (highest priority)
    xTaskCreatePinnedToCore(
        audioTask,           // Task function
        "AudioTask",         // Name
        8192,                // Stack size
        NULL,                // Parameters
        configMAX_PRIORITIES - 1,  // Highest priority
        &audioTaskHandle,    // Task handle
        0                    // Core 0 (audio dedicated core)
    );
    
    Serial.println("[Main] Audio on Core 0, UI on Core 1");
}

// =============================================================================
// MAIN LOOP - UI/Input on Core 1 (does not interfere with audio)
// =============================================================================
void loop() {
    // Core 1: All UI, input, sequencer, LEDs
    handleInput();
    
    static uint32_t lastBgTask = 0;
    uint32_t now = millis();
    if (now - lastBgTask >= 20) {  // 50 Hz
        sequencer.update();
        
        if (currentMode == MODE_SEQUENCER && sequencer.isPlayingState()) {
            hardware.setStepLEDs(sequencer.getCurrentStep());
        } else {
            hardware.setGroupLEDs(sequencer.getCurrentTrack());
        }
        
        lastBgTask = now;
    }
    
    // Display updates at 20 Hz
    static uint32_t lastDisplay = 0;
    if (now - lastDisplay >= 50) {
        ui.draw(currentMode);
        lastDisplay = now;
    }
    
    delay(1);  // OK on Core 1 (UI) - Core 0 (audio) is independent
}
