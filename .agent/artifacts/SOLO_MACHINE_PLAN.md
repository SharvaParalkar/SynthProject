# Solo Music Machine - Implementation Plan

## Executive Summary

Transform the N8R2 synth from a 4-track sequencer with math-generated oscillators into a complete "Solo Music Machine" capable of:
- **Loading samples/wavetables** from internal flash via USB drag-and-drop
- **Professional-grade playback** via PSRAM buffering
- **Song arrangement** via pattern chaining
- **Per-step automation** via parameter locks
- **Polished output** via master DSP effects (delay/reverb)

---

## Current State Analysis

### ✅ What You Have (Foundation)
| Component | Status | Location |
|-----------|--------|----------|
| 4-track sequencer | Working | `Sequencer.cpp` |
| 16-step grid per track | Working | `sequence[4][16]` |
| Basic oscillators (Sine/Square/Saw/Triangle) | Working | `AudioEngine.cpp` via Maximilian |
| 8-voice polyphony | Working | `POLYPHONY = 8` |
| I2S audio output @ 32kHz | Working | PCM5102A DAC |
| Pattern save/load (NVS) | Partial | `Preferences` library |
| Swing & Gate controls | Working | `Sequencer.cpp` |
| Partition table for 8MB | Exists | `partitions.csv` |
| PSRAM declared | Build flag | `-DBOARD_HAS_PSRAM` |

### ❌ What's Missing (The Gap)
| Component | Current State | Required State |
|-----------|---------------|----------------|
| File Storage | Using `const` arrays | LittleFS on flash |
| USB Transfer | None | USB Mass Storage Class |
| Sample Engine | Math oscillators only | WAV/Wavetable playback |
| Physical Modeling | None | Karplus-Strong for plucks |
| PSRAM Usage | Unused | Active audio buffer pool |
| Pattern Chaining | Single loop | Multi-pattern song mode |
| Parameter Locks | Notes only | Per-step metadata |
| Master Effects | Filter only | Delay + Reverb |

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           SOLO MUSIC MACHINE                                 │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  ┌─────────────┐    ┌──────────────┐    ┌─────────────────────────────────┐  │
│  │   USB MSC   │───▶│   LittleFS   │───▶│         PSRAM Loader            │  │
│  │  (PC ↔ ESP) │    │  (8MB Flash) │    │  ┌───────┐ ┌───────┐ ┌───────┐  │  │
│  └─────────────┘    └──────────────┘    │  │ Bank 0│ │ Bank 1│ │ Bank 2│  │  │
│                                         │  │ ~600KB│ │ ~600KB│ │ ~600KB│  │  │
│                                         │  └───────┘ └───────┘ └───────┘  │  │
│                                         └─────────────────────────────────┘  │
│                                                        │                     │
│                           ┌────────────────────────────┘                     │
│                           ▼                                                  │
│  ┌─────────────────────────────────────────────────────────────────────────┐ │
│  │                        SYNTHESIS LAYER                                   │ │
│  │  ┌────────────┐  ┌────────────┐  ┌────────────┐  ┌────────────────────┐ │ │
│  │  │  Wavetable │  │   Sample   │  │  Karplus-  │  │ Classic Oscillators│ │ │
│  │  │   Engine   │  │   Player   │  │   Strong   │  │  (Sine/Saw/Square) │ │ │
│  │  └────────────┘  └────────────┘  └────────────┘  └────────────────────┘ │ │
│  └─────────────────────────────────────────────────────────────────────────┘ │
│                           │                                                  │
│                           ▼                                                  │
│  ┌─────────────────────────────────────────────────────────────────────────┐ │
│  │                        SEQUENCER LAYER                                   │ │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────────────┐  │ │
│  │  │  Pattern Chain  │  │  Parameter Lock │  │    Step Metadata        │  │ │
│  │  │   (Song Mode)   │  │   (Per-Step)    │  │  ┌────┬────┬────┬────┐  │  │ │
│  │  │  P0→P1→P0→P2    │  │  Filter, Vol,   │  │  │Note│Inst│Lock│Vel │  │  │ │
│  │  │                 │  │  Pitch, Pan     │  │  └────┴────┴────┴────┘  │  │ │
│  │  └─────────────────┘  └─────────────────┘  └─────────────────────────┘  │ │
│  └─────────────────────────────────────────────────────────────────────────┘ │
│                           │                                                  │
│                           ▼                                                  │
│  ┌─────────────────────────────────────────────────────────────────────────┐ │
│  │                      MASTER DSP LAYER                                    │ │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐                   │ │
│  │  │    Delay     │  │    Reverb    │  │  Soft Clip   │                   │ │
│  │  │ (PSRAM ring) │  │  (Allpass)   │  │   + Limiter  │                   │ │
│  │  └──────────────┘  └──────────────┘  └──────────────┘                   │ │
│  └─────────────────────────────────────────────────────────────────────────┘ │
│                           │                                                  │
│                           ▼                                                  │
│                    ┌─────────────┐                                           │
│                    │  I2S Output │                                           │
│                    │  PCM5102A   │                                           │
│                    └─────────────┘                                           │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Implementation Milestones

