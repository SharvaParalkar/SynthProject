#include "UI.h"
#include <string.h>

// =========================================================================================
// LOFIOS DESIGN SYSTEM - LAYOUT CONSTANTS
// =========================================================================================

// Screen Layout Grid (128x64)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Top Bar (12px)
#define TOP_BAR_HEIGHT 12
#define TOP_BAR_Y 0
#define TOP_ICON_X 2
#define TOP_ICON_Y 2
#define TOP_TEXT_X 14
#define TOP_TEXT_Y 2
#define TOP_BPM_X 70
#define TOP_BPM_Y 2
#define TOP_VOL_X 104
#define TOP_VOL_Y 3
#define TOP_VOL_W 22
#define TOP_VOL_H 8

// Main Content Area (40px)
#define MAIN_AREA_Y 12
#define MAIN_AREA_HEIGHT 40
#define MAIN_AREA_BOTTOM 52

// Bottom Bar (12px)
#define BOTTOM_BAR_Y 52
#define BOTTOM_BAR_HEIGHT 12
#define BOTTOM_HINT_LEFT_X 2
#define BOTTOM_HINT_LEFT_Y 54
#define BOTTOM_HINT_RIGHT_X 94
#define BOTTOM_HINT_RIGHT_Y 54

// Standard Margins
#define MARGIN_LEFT 2
#define MARGIN_RIGHT 2
#define MARGIN_TOP 2
#define MARGIN_BOTTOM 2
#define ELEMENT_SPACING 2
#define ELEMENT_SPACING_LARGE 4

// Typography
#define FONT_SMALL u8g2_font_5x7_tf       // 5x7px - Labels, hints, secondary info
#define FONT_MEDIUM u8g2_font_6x10_tf     // 6x10px - Body text, menu items, parameters
#define FONT_LARGE u8g2_font_7x13B_tf     // 7x13px bold - Headers, values being edited

// Icon Size
#define ICON_SIZE 8

// Animation Timing
#define MODE_TRANSITION_MS 66              // 2 frames at 30fps
#define PARAM_EDIT_TIMEOUT 3000            // 3 seconds
#define CONFIRMATION_TIMEOUT 1000          // 1 second
#define BLINK_INTERVAL 500                 // 500ms on/off
#define LOADING_FRAME_MS 100               // 100ms per loading frame

// Auto-exit timeouts
#define VOLUME_MODE_TIMEOUT 4000  // Increased from 2000 to 4000ms
#define STEP_EDIT_TIMEOUT 5000

// Performance Mode Layout
#define PERF_PAD_GRID_X 40
#define PERF_PAD_GRID_Y 16
#define PERF_PAD_SIZE 9
#define PERF_PAD_GAP 3
#define PERF_SCOPE_X 4
#define PERF_SCOPE_Y 16
#define PERF_SCOPE_W 30
#define PERF_SCOPE_H 32

// Sequencer Mode Layout
#define SEQ_STEP_SIZE 9
#define SEQ_STEP_GAP 2
#define SEQ_GRID_X 2
#define SEQ_GRID_Y 16
#define SEQ_TRACK_LABEL_X 2
#define SEQ_TRACK_LABEL_Y 14

// Step Edit Layout
#define STEP_EDIT_TITLE_Y 14
#define STEP_EDIT_PARAM_Y 24
#define STEP_EDIT_BAR_Y 32
#define STEP_EDIT_BAR_W 100
#define STEP_EDIT_BAR_H 8
#define STEP_EDIT_VALUE_Y 44

// =========================================================================================
// CONSTRUCTOR & INITIALIZATION
// =========================================================================================

SynthUI::SynthUI(U8G2 &display) : _display(display) {
    _currentMode = Mode::PERFORMANCE;
    _previousMode = Mode::PERFORMANCE;
    _currentSubMode = SubMode::NONE;
    
    _bpm = 120;
    _masterVolume = 80;
    _activeVoices = 0;
    _pressedButtonId = -1;
    strcpy(_contextHint, "READY");
    strcpy(_kitName, "DRUMS");
    
    _scopeData = NULL;
    _isEditingParam = false;
    _paramEditStartTime = 0;
    
    _menuCursor = 0;
    _currentPage = 0;
    _shiftHeld = false;
    _lastInteraction = 0;
    
    _seqPattern = NULL;
    _seqCurrentStep = 0;
    _seqPlaying = false;
    _seqActiveTrack = 0;
    
    _songSteps = NULL;
    _songLength = 0;
    _songCursor = 0;
    _songLoopEnabled = true;
    
    _fxBC = false;
    _fxSR = false;
    _fxFL = false;
    _fxDL = false;
    
    _stepEditIndex = 0;
    _stepEditParam = 0;
    
    _isLoading = false;
    _loadingProgress = 0;
    
    _isTransitioning = false;
    _lastModeChangeTime = 0;
    
    // Initialize settings (12 items across 3 pages)
    _settingsCount = 12;
    
    // Page 1 (0-3): Audio settings
    _settings[0] = {"Master Vol", 0, 80, 0, 100};      // Type 0 = percentage
    _settings[1] = {"Master Tune", 2, 0, -12, 12};     // Type 2 = numeric
    _settings[2] = {"Attack", 2, 2, 0, 100};           // Type 2 = numeric (ms)
    _settings[3] = {"Release", 2, 200, 0, 500};        // Type 2 = numeric (ms)
    
    // Page 2 (4-7): Sequencer settings
    _settings[4] = {"BPM", 2, 120, 60, 240};           // Type 2 = numeric
    _settings[5] = {"Length", 2, 16, 8, 16};           // Type 2 = numeric
    _settings[6] = {"Metro", 1, 0, 0, 1};              // Type 1 = toggle
    _settings[7] = {"Swing", 0, 0, 0, 100};            // Type 0 = percentage
    
    // Page 3 (8-11): System settings
    _settings[8] = {"Auto-Save", 1, 1, 0, 1};          // Type 1 = toggle
    _settings[9] = {"USB", 1, 0, 0, 1};                // Type 1 = toggle
    _settings[10] = {"Brightness", 2, 1, 0, 2};        // Type 2 = numeric (0=low, 1=med, 2=high)
    _settings[11] = {"Factory Reset", 3, 0, 0, 0};     // Type 3 = action
    
    // Initialize kit list
    _availableKits[0] = "DRUMS";
    _availableKits[1] = "TONAL";
    _availableKits[2] = "TEXTURE";
    _availableKits[3] = "SYNTH";
    _kitCount = 4;
    _selectedKit = 0;
}

void SynthUI::begin() {
    _display.setFontPosTop();
}

// =========================================================================================
// UPDATE & STATE MANAGEMENT
// =========================================================================================

void SynthUI::update() {
    unsigned long now = millis();
    
    // Handle transitions
    if (_isTransitioning) {
        if (now - _lastModeChangeTime > MODE_TRANSITION_MS) {
            _isTransitioning = false;
        }
    }
    
    // Auto-dismiss parameter overlay
    if (_isEditingParam) {
        if (now - _paramEditStartTime > PARAM_EDIT_TIMEOUT) {
            _isEditingParam = false;
        }
    }
    
    // Auto-exit volume mode
    if (_currentSubMode == SubMode::VOLUME) {
        if (now - _lastInteraction > VOLUME_MODE_TIMEOUT) {
            exitSubMode();
        }
    }
    
    // Auto-exit step edit mode
    if (_currentSubMode == SubMode::STEP_EDIT) {
        if (now - _lastInteraction > STEP_EDIT_TIMEOUT) {
            exitSubMode();
        }
    }
    
    // Update loading animation
    if (_isLoading) {
        _loadingProgress = ((now - _loadingStartTime) * 100) / 500;
        if (_loadingProgress >= 100) {
            _isLoading = false;
            _loadingProgress = 0;
            // After loading kit, cycle to next mode
            if (_currentMode == Mode::KIT) {
                int nextMode = (int)_currentMode + 1;
                if (nextMode >= (int)Mode::MODE_COUNT) {
                    nextMode = 0;
                }
                setMode((Mode)nextMode);
            }
        }
    }
}

