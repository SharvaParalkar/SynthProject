#include "UI.h"
#include <stdio.h>
#include <Wire.h>

UI::UI() : display(U8G2_R0, U8X8_PIN_NONE, U8X8_PIN_NONE, U8X8_PIN_NONE) {
    currentMode = MODE_PLAY;
    lastModeChange = 0;
    lastDisplayUpdate = 0;
    selectedPattern = 0;
    selectedStep = 0;
    editingStep = false;
    selectedSample = 0;
    settingIndex = 0;
    adjustingValue = false;
    buttonMatrix = nullptr;
    sequencer = nullptr;
    effects = nullptr;
    sampleEngine = nullptr;
}

void UI::begin() {
    // Initialize I2C
    Wire.begin(I2C_SDA, I2C_SCL);
    
    // Initialize display
    display.begin();
    display.setFont(u8g2_font_6x10_tf);
    display.setFontRefHeightExtendedText();
    display.setDrawColor(1);
    display.setFontPosTop();
    display.setFontDirection(0);
    
    // Initialize LEDs
    for (int i = 0; i < 4; i++) {
        pinMode(LED_PINS[i], OUTPUT);
        digitalWrite(LED_PINS[i], LOW);
    }
    
    Serial.println("UI initialized");
}

void UI::setMode(Mode mode) {
    if (mode != currentMode) {
        currentMode = mode;
        lastModeChange = millis();
        Serial.printf("Mode changed to: %d\n", mode);
    }
}

void UI::update() {
    if (!buttonMatrix) return;
    
    buttonMatrix->update();
    handleInput();
    
    // Update display periodically
    unsigned long now = millis();
    if (now - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
        updateDisplay();
        updateLEDs();
        lastDisplayUpdate = now;
    }
}

void UI::handleInput() {
    // Mode switching
    if (buttonMatrix->wasModePressed()) {
        Mode nextMode = (Mode)((currentMode + 1) % 3);
        setMode(nextMode);
        return;
    }
    
    // Mode-specific input handling
    switch (currentMode) {
        case MODE_PLAY:
            handlePlayMode();
            break;
        case MODE_PATTERN:
            handlePatternMode();
            break;
        case MODE_SETTINGS:
            handleSettingsMode();
            break;
    }
}

void UI::handlePlayMode() {
    if (!sequencer || !sampleEngine) return;
    
    // OCTAVE button = start/stop sequencer
    if (buttonMatrix->wasOctavePressed()) {
        if (sequencer->isPlaying()) {
            sequencer->stop();
        } else {
            sequencer->start();
        }
    }
    
    // Check for button presses (0-15)
    for (int i = 0; i < 16; i++) {
        if (buttonMatrix->wasPressed(i)) {
            // Trigger sample directly
            sampleEngine->triggerSample(i, 0.0f);
        }
    }
    
    // Hold OCTAVE + button = adjust pitch (when sequencer not playing)
    if (buttonMatrix->isOctavePressed() && !sequencer->isPlaying()) {
        for (int i = 0; i < 16; i++) {
            if (buttonMatrix->wasPressed(i)) {
                // Could implement pitch adjustment here
            }
        }
    }
}

void UI::handlePatternMode() {
    if (!sequencer) return;
    
    // Button 0-15 = select step or toggle step
    for (int i = 0; i < 16; i++) {
        if (buttonMatrix->wasPressed(i)) {
            if (editingStep && selectedStep == i) {
                // Toggle step
                sequencer->toggleStep(selectedPattern, i);
            } else {
                // Select step
                selectedStep = i;
                editingStep = true;
            }
        }
    }
    
    // OCTAVE + button = select sample for step
    if (buttonMatrix->isOctavePressed() && editingStep) {
        for (int i = 0; i < 16; i++) {
            if (buttonMatrix->wasPressed(i)) {
                sequencer->setStep(selectedPattern, selectedStep, i, 0.0f);
            }
        }
    }
    
    // MODE + button = select pattern
    if (buttonMatrix->isModePressed()) {
        for (int i = 0; i < 4; i++) {
            if (buttonMatrix->wasPressed(i)) {
                selectedPattern = i;
                sequencer->setCurrentPattern(i);
            }
        }
    }
}

void UI::handleSettingsMode() {
    if (!sequencer || !effects) return;
    
    // Button 0-2 = select setting (BPM, BitCrusher, Filter)
    for (int i = 0; i < 3; i++) {
        if (buttonMatrix->wasPressed(i)) {
            settingIndex = i;
            adjustingValue = true;
        }
    }
    
    // Buttons 4-15 = adjust value
    if (adjustingValue) {
        for (int i = 4; i < 16; i++) {
            if (buttonMatrix->wasPressed(i)) {
                int value = i - 4; // 0-11
                
                switch (settingIndex) {
                    case 0: // BPM
                        sequencer->setBPM(MIN_BPM + (value * 10)); // 60-170 in steps of 10
                        break;
                    case 1: // BitCrusher bits
                        effects->setBitCrusherBits(4 + value); // 4-15 bits
                        effects->enableBitCrusher(value > 0);
                        break;
                    case 2: // Filter cutoff (rough)
                        {
                            float cutoff = FILTER_CUTOFF_MIN + (value * (FILTER_CUTOFF_MAX - FILTER_CUTOFF_MIN) / 12.0f);
                            effects->setFilterCutoff(cutoff);
                            effects->enableFilter(value > 0);
                        }
                        break;
                }
            }
        }
    }
}

