#include <Arduino.h>
#include <driver/i2s.h>
#include <string.h>
#include <math.h>
#include "config.h"
#include "SampleEngine.h"
#include "Sequencer.h"
#include "Effects.h"
#include "UI.h"
#include "ButtonMatrix.h"

// Global objects
SampleEngine sampleEngine;
Sequencer sequencer(&sampleEngine);
Effects effects;
ButtonMatrix buttonMatrix;
UI ui;

// Forward declarations
void loadDefaultSamples();

// Audio buffer (using PSRAM)
int16_t* audioBuffer = nullptr;
const size_t BUFFER_SIZE = 512; // Samples per buffer

// I2S configuration
i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = AUDIO_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = BUFFER_SIZE,
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

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Lofi Beat Maker Starting...");

    // Allocate audio buffer in PSRAM
    if (psramFound()) {
        audioBuffer = (int16_t*)ps_malloc(BUFFER_SIZE * sizeof(int16_t));
        Serial.printf("PSRAM found, allocated %d bytes\n", BUFFER_SIZE * sizeof(int16_t));
    } else {
        audioBuffer = (int16_t*)malloc(BUFFER_SIZE * sizeof(int16_t));
        Serial.println("PSRAM not found, using heap");
    }

    if (!audioBuffer) {
        Serial.println("Failed to allocate audio buffer!");
        while(1) delay(1000);
    }

    // Initialize I2S
    esp_err_t err = i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("I2S driver install failed: %d\n", err);
    } else {
        Serial.println("I2S driver installed successfully");
    }
    
    err = i2s_set_pin(I2S_NUM, &pin_config);
    if (err != ESP_OK) {
        Serial.printf("I2S pin config failed: %d\n", err);
    } else {
        Serial.println("I2S pins configured");
    }
    
    i2s_zero_dma_buffer(I2S_NUM);
    Serial.println("I2S initialized");

    // Initialize components
    sampleEngine.begin();
    sequencer.begin();
    effects.begin();
    buttonMatrix.begin();
    
    // Wire up UI references
    ui.setSequencer(&sequencer);
    ui.setEffects(&effects);
    ui.setSampleEngine(&sampleEngine);
    ui.setButtonMatrix(&buttonMatrix);
    ui.begin();

    // Load default samples (placeholder - user needs to add actual samples)
    loadDefaultSamples();

    Serial.println("Initialization complete!");
    Serial.println("Ready to make beats!");
}

void loop() {
    // Update UI (handles button scanning, display, LEDs)
    ui.update();

    // Update sequencer
    sequencer.update();

    // Generate audio buffer
    memset(audioBuffer, 0, BUFFER_SIZE * sizeof(int16_t));
    
    // Mix all active voices
    sampleEngine.render(audioBuffer, BUFFER_SIZE);

    // Apply effects
    effects.process(audioBuffer, BUFFER_SIZE);

    // Send to I2S
    size_t bytes_written;
    esp_err_t err = i2s_write(I2S_NUM, audioBuffer, BUFFER_SIZE * sizeof(int16_t), &bytes_written, portMAX_DELAY);
    if (err != ESP_OK && err != ESP_ERR_TIMEOUT) {
        static unsigned long lastError = 0;
        if (millis() - lastError > 1000) { // Print error max once per second
            Serial.printf("I2S write error: %d, bytes_written: %d\n", err, bytes_written);
            lastError = millis();
        }
    }
}

// Generate a simple test tone (sine wave) for testing audio output
void generateTestTone(int16_t* buffer, size_t length, float frequency, float sampleRate) {
    float phase = 0.0f; // Start from 0 each time (not static)
    float phaseIncrement = (frequency * 2.0f * PI) / sampleRate;
    
    for (size_t i = 0; i < length; i++) {
        buffer[i] = (int16_t)(sin(phase) * 20000.0f); // ~60% volume
        phase += phaseIncrement;
        if (phase >= 2.0f * PI) phase -= 2.0f * PI;
    }
}

// Placeholder function - user needs to implement with actual sample data
void loadDefaultSamples() {
    // Generate test tones for all 16 keys (chromatic scale starting from C4)
    // Frequencies: C4, C#4, D4, D#4, E4, F4, F#4, G4, G#4, A4, A#4, B4, C5, C#5, D5, D#5
    const float baseFreq = 261.63f; // C4
    const size_t testToneLength = AUDIO_RATE * 2; // 2 seconds - longer to avoid immediate fade
    
    Serial.println("Loading test tones for all 16 keys...");
    
    for (uint8_t key = 0; key < MAX_SAMPLES; key++) {
        // Calculate frequency: each key is one semitone higher
        float frequency = baseFreq * powf(2.0f, key / 12.0f);
        
        int16_t* testTone = (int16_t*)malloc(testToneLength * sizeof(int16_t));
        
        if (testTone) {
            generateTestTone(testTone, testToneLength, frequency, (float)AUDIO_RATE);
            if (sampleEngine.loadSample(key, testTone, testToneLength, AUDIO_RATE)) {
                Serial.printf("Test tone %d loaded: %.2f Hz\n", key, frequency);
            } else {
                Serial.printf("Failed to load test tone %d\n", key);
            }
            // Note: We don't free testTone here because loadSample copies it
            // The SampleEngine will manage the memory
        } else {
            Serial.printf("Failed to allocate test tone buffer for key %d\n", key);
        }
    }
    
    // Auto-trigger the first test tone (C4) on startup
    sampleEngine.triggerSample(0, 0.0f);
    Serial.println("Test tone 0 (C4) triggered on startup");
    
    // TODO: Load built-in lofi samples
    // This should load 8-12 essential samples:
    // - Kick (2 variations)
    // - Snare (2 variations)
    // - Hi-hat (closed/open)
    // - Clap
    // - Bass note (one-shot)
    // - Melodic sample (vinyl chord/keys)
    // - Vinyl crackle/noise loop
    
    // Example structure (user needs to add actual sample data):
    // int16_t kickSample[] = { ... }; // Sample data
    // sampleEngine.loadSample(1, kickSample, sizeof(kickSample)/sizeof(int16_t), 22050);
    
    Serial.println("Sample loading complete - test tones available in all 16 slots");
}