### 🔴 MILESTONE 1: The File System
**Goal:** Enable drag-and-drop file transfer via USB and persistent sample storage.

#### 1.1 Update Partition Table
**File:** `partitions.csv`

Change from SPIFFS to LittleFS layout with explicit OTA support:

```csv
# Name,   Type, SubType, Offset,  Size,   Flags
nvs,      data, nvs,     0x9000,  0x5000,
otadata,  data, ota,     0xe000,  0x2000,
app0,     app,  ota_0,   0x10000, 0x1E0000,
storage,  data, littlefs,0x1F0000,0x600000,
```

**Memory Layout:**
- `nvs`: 20KB for settings/preferences
- `otadata`: 8KB for OTA boot tracking
- `app0`: 1.875MB for firmware
- `storage`: 6MB LittleFS for samples/wavetables

#### 1.2 LittleFS Integration
**New File:** `src/FileSystem.h` / `src/FileSystem.cpp`

```cpp
// FileSystem.h
#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <LittleFS.h>
#include <Arduino.h>

#define FS_MOUNT_POINT "/samples"
#define MAX_SAMPLE_FILES 64

struct SampleFile {
    char filename[48];
    uint32_t size;
    uint8_t type;  // 0=WAV, 1=Wavetable
};

class FileSystemManager {
public:
    bool init();
    void listFiles(SampleFile* files, int* count);
    bool readWavHeader(const char* path, uint32_t* sampleRate, 
                       uint16_t* bitsPerSample, uint32_t* dataSize);
    bool loadSampleToPSRAM(const char* path, int16_t* dest, uint32_t maxSamples);
    uint32_t getFreeSpace();
    uint32_t getTotalSpace();
    
private:
    bool mounted;
};

extern FileSystemManager fileSystem;

#endif
```

#### 1.3 USB Mass Storage Class (MSC)
**New File:** `src/USBStorage.h` / `src/USBStorage.cpp`

This is a complex feature requiring the TinyUSB stack:

```cpp
// USBStorage.h
#ifndef USB_STORAGE_H
#define USB_STORAGE_H

#include <Arduino.h>
#include "USB.h"
#include "USBMSC.h"

class USBStorageManager {
public:
    bool init();
    void setMounted(bool state);
    bool isMounted();
    bool isConnectedToPC();
    
    // Callbacks for TinyUSB
    static int32_t onRead(uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize);
    static int32_t onWrite(uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize);
    static bool onStartStop(uint8_t lun, bool eject, bool load);
    
private:
    static USBMSC msc;
    static bool pcConnected;
};

extern USBStorageManager usbStorage;

#endif
```

**Key Implementation Notes:**
- USB MSC exposes LittleFS partition as a FAT-like block device
- Must unmount LittleFS before enabling MSC mode
- Toggle via button or auto-detect USB connection

---

### 🟡 MILESTONE 2: The Memory Bridge (PSRAM Loader)
**Goal:** Pre-load samples from Flash into PSRAM for glitch-free playback.

#### 2.1 PSRAM Memory Pool
**New File:** `src/MemoryPool.h` / `src/MemoryPool.cpp`

