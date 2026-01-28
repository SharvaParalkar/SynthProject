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
    bool releasing;
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
    
    // Volume & Visualizer
    void setVolume(int vol); // 0-100
    int getVolume();
    float getVisualizerLevel(); // 0.0 - 1.0 (Approx amplitude)
    
private:
    Voice voices[POLYPHONY];
    float midiToFreq(int note);
    int findFreeVoice();
    
    // Synthesis methods
    float generateWaveform(Voice& voice);
    double poly_blep(double t, double dt);
    
    // DC Blocker state
    float prevX_L = 0.0f;
    float prevY_L = 0.0f;
    float prevX_R = 0.0f;
    float prevY_R = 0.0f;
    
    float masterVolume = 0.8f;
    
    // Waveform Buffer for UI
    float visualizerBuffer[128];
    
public: 
    const float* getWaveform() { return visualizerBuffer; }
};

#endif