void SynthUI::setMode(Mode m) {
    if (m == _currentMode) return;
    
    _previousMode = _currentMode;
    _currentMode = m;
    _currentSubMode = SubMode::NONE;
    _menuCursor = 0;
    _currentPage = 0;
    
    _lastModeChangeTime = millis();
    _isTransitioning = true;
    
    // Update context hints - these will be shown in bottom bar
    switch(_currentMode) {
        case Mode::PERFORMANCE:
            setContextHint("F1:SHIFT F2:MODE");
            break;
        case Mode::SEQUENCER:
            setContextHint("F1:PLAY F2:MODE");
            break;
        case Mode::SONG:
            setContextHint("F1:PLAY F2:MODE");
            break;
        case Mode::KIT:
            setContextHint("F1:BACK F2:LOAD");
            break;
        case Mode::SETTINGS:
            setContextHint("F1:BACK F2:EDIT");
            break;
        default:
            setContextHint("");
            break;
    }
}

void SynthUI::enterSubMode(SubMode sm) {
    _currentSubMode = sm;
    _lastInteraction = millis();
    
    // Update context hint for sub-modes
    switch(sm) {
        case SubMode::SETTING_EDIT:
            setContextHint("F1:CANCEL F2:SAVE");
            break;
        case SubMode::STEP_EDIT:
            setContextHint("F1:BACK F2:NEXT");
            break;
        case SubMode::VOLUME:
            setContextHint("PADS:SET VOL");
            break;
        case SubMode::FX_EDIT:
            setContextHint("PADS:TOGGLE F1:EXIT");
            break;
        default:
            break;
    }
}

void SynthUI::exitSubMode() {
    _currentSubMode = SubMode::NONE;
    // Restore context hint for current mode without resetting everything
    switch(_currentMode) {
        case Mode::PERFORMANCE:
            setContextHint("F1:SHIFT F2:MODE");
            break;
        case Mode::SEQUENCER:
            setContextHint("F1:PLAY F2:MODE");
            break;
        case Mode::SONG:
            setContextHint("F1:PLAY F2:MODE");
            break;
        case Mode::KIT:
            setContextHint("F1:BACK F2:LOAD");
            break;
        case Mode::SETTINGS:
            setContextHint("F1:BACK F2:EDIT");
            break;
        default:
            setContextHint("");
            break;
    }
}

// =========================================================================================
// INPUT HANDLING
// =========================================================================================

void SynthUI::onFunctionKey1(bool pressed, bool longPress) {
    if (pressed && longPress) {
        // Long press: Enter FX mode if in PERFORMANCE
        if (_currentMode == Mode::PERFORMANCE && _currentSubMode == SubMode::NONE) {
            enterSubMode(SubMode::FX_EDIT);
        }
    } else if (!pressed) {
        // Release: Back/Cancel action
        if (_currentSubMode != SubMode::NONE) {
            exitSubMode();
        } else if (_currentMode != Mode::PERFORMANCE) {
            setMode(Mode::PERFORMANCE);
        }
    }
}

void SynthUI::onFunctionKey2() {
    // In sub-modes, F2 acts as confirm/next
    if (_currentSubMode == SubMode::SETTING_EDIT) {
        // Confirm and exit
        exitSubMode();
        return;
    }
    
    if (_currentSubMode == SubMode::STEP_EDIT) {
        // Next parameter
        _stepEditParam = (_stepEditParam + 1) % 4;
        _lastInteraction = millis();
        return;
    }
    
    // In KIT mode, F2 loads the selected kit
    if (_currentMode == Mode::KIT) {
        _isLoading = true;
        _loadingStartTime = millis();
        _loadingProgress = 0;
        return;
    }
    
    // In SETTINGS mode, F2 enters edit mode
    if (_currentMode == Mode::SETTINGS) {
        enterSubMode(SubMode::SETTING_EDIT);
        return;
    }
    
    // Otherwise, cycle to next mode
    int nextMode = (int)_currentMode + 1;
    if (nextMode >= (int)Mode::MODE_COUNT) {
        nextMode = 0;
    }
    setMode((Mode)nextMode);
}

void SynthUI::onButtonPress(int buttonId) {
    _pressedButtonId = buttonId;
    _lastButtonPressTime = millis();
    _lastInteraction = millis();
    
    // VOLUME SUB-MODE
    if (_currentSubMode == SubMode::VOLUME) {
        if (buttonId >= 0 && buttonId <= 9) {
            _masterVolume = buttonId * 10;
        } else if (buttonId == 10) {
            if (_masterVolume > 0) _masterVolume--;
        } else if (buttonId == 11) {
            if (_masterVolume < 100) _masterVolume++;
        }
        return;
    }
    
    // STEP EDIT SUB-MODE
    if (_currentSubMode == SubMode::STEP_EDIT && _seqPattern) {
        Track& trk = _seqPattern->tracks[_seqActiveTrack];
        Step& step = trk.steps[_stepEditIndex];
        
        if (_stepEditParam == 0) {  // Velocity
            if (buttonId >= 0 && buttonId < 8) {
                const uint8_t velocityLevels[8] = {0, 14, 28, 42, 57, 71, 85, 100};
                step.velocity = velocityLevels[buttonId];
            }
        } else if (_stepEditParam == 1) {  // Probability
            if (buttonId >= 0 && buttonId < 4) {
                const uint8_t probLevels[4] = {25, 50, 75, 100};
                step.probability = probLevels[buttonId];
            }
        } else if (_stepEditParam == 2) {  // Pitch
            if (buttonId == 12) step.pitchOffset = max(-12, step.pitchOffset - 1);
            if (buttonId == 13) step.pitchOffset = min(12, step.pitchOffset + 1);
        } else if (_stepEditParam == 3) {  // Filter
            if (buttonId == 14) step.filterOffset = max(-64, step.filterOffset - 8);
            if (buttonId == 15) step.filterOffset = min(64, step.filterOffset + 8);
        }
        return;
    }
    
    // SETTING EDIT SUB-MODE
    if (_currentSubMode == SubMode::SETTING_EDIT) {
        // Handle setting value changes based on type
        // Implementation depends on specific setting
        return;
    }
    
    // KIT SELECT MODE
    if (_currentMode == Mode::KIT) {
        if (buttonId == 0 && _menuCursor > 0) {
            _menuCursor--;
        } else if (buttonId == 1 && _menuCursor < _kitCount - 1) {
            _menuCursor++;
        } else if (buttonId >= 2 && buttonId <= 5 && buttonId - 2 < _kitCount) {
            _menuCursor = buttonId - 2;
            _selectedKit = _menuCursor;
        }
        return;
    }
    
    // SETTINGS MODE
    if (_currentMode == Mode::SETTINGS) {
        if (buttonId == 0 && _menuCursor > 0) {
            _menuCursor--;
            // Update page to match cursor position
            _currentPage = _menuCursor / 4;
        } else if (buttonId == 1 && _menuCursor < _settingsCount - 1) {
            _menuCursor++;
            // Update page to match cursor position
            _currentPage = _menuCursor / 4;
        } else if (buttonId == 2 && _currentPage > 0) {
            _currentPage--;
            _menuCursor = _currentPage * 4;
        } else if (buttonId == 3 && _currentPage < 2) {
            _currentPage++;
            _menuCursor = _currentPage * 4;
        }
        return;
    }
    
    // SONG MODE
    if (_currentMode == Mode::SONG) {
        if (buttonId == 0 && _songCursor > 0) {
            _songCursor--;
        } else if (buttonId == 1 && _songCursor < _songLength - 1) {
            _songCursor++;
        }
        // Additional song editing controls would go here
        return;
    }
}

