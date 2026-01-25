#include "SampleEngine.h"
#include <string.h>
#include <math.h>

SampleEngine::SampleEngine() {
    memset(samples, 0, sizeof(samples));
    memset(voices, 0, sizeof(voices));
}

SampleEngine::~SampleEngine() {
    // Clean up sample data
    for (int i = 0; i < MAX_SAMPLES; i++) {
        if (samples[i].data) {
            free(samples[i].data);
        }
    }
}

void SampleEngine::begin() {
    // Initialize all samples as empty
    for (int i = 0; i < MAX_SAMPLES; i++) {
        samples[i].loaded = false;
        samples[i].data = nullptr;
        samples[i].length = 0;
        samples[i].startPoint = 0;
        samples[i].endPoint = 0;
        samples[i].sampleRate = DEFAULT_SAMPLE_RATE;
    }
    
    // Initialize all voices as inactive
    for (int i = 0; i < MAX_VOICES; i++) {
        voices[i].active = false;
        voices[i].sample = nullptr;
        voices[i].position = 0.0f;
        voices[i].pitch = 1.0f;
        voices[i].volume = 0.0f;
        voices[i].attackTime = 0.01f;  // 10ms attack
        voices[i].releaseTime = 0.1f;   // 100ms release
        voices[i].envelopePhase = 0;
        voices[i].envelopePos = 0.0f;
    }
}

uint8_t SampleEngine::findFreeVoice() {
    for (int i = 0; i < MAX_VOICES; i++) {
        if (!voices[i].active) {
            return i;
        }
    }
    // If all voices busy, steal the oldest one
    return 0;
}

uint8_t SampleEngine::triggerSample(uint8_t sampleIndex, float pitch) {
    if (sampleIndex >= MAX_SAMPLES || !samples[sampleIndex].loaded) {
        return 255; // Invalid
    }
    
    uint8_t voiceIndex = findFreeVoice();
    Voice* voice = &voices[voiceIndex];
    Sample* sample = &samples[sampleIndex];
    
    voice->sample = sample;
    voice->position = (float)sample->startPoint;
    voice->pitch = powf(2.0f, pitch / 12.0f); // Convert semitones to pitch multiplier
    voice->active = true;
    voice->envelopePhase = 0; // Attack
    voice->envelopePos = 0.0f;
    voice->volume = 0.0f;
    
    return voiceIndex;
}

void SampleEngine::releaseVoice(uint8_t voiceIndex) {
    if (voiceIndex >= MAX_VOICES) return;
    
    Voice* voice = &voices[voiceIndex];
    if (voice->active) {
        voice->envelopePhase = 2; // Release
        voice->envelopePos = 0.0f;
    }
}

void SampleEngine::releaseAll() {
    for (int i = 0; i < MAX_VOICES; i++) {
        if (voices[i].active) {
            voices[i].envelopePhase = 2; // Release
            voices[i].envelopePos = 0.0f;
        }
    }
}

bool SampleEngine::loadSample(uint8_t index, int16_t* data, uint32_t length, uint32_t sampleRate) {
    if (index >= MAX_SAMPLES) return false;
    
    Sample* sample = &samples[index];
    
    // Free old data if exists
    if (sample->data) {
        free(sample->data);
    }
    
    // Allocate and copy
    sample->data = (int16_t*)malloc(length * sizeof(int16_t));
    if (!sample->data) return false;
    
    memcpy(sample->data, data, length * sizeof(int16_t));
    sample->length = length;
    sample->startPoint = 0;
    sample->endPoint = length;
    sample->sampleRate = sampleRate;
    sample->loaded = true;
    
    return true;
}

void SampleEngine::setSampleTrim(uint8_t index, uint32_t start, uint32_t end) {
    if (index >= MAX_SAMPLES || !samples[index].loaded) return;
    
    Sample* sample = &samples[index];
    sample->startPoint = min(start, sample->length - 1);
    sample->endPoint = min(end, sample->length);
}

void SampleEngine::setSamplePitch(uint8_t index, float semitones) {
    // This affects future triggers, not currently playing voices
    // Could store per-sample pitch offset if needed
}

void SampleEngine::setEnvelope(uint8_t voiceIndex, float attack, float release) {
    if (voiceIndex >= MAX_VOICES) return;
    
    voices[voiceIndex].attackTime = attack;
    voices[voiceIndex].releaseTime = release;
}

float SampleEngine::interpolateSample(Sample* sample, float position) {
    if (!sample || !sample->data) return 0.0f;
    
    uint32_t posInt = (uint32_t)position;
    float frac = position - posInt;
    
    if (posInt >= sample->endPoint - 1) {
        return (float)sample->data[sample->endPoint - 1];
    }
    
    // Linear interpolation
    float s1 = (float)sample->data[posInt];
    float s2 = (float)sample->data[posInt + 1];
    return s1 + (s2 - s1) * frac;
}

void SampleEngine::updateEnvelope(Voice* voice, float dt) {
    if (!voice->active) return;
    
    switch ((int)voice->envelopePhase) {
        case 0: // Attack
            voice->envelopePos += dt / voice->attackTime;
            if (voice->envelopePos >= 1.0f) {
                voice->envelopePos = 1.0f;
                voice->envelopePhase = 1.0f; // Sustain
            }
            voice->volume = voice->envelopePos;
            break;
            
        case 1: // Sustain
            voice->volume = 1.0f;
            break;
            
        case 2: // Release
            voice->envelopePos += dt / voice->releaseTime;
            if (voice->envelopePos >= 1.0f) {
                voice->volume = 0.0f;
                voice->active = false;
            } else {
                voice->volume = 1.0f - voice->envelopePos;
            }
            break;
    }
}

void SampleEngine::render(int16_t* buffer, size_t samples) {
    float dt = 1.0f / AUDIO_RATE;
    
    for (size_t i = 0; i < samples; i++) {
        float mix = 0.0f;
        
        // Process all active voices
        for (int v = 0; v < MAX_VOICES; v++) {
            Voice* voice = &voices[v];
            
            if (!voice->active || !voice->sample) continue;
            
            // Update envelope
            updateEnvelope(voice, dt);
            
            // Get sample value
            float sampleValue = interpolateSample(voice->sample, voice->position);
            mix += sampleValue * voice->volume * 0.7f; // Scale down for mixing (70% to prevent clipping)
            
            // Advance position with sample rate conversion
            float sampleRateRatio = (float)voice->sample->sampleRate / (float)AUDIO_RATE;
            voice->position += voice->pitch * sampleRateRatio;
            
            // Check if sample finished
            if (voice->position >= voice->sample->endPoint) {
                if (voice->envelopePhase < 2) {
                    // Start release
                    voice->envelopePhase = 2;
                    voice->envelopePos = 0.0f;
                }
                // Keep playing during release, but don't advance position
                voice->position = voice->sample->endPoint - 1;
            }
        }
        
        // Clamp and convert to int16
        mix = constrain(mix, -32768.0f, 32767.0f);
        buffer[i] = (int16_t)mix;
    }
}
