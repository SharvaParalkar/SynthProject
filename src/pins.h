#ifndef PINS_H
#define PINS_H

// --- I2S Audio (PCM5102A) ---
#define I2S_BCLK       6   // IO6
#define I2S_LRC        7   // IO7
#define I2S_DOUT       5   // IO5
#define I2S_NUM        I2S_NUM_0

// --- I2C Display ---
#define I2C_SDA        48
#define I2C_SCL        47
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64

// --- Button Matrix ---
static const int ROW_PINS[4] = {4, 3, 8, 15};  
static const int COL_PINS[4] = {16, 17, 18, 13}; 

// --- Function Buttons ---
#define BTN_MODE       36  // FUNC KEY 2 (Cycle Mode)
#define BTN_OCTAVE     37  // FUNC KEY 1 (Shift/Back)
#define BTN_BOOT       0

// --- Status LEDs ---
static const int LED_PINS[4] = {9, 10, 11, 12};

#endif // PINS_H