void SynthUI::onButtonRelease(int buttonId) {
    if (_pressedButtonId == buttonId) {
        _pressedButtonId = -1;
    }
}

// =========================================================================================
// SETTERS
// =========================================================================================

void SynthUI::setBPM(int bpm) {
    _bpm = bpm;
}

void SynthUI::setMasterVolume(uint8_t volume) {
    _masterVolume = volume;
}

void SynthUI::setActiveVoices(uint8_t count) {
    _activeVoices = count;
}

void SynthUI::setContextHint(const char* hint) {
    strncpy(_contextHint, hint, 31);
    _contextHint[31] = '\0';
}

void SynthUI::setKitName(const char* name) {
    strncpy(_kitName, name, 15);
    _kitName[15] = '\0';
}

void SynthUI::setScopeData(volatile int16_t* data) {
    _scopeData = data;
}

void SynthUI::setSequencerState(Pattern* pattern, int step, bool playing, int activeTrack) {
    _seqPattern = pattern;
    _seqCurrentStep = step;
    _seqPlaying = playing;
    _seqActiveTrack = activeTrack;
}

void SynthUI::setFXStatus(bool bc, bool sr, bool fl, bool dl) {
    _fxBC = bc;
    _fxSR = sr;
    _fxFL = fl;
    _fxDL = dl;
}

void SynthUI::setSongData(SongStep* steps, uint8_t length, uint8_t cursor, bool loopEnabled) {
    _songSteps = steps;
    _songLength = length;
    _songCursor = cursor;
    _songLoopEnabled = loopEnabled;
}

void SynthUI::showParameter(const char* label, float value, float min, float max, int type, const char* unit) {
    strncpy(_paramLabel, label, 31);
    _paramValue = value;
    _paramMin = min;
    _paramMax = max;
    _paramType = type;
    strncpy(_paramUnit, unit, 15);
    
    _isEditingParam = true;
    _paramEditStartTime = millis();
}

// =========================================================================================
// MAIN DRAW FUNCTION
// =========================================================================================

void SynthUI::draw() {
    _display.firstPage();
    do {
        // Loading animation takes over entire screen
        if (_isLoading) {
            drawLoadingAnimation();
            continue;
        }
        
        // Parameter overlay takes over main area
        if (_isEditingParam) {
            drawTopBar();
            drawBottomBar();
            drawParameterOverlay();
            continue;
        }
        
        // Normal rendering
        drawTopBar();
        drawBottomBar();
        
        int offsetX = 0;
        
        // Transition animation
        if (_isTransitioning) {
            float progress = (float)(millis() - _lastModeChangeTime) / (float)MODE_TRANSITION_MS;
            if (progress > 1.0f) progress = 1.0f;
            progress = 1.0f - pow(1.0f - progress, 3.0f);  // Ease out
            offsetX = (int)((1.0f - progress) * 128);
        }
        
        // Draw current mode/submode
        if (_currentSubMode != SubMode::NONE) {
            switch (_currentSubMode) {
                case SubMode::VOLUME:
                    drawVolumeMode(offsetX, MAIN_AREA_Y);
                    break;
                case SubMode::STEP_EDIT:
                    drawStepEdit(offsetX, MAIN_AREA_Y);
                    break;
                case SubMode::SETTING_EDIT:
                    drawSettingEdit(offsetX, MAIN_AREA_Y);
                    break;
                case SubMode::FX_EDIT:
                    drawFXEdit(offsetX, MAIN_AREA_Y);
                    break;
                default:
                    break;
            }
        } else {
            switch (_currentMode) {
                case Mode::PERFORMANCE:
                    drawPerformance(offsetX, MAIN_AREA_Y);
                    break;
                case Mode::SEQUENCER:
                    drawSequencer(offsetX, MAIN_AREA_Y);
                    break;
                case Mode::SONG:
                    drawSong(offsetX, MAIN_AREA_Y);
                    break;
                case Mode::KIT:
                    drawKit(offsetX, MAIN_AREA_Y);
                    break;
                case Mode::SETTINGS:
                    drawSettings(offsetX, MAIN_AREA_Y);
                    break;
                default:
                    break;
            }
        }
        
    } while (_display.nextPage());
}

// =========================================================================================
// HELPER FUNCTIONS - DRAWING UTILITIES
// =========================================================================================

void SynthUI::drawBox(int x, int y, int w, int h, bool filled, bool inverted) {
    if (inverted) {
        _display.setDrawColor(1);
        _display.drawBox(x, y, w, h);
        _display.setDrawColor(0);
    } else if (filled) {
        _display.drawBox(x, y, w, h);
    } else {
        _display.drawFrame(x, y, w, h);
    }
    
    // Reset draw color
    if (inverted) {
        _display.setDrawColor(1);
    }
}

void SynthUI::drawProgressBar(int x, int y, int w, int h, int value, int max) {
    // Draw outline
    _display.drawFrame(x, y, w, h);
    
    // Calculate fill width
    int fillW = ((w - 2) * value) / max;
    if (fillW > 0) {
        _display.drawBox(x + 1, y + 1, fillW, h - 2);
    }
}

void SynthUI::drawIcon(int x, int y, const unsigned char* icon) {
    _display.drawXBM(x, y, ICON_SIZE, ICON_SIZE, icon);
}

void SynthUI::drawCenteredText(int y, const char* text, const uint8_t* font) {
    _display.setFont(font);
    int textW = _display.getStrWidth(text);
    int x = (SCREEN_WIDTH - textW) / 2;
    _display.setCursor(x, y);
    _display.print(text);
}

void SynthUI::drawRightAlignedText(int x, int y, const char* text) {
    int textW = _display.getStrWidth(text);
    _display.setCursor(x - textW, y);
    _display.print(text);
}

int SynthUI::centerX(int textWidth) {
    return (SCREEN_WIDTH - textWidth) / 2;
}

int SynthUI::rightAlignX(int textWidth) {
    return SCREEN_WIDTH - MARGIN_RIGHT - textWidth;
}

// =========================================================================================
// HELPER FUNCTIONS - PATTERN FILLS
// =========================================================================================

void SynthUI::fillPattern50(int x, int y, int w, int h) {
    // Checkered pattern (50% fill)
    for (int py = 0; py < h; py++) {
        for (int px = 0; px < w; px++) {
            if ((px + py) % 2 == 0) {
                _display.drawPixel(x + px, y + py);
            }
        }
    }
}

void SynthUI::fillPattern25(int x, int y, int w, int h) {
    // Dotted pattern (25% fill)
    for (int py = 0; py < h; py += 2) {
        for (int px = 0; px < w; px += 2) {
            _display.drawPixel(x + px, y + py);
        }
    }
}

