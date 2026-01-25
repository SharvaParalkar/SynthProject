/*
 * ESP32-S3 16-Key Polyphonic Synthesizer
 * 
 * Target Hardware:
 * - ESP32-S3-WROOM-1U-N8R2
 * - PCM5102A DAC (I2S)
 * - SSD1306 OLED (I2C) - Using U8g2
 * - 4x4 Button Matrix + 2 Function Buttons
 * - 4 Status LEDs
 */

#include <Arduino.h>
#include <driver/i2s.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <math.h>
#include "UI.h"

// =========================================================================================
// PINS & CONFIGURATION
// =========================================================================================

// --- I2S Audio (PCM5102A) ---
#define I2S_BCLK       6   // IO6
#define I2S_LRC        7   // IO7
#define I2S_DOUT       5   // IO5
#define I2S_NUM        I2S_NUM_0
#define AUDIO_RATE     44100
#define BITS_PER_SAMPLE 16

// --- I2C Display ---
#define I2C_SDA        48
#define I2C_SCL        47
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64

// --- Button Matrix ---
const int ROW_PINS[4] = {4, 3, 8, 15};  
const int COL_PINS[4] = {16, 17, 18, 13}; 

// --- Function Buttons ---
#define BTN_MODE       36  // FUNC KEY 2 (Cycle Mode)
#define BTN_OCTAVE     37  // FUNC KEY 1 (Shift/Back)
#define BTN_BOOT       0

// --- Status LEDs ---
const int LED_PINS[4] = {9, 10, 11, 12};

// --- Synths Constants ---
#define MAX_VOICES     4
#define BUFFER_SIZE    512 

// =========================================================================================
// SAMPLE DATA (PROGMEM)
// =========================================================================================

// Simple 8-bit PCM Data placeholders (Transient shapes)
// In a real scenario, these would be longer and converted from wav files
const uint8_t s_kick[] PROGMEM = {
  128,135,150,170,190,210,230,245,255,250,230,200,170,140,110,80,60,40,30,20,15,10,
  10,15,20,30,50,80,110,140,160,180,190,195,190,180,160,140,120,100,80,70,60,50,45,40,
  35,30,28,26,24,22,20,18,16,14,12,10,8,6,4,2,0,2,4,6,8,10,12,128,128,128
}; // Short thud

const uint8_t s_snare[] PROGMEM = {
  128,180,250,50,220,60,200,80,180,100,160,128,40,210,30,190,50,150,
  90,140,110,130,120,115,125,118,132,122,128,126,130,128,128,128
}; // Noise burst

const uint8_t s_hat[] PROGMEM = {
  128,200,80,180,100,160,120,140,128,110,135,122,129,126,128
}; // Short click

const uint8_t s_clap[] PROGMEM = {
  128,200,80,210,70,200,90,180,100,128,128,128,180,90,170,100,160,128,128,128,
  160,110,150,120,130,128,128
}; // Multi-burst

// =========================================================================================
// DATA STRUCTURES
// =========================================================================================

struct Sample {
    const char* name;
    const uint8_t* data;
    uint32_t length;
    uint32_t sampleRate;
    bool loop;
};

struct SampleKit {
    char name[16];
    const Sample* samples[16]; // One for each pad
};

const int BASE_MIDI_NOTE = 60; 

struct NoteState {
  size_t voiceIndex; 
  bool isPressed;
  unsigned long lastChanged; 
  bool longPressHandled;
};

NoteState keyStates[16]; 

enum EnvelopeState {
    ENV_IDLE = 0,
    ENV_ATTACK,
    ENV_SUSTAIN,
    ENV_RELEASE
};

struct Voice {
    bool active;
    int note;        
    
    // Sample Playback State
    const Sample* sample;
    float position;      // Current index in sample array (fractional)
    float speed;         // Increment per output sample
    
    // Envelope
    EnvelopeState envState;
    float currentAmp;    
    float attackStep;
    float releaseStep;
    
    unsigned long noteOnTimestamp;
    float baseSpeed; // Store initial speed for pitch modulation
};

Voice voices[MAX_VOICES];

// =========================================================================================
// GLOBALS
// =========================================================================================

Sample sampleObjects[4]; // Concrete sample definitions
SampleKit currentKit;

volatile int currentOctaveOffset = 0; 
volatile int activeVoiceCount = 0;
volatile float masterVolume = 0.8f; 
volatile int16_t scopeBuffer[128]; 

