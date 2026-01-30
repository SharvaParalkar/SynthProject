#include "Hardware.h"

// ============================================================================
// HIGH-QUALITY I2S CONFIGURATION FOR ESP32-S3 + PCM5102A
// ============================================================================
// Uses:
// - 32-bit I2S frames (24-bit audio data left-justified)
// - APLL clock for lower jitter
// - Optimized DMA buffer configuration
// ============================================================================

Hardware::Hardware() {
    memset(padState, 0, sizeof(padState));
    memset(lastPadState, 0, sizeof(lastPadState));
    btnModeState = false;
    lastBtnModeState = false;
    btnOctaveState = false;
    lastBtnOctaveState = false;
    ledBrightness = 127;
    memset(debounceTime, 0, sizeof(debounceTime));
}

void Hardware::init() {
    Serial.println("[Hardware] Initializing...");
    
    // ========================================================================
    // I2S SETUP - Optimized for PCM5102A DAC
    // ========================================================================
    
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = AUDIO_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,  // 32-bit for 24-bit audio
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = I2S_BUFFER_COUNT,             // 4-8 buffers recommended
        .dma_buf_len = I2S_BUFFER_SIZE,                // Frames per buffer
        .use_apll = true,                               // USE AUDIO PLL for cleaner clock!
        .tx_desc_auto_clear = true,                     // Auto-clear on underrun
        .fixed_mclk = 0                                 // Let APLL calculate MCLK
    };
    
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCLK,
        .ws_io_num = I2S_LRC,
        .data_out_num = I2S_DOUT,
        .data_in_num = I2S_PIN_NO_CHANGE
    };
    
    esp_err_t err;
    
    err = i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("[Hardware] I2S Driver Install Failed: %d\n", err);
    } else {
        Serial.println("[Hardware] I2S Driver Installed (32-bit, APLL enabled)");
    }
    
    err = i2s_set_pin(I2S_NUM, &pin_config);
    if (err != ESP_OK) {
        Serial.printf("[Hardware] I2S Pin Setup Failed: %d\n", err);
    } else {
        Serial.printf("[Hardware] I2S Pins: BCK=%d, WS=%d, DOUT=%d\n", 
                      I2S_BCLK, I2S_LRC, I2S_DOUT);
    }
    
    // Clear DMA buffers to prevent startup noise
    i2s_zero_dma_buffer(I2S_NUM);
    
    // ========================================================================
    // GPIO SETUP
    // ========================================================================
    
    for (int i = 0; i < 4; i++) {
        // Set rows to INPUT (High-Z) to avoid shorts
        pinMode(ROW_PINS[i], INPUT);
        
        pinMode(COL_PINS[i], INPUT_PULLUP);
        
        // PWM Setup for LEDs (using LEDC)
        // Channels 0-3, 5000Hz, 8-bit resolution
        ledcSetup(i, 5000, 8);
        ledcAttachPin(LED_PINS[i], i);
        ledcWrite(i, 0); // Start Off
    }
    
    pinMode(BTN_MODE, INPUT_PULLUP);
    pinMode(BTN_OCTAVE, INPUT_PULLUP);
    pinMode(BTN_BOOT, INPUT_PULLUP);
    
    Serial.println("[Hardware] GPIO Initialized");
}

void Hardware::writeAudio(int32_t* buffer, size_t bytes) {
    size_t bytes_written = 0;
    
    // Write with blocking wait to ensure no underruns
    esp_err_t err = i2s_write(I2S_NUM, buffer, bytes, &bytes_written, portMAX_DELAY);
    
    if (err != ESP_OK || bytes_written < bytes) {
        // Buffer underrun occurred
        // Don't print in audio path - just track for diagnostics
        static uint32_t underrunCount = 0;
        underrunCount++;
    }
}

// Legacy 16-bit write support (in case needed)
void Hardware::writeAudio16(int16_t* buffer, size_t bytes) {
    size_t bytes_written = 0;
    i2s_write(I2S_NUM, buffer, bytes, &bytes_written, portMAX_DELAY);
}

void Hardware::scanButtons() {
    uint32_t now = millis();
    
    for (int row = 0; row < 4; row++) {
        // Drive Row LOW
        pinMode(ROW_PINS[row], OUTPUT);
        digitalWrite(ROW_PINS[row], LOW);
        delayMicroseconds(10);  // Small settling time
        
        for (int col = 0; col < 4; col++) {
            bool rawState = !digitalRead(COL_PINS[col]);
            
            // Debounce: Only register change if stable for DEBOUNCE_MS
            int padIndex = row * 4 + col;
            
            if (rawState != padState[row][col]) {
                // State changed - check if enough time has passed
                if (now - debounceTime[padIndex] >= DEBOUNCE_MS) {
                    lastPadState[row][col] = padState[row][col];
                    padState[row][col] = rawState;
                    debounceTime[padIndex] = now;
                }
                // Otherwise, ignore the change (bounce)
            } else {
                // State is stable - update the timestamp for next change
                debounceTime[padIndex] = now;
            }
        }
        
        // Return Row to High-Z
        pinMode(ROW_PINS[row], INPUT);
    }
    
    // Function buttons (with simple debounce)
    static uint32_t modeDebounce = 0;
    static uint32_t octaveDebounce = 0;
    
    bool modeRaw = !digitalRead(BTN_MODE);
    if (modeRaw != btnModeState && (now - modeDebounce) >= DEBOUNCE_MS) {
        lastBtnModeState = btnModeState;
        btnModeState = modeRaw;
        modeDebounce = now;
    }
    
    bool octaveRaw = !digitalRead(BTN_OCTAVE);
    if (octaveRaw != btnOctaveState && (now - octaveDebounce) >= DEBOUNCE_MS) {
        lastBtnOctaveState = btnOctaveState;
        btnOctaveState = octaveRaw;
        octaveDebounce = now;
    }
}

bool Hardware::isPadPressed(int row, int col) {
    return padState[row][col];
}

bool Hardware::isPadJustPressed(int row, int col) {
    return padState[row][col] && !lastPadState[row][col];
}

bool Hardware::isPadJustReleased(int row, int col) {
    return !padState[row][col] && lastPadState[row][col];
}

bool Hardware::isModePressed() {
    return btnModeState;
}

bool Hardware::isModeJustPressed() {
    return btnModeState && !lastBtnModeState;
}

bool Hardware::isOctavePressed() {
    return btnOctaveState;
}

bool Hardware::isOctaveJustPressed() {
    return btnOctaveState && !lastBtnOctaveState;
}

void Hardware::setGroupLEDs(int activeIndex) {
    for (int i = 0; i < 4; i++) {
        if (i == activeIndex) {
            ledcWrite(i, ledBrightness);
        } else {
            ledcWrite(i, 0);
        }
    }
}

void Hardware::setStepLEDs(int step) {
    int page = step / 4;
    setGroupLEDs(page);
}

void Hardware::setBrightness(int b) {
    if (b < 0) b = 0;
    if (b > 255) b = 255;
    ledBrightness = b;
}

int Hardware::getBrightness() {
    return ledBrightness;
}
