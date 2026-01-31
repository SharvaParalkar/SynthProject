#include "UI.h"
#include <Wire.h>

SynthUI::SynthUI(Sequencer& seq, AudioEngine& audio, Hardware& hw) 
    : sequencer(seq), audioEngine(audio), hardware(hw), u8g2(U8G2_R0, U8X8_PIN_NONE) {
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
        case MODE_NOTE_EDITOR: drawNoteEditorMode(); break;
    }
    
    u8g2.sendBuffer();
}

void SynthUI::drawLaunchpadMode() {
    u8g2.setFont(FONT_BODY);
    u8g2.drawStr(0, 10, "Launchpad");
    
    // Status Bar
    u8g2.drawLine(0, 12, 128, 12);
    
    // Info Line (Instrument and Octave)
    u8g2.setFont(FONT_SMALL);
    char buf[32];
    
    // Instrument Name
    int currentTrack = sequencer.getCurrentTrack();
    Instrument inst = sequencer.getInstrument(currentTrack);
    u8g2.drawStr(0, 22, instrumentNames[inst]);

    // Octave (Right Aligned)
    sprintf(buf, "Oct:%d", sequencer.getCurrentOctave());
    int w = u8g2.getStrWidth(buf);
    u8g2.drawStr(128 - w - 2, 22, buf);
    
    // --- Grid (Right Aligned, Larger) ---
    // 4x4 Grid
    int boxSize = 9;
    int gap = 1;
    // Total width = 4*9 + 3*1 = 36 + 3 = 39
    int gridWidth = (4 * boxSize) + (3 * gap);
    int gridStartX = 128 - gridWidth - 2; // Right align with 2px margin
    int gridStartY = 24;
    
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            int x = gridStartX + col * (boxSize + gap);
            int y = gridStartY + row * (boxSize + gap);
            
            // Check if pad is pressed
            if (hardware.isPadPressed(row, col)) {
                u8g2.drawBox(x, y, boxSize, boxSize);
            } else {
                u8g2.drawFrame(x, y, boxSize, boxSize);
            }
        }
    }
    
    // --- Note Visualizer (Waveform on Left) ---
    // Area: x=0 to gridStartX-4, y=24 to 64
    int visX = 0;
    int visY = 24;
    int visW = gridStartX - 4;
    int visH = 40; // 64 - 24
    int midY = visY + (visH / 2);
    
    // Draw Frame
    u8g2.drawFrame(visX, visY, visW, visH);
    
    // Draw Waveform (ring buffer: oldest at ringIdx, newest at ringIdx-1)
    const float* wave = audioEngine.getWaveform();
    int ringIdx = audioEngine.getWaveformRingIndex();
    
    int prevY = midY;
    
    for (int i = 0; i < visW; i++) {
        int bufIdx = (ringIdx + (i * 128) / visW) % 128;
        if (bufIdx < 0) bufIdx += 128;
        
        float sample = wave[bufIdx];
        
        // Scale sample (-1.0 to 1.0) to height with 2.5x gain for bigger waves
        int drawY = midY - (int)(sample * (visH / 2) * 2.5f);
        
        // Clamp
        if (drawY < visY + 1) drawY = visY + 1;
        if (drawY > visY + visH - 2) drawY = visY + visH - 2;
        
        if (i > 0) {
            u8g2.drawLine(visX + i - 1, prevY, visX + i, drawY);
        }
        prevY = drawY;
    }
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
             // Draw underline
             u8g2.drawHLine(x, gridY + 8, 6);
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
    
    // Display 4 items max to fit screen
    for (int i = 0; i < 4; i++) {
        int itemIndex = menuScroll + i;
        if (itemIndex >= MENU_ITEM_COUNT) break;
        
        int y = 24 + (i * 10); // Slightly customized spacing to fit more? Or keep standard
        // Original was 12 spacing. 24, 36, 48, 60. Fits 4 lines.
        y = 24 + (i * 12);
        
        if (itemIndex == menuCursor) {
            u8g2.drawStr(0, y, ">");
        }
        
        u8g2.drawStr(10, y, menuItemNames[itemIndex]);
        
        // Value (Right Aligned)
        char val[32];
        if (itemIndex == MENU_INSTRUMENT) {
            sprintf(val, "%s", instrumentNames[sequencer.getInstrument(sequencer.getCurrentTrack())]);
        } else if (itemIndex == MENU_BPM) {
            sprintf(val, "%d", sequencer.getBPM());
        } else if (itemIndex == MENU_PLAY_PAUSE) {
            sprintf(val, "%s", sequencer.isPlayingState() ? "Play" : "Stop");
        } else if (itemIndex == MENU_CLEAR_TRACK) {
            sprintf(val, "Trk%d", sequencer.getCurrentTrack()+1);
        } else if (itemIndex == MENU_VOLUME) {
            sprintf(val, "%d%%", audioEngine.getVolume());
        } else if (itemIndex == MENU_BRIGHTNESS) {
            int b = hardware.getBrightness();
            int pct = (int)((b / 255.0f) * 100.0f);
            sprintf(val, "%d%%", pct);
        }
        
        int w = u8g2.getStrWidth(val);
        u8g2.drawStr(128 - w - 12, y, val);
    }
    
    if (menuScroll + 4 < MENU_ITEM_COUNT) {
        u8g2.drawStr(116, 60, "v");
    }
}

void SynthUI::drawNoteEditorMode() {
    u8g2.setFont(FONT_BODY);
    u8g2.drawStr(0, 10, "Note Editor");
    u8g2.drawLine(0, 12, 128, 12);
    
    // Scroll Indicators
    if (noteMenuScroll > 0) {
        u8g2.drawStr(116, 24, "^");
    }
    
    // Display 4 items max to fit screen
    for (int i = 0; i < 4; i++) {
        int itemIndex = noteMenuScroll + i;
        if (itemIndex >= NOTE_MENU_ITEM_COUNT) break;
        
        int y = 24 + (i * 12);
        
        if (itemIndex == noteMenuCursor) {
            u8g2.drawStr(0, y, ">");
        }
        
        u8g2.drawStr(10, y, noteMenuItemNames[itemIndex]);
        
        // Value (Right Aligned)
        char val[32];
        if (itemIndex == NOTE_MENU_SWING) {
            sprintf(val, "%d%%", sequencer.getSwing());
        } else if (itemIndex == NOTE_MENU_GATE) {
            int gatePct = (int)(sequencer.getGate() * 100.0f);
            sprintf(val, "%d%%", gatePct);
        } else if (itemIndex == NOTE_MENU_FILTER) {
            int filterPct = (int)(audioEngine.getFilterCutoff() * 100.0f);
            sprintf(val, "%d%%", filterPct);
        }
        
        int w = u8g2.getStrWidth(val);
        u8g2.drawStr(128 - w - 12, y, val);
    }
    
    if (noteMenuScroll + 4 < NOTE_MENU_ITEM_COUNT) {
        u8g2.drawStr(116, 60, "v");
    }
}

void SynthUI::drawPlayIndicator(bool playing) {
    if (playing) {
        // Triangle
        u8g2.drawTriangle(110, 22, 110, 30, 118, 26);
    } else {
        // Pause bars
        u8g2.drawBox(110, 22, 3, 8);
        u8g2.drawBox(115, 22, 3, 8);
    }
}