// Per-pad pitch offset (semitones)
// Per-pad pitch offset (semitones)
int8_t padPitch[16];

int selectedFX = 0; // 0=BitCrush, 1=SR, 2=Filter, 3=Delay

// --- Effects State ---
bool bitCrushEnabled = false;
int bitCrushDepth = 4; // 1-16

bool sampleRateReductionEnabled = false;
int srReductionFactor = 2; // 2, 4, 8

bool filterEnabled = false;
float filterCutoff = 1.0f; // 0.0 - 1.0
float filterResonance = 0.0f; 

bool delayEnabled = false;
int delayMix = 30; // %
int delayFeedback = 40; // %
int delayTime = 200; // ms

// --- LFO State ---
struct LFO {
    float phase;
    float rate;           // Hz
    uint8_t waveform;     // 0=sine, 1=triangle, 2=random
    float depth;          // 0.0-1.0
    uint8_t target;       // 0=pitch, 1=filter, 2=volume
};
LFO lfos[2];

// Global Modulations (Calculated per buffer)
float modPitch = 1.0f;
float modFilter = 0.0f;
float modVolume = 1.0f;
float seqFilterMod = 0.0f; // Sequencer step offset

// --- Sequencer State ---

// --- Sequencer State ---
Pattern patterns[4];
uint8_t currentPattern = 0;
bool sequencerPlaying = false;
uint8_t currentStep = 0;
unsigned long lastStepTime = 0;
int16_t bpm = 120; // Default
uint8_t seqActiveTrack = 0; // Currently selected track for editing (0-7)
bool globalShiftUsed = false; // Track if shift triggered a combo action

// UI Objects


U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ I2C_SCL, /* data=*/ I2C_SDA);
SynthUI ui(u8g2);

// Synchronization
SemaphoreHandle_t voiceMutex;
const unsigned long DEBOUNCE_MS = 20;

// Function Prototypes
void setupI2S();
void audioTask(void *pvParameters);
void triggerNote(int keyIndex, int noteNumber);
void stopNote(int noteNumber);
void noteOn(int keyIndex);
void noteOff(int keyIndex);

void loadKit();
void initSequencer();
void clockSequencer();
void initSequencer();
void clockSequencer();
void handleSequencerInput(int keyIndex, bool pressed);
void handleFXInput(int keyIndex, bool pressed);



// =========================================================================================
// SETUP
// =========================================================================================

void setup() {
    delay(1000); 
    Serial.begin(115200);
    // Wait for Serial
    unsigned long startMillis = millis();
    while (!Serial && (millis() - startMillis < 2000)) delay(10);
    
    Serial.printf("Total PSRAM: %u bytes\n", (unsigned int)ESP.getPsramSize());
    Serial.printf("Free PSRAM: %u bytes\n", (unsigned int)ESP.getFreePsram());
    
    // 1. Initialize Pins
    for(int i=0; i<4; i++) {
        pinMode(ROW_PINS[i], INPUT); 
        pinMode(COL_PINS[i], INPUT_PULLUP);
        pinMode(LED_PINS[i], OUTPUT);
        digitalWrite(LED_PINS[i], LOW);
    }
    
    pinMode(BTN_MODE, INPUT_PULLUP);
    pinMode(BTN_OCTAVE, INPUT_PULLUP);

    // 2. Initialize Display
    Wire.begin(I2C_SDA, I2C_SCL);
    u8g2.setI2CAddress(0x3C * 2); 
    u8g2.setBusClock(400000); 
    u8g2.begin();
    
    // System Info Screen
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(0, 10, "ESP32-S3 SYNTH");
    
    u8g2.setCursor(0, 30);
    u8g2.print("PSRAM Total:");
    u8g2.setCursor(70, 30);
    u8g2.print(ESP.getPsramSize() / 1024);
    u8g2.print(" KB");
    
    u8g2.setCursor(0, 42);
    u8g2.print("PSRAM Free:");
    u8g2.setCursor(70, 42);
    u8g2.print(ESP.getFreePsram() / 1024);
    u8g2.print(" KB");
    
    u8g2.sendBuffer();
    delay(1000);
    
    ui.begin();
    ui.setScopeData(scopeBuffer); 

    // 3. Load Samples & Kit
    loadKit();

    // 4. Initialize Voices
    voiceMutex = xSemaphoreCreateMutex();
    for(int i=0; i<MAX_VOICES; i++) {
        voices[i].active = false;
        voices[i].envState = ENV_IDLE;
        voices[i].currentAmp = 0.0f;
        // Fast attack for drums
        voices[i].attackStep = 1.0f / (0.002f * AUDIO_RATE); 
        // Longer release
        voices[i].releaseStep = 1.0f / (0.200f * AUDIO_RATE); 
    }
    
    for(int i=0; i<16; i++) {
        keyStates[i].isPressed = false;
        keyStates[i].voiceIndex = 255; 
        keyStates[i].lastChanged = 0;
        keyStates[i].longPressHandled = false;
        padPitch[i] = 0; // Default 0 pitch offset
    }

    initSequencer();


    // 5. Setup I2S
    setupI2S();

    // 6. Create Audio Task
    xTaskCreatePinnedToCore(audioTask, "AudioTask", 10000, NULL, 10, NULL, 0);
    
    Serial.println("Synth Ready: Sample Mode");
}

