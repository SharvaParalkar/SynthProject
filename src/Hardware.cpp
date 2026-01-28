#include "Hardware.h"

Hardware::Hardware() {
    memset(padState, 0, sizeof(padState));
    memset(lastPadState, 0, sizeof(lastPadState));
    btnModeState = false;
    lastBtnModeState = false;
    btnOctaveState = false;
    lastBtnOctaveState = false;
    ledBrightness = 127;
}

void Hardware::init() {
    Serial.println("Initializing Hardware...");
    
    // I2S Setup
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT, // Usually works for PCM5102
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 256,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };
    
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCLK,
        .ws_io_num = I2S_LRC,
        .data_out_num = I2S_DOUT,
        .data_in_num = I2S_PIN_NO_CHANGE
    };
    
    if (i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL) != ESP_OK) {
        Serial.println("I2S Driver Install Failed");
    }
    if (i2s_set_pin(I2S_NUM, &pin_config) != ESP_OK) {
        Serial.println("I2S Pin Setup Failed");
    }
    i2s_zero_dma_buffer(I2S_NUM);
    
    // GPIO Setup
    for (int i = 0; i < 4; i++) {
        pinMode(ROW_PINS[i], OUTPUT);
        digitalWrite(ROW_PINS[i], HIGH);
        
        pinMode(COL_PINS[i], INPUT_PULLUP);
        
        // PWM Setup for LEDs
        // Channels 0-3, 5000Hz, 8-bit
        ledcSetup(i, 5000, 8);
        ledcAttachPin(LED_PINS[i], i);
        ledcWrite(i, 0); // Start Off
    }
    
    pinMode(BTN_MODE, INPUT_PULLUP);
    pinMode(BTN_OCTAVE, INPUT_PULLUP);
    pinMode(BTN_BOOT, INPUT_PULLUP);
}

void Hardware::writeAudio(int16_t* buffer, size_t size) {
    size_t bytes_written;
    // Issue #7: Using a longer timeout to ensure data is written. 
    // Short timeouts can cause buffer underruns which sound like glitches/echoes.
    i2s_write(I2S_NUM, buffer, size, &bytes_written, portMAX_DELAY);
    
    // Issue #37: Error handling
    if (bytes_written < size) {
        // Buffer underrun or timeout
        // Serial.println("I2S Underrun"); // Warning: Serial in audio loop is bad
    }
}

void Hardware::scanButtons() {
    for (int row = 0; row < 4; row++) {
        digitalWrite(ROW_PINS[row], LOW);
        delayMicroseconds(10);
        
        for (int col = 0; col < 4; col++) {
            bool current = !digitalRead(COL_PINS[col]);
            lastPadState[row][col] = padState[row][col];
            padState[row][col] = current;
        }
        digitalWrite(ROW_PINS[row], HIGH);
    }
    
    lastBtnModeState = btnModeState;
    btnModeState = !digitalRead(BTN_MODE);
    
    lastBtnOctaveState = btnOctaveState;
    btnOctaveState = !digitalRead(BTN_OCTAVE);
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

bool Hardware::isModeJustPressed() {
    return btnModeState && !lastBtnModeState;
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
