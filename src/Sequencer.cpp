#include "Sequencer.h"

Sequencer::Sequencer(AudioEngine& audio) : audioEngine(audio) {
    bpm = 120;
    isPlaying = false;
    currentStep = 0;
    currentTrack = 0;
    currentOctave = 4;
    lastStepTime = 0;
}

void Sequencer::init() {
    memset(sequence, 0, sizeof(sequence));
    memset(stepNotes, 0, sizeof(stepNotes));
    
    trackInstruments[0] = INST_SINE;
    trackInstruments[1] = INST_SQUARE;
    trackInstruments[2] = INST_SAW;
    trackInstruments[3] = INST_TRIANGLE;
}

void Sequencer::update() {
    if (!isPlaying) return;
    
    unsigned long stepDuration = (60000 / bpm) / 4;
    unsigned long now = millis();
    
    if (now - lastStepTime >= stepDuration) {
        currentStep = (currentStep + 1) % 16;
        lastStepTime = now;
        
        for (int track = 0; track < 4; track++) {
            if (sequence[track][currentStep]) {
                audioEngine.noteOn(stepNotes[track][currentStep], trackInstruments[track]);
            }
        }
    }
}

void Sequencer::start() {
    isPlaying = true;
    currentStep = 15; // So next update hits 0
    lastStepTime = millis(); // Force immediate update? Or wait.
    // Actually, usually we want immediate start.
    currentStep = -1; // Next is 0
    // But logic above: `if (now - last >= duration)`. 
    // To start immediately, set lastStepTime to past.
    lastStepTime = millis() - (60000 / bpm) / 4; 
}

void Sequencer::stop() {
    isPlaying = false;
    currentStep = 0;
    audioEngine.killAll(); // Optional: silence on stop
}

void Sequencer::togglePlay() {
    if (isPlaying) stop();
    else start();
}

void Sequencer::setBPM(int newBpm) {
    bpm = constrain(newBpm, 60, 240);
}

int Sequencer::getBPM() {
    return bpm;
}

void Sequencer::setInstrument(int track, Instrument inst) {
    if (track >= 0 && track < 4) {
        trackInstruments[track] = inst;
    }
}

Instrument Sequencer::getInstrument(int track) {
    if (track >= 0 && track < 4) return trackInstruments[track];
    return INST_SINE;
}

void Sequencer::clearTrack(int track) {
    if (track >= 0 && track < 4) {
        for (int i=0; i<16; i++) sequence[track][i] = false;
    }
}

void Sequencer::toggleStep(int track, int step) {
    if (track >= 0 && track < 4 && step >= 0 && step < 16) {
        sequence[track][step] = !sequence[track][step];
        if (sequence[track][step]) {
            // Set note with current octave
            // Base C2 = 36. 
            // Pad index logic from main: 36 + padIndex + (oct * 12).
            // Here 'step' is the pad index for the sequence step. 
            // Wait, "Launchpad" mode maps pads to notes. "Sequencer" mode maps pads to steps.
            // The note pitch for a step is usually fixed or set by the user. 
            // Original code: `stepNotes[currentTrack][padIndex] = 36 + padIndex + (currentOctave * 12);`
            // This implies the pitch depends on *which step* button you press?
            // "Sequencer ... Toggle scale in sequence". 
            // "If sequence... set note ... = 36 + padIndex...".
            // Yes, it seems pitch is tied to the step index (making it an arpeggiator?) or just mapping.
            // Actually, usually a sequencer step has a pitch. 
            // If the user presses button 0 (Step 1), it sets the note for Step 1.
            // What note? The code says `36 + padIndex`. So Step 1 is C, Step 2 is C#, etc.?
            // Yes, likely.
            
            stepNotes[track][step] = 36 + step + (currentOctave * 12);
        }
    }
}

bool Sequencer::getStep(int track, int step) {
    if (track >= 0 && track < 4 && step >= 0 && step < 16) {
        return sequence[track][step];
    }
    return false;
}

int Sequencer::getCurrentStep() { return currentStep; }
int Sequencer::getCurrentTrack() { return currentTrack; }
void Sequencer::setCurrentTrack(int track) { currentTrack = track % 4; }
int Sequencer::getCurrentOctave() { return currentOctave; }
void Sequencer::setCurrentOctave(int oct) { currentOctave = constrain(oct, 1, 7); }
bool Sequencer::isPlayingState() { return isPlaying; }