void SynthUI::fillPatternVelocity(int x, int y, int w, int h, uint8_t velocity) {
    // Fill based on velocity level - remap to match Step velocity thresholds
    // Step levels: 0, 14, 28, 42, 57, 71, 85, 100
    if (velocity == 0) {
        // Empty - just outline
        return;
    } else if (velocity < 15) {
        // Very light - sparse dots (0%)
        return;
    } else if (velocity < 45) {
        // Light - dotted (25%)
        fillPattern25(x, y, w, h);
    } else if (velocity < 70) {
        // Medium - checkered (50%)
        fillPattern50(x, y, w, h);
    } else if (velocity < 90) {
        // Strong - dense dots (75%)
        for (int py = 0; py < h; py++) {
            for (int px = 0; px < w; px++) {
                if ((px + py) % 2 == 0 || px % 2 == 0) {
                    _display.drawPixel(x + px, y + py);
                }
            }
        }
    } else {
        // Full - solid (100%)
        _display.drawBox(x, y, w, h);
    }
}

// =========================================================================================
// TOP & BOTTOM BARS
// =========================================================================================

void SynthUI::drawTopBar() {
    // Clear top bar area first to prevent text corruption
    _display.setDrawColor(0);  // Black to clear
    _display.drawBox(0, 0, SCREEN_WIDTH, TOP_BAR_HEIGHT);
    _display.setDrawColor(1);  // Reset to white
    
    _display.setFont(FONT_SMALL);
    
    // Mode icon
    const unsigned char* modeIcon = icon_mode_perf_bits;
    switch(_currentMode) {
        case Mode::PERFORMANCE:
            modeIcon = icon_mode_perf_bits;
            break;
        case Mode::SEQUENCER:
            modeIcon = icon_mode_seq_bits;
            break;
        case Mode::SONG:
            modeIcon = icon_mode_song_bits;
            break;
        case Mode::KIT:
            modeIcon = icon_mode_kit_bits;
            break;
        case Mode::SETTINGS:
            modeIcon = icon_settings_bits;
            break;
        default:
            break;
    }
    drawIcon(TOP_ICON_X, TOP_ICON_Y, modeIcon);
    
    // Context text (mode-specific) - BPM removed from here, shown separately on right
    _display.setCursor(TOP_TEXT_X, TOP_TEXT_Y);
    switch(_currentMode) {
        case Mode::PERFORMANCE:
            _display.print(_kitName);
            break;
        case Mode::SEQUENCER:
            if (_seqPattern) {
                _display.print(_seqPattern->name);
                if (_seqPlaying) {
                    _display.print(" ");
                    drawIcon(_display.getCursorX(), TOP_ICON_Y, icon_play_bits);
                }
            } else {
                _display.print("NO PAT");
            }
            break;
        case Mode::SONG:
            _display.print("SONG ");
            _display.print(_songLength);
            break;
        case Mode::KIT:
            _display.print(_kitCount);
            _display.print(" ");  // Space separator
            _display.print("KITS");
            break;
        case Mode::SETTINGS:
            _display.print("PAGE ");
            _display.print(_currentPage + 1);
            _display.print("/3");
            break;
        default:
            break;
    }
    
    // BPM display (right side, before volume)
    char bpmStr[8];
    sprintf(bpmStr, "%d", _bpm);
    _display.setCursor(TOP_BPM_X, TOP_BPM_Y);
    _display.print(bpmStr);
    
    // Volume bar (far right)
    drawProgressBar(TOP_VOL_X, TOP_VOL_Y, TOP_VOL_W, TOP_VOL_H, _masterVolume, 100);
    
    // Separator line
    _display.drawLine(0, TOP_BAR_HEIGHT - 1, SCREEN_WIDTH, TOP_BAR_HEIGHT - 1);
}

void SynthUI::drawBottomBar() {
    // Clear bottom bar area first to prevent text corruption
    _display.setDrawColor(0);  // Black to clear
    _display.drawBox(0, BOTTOM_BAR_Y, SCREEN_WIDTH, BOTTOM_BAR_HEIGHT);
    _display.setDrawColor(1);  // Reset to white
    
    // Separator line
    _display.drawLine(0, BOTTOM_BAR_Y, SCREEN_WIDTH, BOTTOM_BAR_Y);
    
    _display.setFont(FONT_SMALL);
    
    // Parse context hint for left and right hints
    // Format: "LEFT RIGHT" or just "HINT"
    char leftHint[32] = "";
    char rightHint[32] = "";
    
    // Simple parsing - split on space if present
    const char* spacePos = strchr(_contextHint, ' ');
    if (spacePos != NULL) {
        // Copy left part
        int leftLen = spacePos - _contextHint;
        strncpy(leftHint, _contextHint, leftLen);
        leftHint[leftLen] = '\0';
        
        // Copy right part
        strcpy(rightHint, spacePos + 1);
    } else {
        // No space, just show on left
        strcpy(leftHint, _contextHint);
    }
    
    // Draw left hint - adjust Y position to prevent clipping (FONT_SMALL is 5x7, so Y=55 gives 1px margin)
    _display.setCursor(BOTTOM_HINT_LEFT_X, BOTTOM_HINT_LEFT_Y + 1);
    _display.print(leftHint);
    
    // Draw right hint (right-aligned) - adjust Y position to prevent clipping
    if (strlen(rightHint) > 0) {
        int rightW = _display.getStrWidth(rightHint);
        _display.setCursor(SCREEN_WIDTH - MARGIN_RIGHT - rightW, BOTTOM_HINT_RIGHT_Y + 1);
        _display.print(rightHint);
    }
}

// =========================================================================================
// MODE DRAWING FUNCTIONS
// =========================================================================================

void SynthUI::drawPerformance(int offsetX, int offsetY) {
    // Draw waveform scope in background (only if data is available)
    if (_scopeData != NULL) {
        _display.drawFrame(offsetX + PERF_SCOPE_X, offsetY + PERF_SCOPE_Y, 
                          PERF_SCOPE_W, PERF_SCOPE_H);
        
        int midY = offsetY + PERF_SCOPE_Y + (PERF_SCOPE_H / 2);
        int scopeW = PERF_SCOPE_W - 2;
        int scopeH = PERF_SCOPE_H - 2;
        int maxAmp = (scopeH / 2) - 1;
        
        // Draw center line for reference
        _display.drawHLine(offsetX + PERF_SCOPE_X + 1, midY, scopeW);
        
        // Draw waveform with bounds checking and proper scaling
        int prevY = midY;
        for(int i = 0; i < scopeW; i++) {
            int sampleIdx = (i * 128) / scopeW; // Better sample distribution
            if (sampleIdx >= 128) sampleIdx = 127; // Clamp to valid range
            
            int16_t sample = _scopeData[sampleIdx];
            // Calculate and clamp val BEFORE using it
            int val = (sample * maxAmp) / 32768;
            val = max(min(val, maxAmp), -maxAmp);
            
            // Calculate Y position
            int yPos = midY + val;
            
            // Draw pixel at sample point
            _display.drawPixel(offsetX + PERF_SCOPE_X + 1 + i, yPos);
            
            // Connect to previous point for continuous waveform
            if (i > 0) {
                _display.drawLine(offsetX + PERF_SCOPE_X + i, prevY, 
                                 offsetX + PERF_SCOPE_X + 1 + i, yPos);
            }
            
            prevY = yPos;
        }
    }
    
    // Draw 4x4 pad grid
    for(int row = 0; row < 4; row++) {
        for(int col = 0; col < 4; col++) {
            int padIdx = row * 4 + col;
            int px = offsetX + PERF_PAD_GRID_X + col * (PERF_PAD_SIZE + PERF_PAD_GAP);
            int py = offsetY + PERF_PAD_GRID_Y + row * (PERF_PAD_SIZE + PERF_PAD_GAP);
            
            if (padIdx == _pressedButtonId) {
                // Pressed: filled
                _display.drawBox(px, py, PERF_PAD_SIZE, PERF_PAD_SIZE);
            } else {
                // Released: outline
                _display.drawFrame(px, py, PERF_PAD_SIZE, PERF_PAD_SIZE);
            }
        }
    }
    
    // If a pad was just pressed, show sample name overlay (would need timing logic)
    // For now, we'll skip the overlay as it requires additional state tracking
}