void UI::updateLEDs() {
    if (!sequencer) return;
    
    // LED 0-3 show current step (binary)
    uint8_t currentStep = sequencer->getCurrentStep();
    for (int i = 0; i < 4; i++) {
        digitalWrite(LED_PINS[i], (currentStep >> i) & 1);
    }
}

void UI::updateDisplay() {
    display.clearBuffer();
    
    switch (currentMode) {
        case MODE_PLAY:
            drawPlayScreen();
            break;
        case MODE_PATTERN:
            drawPatternScreen();
            break;
        case MODE_SETTINGS:
            drawSettingsScreen();
            break;
    }
    
    display.sendBuffer();
}

void UI::drawPlayScreen() {
    display.setFont(u8g2_font_6x10_tf);
    display.drawStr(0, 0, "PLAY MODE");
    
    if (sequencer) {
        char buf[32];
        snprintf(buf, sizeof(buf), "BPM: %d", sequencer->getBPM());
        display.drawStr(0, 12, buf);
        
        snprintf(buf, sizeof(buf), "Pattern: %d", sequencer->getCurrentPattern());
        display.drawStr(0, 24, buf);
        
        if (sequencer->isPlaying()) {
            display.drawStr(0, 36, "PLAYING");
        } else {
            display.drawStr(0, 36, "STOPPED");
        }
    }
}

void UI::drawPatternScreen() {
    display.setFont(u8g2_font_6x10_tf);
    display.drawStr(0, 0, "PATTERN MODE");
    
    if (sequencer) {
        char buf[32];
        snprintf(buf, sizeof(buf), "P:%d S:%d", selectedPattern, selectedStep);
        display.drawStr(0, 12, buf);
        
        // Draw step grid (4x4 = 16 steps)
        Pattern* pattern = sequencer->getPattern(selectedPattern);
        if (pattern) {
            int cellW = 8;
            int cellH = 8;
            int startX = 0;
            int startY = 20;
            
            for (int s = 0; s < 16; s++) {
                int x = startX + (s % 4) * cellW;
                int y = startY + (s / 4) * cellH;
                
                if (pattern->steps[s].active) {
                    display.drawBox(x, y, cellW - 1, cellH - 1);
                } else {
                    display.drawFrame(x, y, cellW - 1, cellH - 1);
                }
                
                // Highlight current step
                if (s == sequencer->getCurrentStep()) {
                    display.drawFrame(x - 1, y - 1, cellW + 1, cellH + 1);
                }
            }
        }
    }
}

void UI::drawSettingsScreen() {
    display.setFont(u8g2_font_6x10_tf);
    display.drawStr(0, 0, "SETTINGS");
    
    if (!sequencer || !effects) return;
    
    char buf[64];
    
    // BPM
    snprintf(buf, sizeof(buf), "BPM: %d", sequencer->getBPM());
    display.drawStr(0, 12, buf);
    
    // Bit Crusher
    snprintf(buf, sizeof(buf), "Bits: %d %s", 
             effects->getBitCrusherBits(),
             effects->isBitCrusherEnabled() ? "ON" : "OFF");
    display.drawStr(0, 24, buf);
    
    // Filter
    snprintf(buf, sizeof(buf), "Filt: %.0fHz %s",
             effects->getFilterCutoff(),
             effects->isFilterEnabled() ? "ON" : "OFF");
    display.drawStr(0, 36, buf);
    
    // Highlight selected setting
    int yPos = 12 + (settingIndex * 12);
    display.drawHLine(0, yPos + 10, 64);
}

void UI::drawWaveform(int x, int y, int w, int h, Sample* sample) {
    if (!sample || !sample->loaded) return;
    
    // Simple waveform display
    int samplesToShow = min((int)sample->length, w);
    float step = (float)sample->length / (float)samplesToShow;
    
    for (int i = 0; i < samplesToShow - 1; i++) {
        int idx1 = (int)(i * step);
        int idx2 = (int)((i + 1) * step);
        
        if (idx1 < sample->length && idx2 < sample->length) {
            int y1 = y + h/2 + (sample->data[idx1] * h / 65536);
            int y2 = y + h/2 + (sample->data[idx2] * h / 65536);
            display.drawLine(x + i, y1, x + i + 1, y2);
        }
    }
}