```cpp
// MemoryPool.h
#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <Arduino.h>

// PSRAM Budget: 2MB total
// - Audio Buffers: ~64KB
// - Sample Banks: ~1.8MB (3 banks × 600KB)
// - DSP Buffers: ~128KB (delay/reverb)

#define SAMPLE_BANK_COUNT 3
#define SAMPLE_BANK_SIZE  (600 * 1024)  // 600KB per bank
#define DSP_BUFFER_SIZE   (128 * 1024)  // 128KB for effects

struct SampleBank {
    int16_t* data;           // Pointer into PSRAM
    uint32_t sampleCount;    // Number of 16-bit samples
    uint32_t sampleRate;     // Original sample rate
    char sourcePath[48];     // Source file path
    bool loaded;
};

class MemoryPool {
public:
    bool init();
    
    // Sample Bank Management
    SampleBank* getBank(int index);
    bool loadSampleToBank(int bankIndex, const char* filepath);
    void unloadBank(int bankIndex);
    
    // DSP Buffers
    float* getDelayBuffer();
    float* getReverbBuffer();
    
    uint32_t getFreeBytes();
    uint32_t getTotalBytes();
    
private:
    SampleBank banks[SAMPLE_BANK_COUNT];
    float* delayBuffer;
    float* reverbBuffer;
    bool initialized;
};

extern MemoryPool memoryPool;

#endif
```

#### 2.2 Background Loader Task
**Goal:** Load samples in background without blocking audio.

```cpp
// In MemoryPool.cpp

void loadTaskFunc(void* param) {
    LoadRequest* req = (LoadRequest*)param;
    
    // Open file
    File f = LittleFS.open(req->sourcePath, "r");
    if (!f) {
        req->status = LOAD_FAILED;
        vTaskDelete(NULL);
        return;
    }
    
    // Skip WAV header (44 bytes for standard PCM)
    f.seek(44);
    
    // Read in chunks to avoid blocking
    const int CHUNK_SIZE = 4096;
    int16_t chunk[CHUNK_SIZE / 2];
    uint32_t offset = 0;
    
    while (f.available() && offset < req->maxSamples) {
        int bytesRead = f.read((uint8_t*)chunk, CHUNK_SIZE);
        memcpy(req->destBuffer + offset, chunk, bytesRead);
        offset += bytesRead / 2;
        
        // Yield to prevent watchdog
        vTaskDelay(1);
    }
    
    f.close();
    req->actualSamples = offset;
    req->status = LOAD_COMPLETE;
    vTaskDelete(NULL);
}
```

---

### 🟢 MILESTONE 3: The Synthesis Engines
**Goal:** Add sample playback, wavetable synthesis, and Karplus-Strong.

#### 3.1 Sample Playback Engine
**Extend:** `src/AudioEngine.h` / `src/AudioEngine.cpp`

Add new instrument types and a sample playback engine:

```cpp
// New instrument types in Config.h
enum Instrument {
    // Existing
    INST_SINE,
    INST_SQUARE,
    INST_SAW,
    INST_TRIANGLE,
    INST_PLUCK,
    INST_BASS,
    INST_PAD,
    INST_LEAD,
    
    // NEW: Sample-based
    INST_SAMPLE_0,   // Uses SampleBank 0
    INST_SAMPLE_1,   // Uses SampleBank 1
    INST_SAMPLE_2,   // Uses SampleBank 2
    
    // NEW: Wavetable
    INST_WAVETABLE_0,
    INST_WAVETABLE_1,
    
    // NEW: Physical Modeling
    INST_KARPLUS,
    
    INST_COUNT
};
```

```cpp
// Sample Voice Structure
struct SampleVoice {
    float playhead;        // Current position in samples (fractional for pitch)
    float pitchRatio;      // Playback speed (1.0 = original pitch)
    int16_t* sampleData;   // Pointer to sample in PSRAM
    uint32_t sampleLength; // Length of sample
    bool active;
    bool looping;
};

// In playCallback, add sample playback:
float playSample(SampleVoice& voice) {
    if (!voice.active || !voice.sampleData) return 0.0f;
    
    uint32_t idx0 = (uint32_t)voice.playhead;
    uint32_t idx1 = idx0 + 1;
    float frac = voice.playhead - idx0;
    
    if (idx1 >= voice.sampleLength) {
        if (voice.looping) {
            voice.playhead = 0.0f;
            idx0 = 0; idx1 = 1; frac = 0.0f;
        } else {
            voice.active = false;
            return 0.0f;
        }
    }
    
    // Linear interpolation for smooth pitch shifting
    float s0 = voice.sampleData[idx0] / 32768.0f;
    float s1 = voice.sampleData[idx1] / 32768.0f;
    float out = s0 + frac * (s1 - s0);
    
    voice.playhead += voice.pitchRatio;
    return out;
}
```

#### 3.2 Wavetable Engine
**New File:** `src/Wavetable.h` / `src/Wavetable.cpp`

