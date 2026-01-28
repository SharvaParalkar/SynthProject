#include "UI.h"
#include <Wire.h>

SynthUI::SynthUI(Sequencer& seq, AudioEngine& audio) 
    : sequencer(seq), audioEngine(audio), u8g2(U8G2_R0, U8X8_PIN_NONE) {
}

void SynthUI::init() {
    // Initialize I2C with defined pins
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(400000); // 400kHz
    
    u8g2.setI2CAddress(0x3C << 1);
    if (!u8g2.begin()) {
        Serial.println("Display failed!");
    }
    u8g2.setContrast(255);
}

void SynthUI::draw(Mode currentMode) {
    u8g2.clearBuffer();
    
    // Header is commonish
    u8g2.setFont(FONT_BODY);
    
    switch (currentMode) {
        case MODE_LAUNCHPAD: drawLaunchpadMode(); break;
        case MODE_SEQUENCER: drawSequencerMode(); break;
        case MODE_SETTINGS:  drawSettingsMode(); break;
    }
    
    u8g2.sendBuffer();
}

void SynthUI::drawLaunchpadMode() {
    u8g2.setFont(FONT_BODY);
    u8g2.drawStr(0, 10, "Launchpad"); // Issue #22: Title Case
    
    // Status Bar
    u8g2.drawLine(0, 12, 128, 12);
    
    u8g2.drawStr(0, 24, instrumentNames[sequencer.getInstrument(sequencer.getCurrentTrack())]); // Actually assumes track 0 or global inst? 
    // In Launchpad mode, main.cpp used 'currentInstrument'.
    // We should probably expose 'currentInstrument' or use Track 0's.
    // For now, let's assume we display the instrument of the selected track or just "Live".
    
    // Octave (Right Aligned)
    char buf[16];
    sprintf(buf, "Oct:%d", sequencer.getCurrentOctave());
    int w = u8g2.getStrWidth(buf);
    u8g2.drawStr(128 - w - 2, 24, buf); // Issue #11
    
    // Vizualizer?
    // Maybe show active notes?
}

void SynthUI::drawSequencerMode() {
    // Header
    u8g2.setFont(FONT_BODY);
    u8g2.drawStr(0, 10, "Sequencer");
    
    // BPM (Right Aligned)
    char buf[16];
    sprintf(buf, "BPM:%d", sequencer.getBPM());
    int w = u8g2.getStrWidth(buf);
    u8g2.drawStr(128 - w - 2, 10, buf);
    
    // Track Info
    sprintf(buf, "Trk:%d", sequencer.getCurrentTrack() + 1);
    u8g2.drawStr(0, 22, buf);
    
    // Instrument Name
    Instrument inst = sequencer.getInstrument(sequencer.getCurrentTrack());
    u8g2.drawStr(40, 22, instrumentNames[inst]);
    
    // Play Indicator (Issue #19)
    drawPlayIndicator(sequencer.isPlayingState());
    
    // Grid (Issue #12: Y-positioning)
    // Issue #18: Aspect Ratio (Square boxes)
    int gridY = 32;
    for (int i = 0; i < 16; i++) {
        int x = i * 8;
        // 6x6 Box
        if (sequencer.getStep(sequencer.getCurrentTrack(), i)) {
            u8g2.drawBox(x, gridY, 6, 6);
        } else {
            u8g2.drawFrame(x, gridY, 6, 6);
        }
        
        // Highlight Current Step (Issue #13: Highlight)
        if (i == sequencer.getCurrentStep() && sequencer.isPlayingState()) {
             // Draw underline or frame outside
             u8g2.drawHLine(x, gridY + 8, 6);
             // Or frame
             // u8g2.drawFrame(x-1, gridY-1, 8, 8);
        }
    }
    
    // Track Overview (Issue #20: Visibility)
    int overviewY = 46;
    for (int trk = 0; trk < 4; trk++) {
        int y = overviewY + (trk * 4);
        
        // Track Indication
        if (trk == sequencer.getCurrentTrack()) {
            u8g2.drawStr(0, y+4, ">"); 
        }
        
        for (int s = 0; s < 16; s++) {
            if (sequencer.sequence[trk][s]) {
                // Issue #20: 2x2 pixels
                u8g2.drawBox(10 + s * 7, y, 2, 2); 
            }
        }
    }
}

void SynthUI::drawSettingsMode() {
    u8g2.setFont(FONT_BODY);
    u8g2.drawStr(0, 10, "Settings");
    u8g2.drawLine(0, 12, 128, 12);
    
    // Scroll Indicators (Issue #17)
    if (menuScroll > 0) {
        u8g2.drawStr(116, 24, "^");
    }
    
    for (int i = 0; i < 3; i++) {
        int itemIndex = menuScroll + i;
        if (itemIndex >= MENU_ITEM_COUNT) break;
        
        int y = 24 + (i * 12);
        
        if (itemIndex == menuCursor) {
            u8g2.drawStr(0, y, ">");
        }
        
        u8g2.drawStr(10, y, menuItemNames[itemIndex]);
        
        // Value (Right Aligned - Issue #16)
        char val[32];
        if (itemIndex == MENU_INSTRUMENT) {
            // Need global current instrument? 
            // In main logic, Settings changes 'currentInstrument' or Track Instrument.
            // Let's assume we display current generic instrument name
            // But 'currentInstrument' state is logically in Main or Sequencer.
            // For now, placeholder. Main will need to pass this or we use Sequencer's current track inst.
            sprintf(val, "%s", instrumentNames[sequencer.getInstrument(sequencer.getCurrentTrack())]);
        } else if (itemIndex == MENU_BPM) {
            sprintf(val, "%d", sequencer.getBPM());
        } else if (itemIndex == MENU_PLAY_PAUSE) {
            sprintf(val, "%s", sequencer.isPlayingState() ? "Play" : "Stop");
        } else if (itemIndex == MENU_CLEAR_TRACK) {
            sprintf(val, "Trk%d", sequencer.getCurrentTrack()+1);
        }
        
        int w = u8g2.getStrWidth(val);
        u8g2.drawStr(128 - w - 12, y, val); // -12 for scroll bar space
    }
    
    if (menuScroll + 3 < MENU_ITEM_COUNT) {
        u8g2.drawStr(116, 60, "v");
    }
}

void SynthUI::drawPlayIndicator(bool playing) {
    // Issue #19: Graphic
    if (playing) {
        // Triangle
        u8g2.drawTriangle(110, 22, 110, 30, 118, 26);
    } else {
        // Pause bars
        u8g2.drawBox(110, 22, 3, 8);
        u8g2.drawBox(115, 22, 3, 8);
    }
}
