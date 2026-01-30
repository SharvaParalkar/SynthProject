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
        voices[i].attacking = false;
        voices[i].attackEnv = 0.0f;
        voices[i].fadeOutSamples = 0;
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

void AudioEngine::setFilterCutoff(float cutoff) {
    filterCutoff = constrain(cutoff, 0.0f, 1.0f);
}

float AudioEngine::getFilterCutoff() {
    return filterCutoff;
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

void AudioEngine::generate(int32_t* buffer, int samples) {
    for (int i = 0; i < samples; i++) {
        float sampleMix = 0.0f;
        int activeVoices = 0;
        
        for (int v = 0; v < POLYPHONY; v++) {
            if (voices[v].active || voices[v].fadeOutSamples > 0) {
                sampleMix += generateWaveform(voices[v]);
                if (voices[v].active) activeVoices++;
            }
        }
        
        // Soft Limiter / Mixer with headroom
        if (activeVoices > 1) {
            sampleMix /= (1.0f + 0.3f * (activeVoices - 1)); // Gentler scaling
        }
        
        // Master Volume (with slight headroom)
        sampleMix *= masterVolume * 0.85f;

        // Apply Low-Pass Filter (smoother cutoff response)
        // Map 0-1 cutoff to useful frequency range
        float lpfCoeff = 0.1f + filterCutoff * 0.85f;
        lpf_state = lpf_state + lpfCoeff * (sampleMix - lpf_state);
        sampleMix = lpf_state;
        
        // Gentler DC Blocker (reduces droning)
        // Higher coefficient = less bass cut, less artifacts
        const float dcBlockerCoeff = 0.9995f;
        float x = sampleMix;
        float y = x - prevX_L + dcBlockerCoeff * prevY_L;
        prevX_L = x;
        prevY_L = y;
        
        // Soft Clipping (tanh-like, warmer than hard clip)
        if (y > 0.8f) {
            y = 0.8f + 0.2f * tanhf((y - 0.8f) * 5.0f);
        } else if (y < -0.8f) {
            y = -0.8f + 0.2f * tanhf((y + 0.8f) * 5.0f);
        }
        
        // Final hard limit
        if (y > 1.0f) y = 1.0f;
        if (y < -1.0f) y = -1.0f;
        
        // =========================================================================
        // 32-BIT OUTPUT FOR PCM5102A DAC
        // =========================================================================
        // PCM5102A expects 32-bit I2S frames with 24-bit audio data left-justified.
        // We scale to full int32_t range for maximum dynamic range.
        // Using 2147483647 (2^31 - 1) for positive, 2147483648 for negative.
        int32_t sample32 = (int32_t)(y * 2147483647.0f);
        
        buffer[i*2] = sample32;     // Left
        buffer[i*2+1] = sample32;   // Right
        
        // Update Visualizer Buffer
        static int visIdx = 0;
        visualizerBuffer[visIdx] = y;
        visIdx = (visIdx + 1) % 128;
    }
}

void AudioEngine::noteOn(int note, Instrument inst) {
    // Check if note is already playing?
    for (int i = 0; i < POLYPHONY; i++) {
        if (voices[i].active && voices[i].note == note) {
            // Re-trigger with soft restart
            voices[i].releasing = false;
            voices[i].attacking = true;
            voices[i].attackEnv = voices[i].envelope * voices[i].attackEnv; // Maintain current level
            if (voices[i].attackEnv < 0.01f) voices[i].attackEnv = 0.01f;
            return;
        }
    }

    int v = findFreeVoice();
    if (v != -1) {
        // If voice was active, set up crossfade
        if (voices[v].active) {
            voices[v].fadeOutSamples = 128; // Quick crossfade
        }
        
        voices[v].active = true;
        voices[v].releasing = false;
        voices[v].note = note;
        voices[v].frequency = midiToFreq(note);
        voices[v].phase = 0.0f;
        voices[v].instrument = inst;
        voices[v].sampleCount = 0;
        voices[v].envelope = 1.0f;
        
        // Start with attack envelope (prevents pops)
        voices[v].attacking = true;
        voices[v].attackEnv = 0.0f;
        
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
        voices[i].fadeOutSamples = 0;
    }
    // Reset filter state to prevent DC offset droning after silence
    resetFilterState();
}

void AudioEngine::resetFilterState() {
    lpf_state = 0.0f;
    prevX_L = 0.0f;
    prevY_L = 0.0f;
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
    
    // Steal oldest (lowest envelope)
    int oldest = 0;
    float minEnv = voices[0].envelope;
    for (int i = 1; i < POLYPHONY; i++) {
        if (voices[i].envelope < minEnv) {
            minEnv = voices[i].envelope;
            oldest = i;
        }
    }
    return oldest;
}

float AudioEngine::generateWaveform(Voice& voice) {
    // Handle fade-out for stolen voices
    if (voice.fadeOutSamples > 0) {
        voice.fadeOutSamples--;
        // Just fade out the old sample
        float fadeGain = (float)voice.fadeOutSamples / 128.0f;
        return 0.0f; // Old voice silences quickly
    }
    
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
        case INST_PLUCK:
            // Pluck: Starts bright, becomes more sine-like
            {
                float brightness = voice.envelope;
                float saw = (2.0f * phase) - 1.0f;
                saw -= poly_blep(phase, inc);
                float sine = sinf(2.0f * PI * phase);
                out = brightness * saw + (1.0f - brightness) * sine;
            }
            break;
        case INST_BASS:
            // Bass: Sub-octave + slight saw
            {
                float sub = sinf(PI * phase); // Sub octave (half freq)
                float saw = (2.0f * phase) - 1.0f;
                out = 0.7f * sub + 0.3f * saw;
            }
            break;
        case INST_PAD:
            // Pad: Detuned sines for chorus effect
            {
                float p1 = sinf(2.0f * PI * phase);
                float p2 = sinf(2.0f * PI * phase * 1.003f);
                float p3 = sinf(2.0f * PI * phase * 0.997f);
                out = (p1 + p2 + p3) / 3.0f;
            }
            break;
        case INST_LEAD:
            // Lead: Pulse wave with slight PWM feel
            {
                float pw = 0.3f; // Pulse width
                out = (phase < pw) ? 1.0f : -1.0f;
                out += poly_blep(phase, inc);
                out -= poly_blep(fmod(phase + (1.0f - pw), 1.0f), inc);
            }
            break;
        default: 
            out = sinf(2.0f * PI * phase);
            break;
    }
    
    // Attack Envelope (prevents pops on note start)
    if (voice.attacking) {
        voice.attackEnv += 0.002f; // ~23ms attack at 44.1kHz
        if (voice.attackEnv >= 1.0f) {
            voice.attackEnv = 1.0f;
            voice.attacking = false;
        }
        out *= voice.attackEnv;
    }
    
    // Release Envelope
    if (voice.releasing) {
        // Faster release for cleaner cutoff
        voice.envelope *= 0.9985f; // ~150ms release
        if (voice.envelope < 0.001f) {
            voice.active = false;
            voice.fadeOutSamples = 0;
            return 0.0f;
        }
    } else {
        // Decay for pluck-style instruments
        if (voice.instrument == INST_PLUCK) {
            voice.envelope *= 0.99985f; // Natural decay
            if (voice.envelope < 0.001f) {
                voice.active = false;
                return 0.0f;
            }
        }
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