void SynthUI::drawSequencer(int offsetX, int offsetY) {
    if (!_seqPattern) {
        drawCenteredText(offsetY + 15, "NO PATTERN", FONT_MEDIUM);
        return;
    }
    
    _display.setFont(FONT_SMALL);
    
    // Track label
    char trackLabel[8];
    sprintf(trackLabel, "T%d", _seqActiveTrack + 1);
    _display.setCursor(offsetX + SEQ_TRACK_LABEL_X, offsetY + SEQ_TRACK_LABEL_Y);
    _display.print(trackLabel);
    
    // Calculate grid dimensions to use available space efficiently
    // Available width: SCREEN_WIDTH - MARGIN_LEFT - MARGIN_RIGHT = 124px
    // Available height: MAIN_AREA_HEIGHT - (SEQ_GRID_Y - offsetY) = ~24px for grid
    int gridWidth = 4 * (SEQ_STEP_SIZE + SEQ_STEP_GAP) - SEQ_STEP_GAP;
    int gridHeight = 4 * (SEQ_STEP_SIZE + SEQ_STEP_GAP) - SEQ_STEP_GAP;
    int gridX = offsetX + (SCREEN_WIDTH - gridWidth) / 2; // Center the grid
    int gridY = offsetY + SEQ_GRID_Y;
    
    // Draw 16-step grid (4x4 layout)
    Track& trk = _seqPattern->tracks[_seqActiveTrack];
    
    for(int row = 0; row < 4; row++) {
        for(int col = 0; col < 4; col++) {
            int stepIdx = row * 4 + col;
            int sx = gridX + col * (SEQ_STEP_SIZE + SEQ_STEP_GAP);
            int sy = gridY + row * (SEQ_STEP_SIZE + SEQ_STEP_GAP);
            
            // Ensure we don't overflow
            if (sx + SEQ_STEP_SIZE > SCREEN_WIDTH - MARGIN_RIGHT) break;
            if (sy + SEQ_STEP_SIZE > offsetY + MAIN_AREA_HEIGHT - 2) break;
            
            Step& step = trk.steps[stepIdx];
            
            // Draw step cell
            _display.drawFrame(sx, sy, SEQ_STEP_SIZE, SEQ_STEP_SIZE);
            
            // Fill based on velocity if active
            if (step.active) {
                fillPatternVelocity(sx + 1, sy + 1, SEQ_STEP_SIZE - 2, 
                                   SEQ_STEP_SIZE - 2, step.velocity);
            }
            
            // Highlight playing step with inverted border
            if (stepIdx == _seqCurrentStep && _seqPlaying) {
                _display.setDrawColor(2);  // XOR mode
                _display.drawBox(sx, sy, SEQ_STEP_SIZE, SEQ_STEP_SIZE);
                _display.setDrawColor(1);
            }
            
            // Show probability indicator (hollow circle in top-right)
            if (step.probability < 100 && step.active) {
                _display.drawCircle(sx + SEQ_STEP_SIZE - 2, sy + 2, 1);
            }
            
            // Show parameter lock indicator ("L" in bottom-left)
            if ((step.pitchOffset != 0 || step.filterOffset != 0) && step.active) {
                _display.setFont(FONT_SMALL);
                _display.setCursor(sx + 1, sy + SEQ_STEP_SIZE - 6);
                _display.print("L");
            }
        }
    }
    
    // Step counter
    _display.setFont(FONT_SMALL);
    char stepInfo[16];
    sprintf(stepInfo, "STEP:%d/16", _seqCurrentStep + 1);
    _display.setCursor(offsetX + SEQ_GRID_X, offsetY + 2);
    _display.print(stepInfo);
}

void SynthUI::drawSong(int offsetX, int offsetY) {
    _display.setFont(FONT_MEDIUM);
    
    // Title
    drawCenteredText(offsetY + 2, "PATTERN CHAIN", FONT_MEDIUM);
    
    // Check if we have song data
    if (!_songSteps || _songLength == 0) {
        _display.setFont(FONT_SMALL);
        drawCenteredText(offsetY + 20, "NO SONG DATA", FONT_SMALL);
        return;
    }
    
    // Length indicator
    _display.setFont(FONT_SMALL);
    char lengthStr[16];
    sprintf(lengthStr, "LEN: %d", _songLength);
    _display.setCursor(offsetX + 88, offsetY + 2);
    _display.print(lengthStr);
    
    // Calculate scrolling - show 4 items at a time
    int listY = offsetY + 14;
    int visibleItems = 4;
    int scrollOffset = 0;
    
    // Scroll to keep cursor visible, prevent negative offset
    if (_songLength > visibleItems) {
        if (_songCursor >= visibleItems) {
            scrollOffset = _songCursor - visibleItems + 1;
        }
        // Ensure scrollOffset doesn't go beyond available items
        if (scrollOffset + visibleItems > _songLength) {
            scrollOffset = max(0, _songLength - visibleItems);
        }
    }
    
    _display.setFont(FONT_SMALL);
    
    for(int i = 0; i < visibleItems && (i + scrollOffset) < _songLength; i++) {
        int songIdx = i + scrollOffset;
        int itemY = listY + i * 9;
        
        // Ensure we stay within main content area
        if (itemY + 8 > offsetY + MAIN_AREA_HEIGHT - 10) break;
        
        SongStep& step = _songSteps[songIdx];
        
        // Highlight selected
        if (songIdx == _songCursor) {
            _display.drawBox(offsetX + MARGIN_LEFT, itemY, 
                           SCREEN_WIDTH - MARGIN_LEFT - MARGIN_RIGHT, 8);
            _display.setDrawColor(0);
        }
        
        // Cursor arrow
        _display.setCursor(offsetX + MARGIN_LEFT + 2, itemY + 1);
        if (songIdx == _songCursor) {
            _display.print(">");
        } else {
            _display.print(" ");
        }
        
        // Pattern info with text clipping
        _display.setCursor(offsetX + MARGIN_LEFT + 10, itemY + 1);
        char patternText[32];
        sprintf(patternText, "%d:PAT%d x%d", songIdx + 1, step.patternIndex + 1, step.repeatCount);
        
        // Clip text if too long
        int maxTextWidth = SCREEN_WIDTH - MARGIN_LEFT - MARGIN_RIGHT - 20; // Leave room for scroll indicator
        int textWidth = _display.getStrWidth(patternText);
        if (textWidth > maxTextWidth) {
            // Truncate and add ellipsis
            _display.setFont(FONT_SMALL);
            int charWidth = _display.getStrWidth("M"); // Approximate char width
            int maxChars = maxTextWidth / charWidth - 1;
            if (maxChars > 0 && maxChars < (int)strlen(patternText)) {
                char clipped[32];
                strncpy(clipped, patternText, maxChars);
                clipped[maxChars] = '\0';
                _display.print(clipped);
                _display.print(".");
            } else {
                _display.print(patternText);
            }
        } else {
            _display.print(patternText);
        }
        
        // Visual repeat bar (only if there's space)
        int textEndX = offsetX + MARGIN_LEFT + 10 + min(textWidth, maxTextWidth);
        int barX = textEndX + 4;
        if (barX + 24 < SCREEN_WIDTH - MARGIN_RIGHT) {
            int maxBars = min((int)step.repeatCount, 8);  // Cap visual display
            for(int r = 0; r < maxBars && (barX + r * 3 + 2) < SCREEN_WIDTH - MARGIN_RIGHT; r++) {
                _display.drawBox(barX + r * 3, itemY + 2, 2, 4);
            }
        }
        
        // Reset draw color
        if (songIdx == _songCursor) {
            _display.setDrawColor(1);
        }
    }
    
    // Scroll indicator if needed
    if (_songLength > visibleItems) {
        _display.setFont(FONT_SMALL);
        char scrollInfo[16];
        sprintf(scrollInfo, "%d/%d", _songCursor + 1, _songLength);
        int scrollW = _display.getStrWidth(scrollInfo);
        _display.setCursor(SCREEN_WIDTH - MARGIN_RIGHT - scrollW, offsetY + MAIN_AREA_HEIGHT - 8);
        _display.print(scrollInfo);
    }
    
    // Loop status
    _display.setFont(FONT_SMALL);
    _display.setCursor(offsetX + 2, offsetY + MAIN_AREA_HEIGHT - 8);
    _display.print("LOOP: ");
    _display.print(_songLoopEnabled ? "ON" : "OFF");
}

