#include "ButtonMatrix.h"
#include "Arduino.h"

ButtonMatrix::ButtonMatrix() {
    pressedMask = 0;
    lastPressedMask = 0;
    modePressed = false;
    octavePressed = false;
    lastModePressed = false;
    lastOctavePressed = false;
    modePressedEdge = false;
    octavePressedEdge = false;
    lastScanTime = 0;
}

void ButtonMatrix::begin() {
    // Configure row pins as outputs (initially high)
    for (int i = 0; i < NUM_ROWS; i++) {
        pinMode(ROW_PINS[i], OUTPUT);
        digitalWrite(ROW_PINS[i], HIGH);
    }
    
    // Configure column pins as inputs with pull-up
    for (int i = 0; i < NUM_COLS; i++) {
        pinMode(COL_PINS[i], INPUT_PULLUP);
    }
    
    // Configure function buttons
    pinMode(BTN_MODE, INPUT_PULLUP);
    pinMode(BTN_OCTAVE, INPUT_PULLUP);
}

uint8_t ButtonMatrix::getButtonIndex(uint8_t row, uint8_t col) {
    return row * NUM_COLS + col;
}

void ButtonMatrix::scanMatrix() {
    lastPressedMask = pressedMask;
    pressedMask = 0;
    
    for (int row = 0; row < NUM_ROWS; row++) {
        // Set current row low
        digitalWrite(ROW_PINS[row], LOW);
        delayMicroseconds(10); // Small delay for settling
        
        // Read columns
        for (int col = 0; col < NUM_COLS; col++) {
            if (digitalRead(COL_PINS[col]) == LOW) {
                // Button pressed (active low)
                uint8_t buttonIndex = getButtonIndex(row, col);
                pressedMask |= (1 << buttonIndex);
            }
        }
        
        // Set row back high
        digitalWrite(ROW_PINS[row], HIGH);
    }
}

void ButtonMatrix::scanFunctionButtons() {
    lastModePressed = modePressed;
    lastOctavePressed = octavePressed;
    
    modePressed = (digitalRead(BTN_MODE) == LOW);
    octavePressed = (digitalRead(BTN_OCTAVE) == LOW);
    
    modePressedEdge = (modePressed && !lastModePressed);
    octavePressedEdge = (octavePressed && !lastOctavePressed);
}

void ButtonMatrix::update() {
    unsigned long now = millis();
    if (now - lastScanTime >= SCAN_INTERVAL) {
        scanMatrix();
        scanFunctionButtons();
        lastScanTime = now;
    }
}

bool ButtonMatrix::isPressed(uint8_t button) {
    if (button >= 16) return false;
    return (pressedMask & (1 << button)) != 0;
}

bool ButtonMatrix::wasPressed(uint8_t button) {
    if (button >= 16) return false;
    uint16_t mask = 1 << button;
    return ((pressedMask & mask) != 0) && ((lastPressedMask & mask) == 0);
}

bool ButtonMatrix::wasReleased(uint8_t button) {
    if (button >= 16) return false;
    uint16_t mask = 1 << button;
    return ((pressedMask & mask) == 0) && ((lastPressedMask & mask) != 0);
}

uint8_t ButtonMatrix::getPressedCount() {
    uint8_t count = 0;
    for (int i = 0; i < 16; i++) {
        if (isPressed(i)) count++;
    }
    return count;
}
