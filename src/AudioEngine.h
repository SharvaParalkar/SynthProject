#ifndef AUDIO_ENGINE_H
#define AUDIO_ENGINE_H

#include <Arduino.h>
#include "Config.h"

struct Voice {
    float frequency;
    int note;
    float amplitude;
    bool active;
    bool releasing;
    Instrument instrument;
    float envelope;
    bool attacking;
    float attackEnv;
};

class AudioEngine {
public:
    AudioEngine();
    void init();
    /// Called from loop() - fills buffer via play() and writes to I2S (AudioTools + Maximilian)
    void copy();

    void noteOn(int note, Instrument inst);
    void noteOff(int note);
    void killAll();
    int getActiveVoiceCount();

    void setVolume(int vol); // 0-100
    int getVolume();
    float getVisualizerLevel();

    void setFilterCutoff(float cutoff); // 0.0-1.0
    float getFilterCutoff();

    /// Called once per stereo sample by Maximilian (audio rate) - pure DSP, no I/O
    void playCallback(float* channels);

    const float* getWaveform() { return visualizerBuffer; }
    /// Ring-buffer write head: UI should read (getWaveformRingIndex() + i) % 128 for time order
    int getWaveformRingIndex() const { return visualizerIdx; }

private:
    Voice voices[POLYPHONY];
    float midiToFreq(int note);
    int findFreeVoice();

    float masterVolume;
    float filterCutoff;

    float visualizerBuffer[128];
    int visualizerIdx;

    float lpf_state;
    void resetFilterState();
};

#endif
