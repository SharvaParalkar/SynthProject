#ifndef EFFECTS_H
#define EFFECTS_H

#include <Arduino.h>
#include "config.h"

class Effects {
public:
    Effects();
    
    void begin();
    void process(int16_t* buffer, size_t samples);
    
    // Bit crusher
    void setBitCrusherBits(uint8_t bits); // 4, 8, 12, 16
    uint8_t getBitCrusherBits() { return bitCrusherBits; }
    void enableBitCrusher(bool enable) { bitCrusherEnabled = enable; }
    bool isBitCrusherEnabled() { return bitCrusherEnabled; }
    
    // Low-pass filter
    void setFilterCutoff(float cutoff); // Hz
    float getFilterCutoff() { return filterCutoff; }
    void setFilterResonance(float resonance); // 0.0 - 1.0
    float getFilterResonance() { return filterResonance; }
    void enableFilter(bool enable) { filterEnabled = enable; }
    bool isFilterEnabled() { return filterEnabled; }

private:
    // Bit crusher
    bool bitCrusherEnabled;
    uint8_t bitCrusherBits;
    float bitCrusherQuantization;
    
    // Low-pass filter (simple IIR)
    bool filterEnabled;
    float filterCutoff;
    float filterResonance;
    float filterA0, filterA1, filterA2, filterB1, filterB2;
    float filterX1, filterX2, filterY1, filterY2;
    
    void updateFilterCoefficients();
    float processFilter(float input);
    int16_t processBitCrusher(int16_t sample);
};

#endif // EFFECTS_H
