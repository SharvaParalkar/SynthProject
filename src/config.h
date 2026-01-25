#ifndef CONFIG_H
#define CONFIG_H

// Hardware pin definitions
#include "pins.h"

// Audio configuration
#define AUDIO_RATE         44100
#define BITS_PER_SAMPLE    16
#define MAX_VOICES         16
#define MAX_SAMPLES        16

// Sequencer configuration
#define STEPS_PER_PATTERN  16
#define MAX_PATTERNS       4
#define MIN_BPM            60
#define MAX_BPM            180
#define DEFAULT_BPM        120

// Sample configuration
#define MAX_SAMPLE_LENGTH  44100  // 1 second at 44.1kHz
#define DEFAULT_SAMPLE_RATE 22050 // For lofi aesthetic

// Effects configuration
#define BIT_CRUSHER_BITS   8      // Default bit depth
#define FILTER_CUTOFF_MIN  200
#define FILTER_CUTOFF_MAX  20000
#define FILTER_CUTOFF_DEFAULT 8000

// UI configuration
#define DEBOUNCE_MS        20
#define LONG_PRESS_MS      500

// Mode enumeration
enum Mode {
    MODE_PLAY,      // Play samples directly
    MODE_PATTERN,   // Edit/play patterns
    MODE_SETTINGS   // Adjust tempo, effects, etc.
};

#endif // CONFIG_H