void SynthUI::drawKit(int offsetX, int offsetY) {
    _display.setFont(FONT_MEDIUM);
    
    // Title
    drawCenteredText(offsetY + 2, "AVAILABLE KITS", FONT_MEDIUM);
    
    // Check if we have kits
    if (_kitCount == 0) {
        _display.setFont(FONT_SMALL);
        drawCenteredText(offsetY + 20, "NO KITS FOUND", FONT_SMALL);
        return;
    }
    
    // Total count
    _display.setFont(FONT_SMALL);
    char countStr[16];
    sprintf(countStr, "%d TOTAL", _kitCount);
    _display.setCursor(offsetX + 88, offsetY + 2);
    _display.print(countStr);
    
    // Kit list - calculate scrolling for more than 3 items
    int listY = offsetY + 14;
    int visibleItems = 3;  // Show 3 items to fit in 40px area (3 * 10px + margins)
    int scrollOffset = 0;
    
    // Scroll to keep cursor visible
    if (_kitCount > visibleItems) {
        if (_menuCursor >= visibleItems) {
            scrollOffset = _menuCursor - visibleItems + 1;
        }
        // Ensure scrollOffset doesn't go beyond available items
        if (scrollOffset + visibleItems > _kitCount) {
            scrollOffset = max(0, _kitCount - visibleItems);
        }
    }
    
    _display.setFont(FONT_MEDIUM);
    
    for(int i = 0; i < visibleItems && (i + scrollOffset) < _kitCount; i++) {
        int kitIdx = i + scrollOffset;
        int itemY = listY + i * 10;
        
        // Ensure we stay within main content area
        if (itemY + 9 > offsetY + MAIN_AREA_HEIGHT - 2) break;
        
        // Highlight selected
        if (kitIdx == _menuCursor) {
            _display.drawBox(offsetX + MARGIN_LEFT, itemY, 
                           SCREEN_WIDTH - MARGIN_LEFT - MARGIN_RIGHT, 9);
            _display.setDrawColor(0);
        }
        
        // Cursor arrow
        _display.setCursor(offsetX + MARGIN_LEFT + 2, itemY + 1);
        if (kitIdx == _menuCursor) {
            _display.print(">");
        } else {
            _display.print(" ");
        }
        
        // Kit name with text clipping
        _display.setCursor(offsetX + MARGIN_LEFT + 12, itemY + 1);
        const char* kitName = _availableKits[kitIdx];
        int maxTextWidth = SCREEN_WIDTH - MARGIN_LEFT - MARGIN_RIGHT - 20; // Leave room for scroll indicator
        int textWidth = _display.getStrWidth(kitName);
        if (textWidth > maxTextWidth) {
            // Truncate and add ellipsis
            int charWidth = _display.getStrWidth("M"); // Approximate char width
            int maxChars = maxTextWidth / charWidth - 1;
            if (maxChars > 0) {
                char clipped[32];
                strncpy(clipped, kitName, maxChars);
                clipped[maxChars] = '\0';
                _display.print(clipped);
                _display.print(".");
            } else {
                _display.print(kitName);
            }
        } else {
            _display.print(kitName);
        }
        
        // Reset draw color
        if (kitIdx == _menuCursor) {
            _display.setDrawColor(1);
        }
    }
    
    // Scroll indicator if needed
    if (_kitCount > visibleItems) {
        _display.setFont(FONT_SMALL);
        char scrollInfo[16];
        sprintf(scrollInfo, "%d/%d", _menuCursor + 1, _kitCount);
        int scrollW = _display.getStrWidth(scrollInfo);
        _display.setCursor(SCREEN_WIDTH - MARGIN_RIGHT - scrollW, offsetY + MAIN_AREA_HEIGHT - 8);
        _display.print(scrollInfo);
        
        // Draw scroll arrows
        if (scrollOffset > 0) {
            // Up arrow available
            _display.drawPixel(offsetX + MARGIN_LEFT, listY - 2);
            _display.drawLine(offsetX + MARGIN_LEFT - 1, listY - 1, offsetX + MARGIN_LEFT + 1, listY - 1);
        }
        if (scrollOffset + visibleItems < _kitCount) {
            // Down arrow available
            int arrowY = listY + visibleItems * 10;
            _display.drawPixel(offsetX + MARGIN_LEFT, arrowY);
            _display.drawLine(offsetX + MARGIN_LEFT - 1, arrowY - 1, offsetX + MARGIN_LEFT + 1, arrowY - 1);
        }
    }
}