```cpp
// Wavetable.h
#ifndef WAVETABLE_H
#define WAVETABLE_H

#include <Arduino.h>

#define WAVETABLE_SIZE 2048
#define MAX_WAVETABLES 4

struct Wavetable {
    float data[WAVETABLE_SIZE];
    bool loaded;
};

class WavetableEngine {
public:
    void init();
    bool loadFromPSRAM(int tableIndex, int16_t* source, uint32_t length);
    
    // Oscillator function (like maxiOsc but for wavetables)
    float play(int tableIndex, float frequency, float& phase);
    
    // Morph between two wavetables
    float playMorph(int table1, int table2, float morphAmount, 
                    float frequency, float& phase);
    
private:
    Wavetable tables[MAX_WAVETABLES];
    float sampleRate;
};

extern WavetableEngine wavetableEngine;

#endif
```

#### 3.3 Karplus-Strong Engine (Physical Modeling)
**New File:** `src/KarplusStrong.h` / `src/KarplusStrong.cpp`

```cpp
// KarplusStrong.h
#ifndef KARPLUS_STRONG_H
#define KARPLUS_STRONG_H

#include <Arduino.h>

#define KS_MAX_DELAY 4096  // Supports down to ~10Hz

class KarplusStrongVoice {
public:
    void init();
    void trigger(float frequency, float decay = 0.995f, float brightness = 0.5f);
    void release();
    float process();
    bool isActive();
    
private:
    float buffer[KS_MAX_DELAY];
    int writePtr;
    int readPtr;
    int delayLength;
    float feedback;
    float filterState;
    float filterCoeff;
    bool active;
};

// Pool of Karplus-Strong voices for polyphony
class KarplusStrongEngine {
public:
    void init();
    int noteOn(float frequency, float decay = 0.995f);
    void noteOff(int voiceIndex);
    float processMix();
    
private:
    KarplusStrongVoice voices[4];  // 4 pluck voices max
};

extern KarplusStrongEngine karplusEngine;

#endif
```

**Implementation:**
```cpp
void KarplusStrongVoice::trigger(float frequency, float decay, float brightness) {
    delayLength = (int)(32000.0f / frequency);  // 32kHz sample rate
    if (delayLength > KS_MAX_DELAY) delayLength = KS_MAX_DELAY;
    
    // Initialize buffer with noise burst (the "pluck")
    for (int i = 0; i < delayLength; i++) {
        buffer[i] = random(-32768, 32767) / 32768.0f;
    }
    
    writePtr = 0;
    readPtr = 1;
    feedback = decay;
    filterCoeff = brightness;
    filterState = 0.0f;
    active = true;
}

float KarplusStrongVoice::process() {
    if (!active) return 0.0f;
    
    // Read from delay line
    float out = buffer[readPtr];
    
    // Low-pass filtered feedback (averaging filter)
    float filtered = filterState * (1.0f - filterCoeff) + out * filterCoeff;
    filterState = filtered;
    
    // Write back with feedback
    buffer[writePtr] = filtered * feedback;
    
    // Advance pointers
    writePtr = (writePtr + 1) % delayLength;
    readPtr = (readPtr + 1) % delayLength;
    
    // Detect decay (silence threshold)
    if (fabsf(out) < 0.0001f && fabsf(filtered) < 0.0001f) {
        active = false;
    }
    
    return out;
}
```

---

### 🔵 MILESTONE 4: The Metadata Layer (Advanced Sequencer)
**Goal:** Add pattern chaining and per-step parameter locks.

#### 4.1 Enhanced Data Structures
**Update:** `src/Sequencer.h`

```cpp
// Parameter Lock structure - stores 1 value per lock type per step
struct ParameterLock {
    bool enabled;
    uint8_t value;  // 0-255 normalized value
};

enum LockType {
    LOCK_FILTER_CUTOFF,
    LOCK_FILTER_RESO,
    LOCK_VOLUME,
    LOCK_PITCH,      // Pitch offset in semitones (signed: 128 = 0)
    LOCK_PAN,
    LOCK_DECAY,
    LOCK_COUNT
};

// Enhanced step data
struct StepData {
    bool active;
    uint8_t note;
    uint8_t velocity;
    uint8_t instrumentPatch;     // Which instrument/sample bank
    ParameterLock locks[LOCK_COUNT];
};

// Pattern structure (for chaining)
struct Pattern {
    StepData steps[4][16];       // 4 tracks × 16 steps
    uint8_t length;              // 1-16 steps
    uint8_t nextPattern;         // Index of next pattern (255 = loop self)
};

// Song Chain
#define MAX_PATTERNS 16
#define MAX_CHAIN_LENGTH 64

struct SongChain {
    uint8_t chain[MAX_CHAIN_LENGTH];  // Pattern indices
    uint8_t length;                    // Number of patterns in chain
    bool loop;                         // Loop entire chain?
};
```

