#ifndef HARDWARE_H
#define HARDWARE_H

#include <Arduino.h>
#include <driver/i2s.h>
#include "Config.h"

// Debounce time in milliseconds
#define DEBOUNCE_MS 15

class Hardware {
public:
    Hardware();
    void init();
    
    // Audio Output (32-bit for high-quality PCM5102A output)
    void writeAudio(int32_t* buffer, size_t size);
    
    // Legacy 16-bit audio output (if needed)
    void writeAudio16(int16_t* buffer, size_t size);
    
    // Input Scanning
    void scanButtons();
    bool isPadPressed(int row, int col);
    bool isPadJustPressed(int row, int col);
    bool isPadJustReleased(int row, int col);
    
    bool isModePressed();
    bool isModeJustPressed();
    
    bool isOctavePressed();
    bool isOctaveJustPressed();
    
    // LEDs
    void setGroupLEDs(int activeIndex); // 0-3
    void setStepLEDs(int step); // 0-15
    void setBrightness(int b); // 0-255
    int getBrightness();
    
private:
    int ledBrightness = 128; // Default 50%
    
    // Button Matrix State
    bool padState[4][4];
    bool lastPadState[4][4];
    uint32_t debounceTime[16];  // Debounce timestamps for each pad
    
    bool btnModeState;
    bool lastBtnModeState;
    
    bool btnOctaveState;
    bool lastBtnOctaveState;
};

#endif