void loadKit() {
    // 1. Setup Sample Objects
    sampleObjects[0] = { "KICK", s_kick, sizeof(s_kick), 16000, false };
    sampleObjects[1] = { "SNARE", s_snare, sizeof(s_snare), 16000, false };
    sampleObjects[2] = { "HAT", s_hat, sizeof(s_hat), 16000, false };
    sampleObjects[3] = { "CLAP", s_clap, sizeof(s_clap), 16000, false };

    // 2. Setup Kit
    strcpy(currentKit.name, "DRUMS");
    
    // Map pads to samples (Col-major or Row-major? 4x4)
    // Row 0 (Bottom): Kick
    currentKit.samples[0] = &sampleObjects[0]; // K
    currentKit.samples[1] = &sampleObjects[0]; // K
    currentKit.samples[2] = &sampleObjects[0]; // K
    currentKit.samples[3] = &sampleObjects[0]; // K
    
    // Row 1: Snare
    currentKit.samples[4] = &sampleObjects[1]; 
    currentKit.samples[5] = &sampleObjects[1]; 
    currentKit.samples[6] = &sampleObjects[1]; 
    currentKit.samples[7] = &sampleObjects[1]; 

    // Row 2: Hat
    currentKit.samples[8] = &sampleObjects[2];
    currentKit.samples[9] = &sampleObjects[2];
    currentKit.samples[10] = &sampleObjects[2];
    currentKit.samples[11] = &sampleObjects[2];

    // Row 3: Clap
    currentKit.samples[12] = &sampleObjects[3];
    currentKit.samples[13] = &sampleObjects[3];
    currentKit.samples[14] = &sampleObjects[3];
    currentKit.samples[15] = &sampleObjects[3];
    
    ui.setKitName(currentKit.name);
}

// =========================================================================================
// LOOP (UI Thread)
// =========================================================================================

bool lastBtnMode = true; 
bool lastBtnOct = true;
unsigned long f1PressTime = 0;
bool f1LongPressHandled = false;

