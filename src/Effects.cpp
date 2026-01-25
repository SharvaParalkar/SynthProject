#include "Effects.h"
#include <math.h>

Effects::Effects() {
    bitCrusherEnabled = false;
    bitCrusherBits = BIT_CRUSHER_BITS;
    bitCrusherQuantization = 1.0f;
    
    filterEnabled = false;
    filterCutoff = FILTER_CUTOFF_DEFAULT;
    filterResonance = 0.5f;
    filterX1 = filterX2 = filterY1 = filterY2 = 0.0f;
}

void Effects::begin() {
    updateFilterCoefficients();
    setBitCrusherBits(BIT_CRUSHER_BITS);
}

void Effects::setBitCrusherBits(uint8_t bits) {
    bitCrusherBits = constrain(bits, 4, 16);
    // Calculate quantization step
    // For 8 bits: 65536 / 256 = 256 steps
    uint16_t steps = 1 << bitCrusherBits;
    bitCrusherQuantization = 65536.0f / (float)steps;
}

int16_t Effects::processBitCrusher(int16_t sample) {
    if (!bitCrusherEnabled) return sample;
    
    // Quantize
    float quantized = floorf((float)sample / bitCrusherQuantization) * bitCrusherQuantization;
    return (int16_t)quantized;
}

void Effects::updateFilterCoefficients() {
    // Simple 2-pole low-pass filter (Biquad)
    // Using bilinear transform approximation
    float w = 2.0f * PI * filterCutoff / AUDIO_RATE;
    float cosw = cosf(w);
    float sinw = sinf(w);
    float alpha = sinw / (2.0f * filterResonance);
    
    float b0 = (1.0f - cosw) / 2.0f;
    float b1 = 1.0f - cosw;
    float b2 = (1.0f - cosw) / 2.0f;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * cosw;
    float a2 = 1.0f - alpha;
    
    // Normalize
    filterA0 = b0 / a0;
    filterA1 = b1 / a0;
    filterA2 = b2 / a0;
    filterB1 = a1 / a0;
    filterB2 = a2 / a0;
}

float Effects::processFilter(float input) {
    if (!filterEnabled) return input;
    
    // Biquad filter: y[n] = a0*x[n] + a1*x[n-1] + a2*x[n-2] - b1*y[n-1] - b2*y[n-2]
    float output = filterA0 * input + 
                   filterA1 * filterX1 + 
                   filterA2 * filterX2 - 
                   filterB1 * filterY1 - 
                   filterB2 * filterY2;
    
    // Update history
    filterX2 = filterX1;
    filterX1 = input;
    filterY2 = filterY1;
    filterY1 = output;
    
    return output;
}

void Effects::setFilterCutoff(float cutoff) {
    filterCutoff = constrain(cutoff, FILTER_CUTOFF_MIN, FILTER_CUTOFF_MAX);
    updateFilterCoefficients();
}

void Effects::setFilterResonance(float resonance) {
    filterResonance = constrain(resonance, 0.1f, 1.0f);
    updateFilterCoefficients();
}

void Effects::process(int16_t* buffer, size_t samples) {
    for (size_t i = 0; i < samples; i++) {
        float sample = (float)buffer[i];
        
        // Apply bit crusher
        int16_t crushed = processBitCrusher((int16_t)sample);
        sample = (float)crushed;
        
        // Apply filter
        sample = processFilter(sample);
        
        // Clamp and convert back
        sample = constrain(sample, -32768.0f, 32767.0f);
        buffer[i] = (int16_t)sample;
    }
}
