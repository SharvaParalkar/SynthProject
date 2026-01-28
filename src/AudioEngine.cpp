#include "AudioEngine.h"
#include <math.h>

// Constants
#ifndef PI
#define PI 3.14159265358979323846
#endif

AudioEngine::AudioEngine() {
    for (int i = 0; i < POLYPHONY; i++) {
        voices[i].active = false;
        voices[i].releasing = false;
        voices[i].phase = 0.0f;
    }
}

void AudioEngine::init() {
    // Nothing specific yet
}

void AudioEngine::setVolume(int vol) {
    if (vol < 0) vol = 0;
    if (vol > 100) vol = 100;
    masterVolume = vol / 100.0f;
}

int AudioEngine::getVolume() {
    return (int)(masterVolume * 100);
}

// Simple visualizer: Return sum of active voice amplitudes (scaled)
float AudioEngine::getVisualizerLevel() {
    float sum = 0.0f;
    for (int i = 0; i < POLYPHONY; i++) {
        if (voices[i].active) {
            sum += voices[i].envelope; // Envelope tracks ADSR
        }
    }
    // Cap at 1.0 roughly
    if (sum > 1.0f) sum = 1.0f;
    return sum;
}

void AudioEngine::generate(int16_t* buffer, int samples) {
    for (int i = 0; i < samples; i++) {
        float sampleMix = 0.0f;
        int activeVoices = 0;
        
        for (int v = 0; v < POLYPHONY; v++) {
            if (voices[v].active) {
                sampleMix += generateWaveform(voices[v]);
                activeVoices++;
            }
        }
        
        // Simple Limiter / Mixer
        if (activeVoices > 0) {
            sampleMix /= (float)sqrt((float)activeVoices); // Soft scaling
        }
        
        // Master Volume
        sampleMix *= masterVolume;
        
        // DC Blocker (Simple High Pass)
        // y[n] = x[n] - x[n-1] + R * y[n-1]
        // Process Left/Right (Here mono source mixed to stereo)
        float x = sampleMix;
        float y_L = x - prevX_L + 0.995f * prevY_L;
        prevX_L = x;
        prevY_L = y_L;
        
        // Duplicate for Right for now (since source is mono)
        float y_R = y_L; 
        
        // Clipping
        if (y_L > 1.0f) y_L = 1.0f;
        if (y_L < -1.0f) y_L = -1.0f;
        
        // Interleaved Stereo (Right/Left)
        // Note: Some DACs expect Left/Right, some Right/Left.
        // Code here writes i*2 and i*2+1.
        buffer[i*2] = (int16_t)(y_R * 30000.0f);     
        buffer[i*2+1] = (int16_t)(y_L * 30000.0f); 
        
        // Update Visualizer Buffer (Just keep the last 128 samples of the frame)
        // This is a simple circular-ish fill or just overwrite if we are fast enough
        // To keep it stable, we might want to just copy the end of the buffer?
        // Let's just write to it with a static index to "scroll" it or just fill it.
        // For a simple oscilloscope, we often want to fill it continuously.
        static int visIdx = 0;
        visualizerBuffer[visIdx] = y_L; // Use Mono/Left signal
        visIdx = (visIdx + 1) % 128;
    }
}

void AudioEngine::noteOn(int note, Instrument inst) {
    // Check if note is already playing?
    for (int i = 0; i < POLYPHONY; i++) {
        if (voices[i].active && voices[i].note == note) {
            // Re-trigger
            voices[i].releasing = false;
            voices[i].envelope = 1.0f;
            voices[i].phase = 0.0f;
            return;
        }
    }

    int v = findFreeVoice();
    if (v != -1) {
        voices[v].active = true;
        voices[v].releasing = false;
        voices[v].note = note;
        voices[v].frequency = midiToFreq(note);
        voices[v].phase = 0.0f;
        voices[v].instrument = inst;
        voices[v].sampleCount = 0;
        voices[v].envelope = 1.0f; 
        voices[v].currentInc = voices[v].frequency / SAMPLE_RATE;
    }
}

void AudioEngine::noteOff(int note) {
    for (int i = 0; i < POLYPHONY; i++) {
        if (voices[i].active && voices[i].note == note) {
            voices[i].releasing = true; // Trigger release phase
        }
    }
}

void AudioEngine::killAll() {
    for (int i = 0; i < POLYPHONY; i++) {
        voices[i].active = false;
        voices[i].releasing = false;
    }
}

int AudioEngine::getActiveVoiceCount() {
    int c = 0;
    for (int i = 0; i < POLYPHONY; i++) {
        if (voices[i].active) c++;
    }
    return c;
}

float AudioEngine::midiToFreq(int note) {
    return 440.0f * pow(2.0f, (note - 69) / 12.0f);
}

int AudioEngine::findFreeVoice() {
    // First try to find non-active
    for (int i = 0; i < POLYPHONY; i++) {
        if (!voices[i].active) return i;
    }
    
    // If all active, find one that is releasing
    for (int i = 0; i < POLYPHONY; i++) {
        if (voices[i].releasing) return i;
    }
    
    // Steal oldest (lowest envelope?)
    return -1;
}

float AudioEngine::generateWaveform(Voice& voice) {
    float out = 0.0f;
    float phase = voice.phase;
    float inc = voice.currentInc;
    
    switch (voice.instrument) {
        case INST_SINE:
            out = sinf(2.0f * PI * phase);
            break;
        case INST_SQUARE:
            out = (phase < 0.5f) ? 1.0f : -1.0f;
            out += poly_blep(phase, inc);
            out -= poly_blep(fmod(phase + 0.5f, 1.0f), inc);
            break;
        case INST_SAW:
            out = (2.0f * phase) - 1.0f;
            out -= poly_blep(phase, inc);
            break;
        case INST_TRIANGLE:
            out = (phase < 0.5f) ? (4.0f * phase - 1.0f) : (3.0f - 4.0f * phase);
            break;
        default: 
            out = sinf(2.0f * PI * phase); 
            if (voice.instrument == INST_BASS) {
                 out = (phase < 0.5f) ? 1.0f : -1.0f; 
            }
            break;
    }
    
    // Envelope Logic
    if (voice.releasing) {
        // Fast Release
        voice.envelope *= 0.9f;
        if (voice.envelope < 0.001f) {
            voice.active = false;
            return 0.0f;
        }
    } else {
        // Attack/Sustain/Decay
        if (voice.instrument == INST_PLUCK) {
            voice.envelope *= 0.9999f;
            if (voice.envelope < 0.001f) voice.active = false;
        }
        // Others sustain at 1.0f
    }
    
    out *= voice.envelope;
    
    voice.sampleCount++;
    
    voice.phase += inc;
    if (voice.phase >= 1.0f) voice.phase -= 1.0f;
    
    return out;
}

// PolyBLEP for antialiasing
double AudioEngine::poly_blep(double t, double dt) {
    if (t < dt) {
        t /= dt;
        return t+t - t*t - 1.0;
    } else if (t > 1.0 - dt) {
        t = (t - 1.0) / dt;
        return t*t + t+t + 1.0;
    }
    return 0.0;
}
