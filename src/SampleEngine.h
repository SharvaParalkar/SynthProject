#ifndef SAMPLE_ENGINE_H
#define SAMPLE_ENGINE_H

#include <Arduino.h>
#include "config.h"

struct Sample {
    int16_t* data;
    uint32_t length;
    uint32_t startPoint;
    uint32_t endPoint;
    uint32_t sampleRate;
    bool loaded;
};

struct Voice {
    Sample* sample;
    float position;        // Current playback position (in samples)
    float pitch;           // Pitch multiplier (1.0 = original)
    float volume;          // Current volume (0.0 - 1.0)
    bool active;           // Is voice currently playing?
    
    // Envelope
    float attackTime;      // Attack time in seconds
    float releaseTime;     // Release time in seconds
    float envelopePhase;   // 0=attack, 1=sustain, 2=release
    float envelopePos;     // Position within current phase (0-1)
};

class SampleEngine {
public:
    SampleEngine();
    ~SampleEngine();
    
    void begin();
    void render(int16_t* buffer, size_t samples);
    
    // Voice management
    uint8_t triggerSample(uint8_t sampleIndex, float pitch = 1.0f);
    void releaseVoice(uint8_t voiceIndex);
    void releaseAll();
    
    // Sample management
    bool loadSample(uint8_t index, int16_t* data, uint32_t length, uint32_t sampleRate = DEFAULT_SAMPLE_RATE);
    void setSampleTrim(uint8_t index, uint32_t start, uint32_t end);
    void setSamplePitch(uint8_t index, float semitones); // -12 to +12
    
    // Envelope settings
    void setEnvelope(uint8_t voiceIndex, float attack, float release);
    
    // Get sample info
    Sample* getSample(uint8_t index) { return (index < MAX_SAMPLES) ? &samples[index] : nullptr; }
    Voice* getVoice(uint8_t index) { return (index < MAX_VOICES) ? &voices[index] : nullptr; }

private:
    Sample samples[MAX_SAMPLES];
    Voice voices[MAX_VOICES];
    
    float interpolateSample(Sample* sample, float position);
    void updateEnvelope(Voice* voice, float dt);
    uint8_t findFreeVoice();
};

#endif // SAMPLE_ENGINE_H
