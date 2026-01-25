#ifndef SEQUENCER_H
#define SEQUENCER_H

#include <Arduino.h>
#include "config.h"
#include "SampleEngine.h"

struct Step {
    uint8_t sampleIndex;  // Which sample to play (0-15, 255 = empty)
    float pitch;          // Pitch offset in semitones (-12 to +12)
    bool active;          // Is this step active?
};

struct Pattern {
    Step steps[STEPS_PER_PATTERN];
    uint8_t length;       // Pattern length (1-16)
};

class Sequencer {
public:
    Sequencer(SampleEngine* engine);
    
    void begin();
    void update();
    
    // Playback control
    void start();
    void stop();
    void pause();
    bool isPlaying() { return playing; }
    
    // Pattern management
    void setCurrentPattern(uint8_t pattern);
    uint8_t getCurrentPattern() { return currentPattern; }
    Pattern* getPattern(uint8_t index) { return (index < MAX_PATTERNS) ? &patterns[index] : nullptr; }
    
    // Step editing
    void setStep(uint8_t pattern, uint8_t step, uint8_t sampleIndex, float pitch = 0.0f);
    void toggleStep(uint8_t pattern, uint8_t step);
    void clearStep(uint8_t pattern, uint8_t step);
    void clearPattern(uint8_t pattern);
    
    // Tempo control
    void setBPM(uint8_t bpm);
    uint8_t getBPM() { return bpm; }
    
    // Pattern chaining
    void setChain(uint8_t* chain, uint8_t length);
    void clearChain();
    bool isChaining() { return chainLength > 0; }
    
    // Get current step (for LED feedback)
    uint8_t getCurrentStep() { return currentStep; }

private:
    SampleEngine* sampleEngine;
    Pattern patterns[MAX_PATTERNS];
    
    bool playing;
    uint8_t currentPattern;
    uint8_t currentStep;
    uint8_t bpm;
    
    // Timing
    unsigned long lastStepTime;
    unsigned long stepInterval; // microseconds per step
    
    // Pattern chaining
    uint8_t chain[MAX_PATTERNS];
    uint8_t chainLength;
    uint8_t chainPosition;
    
    void calculateStepInterval();
    void triggerStep();
    void advanceStep();
};

#endif // SEQUENCER_H
