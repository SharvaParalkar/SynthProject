#include "AudioEngine.h"
#include "AudioTools.h"
#include "AudioTools/AudioLibs/MaximilianDSP.h"
#include <math.h>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

// -----------------------------------------------------------------------------
// Static Maximilian objects - live in .cpp to avoid pulling headers into AudioEngine.h
// play() is called once per stereo sample at audio rate (e.g. 32000 Hz)
// -----------------------------------------------------------------------------
static audio_tools::I2SStream i2sOut;
static audio_tools::Maximilian* s_maximilian = nullptr;
static maxiOsc s_osc[POLYPHONY];
static maxiFilter s_filter;
static AudioEngine* g_audioEngine = nullptr;

// DC blocker state (removes droning from filter/osc DC)
static float s_dcPrevX = 0.0f;
static float s_dcPrevY = 0.0f;
static const float DC_COEFF = 0.9992f;

// Forward declare so we can pass to Maximilian constructor
void play(maxi_float_t* channels);

// -----------------------------------------------------------------------------
// play() - Called once per stereo sample by maximilian.copy()
// No I/O here - pure DSP. Buttons/UI are handled in loop().
// -----------------------------------------------------------------------------
void play(maxi_float_t* channels) {
    if (g_audioEngine)
        g_audioEngine->playCallback(channels);
    else {
        channels[0] = 0;
        channels[1] = 0;
    }
}

// -----------------------------------------------------------------------------
// AudioEngine
// -----------------------------------------------------------------------------
AudioEngine::AudioEngine() {
    for (int i = 0; i < POLYPHONY; i++) {
        voices[i].active = false;
        voices[i].releasing = false;
        voices[i].envelope = 0.0f;
        voices[i].attacking = false;
        voices[i].attackEnv = 0.0f;
    }
    masterVolume = 0.8f;
    filterCutoff = 0.5f;
    visualizerIdx = 0;
    lpf_state = 0.0f;
}

void AudioEngine::init() {
    g_audioEngine = this;

    // Match working reference: 32 kHz, 16-bit (default), let Maximilian handle writes
    auto cfg = i2sOut.defaultConfig(audio_tools::TX_MODE);
    cfg.sample_rate = 32000;
    cfg.channels = 2;
    cfg.pin_bck = I2S_BCLK;
    cfg.pin_ws = I2S_LRC;
    cfg.pin_data = I2S_DOUT;
    cfg.is_master = true;
    cfg.buffer_size = 256;

    if (!i2sOut.begin(cfg)) {
        Serial.println("[AudioEngine] I2S begin FAILED");
        return;
    }
    Serial.println("[AudioEngine] I2S started @ 32 kHz");

    // Use Maximilian wrapper (handles buffer + I2S writes)
    s_maximilian = new audio_tools::Maximilian(i2sOut);
    s_maximilian->begin(cfg);
    s_maximilian->setVolume(1.0f);  // we apply volume in playCallback

    resetFilterState();
}

void AudioEngine::copy() {
    if (s_maximilian)
        s_maximilian->copy();
}