void SynthUI::drawSettings(int offsetX, int offsetY) {
    _display.setFont(FONT_MEDIUM);
    
    // Title
    drawCenteredText(offsetY + 2, "SETTINGS", FONT_MEDIUM);
    
    // Settings list - render from _settings[] array
    int listY = offsetY + 14;
    _display.setFont(FONT_SMALL);
    
    for(int i = 0; i < 4; i++) {
        int itemIdx = _currentPage * 4 + i;
        if (itemIdx >= _settingsCount) break;  // Don't exceed settings count
        
        int itemY = listY + i * 9;
        
        // Ensure we stay within main content area
        if (itemY + 8 > offsetY + MAIN_AREA_HEIGHT - 2) break;
        
        // Highlight selected
        if (itemIdx == _menuCursor) {
            _display.drawBox(offsetX + MARGIN_LEFT, itemY, 
                           SCREEN_WIDTH - MARGIN_LEFT - MARGIN_RIGHT, 8);
            _display.setDrawColor(0);
        }
        
        // Cursor arrow
        _display.setCursor(offsetX + MARGIN_LEFT + 2, itemY + 1);
        if (itemIdx == _menuCursor) {
            _display.print(">");
        } else {
            _display.print(" ");
        }
        
        // Setting label and value with text clipping
        _display.setCursor(offsetX + MARGIN_LEFT + 10, itemY + 1);
        
        // Format value based on type
        SettingItem& setting = _settings[itemIdx];
        char valueStr[16];
        switch(setting.type) {
            case 0:  // Percentage
                sprintf(valueStr, "%d%%", setting.value);
                break;
            case 1:  // Toggle
                sprintf(valueStr, "%s", setting.value ? "ON" : "OFF");
                break;
            case 2:  // Numeric
                if (itemIdx == 10) {  // Brightness special case
                    const char* brightLevels[] = {"LOW", "MED", "HI"};
                    sprintf(valueStr, "%s", brightLevels[setting.value]);
                } else if (itemIdx == 1) {  // Master Tune with +/-
                    sprintf(valueStr, "%+d", setting.value);
                } else {
                    sprintf(valueStr, "%d", setting.value);
                }
                break;
            case 3:  // Action
                sprintf(valueStr, "PRESS");
                break;
            default:
                sprintf(valueStr, "%d", setting.value);
                break;
        }
        
        // Build full text string
        char fullText[48];
        sprintf(fullText, "%s: %s", setting.label, valueStr);
        
        // Clip text if too long
        int maxTextWidth = SCREEN_WIDTH - MARGIN_LEFT - MARGIN_RIGHT - 2;
        int textWidth = _display.getStrWidth(fullText);
        if (textWidth > maxTextWidth) {
            // Truncate and add ellipsis
            int charWidth = _display.getStrWidth("M"); // Approximate char width
            int maxChars = maxTextWidth / charWidth - 1;
            if (maxChars > 0 && maxChars < (int)strlen(fullText)) {
                char clipped[48];
                strncpy(clipped, fullText, maxChars);
                clipped[maxChars] = '\0';
                _display.print(clipped);
                _display.print(".");
            } else {
                _display.print(fullText);
            }
        } else {
            _display.print(fullText);
        }
        
        // Reset draw color
        if (itemIdx == _menuCursor) {
            _display.setDrawColor(1);
        }
    }
}

// =========================================================================================
// SUB-MODE DRAWING FUNCTIONS
// =========================================================================================

void SynthUI::drawVolumeMode(int offsetX, int offsetY) {
    // Title
    drawCenteredText(offsetY + 4, "VOLUME", FONT_LARGE);
    
    // Large volume bar
    int barX = offsetX + (SCREEN_WIDTH - STEP_EDIT_BAR_W) / 2;
    drawProgressBar(barX, offsetY + 20, STEP_EDIT_BAR_W, 10, _masterVolume, 100);
    
    // Value display
    _display.setFont(FONT_LARGE);
    char valueStr[8];
    sprintf(valueStr, "%d%%", _masterVolume);
    int valueW = _display.getStrWidth(valueStr);
    _display.setCursor((SCREEN_WIDTH - valueW) / 2, offsetY + 32);
    _display.print(valueStr);
}

void SynthUI::drawStepEdit(int offsetX, int offsetY) {
    if (!_seqPattern) return;
    
    Track& trk = _seqPattern->tracks[_seqActiveTrack];
    Step& step = trk.steps[_stepEditIndex];
    
    // Title
    _display.setFont(FONT_LARGE);
    char title[16];
    sprintf(title, "STEP %d EDIT", _stepEditIndex + 1);
    drawCenteredText(offsetY + 2, title, FONT_LARGE);
    
    // Parameter names and values
    const char* paramNames[] = {"VELOCITY", "PROBABILITY", "PITCH", "FILTER"};
    int values[] = {
        step.velocity, 
        step.probability, 
        step.pitchOffset + 12, 
        step.filterOffset + 64
    };
    int maxVals[] = {100, 100, 24, 128};
    
    // Parameter name with cycle indicator (1/4, 2/4, etc.)
    _display.setFont(FONT_MEDIUM);
    char titleStr[32];
    sprintf(titleStr, "%s (%d/4)", paramNames[_stepEditParam], _stepEditParam + 1);
    drawCenteredText(offsetY + 16, titleStr, FONT_MEDIUM);
    
    // Value bar
    int barX = offsetX + (SCREEN_WIDTH - STEP_EDIT_BAR_W) / 2;
    drawProgressBar(barX, offsetY + 26, STEP_EDIT_BAR_W, STEP_EDIT_BAR_H, 
                   values[_stepEditParam], maxVals[_stepEditParam]);
    
    // Numeric value (large, centered)
    _display.setFont(FONT_LARGE);
    char valueStr[8];
    if (_stepEditParam == 2) {
        // Pitch shows as +/-
        sprintf(valueStr, "%+d", step.pitchOffset);
    } else if (_stepEditParam == 1 || _stepEditParam == 0) {
        // Percentage
        sprintf(valueStr, "%d%%", values[_stepEditParam]);
    } else {
        sprintf(valueStr, "%d", values[_stepEditParam]);
    }
    int valueW = _display.getStrWidth(valueStr);
    _display.setCursor((SCREEN_WIDTH - valueW) / 2, offsetY + 36);
    _display.print(valueStr);
    
    // Hint - clear area first to prevent text corruption
    _display.setDrawColor(0);  // Clear with black
    _display.drawBox(offsetX + MARGIN_LEFT, offsetY + MAIN_AREA_HEIGHT - 8, 
                     SCREEN_WIDTH - MARGIN_LEFT - MARGIN_RIGHT, 8);
    _display.setDrawColor(1);  // Reset to white
    _display.setFont(FONT_SMALL);
    drawCenteredText(offsetY + MAIN_AREA_HEIGHT - 6, "F2:NEXT PARAM", FONT_SMALL);
}

void SynthUI::drawSettingEdit(int offsetX, int offsetY) {
    if (_menuCursor >= _settingsCount) return;
    
    SettingItem& setting = _settings[_menuCursor];
    
    // Title
    _display.setFont(FONT_LARGE);
    drawCenteredText(offsetY + 2, setting.label, FONT_LARGE);
    
    // Value bar (for numeric and percentage types)
    if (setting.type == 0 || setting.type == 2) {
        int barX = offsetX + (SCREEN_WIDTH - STEP_EDIT_BAR_W) / 2;
        int range = setting.maxVal - setting.minVal;
        int progress = ((setting.value - setting.minVal) * 100) / range;
        drawProgressBar(barX, offsetY + 20, STEP_EDIT_BAR_W, 10, progress, 100);
    }
    
    // Current value display
    _display.setFont(FONT_LARGE);
    char valueStr[32];
    switch(setting.type) {
        case 0:  // Percentage
            sprintf(valueStr, "%d%%", setting.value);
            break;
        case 1:  // Toggle
            sprintf(valueStr, "%s", setting.value ? "ON" : "OFF");
            break;
        case 2:  // Numeric
            if (_menuCursor == 10) {  // Brightness
                const char* brightLevels[] = {"LOW", "MED", "HI"};
                sprintf(valueStr, "%s", brightLevels[setting.value]);
            } else if (_menuCursor == 1) {  // Master Tune
                sprintf(valueStr, "%+d", setting.value);
            } else {
                sprintf(valueStr, "%d", setting.value);
            }
            break;
        case 3:  // Action
            sprintf(valueStr, "PRESS PAD");
            break;
        default:
            sprintf(valueStr, "%d", setting.value);
            break;
    }
    int valueW = _display.getStrWidth(valueStr);
    _display.setCursor((SCREEN_WIDTH - valueW) / 2, offsetY + 32);
    _display.print(valueStr);
    
    // Hint - clear area first to prevent text corruption
    _display.setDrawColor(0);  // Clear with black
    _display.drawBox(offsetX + MARGIN_LEFT, offsetY + MAIN_AREA_HEIGHT - 8, 
                     SCREEN_WIDTH - MARGIN_LEFT - MARGIN_RIGHT, 8);
    _display.setDrawColor(1);  // Reset to white
    _display.setFont(FONT_SMALL);
    drawCenteredText(offsetY + MAIN_AREA_HEIGHT - 6, "PADS:ADJUST F2:SAVE", FONT_SMALL);
}

