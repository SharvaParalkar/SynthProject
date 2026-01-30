#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// --- I2S Audio (PCM5102A) ---
#define I2S_BCLK       6
#define I2S_LRC        7
#define I2S_DOUT       5
#define I2S_NUM        I2S_NUM_0
#define AUDIO_RATE     44100
#define I2S_BUFFER_COUNT 8
#define I2S_BUFFER_SIZE 256

// --- I2C Display ---
#define I2C_SDA        48
#define I2C_SCL        47
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64

// --- Button Matrix ---
// ROW_PINS: 4, 3, 8, 15
// COL_PINS: 16, 17, 18, 13
const int ROW_PINS[4] = {4, 3, 8, 15};
const int COL_PINS[4] = {16, 17, 18, 13};

// --- Function Buttons ---
#define BTN_MODE       36
#define BTN_OCTAVE     37
#define BTN_BOOT       0

// --- Status LEDs ---
const int LED_PINS[4] = {9, 10, 11, 12};

// --- Audio Constants ---
#define SAMPLE_RATE 44100
#define POLYPHONY 8  // Configurable dynamic voice allocation could go here (Issue #40)

// --- Mode Definitions ---
enum Mode {
  MODE_LAUNCHPAD,
  MODE_SEQUENCER,
  MODE_SETTINGS,
  MODE_NOTE_EDITOR
};

// --- Instrument Types ---
enum Instrument {
  INST_SINE,
  INST_SQUARE,
  INST_SAW,
  INST_TRIANGLE,
  INST_PLUCK,
  INST_BASS,
  INST_PAD,
  INST_LEAD,
  INST_COUNT
};

static const char* instrumentNames[] = {
  "Sine", "Square", "Saw", "Triangle",
  "Pluck", "Bass", "Pad", "Lead"
};

// --- Settings Menu Items ---
enum SettingsMenuItem {
  MENU_INSTRUMENT,
  MENU_BPM,
  MENU_PLAY_PAUSE,
  MENU_CLEAR_TRACK,
  MENU_VOLUME,
  MENU_BRIGHTNESS,
  MENU_ITEM_COUNT
};

static const char* menuItemNames[] = {
  "Instrument",
  "BPM",
  "Play/Pause",
  "Clear Track",
  "Volume",
  "Brightness"
};

// --- Note Editor Menu Items ---
enum NoteEditorMenuItem {
  NOTE_MENU_SWING,
  NOTE_MENU_GATE,
  NOTE_MENU_FILTER,
  NOTE_MENU_ITEM_COUNT
};

static const char* noteMenuItemNames[] = {
  "Swing",
  "Gate",
  "Filter"
};

#endif
