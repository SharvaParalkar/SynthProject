#include <Arduino.h>
#include "Config.h"
#include "Hardware.h"
#include "AudioEngine.h"
#include "Sequencer.h"
#include "UI.h"

// Modules
Hardware hardware;
AudioEngine audioEngine;
Sequencer sequencer(audioEngine);
SynthUI ui(sequencer, audioEngine);

// Application State
Mode currentMode = MODE_LAUNCHPAD;

// buffers
int16_t audioBuffer[I2S_BUFFER_SIZE * 2]; // Stereo

void handleInput() {
    hardware.scanButtons();
    
    // Mode Switching
    if (hardware.isModeJustPressed()) {
        currentMode = (Mode)((currentMode + 1) % 3);
        audioEngine.killAll();
        // Force Display Update
        ui.draw(currentMode); // Immediate feedback (Issue #21/23)
        return;
    }
    
    // Octave / Function
    if (hardware.isOctaveJustPressed()) {
        if (currentMode == MODE_LAUNCHPAD) {
            int oct = sequencer.getCurrentOctave();
            oct = (oct + 1) % 7;
            if (oct == 0) oct = 2;
            sequencer.setCurrentOctave(oct);
        } else if (currentMode == MODE_SEQUENCER) {
            int trk = sequencer.getCurrentTrack();
            sequencer.setCurrentTrack((trk + 1) % 4);
        }
    }
    
    // Matrix Handling
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            if (hardware.isPadJustPressed(r, c)) {
                int padIndex = r * 4 + c;
                
                if (currentMode == MODE_LAUNCHPAD) {
                    // Play Note
                    int note = 36 + padIndex + (sequencer.getCurrentOctave() * 12);
                    // Use track instrument or default?
                    // Old code used 'currentInstrument'. 
                    // Let's use the instrument of the CURRENT TRACK.
                    Instrument inst = sequencer.getInstrument(sequencer.getCurrentTrack());
                    audioEngine.noteOn(note, inst);
                    
                } else if (currentMode == MODE_SEQUENCER) {
                    // Toggle Step
                    sequencer.toggleStep(sequencer.getCurrentTrack(), padIndex);
                    
                } else if (currentMode == MODE_SETTINGS) {
                    // Menu Navigation
                    // 0: Up, 1: Down, 3: Select (User Requested)
                    if (padIndex == 0) {
                        ui.menuCursor--;
                        if (ui.menuCursor < 0) {
                            ui.menuCursor = MENU_ITEM_COUNT - 1;
                            ui.menuScroll = max(0, ui.menuCursor - 2);
                        } else if (ui.menuCursor < ui.menuScroll) {
                            ui.menuScroll = ui.menuCursor;
                        }
                    } else if (padIndex == 1) { // Changed from 2 to 1 (User Request)
                        ui.menuCursor++;
                        if (ui.menuCursor >= MENU_ITEM_COUNT) {
                            ui.menuCursor = 0;
                            ui.menuScroll = 0;
                        } else if (ui.menuCursor >= ui.menuScroll + 3) {
                            ui.menuScroll = ui.menuCursor - 2;
                        }
                    } else if (padIndex == 2) { // Changed to 2 (User Request)
                        // Select
                        int item = ui.menuCursor;
                         if (item == MENU_INSTRUMENT) {
                            int trk = sequencer.getCurrentTrack();
                            Instrument inst = sequencer.getInstrument(trk);
                            inst = (Instrument)((inst + 1) % INST_COUNT);
                            sequencer.setInstrument(trk, inst);
                         } else if (item == MENU_BPM) {
                             int b = sequencer.getBPM();
                             b += 20;
                             if (b > 180) b = 60;
                             sequencer.setBPM(b);
                         } else if (item == MENU_PLAY_PAUSE) {
                             sequencer.togglePlay();
                         } else if (item == MENU_CLEAR_TRACK) {
                             sequencer.clearTrack(sequencer.getCurrentTrack());
                             // Fix: Reset cursor to top (User Request)
                             ui.menuCursor = 0;
                             ui.menuScroll = 0;
                         }
                    } 
                    // Moved Fine tune from 2 to 3
                     else if (padIndex == 3 && ui.menuCursor == MENU_BPM) {
                         sequencer.setBPM(max(60, sequencer.getBPM() - 5));
                     } else if (padIndex == 4 && ui.menuCursor == MENU_BPM) {
                         sequencer.setBPM(min(180, sequencer.getBPM() + 5));
                     }
                }
            }
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("Booting Modular Synth...");
    
    // Init Modules
    hardware.init();
    ui.init();
    audioEngine.init();
    sequencer.init();
    
    ui.draw(currentMode);
}

void loop() {
    // 1. Audio Generation (Highest Priority)
    // Fill buffer
    // BUFFER_SIZE (256) samples per channel. 
    // generate takes frames?
    // main.cpp: generateAudio(buffer, BUFFER_SIZE/2) -> 128 frames.
    // audioBuffer is int16_t * 2 * 256?
    // Old code: int16_t audioBuffer[BUFFER_SIZE]; generate(..., BUFFER_SIZE/2). 
    // And BUFFER_SIZE was 256. 
    // So 128 stereo frames.
    // Config.h: I2S_BUFFER_SIZE 256.
    
    memset(audioBuffer, 0, sizeof(audioBuffer));
    audioEngine.generate(audioBuffer, I2S_BUFFER_SIZE / 2);
    
    // 2. Output
    // Size in bytes: I2S_BUFFER_SIZE * 2 (stereo) * 2 (bytes) ? Or is BUFFER_SIZE total samples (L+R)?
    // Old code: i2s_write(..., BUFFER_SIZE * sizeof(int16_t)...)
    // So BUFFER_SIZE was total individual samples (L+R). 
    // Which means 128 frames.
    hardware.writeAudio(audioBuffer, I2S_BUFFER_SIZE * sizeof(int16_t));
    
    // 3. Inputs (Throttled)
    static int inputDiv = 0;
    if (++inputDiv >= 4) { // Every ~25ms
        handleInput();
        inputDiv = 0;
    }
    
    // 4. Logic
    sequencer.update();
    
    // 5. LEDs
    static int ledDiv = 0;
    if (++ledDiv >= 8) {
        if (currentMode == MODE_SEQUENCER && sequencer.isPlayingState()) {
            hardware.setStepLEDs(sequencer.getCurrentStep());
        } else {
            hardware.setGroupLEDs(sequencer.getCurrentTrack());
        }
        ledDiv = 0;
    }
    
    // 6. Display (Low Priority)
    static int dispDiv = 0;
    if (++dispDiv >= 30) { // Every ~200ms
        ui.draw(currentMode);
        dispDiv = 0;
    }
}
