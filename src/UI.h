#ifndef UI_H
#define UI_H

#include <Arduino.h>
#include <U8g2lib.h>
#include "Config.h"
#include "Sequencer.h"
#include "AudioEngine.h"

// Font Constants (Issue #14)
#define FONT_HEADER u8g2_font_ncenB10_tr
#define FONT_BODY   u8g2_font_6x10_tf
#define FONT_SMALL  u8g2_font_5x7_tf

class SynthUI {
public:
    SynthUI(Sequencer& seq, AudioEngine& audio);
    void init();
    void draw(Mode currentMode);
    
    // Menu State (Managed by Main/Input, Displayed by UI)
    int menuCursor = 0;
    int menuScroll = 0;
    
private:
    Sequencer& sequencer;
    AudioEngine& audioEngine;
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2; // Owned by UI
    
    void drawHeader(const char* title);
    void drawSequencerMode();
    void drawLaunchpadMode();
    void drawSettingsMode();
    void drawPlayIndicator(bool playing); // Issue #19
};

#endif