void AudioEngine::setVolume(int vol) {
    if (vol < 0) vol = 0;
    if (vol > 100) vol = 100;
    masterVolume = vol / 100.0f;
    if (s_maximilian)
        s_maximilian->setVolume(masterVolume);
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

float AudioEngine::getVisualizerLevel() {
    float sum = 0.0f;
    for (int i = 0; i < POLYPHONY; i++) {
        if (voices[i].active)
            sum += voices[i].envelope;
    }
    return sum > 1.0f ? 1.0f : sum;
}

void AudioEngine::playCallback(float* channels) {
    // Always call all oscillators (keep them free-running even when silent)
    // to avoid phase discontinuities - match reference implementation
    
    float sampleMix = 0.0f;
    int activeCount = 0;

    for (int v = 0; v < POLYPHONY; v++) {
        float freq = voices[v].frequency;
        if (freq < 20.0f) freq = 220.0f;  // valid freq for free-running
        
        // Always call oscillator to keep phase continuous
        float out = 0.0f;
        switch (voices[v].instrument) {
            case INST_SINE:
                out = s_osc[v].sinewave(freq);
                break;
            case INST_SQUARE:
                out = s_osc[v].square(freq);
                break;
            case INST_SAW:
                out = s_osc[v].sawn(freq);
                break;
            case INST_TRIANGLE:
                out = s_osc[v].triangle(freq);
                break;
            case INST_PLUCK:
            case INST_BASS:
                out = s_osc[v].sawn(freq);
                break;
            case INST_PAD:
            case INST_LEAD:
                out = s_osc[v].pulse(freq, 0.3f);
                break;
            default:
                out = s_osc[v].sinewave(freq);
                break;
        }
        
        // Only output if voice is active (gate logic - no complex envelopes)
        if (voices[v].active) {
            sampleMix += out;
            activeCount++;
        }
    }

    if (activeCount == 0) {
        channels[0] = 0.0f;
        channels[1] = 0.0f;
        s_dcPrevX = 0.0f;
        s_dcPrevY = 0.0f;
        visualizerBuffer[visualizerIdx] = 0.0f;
        visualizerIdx = (visualizerIdx + 1) % 128;
        return;
    }

    // Average voices
    if (activeCount > 1)
        sampleMix /= (float)activeCount;

    // Filter (match reference: lores with low resonance)
    float cutoffHz = 200.0f + filterCutoff * 2000.0f;
    sampleMix = s_filter.lores(sampleMix, cutoffHz, 0.1);

    // Scale (match reference output level)
    sampleMix *= 0.3f * masterVolume;

    // DC blocker
    float x = sampleMix;
    float y = x - s_dcPrevX + DC_COEFF * s_dcPrevY;
    s_dcPrevX = x;
    s_dcPrevY = y;

    // Soft clip
    if (y > 0.9f) y = 0.9f;
    if (y < -0.9f) y = -0.9f;

    channels[0] = y;
    channels[1] = y;

    visualizerBuffer[visualizerIdx] = y;
    visualizerIdx = (visualizerIdx + 1) % 128;
}

void AudioEngine::noteOn(int note, Instrument inst) {
    for (int i = 0; i < POLYPHONY; i++) {
        if (voices[i].active && voices[i].note == note) {
            voices[i].releasing = false;
            voices[i].attacking = true;
            voices[i].attackEnv = voices[i].envelope * voices[i].attackEnv;
            if (voices[i].attackEnv < 0.01f) voices[i].attackEnv = 0.01f;
            return;
        }
    }

    int v = findFreeVoice();
    if (v == -1) return;

    voices[v].active = true;
    voices[v].releasing = false;
    voices[v].note = note;
    voices[v].frequency = midiToFreq(note);
    voices[v].instrument = inst;
    voices[v].envelope = 1.0f;
    
    // Oscillators free-run (no phase reset) - smooth continuous phase like reference
}

void AudioEngine::noteOff(int note) {
    for (int i = 0; i < POLYPHONY; i++) {
        if (voices[i].active && voices[i].note == note)
            voices[i].active = false;  // instant off (oscillator keeps running)
    }
}

void AudioEngine::killAll() {
    for (int i = 0; i < POLYPHONY; i++) {
        voices[i].active = false;
        voices[i].releasing = false;
    }
    resetFilterState();
}

void AudioEngine::resetFilterState() {
    lpf_state = 0.0f;
    s_dcPrevX = 0.0f;
    s_dcPrevY = 0.0f;
}

int AudioEngine::getActiveVoiceCount() {
    int c = 0;
    for (int i = 0; i < POLYPHONY; i++)
        if (voices[i].active) c++;
    return c;
}

float AudioEngine::midiToFreq(int note) {
    return 440.0f * powf(2.0f, (note - 69) / 12.0f);
}

int AudioEngine::findFreeVoice() {
    for (int i = 0; i < POLYPHONY; i++)
        if (!voices[i].active) return i;
    for (int i = 0; i < POLYPHONY; i++)
        if (voices[i].releasing) return i;
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
