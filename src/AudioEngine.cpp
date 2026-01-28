#include "AudioEngine.h"
#include <math.h>

AudioEngine::AudioEngine() {
}

void AudioEngine::init() {
    for (int i = 0; i < POLYPHONY; i++) {
        voices[i].active = false;
        voices[i].phase = 0;
        voices[i].amplitude = 0;
        voices[i].currentInc = 0;
    }
}

double AudioEngine::poly_blep(double t, double dt) {
    // 0 <= t < 1
    if (t < dt) {
        t /= dt;
        return t + t - t * t - 1.0;
    }
    // -1 < t < 0
    else if (t > 1.0 - dt) {
        t = (t - 1.0) / dt;
        return t * t + t + t + 1.0;
    }
    return 0.0;
}

float AudioEngine::midiToFreq(int note) {
    return 440.0f * powf(2.0f, (note - 69) / 12.0f);
}

int AudioEngine::findFreeVoice() {
    // 1. Find inactive
    for (int i = 0; i < POLYPHONY; i++) {
        if (!voices[i].active) return i;
    }

    // 2. Find releasing (Issue #10: Anti-click will be handled by crossfade in noteOn/Off if generic, 
    // but here we just pick the voice. Logic remains same as original for selection)
    const uint32_t ATTACK_SAMPLES = 441;
    const uint32_t SUSTAIN_SAMPLES = 13230;
    
    int oldest = 0;
    uint32_t oldestCount = 0;
    
    for (int i = 0; i < POLYPHONY; i++) {
       if (voices[i].sampleCount > ATTACK_SAMPLES + SUSTAIN_SAMPLES) return i;
       if (voices[i].sampleCount > oldestCount) {
           oldest = i;
           oldestCount = voices[i].sampleCount;
       }
    }
    return oldest;
}

void AudioEngine::noteOn(int note, Instrument inst) {
    // Check if this note is already playing
    for (int i = 0; i < POLYPHONY; i++) {
        if (voices[i].active && voices[i].note == note) {
            // Re-trigger existing voice
            voices[i].instrument = inst;
            voices[i].sampleCount = 0;
            voices[i].envelope = 0;
            // Optional: Randomize phase to reduce identical-note flagging if retriggered fast?
            // voices[i].phase = 0; 
            return;
        }
    }

    // Otherwise find a new voice
    int v = findFreeVoice();
    
    voices[v].active = true;
    voices[v].note = note;
    voices[v].frequency = midiToFreq(note);
    voices[v].instrument = inst;
    voices[v].sampleCount = 0;
    voices[v].envelope = 0;
    voices[v].phase = 0;
}

void AudioEngine::killAll() {
    for (int i = 0; i < POLYPHONY; i++) {
        voices[i].active = false;
    }
}

int AudioEngine::getActiveVoiceCount() {
    int count = 0;
    for (int i = 0; i < POLYPHONY; i++) {
        if (voices[i].active) count++;
    }
    return count;
}

float AudioEngine::generateWaveform(Voice& voice) {
    float sample = 0;
    float phase = voice.phase;
    float inc = voice.frequency / SAMPLE_RATE;
    
    // PolyBLEP requiring dt = inc
    switch (voice.instrument) {
        case INST_SINE:
            sample = sinf(phase * 2 * PI);
            break;
            
        case INST_SQUARE:
            // Naive: sample = phase < 0.5 ? 1.0 : -1.0;
            // PolyBLEP:
            sample = (phase < 0.5f) ? 1.0f : -1.0f;
            sample += poly_blep(phase, inc);
            sample -= poly_blep(fmod(phase + 0.5f, 1.0f), inc);
            break;
            
        case INST_SAW:
            // Naive: sample = 2.0 * phase - 1.0;
            sample = (2.0f * phase) - 1.0f;
            sample -= poly_blep(phase, inc);
            break;
            
        case INST_TRIANGLE:
            // Naive: sample = phase < 0.5 ? (4.0 * phase - 1.0) : (3.0 - 4.0 * phase);
            // It's harder to aliasing-reduce triangle without integration. 
            // Simple approach: Use naive but maybe smooth it? 
            // Let's stick to naive for Triangle or maybe sin approximation?
            // "Triangle" in synth terms:
            sample = (phase < 0.5f) ? (4.0f * phase - 1.0f) : (3.0f - 4.0f * phase);
            break;
            
        case INST_PLUCK:
             // Damped sine
            sample = sinf(phase * 2 * PI) * expf(-voice.envelope * 3.0f);
            break;
            
        case INST_BASS:
             // Two sines
            sample = sinf(phase * 2 * PI) * 0.7f + sinf(phase * 4 * PI) * 0.3f;
            break;

        case INST_PAD:
            sample = sinf(phase * 2 * PI) * 0.5f + 
                     sinf(phase * 4.01f * PI) * 0.3f +
                     sinf(phase * 8.02f * PI) * 0.2f;
            break;

        case INST_LEAD:
            // Mixed Pulse/Sine
            {
                float pulse = (phase < 0.5f ? 1.0f : -1.0f);
                pulse += poly_blep(phase, inc);
                pulse -= poly_blep(fmod(phase + 0.5f, 1.0f), inc);
                sample = pulse * 0.6f + sinf(phase * 2 * PI) * 0.4f;
            }
            break;
         default:
            sample = 0;
            break;
    }
    
    // Advance phase
    voice.phase += inc;
    if (voice.phase >= 1.0f) {
        voice.phase -= 1.0f;
    }
    
    return sample;
}

