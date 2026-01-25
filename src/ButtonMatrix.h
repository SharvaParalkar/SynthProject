#ifndef BUTTON_MATRIX_H
#define BUTTON_MATRIX_H

#include <Arduino.h>
#include "pins.h"

class ButtonMatrix {
public:
    ButtonMatrix();
    
    void begin();
    void update();
    
    // Get button state (0-15 for matrix, or special buttons)
    bool isPressed(uint8_t button);
    bool wasPressed(uint8_t button); // Edge detection
    bool wasReleased(uint8_t button);
    
    // Get function button states
    bool isModePressed() { return modePressed; }
    bool isOctavePressed() { return octavePressed; }
    bool wasModePressed() { return modePressedEdge; }
    bool wasOctavePressed() { return octavePressedEdge; }
    
    // Get all pressed buttons as bitmask
    uint16_t getPressedMask() { return pressedMask; }
    uint8_t getPressedCount();

private:
    static const uint8_t NUM_ROWS = 4;
    static const uint8_t NUM_COLS = 4;
    
    uint16_t pressedMask;      // Current state (bit 0-15 = buttons 0-15)
    uint16_t lastPressedMask;  // Previous state
    bool modePressed;
    bool octavePressed;
    bool lastModePressed;
    bool lastOctavePressed;
    bool modePressedEdge;
    bool octavePressedEdge;
    
    unsigned long lastScanTime;
    static const unsigned long SCAN_INTERVAL = 5; // ms
    
    void scanMatrix();
    void scanFunctionButtons();
    uint8_t getButtonIndex(uint8_t row, uint8_t col);
};

#endif // BUTTON_MATRIX_H