void loop() {
    unsigned long now = millis();
    
    // Matrix Scanning
    for(int r=0; r<4; r++) {
        pinMode(ROW_PINS[r], OUTPUT);
        digitalWrite(ROW_PINS[r], LOW);
        delayMicroseconds(50); 
        
        for(int c=0; c<4; c++) {
            int keyIndex = (r * 4) + c;
            bool pressed = !digitalRead(COL_PINS[c]); 
            
            if (pressed != keyStates[keyIndex].isPressed) {
                if (now - keyStates[keyIndex].lastChanged > DEBOUNCE_MS) {
                    keyStates[keyIndex].isPressed = pressed;
                    keyStates[keyIndex].lastChanged = now;
                    
                    if(pressed) {
                        ui.onButtonPress(keyIndex);
                        if(ui.isPerformanceMode()) {
                            noteOn(keyIndex);
                        } else if (ui.isSequencerMode()) {
                            handleSequencerInput(keyIndex, true);
                        } else if (ui.isFXEditMode()) {
                            handleFXInput(keyIndex, true);
                        }
                    } else {
                        ui.onButtonRelease(keyIndex);
                        if(ui.isPerformanceMode()) {
                            noteOff(keyIndex);
                        }
                    }

                }
            }
        }
        pinMode(ROW_PINS[r], INPUT);
    }
    
    // Function Buttons
    bool btnModeVal = digitalRead(BTN_MODE);  // F2
    bool btnOctVal = digitalRead(BTN_OCTAVE); // F1 (Shift/Back)
    
    // F2: Next Mode / Confirm
    if (lastBtnMode == HIGH && btnModeVal == LOW) {
        ui.onFunctionKey2(); 
    }
    lastBtnMode = btnModeVal;
    
    // F1 Logic (Back / Shift / Long Press for FX)
    if (lastBtnOct == HIGH && btnOctVal == LOW) {
        // Press
        f1PressTime = now;
        f1LongPressHandled = false;
        globalShiftUsed = false;
    }
    
    // Check F1 Hold (800ms for long press)
    if (btnOctVal == LOW && !f1LongPressHandled) {
        if (now - f1PressTime > 800) {
            f1LongPressHandled = true;
            // Notify UI of long press
            ui.onFunctionKey1(true, true);
        }
    }
    
    if (lastBtnOct == LOW && btnOctVal == HIGH) {
        // Release
        if (!f1LongPressHandled) {
            // Short Press
            ui.onFunctionKey1(false, false);
            
            // Special handling for sequencer play/stop
            if (ui.isSequencerMode() && !globalShiftUsed) {
                sequencerPlaying = !sequencerPlaying;
                if(sequencerPlaying) lastStepTime = micros();
            }
        }
    }
    lastBtnOct = btnOctVal;


    // Sequencer Clock
    clockSequencer();


    // Update UI Volume State
    // Read from UI because UI controls volume via menus now
    masterVolume = (float)ui.getMasterVolume() / 100.0f; 
    
    ui.setActiveVoices(activeVoiceCount);
    ui.setFXStatus(bitCrushEnabled, sampleRateReductionEnabled, filterEnabled, delayEnabled);
    ui.setBPM(bpm); // Push BPM to UI
    
    // Push Sequencer State to UI
    ui.setSequencerState(&patterns[currentPattern], currentStep, sequencerPlaying, seqActiveTrack);
    
    static unsigned long lastDraw = 0;
    if (now - lastDraw > 33) {
        ui.update();
        ui.draw();
        lastDraw = now;
    }
    
    // LEDs
    bool vActive[4] = {false, false, false, false};
    
    if (ui.isPerformanceMode()) {
        if(xSemaphoreTake(voiceMutex, 1)) {
            for(int i=0; i<4; i++) vActive[i] = voices[i].active;
            xSemaphoreGive(voiceMutex);
        }
    } else if (ui.isSequencerMode()) {
        // LED 1: Playing?
        vActive[0] = sequencerPlaying;
        // LED 2: Beat indicator?
        vActive[1] = (currentStep % 4 == 0);
    } else {
        // Menu Navigation Feedback
        // Top row buttons 0-3 map to LEDs 0-3
        for(int i=0; i<4; i++) {
            if(keyStates[i].isPressed) vActive[i] = true;
        }
    }
    
    for(int i=0; i<4; i++) digitalWrite(LED_PINS[i], vActive[i] ? HIGH : LOW);
    delay(2);
}


// =========================================================================================
// AUDIO & LOGIC
// =========================================================================================

void noteOn(int keyIndex) {
    if(keyIndex < 0 || keyIndex > 15) return;
    
    // Show Sample Name
    if(currentKit.samples[keyIndex]) {
        ui.setContextHint(currentKit.samples[keyIndex]->name);
    }

    // Base pitch offset comes from Pad Pitch + Global Octave
    int semitoneOffset = padPitch[keyIndex] + (currentOctaveOffset * 12);
    triggerNote(keyIndex, semitoneOffset);
}

void noteOff(int keyIndex) {
    // For drums/one-shots, we might ignore note-off or use it to cut short
    // Using existing stopNote logic, but mapped to keyIndex if needed.
    // Since we don't have a 1:1 mapping of note numbers anymore easily, 
    // we iterate voices for the keyIndex or just let envelopes finish if they are one-shot.
    // For now, let's allow "Releasing" the key to trigger envelope release (gate mode)
    // or we can ignore it for one-shot.
    // Implementation: Search for voice playing this sample/pad and release it.
    // But triggerNote assigns voices. We need to track which voice is playing which key?
    // tracking by note value was old way. Let's use keyIndex as "note" substitute in voice
    stopNote(keyIndex); 
}