void AudioEngine::generate(int16_t* buffer, int samples) {
    const uint32_t ATTACK_SAMPLES = 441;   // 10ms
    const uint32_t SUSTAIN_SAMPLES = 8000;  // Reduced from 13230 (~180ms)
    const uint32_t RELEASE_SAMPLES = 2000;  // Drastically reduced from 8820 (~45ms) to prevent muddy overlaps
    
    int activeVoices = getActiveVoiceCount();
    // Issue #3: Dynamic Gain Compensation
    // If 1 voice, amp=1.0. If 4 voices, amp=0.5. 
    // User suggested: 1/sqrt(N).
    float dynamicGain = 1.0f;
    if (activeVoices > 0) {
        dynamicGain = 1.0f / sqrt((float)activeVoices);
    }
    
    for (int i = 0; i < samples; i++) {
        float mixL = 0;
        float mixR = 0;
        
        for (int v = 0; v < POLYPHONY; v++) {
            if (voices[v].active) {
                float sample = generateWaveform(voices[v]);
                
                // Instrument specific gain (Sines are quiet, others need attenuation)
                float instGain = (voices[v].instrument == INST_SINE) ? 1.0f : 0.6f;

                // Envelope
                // Issue #38 suggests Lookup Table. For now, we stick to calc but optimize if needed.
                // Keeping original logic for now as it's readable.
                float env = 0.0f;
                uint32_t s = voices[v].sampleCount;
                
                if (s < ATTACK_SAMPLES) {
                    env = (float)s / ATTACK_SAMPLES;
                } else if (s < ATTACK_SAMPLES + SUSTAIN_SAMPLES) {
                    env = 1.0f;
                } else if (s < ATTACK_SAMPLES + SUSTAIN_SAMPLES + RELEASE_SAMPLES) {
                    uint32_t r = s - ATTACK_SAMPLES - SUSTAIN_SAMPLES;
                    env = 1.0f - ((float)r / RELEASE_SAMPLES);
                } else {
                    voices[v].active = false;
                    env = 0.0f;
                }
                
                voices[v].envelope = env;
                voices[v].sampleCount++;
                
                sample *= env * dynamicGain * instGain;
                
                mixL += sample;
                mixR += sample;
            }
        }
        
        // Issue #4: DC Blocker
        // output = output - (dcAccumulator += (output - dcAccumulator) * 0.002)
        // High-pass filter at ~20Hz
        // Applying to Mix
        dcAccumulator += (mixL - dcAccumulator) * 0.005f; // approx 20-30Hz at 44k
        mixL -= dcAccumulator;
        mixR -= dcAccumulator; // Assuming Mono for now, or use separate DC states
        
        // Issue #2: Soft Clipping
        // Reduce pre-gain from 1.5 to 1.0 or 0.8
        mixL = tanhf(mixL * 1.0f) * 0.8f; 
        mixR = tanhf(mixR * 1.0f) * 0.8f;
        
        // Issue #9: Rounding
        // int16 range: 32767. 
        // 0.8 * 32767 = 26213.
        // Original code used 8000? That's very low. 
        // If tanh limits to 0.8, max is 0.8.
        // 0.8 * 28000 = 22400. Safe.
        // Using lrintf for rounding.
        buffer[i*2] = (int16_t)lrintf(mixL * 28000.0f);
        buffer[i*2+1] = (int16_t)lrintf(mixR * 28000.0f);
    }
}
