#include "Sequencer.h"

Sequencer::Sequencer(SampleEngine* engine) : sampleEngine(engine) {
    playing = false;
    currentPattern = 0;
    currentStep = 0;
    bpm = DEFAULT_BPM;
    lastStepTime = 0;
    stepInterval = 0;
    chainLength = 0;
    chainPosition = 0;
    
    // Initialize all patterns
    for (int p = 0; p < MAX_PATTERNS; p++) {
        patterns[p].length = STEPS_PER_PATTERN;
        for (int s = 0; s < STEPS_PER_PATTERN; s++) {
            patterns[p].steps[s].sampleIndex = 255; // Empty
            patterns[p].steps[s].pitch = 0.0f;
            patterns[p].steps[s].active = false;
        }
    }
}

void Sequencer::begin() {
    calculateStepInterval();
}

void Sequencer::calculateStepInterval() {
    // Calculate microseconds per step
    // At 120 BPM: 60 seconds / 120 beats = 0.5 seconds per beat
    // 16 steps per beat = 0.5 / 16 = 0.03125 seconds = 31250 microseconds
    float beatsPerSecond = bpm / 60.0f;
    float stepsPerSecond = beatsPerSecond * 4.0f; // 16 steps = 4 beats
    stepInterval = (unsigned long)(1000000.0f / stepsPerSecond);
}

void Sequencer::start() {
    playing = true;
    currentStep = 0;
    lastStepTime = micros();
    triggerStep();
}

void Sequencer::stop() {
    playing = false;
    currentStep = 0;
    chainPosition = 0;
    sampleEngine->releaseAll();
}

void Sequencer::pause() {
    playing = false;
}

void Sequencer::setBPM(uint8_t newBPM) {
    bpm = constrain(newBPM, MIN_BPM, MAX_BPM);
    calculateStepInterval();
}

void Sequencer::setCurrentPattern(uint8_t pattern) {
    if (pattern < MAX_PATTERNS) {
        currentPattern = pattern;
    }
}

void Sequencer::setStep(uint8_t pattern, uint8_t step, uint8_t sampleIndex, float pitch) {
    if (pattern >= MAX_PATTERNS || step >= STEPS_PER_PATTERN) return;
    
    Step* s = &patterns[pattern].steps[step];
    s->sampleIndex = sampleIndex;
    s->pitch = constrain(pitch, -12.0f, 12.0f);
    s->active = (sampleIndex < MAX_SAMPLES);
}

void Sequencer::toggleStep(uint8_t pattern, uint8_t step) {
    if (pattern >= MAX_PATTERNS || step >= STEPS_PER_PATTERN) return;
    
    Step* s = &patterns[pattern].steps[step];
    s->active = !s->active;
}

void Sequencer::clearStep(uint8_t pattern, uint8_t step) {
    if (pattern >= MAX_PATTERNS || step >= STEPS_PER_PATTERN) return;
    
    Step* s = &patterns[pattern].steps[step];
    s->sampleIndex = 255;
    s->active = false;
}

void Sequencer::clearPattern(uint8_t pattern) {
    if (pattern >= MAX_PATTERNS) return;
    
    for (int s = 0; s < STEPS_PER_PATTERN; s++) {
        clearStep(pattern, s);
    }
}

void Sequencer::setChain(uint8_t* chainArray, uint8_t length) {
    chainLength = min(length, (uint8_t)MAX_PATTERNS);
    for (int i = 0; i < chainLength; i++) {
        chain[i] = chainArray[i];
    }
    chainPosition = 0;
}

void Sequencer::clearChain() {
    chainLength = 0;
    chainPosition = 0;
}

void Sequencer::triggerStep() {
    Pattern* pattern = &patterns[currentPattern];
    Step* step = &pattern->steps[currentStep];
    
    if (step->active && step->sampleIndex < MAX_SAMPLES) {
        sampleEngine->triggerSample(step->sampleIndex, step->pitch);
    }
}

void Sequencer::advanceStep() {
    currentStep++;
    
    // Check if pattern finished
    if (currentStep >= patterns[currentPattern].length) {
        currentStep = 0;
        
        // Handle pattern chaining
        if (chainLength > 0) {
            chainPosition++;
            if (chainPosition >= chainLength) {
                chainPosition = 0;
            }
            currentPattern = chain[chainPosition];
        }
    }
}

void Sequencer::update() {
    if (!playing) return;
    
    unsigned long now = micros();
    if (now - lastStepTime >= stepInterval) {
        lastStepTime = now;
        
        // Trigger current step
        triggerStep();
        
        // Advance to next step
        advanceStep();
    }
}