void triggerNote(int keyIndex, int pitchOffset) {
    const Sample* s = currentKit.samples[keyIndex];
    if(!s) return;

    // Calculate Playback Speed
    // speed = (SourceRate / OutputRate) * 2^(semitones/12)
    float pitchFactor = pow(2.0f, pitchOffset / 12.0f);
    float speed = ((float)s->sampleRate / (float)AUDIO_RATE) * pitchFactor;

    xSemaphoreTake(voiceMutex, portMAX_DELAY);
    
    // Voice Stealing Logic (Same as before)
    int voiceId = -1;
    for(int i=0; i<MAX_VOICES; i++) {
        if(!voices[i].active) { voiceId = i; break; }
    }
    if(voiceId == -1) {
        unsigned long oldestTime = millis();
        for(int i=0; i<MAX_VOICES; i++) {
            if(voices[i].noteOnTimestamp < oldestTime) {
                oldestTime = voices[i].noteOnTimestamp;
                voiceId = i;
            }
        }
    }
    
    if(voiceId != -1) {
        voices[voiceId].active = true;
        voices[voiceId].note = keyIndex; // Storing keyIndex as identifier
        voices[voiceId].sample = s;
        voices[voiceId].position = 0.0f;
        voices[voiceId].baseSpeed = speed;
        voices[voiceId].speed = speed;
        voices[voiceId].envState = ENV_ATTACK;
        voices[voiceId].currentAmp = 0.0f; 
        voices[voiceId].noteOnTimestamp = millis();
    }
    xSemaphoreGive(voiceMutex);
}

void stopNote(int keyIndex) {
    xSemaphoreTake(voiceMutex, portMAX_DELAY);
    for(int i=0; i<MAX_VOICES; i++) {
        if(voices[i].active && voices[i].note == keyIndex && voices[i].envState != ENV_RELEASE) {
            voices[i].envState = ENV_RELEASE;
        }
    }
    xSemaphoreGive(voiceMutex);
}

void setupI2S() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = AUDIO_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT, 
        .communication_format = I2S_COMM_FORMAT_STAND_MSB, 
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = BUFFER_SIZE,
        .use_apll = false,
        .tx_desc_auto_clear = true
    };
    i2s_pin_config_t pin_config = {
        .mck_io_num = I2S_PIN_NO_CHANGE,
        .bck_io_num = I2S_BCLK,
        .ws_io_num = I2S_LRC,
        .data_out_num = I2S_DOUT,
        .data_in_num = I2S_PIN_NO_CHANGE
    };
    i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM, &pin_config);
    i2s_zero_dma_buffer(I2S_NUM);
}