#### 4.2 Pattern Chaining (Song Mode)
**Update:** `src/Sequencer.cpp`

```cpp
// Add to Sequencer class
class Sequencer {
public:
    // ... existing ...
    
    // Song Mode
    void setSongMode(bool enabled);
    bool isSongMode();
    void addPatternToChain(uint8_t patternIndex);
    void clearChain();
    void setChainPosition(uint8_t pos);
    uint8_t getChainPosition();
    uint8_t getCurrentPatternIndex();
    
private:
    Pattern patterns[MAX_PATTERNS];
    SongChain songChain;
    bool songMode;
    uint8_t chainPosition;
};

// Song mode update logic
void Sequencer::update() {
    // ... existing step timing logic ...
    
    if (now - lastStepTime >= currentDuration) {
        currentStep++;
        
        // Check for pattern end
        if (currentStep >= patterns[currentPatternIndex].length) {
            currentStep = 0;
            
            if (songMode && songChain.length > 0) {
                // Advance to next pattern in chain
                chainPosition++;
                if (chainPosition >= songChain.length) {
                    chainPosition = songChain.loop ? 0 : songChain.length - 1;
                }
                currentPatternIndex = songChain.chain[chainPosition];
            }
        }
        
        // ... trigger notes with parameter locks ...
    }
}
```

#### 4.3 Parameter Lock Application
```cpp
void Sequencer::triggerStep(int track, int step) {
    StepData& s = patterns[currentPatternIndex].steps[track][step];
    if (!s.active) return;
    
    // Apply parameter locks before triggering note
    if (s.locks[LOCK_FILTER_CUTOFF].enabled) {
        float cutoff = s.locks[LOCK_FILTER_CUTOFF].value / 255.0f;
        audioEngine.setFilterCutoff(cutoff);
    }
    
    if (s.locks[LOCK_VOLUME].enabled) {
        float vol = s.locks[LOCK_VOLUME].value / 255.0f;
        // Apply per-voice volume
    }
    
    if (s.locks[LOCK_PITCH].enabled) {
        int pitchOffset = (int)s.locks[LOCK_PITCH].value - 128;
        // Adjust note by semitones
        int finalNote = s.note + pitchOffset;
        audioEngine.noteOn(finalNote, (Instrument)s.instrumentPatch);
    } else {
        audioEngine.noteOn(s.note, (Instrument)s.instrumentPatch);
    }
}
```

---

### 🟣 MILESTONE 5: Master DSP Effects
**Goal:** Add delay and reverb using PSRAM ring buffers.

#### 5.1 Delay Effect
**New File:** `src/Effects.h` / `src/Effects.cpp`

```cpp
// Effects.h
#ifndef EFFECTS_H
#define EFFECTS_H

#include <Arduino.h>

// Maximum delay time: 32000 samples × 4 = 128KB buffer for stereo
#define MAX_DELAY_SAMPLES 32000

class DelayEffect {
public:
    void init(float* psramBuffer);
    void setTime(float seconds);      // 0.0 - 1.0 (0 to 1 second)
    void setFeedback(float amount);   // 0.0 - 0.95
    void setMix(float amount);        // 0.0 = dry, 1.0 = wet
    float process(float input);
    
private:
    float* buffer;
    int writePtr;
    int readPtr;
    int delaySamples;
    float feedback;
    float mix;
    float lastOutput;
};

class ReverbEffect {
public:
    void init(float* psramBuffer);
    void setDecay(float amount);      // 0.0 - 0.99
    void setDamping(float amount);    // 0.0 - 1.0
    void setMix(float amount);
    float process(float input);
    
private:
    // Schroeder reverb: 4 comb filters + 2 allpass
    struct CombFilter {
        float* buffer;
        int length;
        int ptr;
        float feedback;
        float filterStore;
    };
    
    struct AllpassFilter {
        float* buffer;
        int length;
        int ptr;
        float feedback;
    };
    
    CombFilter combs[4];
    AllpassFilter allpasses[2];
    float decay;
    float damping;
    float mix;
};

extern DelayEffect masterDelay;
extern ReverbEffect masterReverb;

#endif
```

