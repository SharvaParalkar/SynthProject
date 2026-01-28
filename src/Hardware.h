#ifndef HARDWARE_H
#define HARDWARE_H

#include <Arduino.h>
#include <driver/i2s.h>
#include "Config.h"

class Hardware {
public:
    Hardware();
    void init();
    
    // Audio Output
    void writeAudio(int16_t* buffer, size_t size);
    
    // Input Scanning
    void scanButtons();
    bool isPadPressed(int row, int col);
    bool isPadJustPressed(int row, int col);
    
    bool isModePressed();
    bool isModeJustPressed();
    
    bool isOctavePressed();
    bool isOctaveJustPressed();
    
    // LEDs
    void setGroupLEDs(int activeIndex); // 0-3
    void setStepLEDs(int step); // 0-15
    
private:
    // Button Matrix State
    bool padState[4][4];
    bool lastPadState[4][4];
    
    bool btnModeState;
    bool lastBtnModeState;
    
    bool btnOctaveState;
    bool lastBtnOctaveState;
};

#endif
