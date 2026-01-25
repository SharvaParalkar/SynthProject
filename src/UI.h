#ifndef UI_H
#define UI_H

#include <Arduino.h>
#include <U8g2lib.h>
#include "config.h"
#include "pins.h"
#include "ButtonMatrix.h"
#include "Sequencer.h"
#include "Effects.h"
#include "SampleEngine.h"

class UI {
public:
    UI();
    
    void begin();
    void update();
    
    void setMode(Mode mode);
    Mode getMode() { return currentMode; }
    
    void setSequencer(Sequencer* seq) { sequencer = seq; }
    void setEffects(Effects* eff) { effects = eff; }
    void setSampleEngine(SampleEngine* eng) { sampleEngine = eng; }
    void setButtonMatrix(ButtonMatrix* btn) { buttonMatrix = btn; }

private:
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C display;
    ButtonMatrix* buttonMatrix;
    Sequencer* sequencer;
    Effects* effects;
    SampleEngine* sampleEngine;
    
    Mode currentMode;
    unsigned long lastModeChange;
    unsigned long lastDisplayUpdate;
    static const unsigned long DISPLAY_UPDATE_INTERVAL = 50; // ms
    
    // UI state
    uint8_t selectedPattern;
    uint8_t selectedStep;
    bool editingStep;
    uint8_t selectedSample;
    
    // Settings mode state
    uint8_t settingIndex; // 0=BPM, 1=BitCrusher, 2=Filter
    bool adjustingValue;
    
    void updateDisplay();
    void updateLEDs();
    void handleInput();
    
    // Mode-specific handlers
    void handlePlayMode();
    void handlePatternMode();
    void handleSettingsMode();
    
    // Display functions
    void drawPlayScreen();
    void drawPatternScreen();
    void drawSettingsScreen();
    void drawWaveform(int x, int y, int w, int h, Sample* sample);
};

#endif // UI_H