void SynthUI::drawFXEdit(int offsetX, int offsetY) {
    // Title
    drawCenteredText(offsetY + 2, "FX TOGGLE", FONT_MEDIUM);
    
    // FX slots
    const char* fxLabels[] = {"CRSH", "SR", "FILT", "DLAY"};
    bool fxStates[] = {_fxBC, _fxSR, _fxFL, _fxDL};
    
    int slotY = offsetY + 14;
    int slotW = 30;  // Increased from 28 to prevent overlap
    int slotH = 18;
    int slotSpacing = 2;
    int totalW = (slotW * 4) + (slotSpacing * 3);
    int startX = offsetX + (SCREEN_WIDTH - totalW) / 2;
    
    _display.setFont(FONT_SMALL);
    
    for(int i = 0; i < 4; i++) {
        int slotX = startX + i * (slotW + slotSpacing);
        
        // Draw slot
        if (fxStates[i]) {
            // Active: inverted
            _display.drawBox(slotX, slotY, slotW, slotH);
            _display.setDrawColor(0);
        } else {
            // Inactive: outline
            _display.drawFrame(slotX, slotY, slotW, slotH);
        }
        
        // Label
        int labelW = _display.getStrWidth(fxLabels[i]);
        _display.setCursor(slotX + (slotW - labelW) / 2, slotY + 4);
        _display.print(fxLabels[i]);
        
        // Pad number hint
        _display.setCursor(slotX + (slotW / 2) - 3, slotY + 12);
        _display.print(i + 9);
        
        // Reset draw color
        if (fxStates[i]) {
            _display.setDrawColor(1);
        }
    }
}

// =========================================================================================
// OVERLAYS
// =========================================================================================

void SynthUI::drawParameterOverlay() {
    // Clear main area with filled white box first for proper background
    _display.setDrawColor(1);
    _display.drawBox(MARGIN_LEFT, MAIN_AREA_Y + 2, 
                      SCREEN_WIDTH - MARGIN_LEFT - MARGIN_RIGHT, 
                      MAIN_AREA_HEIGHT - 4);
    
    // Draw frame border
    _display.setDrawColor(0);
    _display.drawFrame(MARGIN_LEFT, MAIN_AREA_Y + 2, 
                       SCREEN_WIDTH - MARGIN_LEFT - MARGIN_RIGHT, 
                       MAIN_AREA_HEIGHT - 4);
    _display.setDrawColor(1);
    
    // Label
    _display.setFont(FONT_LARGE);
    drawCenteredText(MAIN_AREA_Y + 6, _paramLabel, FONT_LARGE);
    
    // Value bar
    int barX = (SCREEN_WIDTH - STEP_EDIT_BAR_W) / 2;
    float progress = (_paramValue - _paramMin) / (_paramMax - _paramMin);
    drawProgressBar(barX, MAIN_AREA_Y + 22, STEP_EDIT_BAR_W, 8, 
                   (int)(progress * 100), 100);
    
    // Numeric value
    _display.setFont(FONT_LARGE);
    char valueStr[16];
    sprintf(valueStr, "%d", (int)_paramValue);
    if (strlen(_paramUnit) > 0) {
        strcat(valueStr, _paramUnit);
    }
    int valueW = _display.getStrWidth(valueStr);
    _display.setCursor((SCREEN_WIDTH - valueW) / 2, MAIN_AREA_Y + 34);
    _display.print(valueStr);
}

void SynthUI::drawLoadingAnimation() {
    _display.clearBuffer();
    
    // Title
    drawCenteredText(20, "LOADING KIT...", FONT_LARGE);
    
    // Progress bar
    int barX = (SCREEN_WIDTH - STEP_EDIT_BAR_W) / 2;
    drawProgressBar(barX, 38, STEP_EDIT_BAR_W, 8, _loadingProgress, 100);
    
    // Spinner (rotating)
    const char* spinFrames[] = {"|", "/", "-", "\\"};
    int frameIdx = (millis() / LOADING_FRAME_MS) % 4;
    _display.setFont(FONT_MEDIUM);
    _display.setCursor(SCREEN_WIDTH / 2 - 3, 50);
    _display.print(spinFrames[frameIdx]);
}

// =========================================================================================
// ANIMATION FUNCTIONS
// =========================================================================================

void SynthUI::animateTransition(Mode from, Mode to) {
    // Handled in main draw loop via _isTransitioning flag
}

void SynthUI::animateConfirmation(const char* message) {
    // Draw confirmation box
    int boxW = 100;
    int boxH = 20;
    int boxX = (SCREEN_WIDTH - boxW) / 2;
    int boxY = (SCREEN_HEIGHT - boxH) / 2;
    
    _display.drawBox(boxX, boxY, boxW, boxH);
    _display.setDrawColor(0);
    drawCenteredText(boxY + 6, message, FONT_MEDIUM);
    _display.setDrawColor(1);
}

void SynthUI::animateError(const char* message) {
    // Draw error box with double border
    int boxW = 108;
    int boxH = 24;
    int boxX = (SCREEN_WIDTH - boxW) / 2;
    int boxY = (SCREEN_HEIGHT - boxH) / 2;
    
    _display.drawFrame(boxX, boxY, boxW, boxH);
    _display.drawFrame(boxX + 1, boxY + 1, boxW - 2, boxH - 2);
    
    drawCenteredText(boxY + 8, message, FONT_SMALL);
}

void SynthUI::animateLoading(int progress) {
    _loadingProgress = progress;
}

// =========================================================================================
// PRIMITIVE DRAWING FUNCTIONS (Legacy compatibility)
// =========================================================================================

void SynthUI::drawProgressBar(int x, int y, int w, int h, float progress) {
    // Legacy version with float progress (0.0 - 1.0)
    drawProgressBar(x, y, w, h, (int)(progress * 100), 100);
}

void SynthUI::drawMenuList(int x, int y, const char* items[], int count, int selected) {
    _display.setFont(FONT_MEDIUM);
    
    for(int i = 0; i < count && i < 4; i++) {
        int itemY = y + (i * 10);
        
        if (i == selected) {
            _display.drawBox(x, itemY, SCREEN_WIDTH - x - MARGIN_RIGHT, 9);
            _display.setDrawColor(0);
        }
        
        _display.setCursor(x + 2, itemY + 1);
        _display.print(items[i]);
        
        if (i == selected) {
            _display.setDrawColor(1);
        }
    }
}

void SynthUI::drawGrid4x4(int x, int y, int w, int h, bool* states, int pressed) {
    int cellW = w / 4;
    int cellH = h / 4;
    
    for(int r = 0; r < 4; r++) {
        for(int c = 0; c < 4; c++) {
            int idx = r * 4 + c;
            int cx = x + c * cellW;
            int cy = y + r * cellH;
            
            if (idx == pressed || (states && states[idx])) {
                _display.drawBox(cx, cy, cellW - 2, cellH - 2);
            } else {
                _display.drawFrame(cx, cy, cellW - 2, cellH - 2);
            }
        }
    }
}

void SynthUI::drawConfirmation(const char* message) {
    animateConfirmation(message);
}

void SynthUI::drawError(const char* message) {
    animateError(message);
}