void audioTask(void *pvParameters) {
    static int32_t mixBuffer[BUFFER_SIZE * 2]; 
    static int16_t i2sBuffer[BUFFER_SIZE * 2]; 
    size_t bytes_written;
    
    // Initialize LFOs
    lfos[0] = {0, 0.5f, 0, 0.0f, 1}; // Default LFO 1
    lfos[1] = {0, 2.0f, 1, 0.0f, 0}; // Default LFO 2

    while(true) {
        memset(mixBuffer, 0, sizeof(mixBuffer));

        // --- LFO Calculation (Per Buffer) ---
        // BUFFER_SIZE samples at AUDIO_RATE. Time step = BUFFER_SIZE / 44100.
        // We advance LFO phase.
        float dt = (float)BUFFER_SIZE / (float)AUDIO_RATE;
        
        float currentPitchMod = 1.0f;
        float currentFilterMod = 0.0f;
        float currentVolMod = 1.0f;

        for(int l=0; l<2; l++) {
            lfos[l].phase += lfos[l].rate * dt;
            if(lfos[l].phase >= 1.0f) lfos[l].phase -= 1.0f;
            
            float oscVal = 0.0f;
            if(lfos[l].waveform == 0) { // Sine
                oscVal = sinf(lfos[l].phase * 2.0f * PI);
            } else if (lfos[l].waveform == 1) { // Triangle
                oscVal = (lfos[l].phase < 0.5f) ? (4.0f * lfos[l].phase - 1.0f) : (3.0f - 4.0f * lfos[l].phase);
            } else { // Random (Sample and Hold - approximated by noise or changing on phase wrap)
                // For simplicity, just use sin for now or true random
                 oscVal = ((float)random(0, 1000) / 500.0f) - 1.0f;
            }

            // Apply to targets
            // Target 0: Pitch (Multiplicative: 0.5x to 2.0x?)
            if(lfos[l].target == 0 && lfos[l].depth > 0) {
                // Depth 1.0 -> +/- 1 octave?
                currentPitchMod *= pow(2.0f, oscVal * lfos[l].depth); 
            }
            // Target 1: Filter (Additive to cutoff)
            if(lfos[l].target == 1 && lfos[l].depth > 0) {
                currentFilterMod += (oscVal * lfos[l].depth * 0.5f); // Modulate +/- 0.5 range
            }
            // Target 2: Volume (Multiplicative)
            if(lfos[l].target == 2 && lfos[l].depth > 0) {
                 currentVolMod *= (1.0f + (oscVal * lfos[l].depth * 0.5f));
            }
        }
        modPitch = currentPitchMod;
        modFilter = currentFilterMod;
        modVolume = currentVolMod;
        if(modVolume < 0.0f) modVolume = 0.0f;

        
        if(xSemaphoreTake(voiceMutex, portMAX_DELAY)) {
            int count = 0;
            for(int v=0; v<MAX_VOICES; v++) {
                if(!voices[v].active) continue;
                count++;
                Voice* vx = &voices[v];
                const Sample* s = vx->sample;

                // Update Pitch Modulation
                vx->speed = vx->baseSpeed * modPitch;
                
                for(int i=0; i<BUFFER_SIZE; i++) {
                    // Envelope
                    if (vx->envState == ENV_ATTACK) {
                        vx->currentAmp += vx->attackStep;
                        if (vx->currentAmp >= 1.0f) { vx->currentAmp = 1.0f; vx->envState = ENV_SUSTAIN; }
                    } else if (vx->envState == ENV_RELEASE) {
                        vx->currentAmp -= vx->releaseStep;
                        if (vx->currentAmp <= 0.0f) { 
                            vx->currentAmp = 0.0f; 
                            vx->active = false; 
                            vx->envState = ENV_IDLE; 
                        }
                    } 
                    if(!vx->active) break; 
                    
                    // Sample Playback
                    float idx = vx->position;
                    int idx_i = (int)idx;
                    
                    // Boundary Check
                    if (idx_i >= s->length) {
                        if (s->loop) {
                            vx->position = 0; // Simple loop to start
                            idx_i = 0;
                        } else {
                            vx->active = false;
                            break;
                        }
                    }
                    
                    // Linear Interpolation
                    int8_t s1 = (int8_t)((int)s->data[idx_i] - 128);
                    int8_t s2 = 0;
                    if (idx_i + 1 < s->length) {
                        s2 = (int8_t)((int)s->data[idx_i + 1] - 128);
                    } else {
                         s2 = s->loop ? (int8_t)((int)s->data[0] - 128) : 0;
                    }
                    
                    float frac = idx - idx_i;
                    float val = (float)s1 + frac * ((float)s2 - (float)s1);
                    
                    // Scale to 16-bit (~256) and volume
                    int16_t out = (int16_t)(val * 128.0f * vx->currentAmp * masterVolume * modVolume);
                    
                    mixBuffer[i*2] += out;
                    mixBuffer[i*2+1] += out;
                    
                    vx->position += vx->speed;
                }
            }
            activeVoiceCount = count;
            xSemaphoreGive(voiceMutex);
        }
        
        // --- PRE-OUTPUT EFFECTS ---
        // Working on 16-bit Stereo Interleaved buffer (but mixBuffer is 32-bit for headroom)
        // Convert to 16-bit first for FX that need it, or apply on mixBuffer (better quality).
        // The user snippet used i2sBuffer (16-bit). Let's convert mixBuffer to i2sBuffer first, then apply FX.
        
        // Initial Mix Down
        for(int i=0; i<BUFFER_SIZE*2; i++) {
            if(mixBuffer[i] > 32767) mixBuffer[i] = 32767;
            if(mixBuffer[i] < -32768) mixBuffer[i] = -32768;
            i2sBuffer[i] = (int16_t)mixBuffer[i];
        }

        // 1. Bit Crusher
        if(bitCrushEnabled) {
            int bits = bitCrushDepth; // 1-16
            int step = 32768 / (1 << bits);
            if(step < 1) step = 1;
            for(int i=0; i<BUFFER_SIZE*2; i++) {
                i2sBuffer[i] = (i2sBuffer[i] / step) * step;
            }
        }

        // 2. Sample Rate Reduction
        if(sampleRateReductionEnabled) {
            int factor = srReductionFactor; 
            for(int i=0; i<BUFFER_SIZE*2; i+=factor) {
                int16_t held = i2sBuffer[i];
                for(int j=1; j<factor && i+j<BUFFER_SIZE*2; j++) {
                    i2sBuffer[i+j] = held;
                }
            }
        }

        // 3. Low-Pass Filter
        static int32_t filterState[2] = {0, 0};
        if(filterEnabled) {
            // Apply Modulation
            float effectiveCutoff = filterCutoff + modFilter + seqFilterMod;
            if(effectiveCutoff > 0.99f) effectiveCutoff = 0.99f;
            if(effectiveCutoff < 0.01f) effectiveCutoff = 0.01f;
            
            for(int i=0; i<BUFFER_SIZE*2; i++) {
                int ch = i & 1;
                filterState[ch] = filterState[ch] + effectiveCutoff * (i2sBuffer[i] - filterState[ch]);
                i2sBuffer[i] = (int16_t)filterState[ch];
            }
        }

        // 4. Simple Delay
        static int16_t delayBuf[22050]; 
        static int delayIndex = 0;
        
        if(delayEnabled) {
            int samps = (delayTime * 44100) / 1000;
            if(samps > 22049) samps = 22049;
            if(samps < 100) samps = 100;
            
            for(int i=0; i<BUFFER_SIZE*2; i++) {
               // Mono delay for stereo processing: usually one buffer per channel or interleave?
               // The snippet used single buffer. Let's do simple mix.
               // Actually we are processing interleaved. Ideally we separate channels.
               // But for lofi simple delay, mixing them is 'ping pong' if naive, or 'slapback'.
               // Let's assume the delay buffer stores summed mono or interleaved?
               // Code snippet: i2sBuffer[i] = dry... delayBuffer[delayIndex]...
               // This implies delayBuffer matches i2sBuffer structure (Interleaved).
               // So samps limit should be in "shorts" (samples * channels).
               // delayTime in ms. 44100 samples/sec = 44.1 samples/ms.
               // Stereo = 88.2 shorts/ms.
               
               int dLen = samps * 2; // Stereo
               if(dLen > 22040) dLen = 22040;
               
               int16_t dry = i2sBuffer[i];
               int16_t wet = delayBuf[delayIndex];
               
               i2sBuffer[i] = (dry * (100-delayMix) + wet * delayMix) / 100;
               
               // Feedback
               int16_t nextVal = dry + (wet * delayFeedback / 100);
               delayBuf[delayIndex] = nextVal;
               
               delayIndex++;
               if(delayIndex >= dLen) delayIndex = 0;
            }
        }
        
        i2s_write(I2S_NUM, i2sBuffer, sizeof(i2sBuffer), &bytes_written, portMAX_DELAY);
        
        // Update Scope
        for(int k=0; k<128; k++) {
            scopeBuffer[k] = i2sBuffer[k * 8]; 
        }
    }
}
// =========================================================================================
// SEQUENCER LOGIC
// =========================================================================================

