#ifndef SEQUENCER_H
#define SEQUENCER_H

#include <Arduino.h>
#include "Config.h"
#include "AudioEngine.h"

class Sequencer {
public:
    Sequencer(AudioEngine& audio);
    void init();
    void update();
    void start();
    void stop();
    void togglePlay();
    void setBPM(int newBpm);
    int getBPM();
    void setInstrument(int track, Instrument inst);
    Instrument getInstrument(int track);
    void clearTrack(int track);
    
    // Grid interaction
    void toggleStep(int track, int step);
    bool getStep(int track, int step);
    
    // State Accessors for UI
    int getCurrentStep();
    int getCurrentTrack();
    void setCurrentTrack(int track);
    int getCurrentOctave();
    void setCurrentOctave(int oct);
    bool isPlayingState();
    
    // Data
    // Making these public for easier UI access for now (or write comprehensive getters)
    bool sequence[4][16];
    Instrument trackInstruments[4];

private:
    AudioEngine& audioEngine;
    int bpm;
    bool isPlaying;
    int currentStep;
    int currentTrack;
    int currentOctave;
    unsigned long lastStepTime;
    
    uint8_t stepNotes[4][16];
};

#endif
