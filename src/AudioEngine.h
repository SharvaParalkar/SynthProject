#ifndef AUDIO_ENGINE_H
#define AUDIO_ENGINE_H

#include <Arduino.h>
#include "Config.h"

struct Voice {
    float phase;
    float frequency;
    int note; // Track MIDI note for re-triggering logic
    float amplitude;
    bool active;
    Instrument instrument;
    uint32_t sampleCount;
    float envelope;
    
    // For PolyBLEP
    float currentInc;
};

class AudioEngine {
public:
    AudioEngine();
    void init();
    void generate(int16_t* buffer, int samples);
    void noteOn(int note, Instrument inst);
    void noteOff(int note);
    void killAll();
    int getActiveVoiceCount();
    
private:
    Voice voices[POLYPHONY];
    float midiToFreq(int note);
    int findFreeVoice();
    
    // Synthesis methods
    float generateWaveform(Voice& voice);
    double poly_blep(double t, double dt);
    
    // DC Blocker state (Issue #4)
    float dcAccumulator = 0.0f;
};

#endif