void initSequencer() {
    // Clear all patterns
    for(int p=0; p<4; p++) {
        sprintf(patterns[p].name, "PAT %d", p+1);
        patterns[p].length = 16;
        
        // Map 8 Tracks to useful sounds (Kick, Snare, Hat, Clap, etc)
        // Kit Layout: 0-3 Kick, 4-7 Snare, 8-11 Hat, 12-15 Clap
        int defaultMap[8] = {0, 4, 8, 12, 1, 5, 9, 13};
        
        for(int t=0; t<8; t++) {
            patterns[p].tracks[t].padIndex = defaultMap[t]; 
            patterns[p].tracks[t].muted = false;
            for(int s=0; s<16; s++) {
                patterns[p].tracks[t].steps[s] = {false, 100, 100, 0, 0};
            }
        }
    }
}


void clockSequencer() {
    if(!sequencerPlaying) return;
    
    unsigned long now = micros();
    // BPM = Beats Per Minute. 1 Beat = 1 Quarter Note.
    // Standard sequencer usually does 16th notes (4 steps per beat).
    // Steps Per Minute = BPM * 4.
    // Steps Per Second = (BPM * 4) / 60.
    // Microseconds per step = 60000000 / (BPM * 4) = 15000000 / BPM.
    
    unsigned long micPerStep = 15000000 / bpm;
    
    if (now - lastStepTime >= micPerStep) {
        lastStepTime = now;
        
        // Trigger Steps
        Pattern& p = patterns[currentPattern];
        
        // For each track
        for(int t=0; t<8; t++) {
            Track& trk = p.tracks[t];
            if(trk.muted) continue;
            
            Step& s = trk.steps[currentStep];
            if(s.active) {
                // Check probability
                if(s.probability >= 100 || (random(0, 100) < s.probability)) {
                    // Apply Filter Offset
                    // Map -127..127 to -1.0..1.0 approximately?
                    // Filter Cutoff is 0.0-1.0. Let's map full range.
                    seqFilterMod = (float)s.filterOffset / 128.0f; 
                    
                    // Trigger
                    triggerNote(trk.padIndex, s.pitchOffset); 
                }
            } else {
                 // Even if inactive, maybe reset filter mod if step is empty?
                 // Or keep previous? Usually step locks only apply on trigger.
                 // Let's reset to 0 if no trigger? No, that would cause glitchy jumping.
                 // Ideally it glides or stays. For now, reset to 0 if we want default,
                 // or keep it if we want 'sample and hold' style.
                 // Let's reset to 0 for cleaner behavior on empty steps.
                 seqFilterMod = 0.0f;
            }
        }
        
        // Advance
        currentStep++;
        if(currentStep >= p.length) currentStep = 0;
    }
}

