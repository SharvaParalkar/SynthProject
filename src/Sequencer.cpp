#include "Sequencer.h"
#include <Preferences.h>

Preferences prefs;

Sequencer::Sequencer(AudioEngine& audio) : audioEngine(audio) {
    bpm = 120;
    isPlaying = false;
    currentStep = 0;
    currentTrack = 0;
    currentOctave = 4;
    lastStepTime = 0;
    
    // Default Settings
    swingAmount = 0; // 0%
    gateLength = 0.8f; // 80% duration
}

void Sequencer::init() {
    memset(sequence, 0, sizeof(sequence));
    memset(stepNotes, 0, sizeof(stepNotes));
    
    trackInstruments[0] = INST_SINE;
    trackInstruments[1] = INST_SQUARE;
    trackInstruments[2] = INST_SAW;
    trackInstruments[3] = INST_TRIANGLE;

    for(int i=0; i<4; i++) activeStepNotes[i] = -1;

    // Load last pattern on startup?
    // loadPattern(0);
}

void Sequencer::savePattern(int patternNum) {
    prefs.begin("synth", false);
    char key[16];
    // Save Sequence
    for (int t = 0; t < 4; t++) {
        sprintf(key, "p%d_t%d", patternNum, t);
        prefs.putBytes(key, sequence[t], 16);
        
        // Save Notes
        sprintf(key, "n%d_t%d", patternNum, t);
        prefs.putBytes(key, stepNotes[t], 16);
    }
    prefs.end();
}

void Sequencer::loadPattern(int patternNum) {
    prefs.begin("synth", true);
    char key[16];
    for (int t = 0; t < 4; t++) {
        sprintf(key, "p%d_t%d", patternNum, t);
        prefs.getBytes(key, sequence[t], 16);
        
        sprintf(key, "n%d_t%d", patternNum, t);
        prefs.getBytes(key, stepNotes[t], 16);
    }
    prefs.end();
}

void Sequencer::update() {
    if (!isPlaying) return;
    
    unsigned long baseStepDuration = (60000 / bpm) / 4;
    unsigned long currentDuration = baseStepDuration;

    // Swing Logic
    // Even steps (0, 2...) are longer, Odd steps are shorter?
    // Or delay odd steps.
    // Standard swing: 1st 8th note is longer, 2nd is shorter.
    // If swingAmount is 0-100% of the second note's duration?
    // Let's use simple delay offset logic.
    long swingOffset = 0;
    if (swingAmount > 0) {
        swingOffset = (long)(baseStepDuration * (swingAmount / 100.0f) * 0.5f); 
    }
    
    // If Step is Even (0, 2...), duration = base + offset
    // If Step is Odd (1, 3...), duration = base - offset
    if (currentStep % 2 == 0) currentDuration = baseStepDuration + swingOffset;
    else currentDuration = baseStepDuration - swingOffset;

    unsigned long now = millis();
    
    // Gate/Note Off Check
    // We need to turn off notes from the *current* step if they exceeded gate length
    // This is tricky without per-step note tracking. 
    // Simplified: Turn off *all* active notes if we are at gate% of duration?
    // Better: Just use `noteOff` for the notes played at `lastStepTime`.
    static bool gateOpen = false;
    
    if (gateOpen && (now - lastStepTime >= (currentDuration * gateLength))) {
        // Close Gate
         for (int track = 0; track < 4; track++) {
             // We don't track exactly which note was played easily without extra state.
             // But we can guess or tracking it.
             // Let's just turn off the note associated with the current step's settings?
             // Or rely on AudioEngine taking care of it?
             // Issue: If we changed pitch, `stepNotes` might be different?
             // Let's track `activeStepNotes[4]`
             if (activeStepNotes[track] != -1) {
                 audioEngine.noteOff(activeStepNotes[track]);
                 activeStepNotes[track] = -1;
             }
         }
         gateOpen = false;
    }

    if (now - lastStepTime >= currentDuration) {
        // Next Step
        currentStep = (currentStep + 1) % 16;
        lastStepTime = now;
        gateOpen = true; 
        
        for (int track = 0; track < 4; track++) {
            activeStepNotes[track] = -1; // Reset
            if (sequence[track][currentStep]) {
                int note = stepNotes[track][currentStep];
                audioEngine.noteOn(note, trackInstruments[track]);
                activeStepNotes[track] = note;
            }
        }
    }
}

void Sequencer::start() {
    isPlaying = true;
    currentStep = 15; 
    lastStepTime = millis(); 
}

void Sequencer::stop() {
    isPlaying = false;
    currentStep = 0;
    audioEngine.killAll(); 
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

void Sequencer::setSwing(int amount) {
    swingAmount = constrain(amount, 0, 100);
}

int Sequencer::getSwing() {
    return swingAmount;
}

void Sequencer::setGate(float length) {
    gateLength = constrain(length, 0.0f, 1.0f);
}

float Sequencer::getGate() {
    return gateLength;
}