#### 5.2 DSP Chain Integration
**Update:** `src/AudioEngine.cpp`

```cpp
void AudioEngine::playCallback(float* channels) {
    // ... existing voice mixing ...
    
    float dry = sampleMix * masterVolume;
    
    // Apply effects chain
    float delayed = masterDelay.process(dry);
    float reverbed = masterReverb.process(dry + delayed * delayToReverbSend);
    
    // Final mix
    float finalL = dry * (1.0f - effectsWetMix) + 
                   (delayed + reverbed) * effectsWetMix;
    float finalR = finalL;  // Mono for now; stereo enhancement possible
    
    // DC block and soft clip
    // ... existing ...
    
    channels[0] = finalL;
    channels[1] = finalR;
}
```

---

## File Structure After Implementation

```
SynthProject/
├── platformio.ini              (updated with LittleFS, USB)
├── partitions.csv              (updated for LittleFS)
├── src/
│   ├── main.cpp                (updated)
│   ├── Config.h                (expanded instrument enum)
│   ├── AudioEngine.h/.cpp      (sample playback added)
│   ├── Sequencer.h/.cpp        (pattern chains, p-locks)
│   ├── Hardware.h/.cpp         (USB mode switch)
│   ├── UI.h/.cpp               (new menus)
│   │
│   ├── FileSystem.h/.cpp       [NEW] LittleFS management
│   ├── USBStorage.h/.cpp       [NEW] USB MSC driver
│   ├── MemoryPool.h/.cpp       [NEW] PSRAM allocator
│   ├── Wavetable.h/.cpp        [NEW] Wavetable engine
│   ├── KarplusStrong.h/.cpp    [NEW] Physical modeling
│   └── Effects.h/.cpp          [NEW] Delay/Reverb
└── data/                       [NEW] Default samples folder
    ├── kick.wav
    ├── snare.wav
    └── wavetable_saw.wav
```

---

## Implementation Order (Recommended)

### Phase 1: Storage Foundation (Week 1)
1. ✅ Update `partitions.csv` for LittleFS
2. ✅ Implement `FileSystem.h/.cpp`
3. ✅ Test file read/write
4. ✅ Implement basic USB MSC
5. ✅ Test USB drag-and-drop

### Phase 2: PSRAM & Sample Engine (Week 2)
1. ✅ Implement `MemoryPool.h/.cpp`
2. ✅ Test PSRAM allocation
3. ✅ Add sample loading to MemoryPool
4. ✅ Extend AudioEngine for sample playback
5. ✅ Test pitch-shifted sample playback

### Phase 3: Advanced Synthesis (Week 3)
1. ✅ Implement `Wavetable.h/.cpp`
2. ✅ Implement `KarplusStrong.h/.cpp`
3. ✅ Integrate into AudioEngine
4. ✅ Add new instrument types to Config.h
5. ✅ Test pluck sounds

### Phase 4: Sequencer Upgrade (Week 4)
1. ✅ Update data structures for Parameter Locks
2. ✅ Implement Pattern Chaining
3. ✅ Add Song Mode
4. ✅ Update UI for pattern selection
5. ✅ Add p-lock editing UI

### Phase 5: Master Effects (Week 5)
1. ✅ Implement `Effects.h/.cpp`
2. ✅ Allocate PSRAM for effect buffers
3. ✅ Integrate into AudioEngine
4. ✅ Add effects controls to UI
5. ✅ Final polish and testing

---

## Memory Budget Summary

| Component | Internal RAM | PSRAM | Flash |
|-----------|-------------|-------|-------|
| Code + Data | ~200KB | - | ~1.9MB |
| Audio Buffers | ~4KB | ~64KB | - |
| Sample Banks (×3) | - | ~1.8MB | - |
| DSP Buffers | - | ~128KB | - |
| Wavetables | - | ~32KB | - |
| LittleFS Data | - | - | ~5.5MB |
| **Total** | **~204KB** | **~2MB** | **~7.4MB** |

---

## Next Steps

Would you like me to begin implementation with **Milestone 1: The File System**? This involves:

1. Updating `partitions.csv` to use LittleFS instead of SPIFFS
2. Creating `FileSystem.h` and `FileSystem.cpp`
3. Updating `platformio.ini` with LittleFS configuration
4. Creating a test that mounts and lists files

Or if you prefer, I can start with a different milestone. Let me know!