void handleSequencerInput(int keyIndex, bool pressed) {
    if(!pressed) return;
    
    // Check Shift (F1)
    bool shift = (digitalRead(BTN_OCTAVE) == LOW);
    
    if (shift) {
        globalShiftUsed = true; // Mark as used for combo
        
        // Shift + Pad logic
        if (keyIndex >= 0 && keyIndex < 8) {
            // Select Track
            seqActiveTrack = keyIndex;

            // Hint UI?
        } else if (keyIndex >= 8) {
            // BPM Control?
            // Use Pad 15/16 for BPM -/+
            if (keyIndex == 14) bpm = max(60, bpm - 5);
            if (keyIndex == 15) bpm = min(180, bpm + 5);
        }
    } else {
        // Toggle Step
        Pattern& p = patterns[currentPattern];
        Track& trk = p.tracks[seqActiveTrack];
        
        // Toggling active state
        trk.steps[keyIndex].active = !trk.steps[keyIndex].active;
        
        // Preview note when adding?
        if(trk.steps[keyIndex].active) {
            triggerNote(trk.padIndex, 0);
        }
    }
    }


void handleFXInput(int keyIndex, bool pressed) {
    if(!pressed) return;
    
    // Pads 1-4 (0-3): Select Effect
    if(keyIndex < 4) {
        selectedFX = keyIndex;
        const char* fxNames[] = {"BIT CRUSH", "SAMPLERATE", "FILTER", "DELAY"};
        ui.showParameter(fxNames[selectedFX], 0, 0, 0); // Just show name
        return;
    }
    
    // Pads 5-16 (4-15): Adjust Parameter (12 steps)
    // Map 4-15 -> 0-11
    int valIndex = keyIndex - 4;
    float paramVal = (float)valIndex / 11.0f; // 0.0 to 1.0
    
    if(selectedFX == 0) { // Bit Crush
        bitCrushEnabled = true;
        bitCrushDepth = 12 - valIndex; // 12 down to 1?
        if (bitCrushDepth < 1) bitCrushDepth = 1;
        ui.showParameter("BIT DEPTH", bitCrushDepth, 1, 16);
    } else if (selectedFX == 1) { // Sample Rate
        sampleRateReductionEnabled = true;
        srReductionFactor = valIndex + 1;
        if(srReductionFactor == 1) sampleRateReductionEnabled = false;
        ui.showParameter("SR FACTOR", srReductionFactor, 1, 12);
    } else if (selectedFX == 2) { // Filter
        filterEnabled = true;
        // Cutoff
        filterCutoff = paramVal;
        ui.showParameter("CUTOFF", filterCutoff * 100, 0, 100, 0, "%");
    } else if (selectedFX == 3) { // Delay
        delayEnabled = true;
        delayMix = (int)(paramVal * 100);
        ui.showParameter("DELAY MIX", delayMix, 0, 100, 0, "%");
    }
}
