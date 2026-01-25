#ifndef UI_H
#define UI_H

#include <U8g2lib.h>
#include "Icons.h"

// =========================================================================================
// MODE ENUMS
// =========================================================================================

// Primary modes (F2 cycles through these)
enum class Mode {
    PERFORMANCE,    // Live pad playing
    SEQUENCER,      // Step programming
    SONG,           // Pattern chaining
    KIT,            // Sample kit selection
    SETTINGS,       // Global parameters
    MODE_COUNT      // Helper for cycling
};

// Sub-modes (accessed via special key combinations)
enum class SubMode {
    NONE,           // No sub-mode active
    VOLUME,         // Volume adjustment (F1 HOLD + Pad 16 in PERFORMANCE)
    STEP_EDIT,      // Step parameter editing (Long press step in SEQUENCER)
    SETTING_EDIT,   // Setting value editing (F2 in SETTINGS)
    FX_EDIT         // Quick FX toggle (F1 HOLD + Pads 9-12 in PERFORMANCE)
};

// =========================================================================================
// SEQUENCER DATA STRUCTURES
// =========================================================================================

struct Step {
    bool active;           // Note on/off
    uint8_t velocity;      // 0-100 (8 levels: 0, 14, 28, 42, 57, 71, 85, 100)
    uint8_t probability;   // 25, 50, 75, 100
    int8_t pitchOffset;    // -12 to +12 semitones
    int8_t filterOffset;   // -64 to +64 relative control
};

struct Track {
    uint8_t padIndex;      // Which sample pad (0-15)
    Step steps[16];
    bool muted;
};

struct Pattern {
    Track tracks[8];
    uint8_t length;        // 8 or 16
    char name[8];
};

// Song chain entry
struct SongStep {
    uint8_t patternIndex;  // 0-3
    uint8_t repeatCount;   // 1-8
};

// =========================================================================================
// SETTINGS STRUCTURES
// =========================================================================================

struct SettingItem {
    const char* label;
    uint8_t type;          // 0=percentage, 1=toggle, 2=numeric, 3=action
    int16_t value;
    int16_t minVal;
    int16_t maxVal;
};

// =========================================================================================
// MAIN UI CLASS
// =========================================================================================

class SynthUI {
public:
    SynthUI(U8G2 &display);

    void begin();
    void update();         // Call in loop
    void draw();           // Call to render

    // Input handling
    void onFunctionKey1(bool pressed, bool longPress);  // F1: Back/Shift
    void onFunctionKey2();                               // F2: Next mode/Confirm
    void onButtonPress(int buttonId);
    void onButtonRelease(int buttonId);
    
    // State query
    bool isPerformanceMode() const { return _currentMode == Mode::PERFORMANCE && _currentSubMode == SubMode::NONE; }
    bool isSequencerMode() const { return _currentMode == Mode::SEQUENCER && _currentSubMode == SubMode::NONE; }
    bool isFXEditMode() const { return _currentSubMode == SubMode::FX_EDIT; }
    
    void setMode(Mode m);
    Mode getCurrentMode() const { return _currentMode; }
    SubMode getCurrentSubMode() const { return _currentSubMode; }

    // Sequencer link
    void setSequencerState(Pattern* pattern, int step, bool playing, int activeTrack);
    int getSequencerActiveTrack() const { return _seqActiveTrack; }
    
    // Setters for global state
    void setBPM(int bpm);
    void setMasterVolume(uint8_t volume);  // 0-100
    uint8_t getMasterVolume() const { return _masterVolume; }
    void setActiveVoices(uint8_t count);
    void setContextHint(const char* hint);
    void setKitName(const char* name);
    void setFXStatus(bool bc, bool sr, bool fl, bool dl);
    void setScopeData(volatile int16_t* data);
    
    // Parameter display overlay
    void showParameter(const char* label, float value, float min, float max, int type=0, const char* unit="");

    // Song mode data
    void setSongData(SongStep* steps, uint8_t length, uint8_t cursor, bool loopEnabled);

private:
    U8G2 &_display;
    
    // Mode state
    Mode _currentMode;
    Mode _previousMode;
    SubMode _currentSubMode;
    
    // Global state
    int _bpm;
    uint8_t _masterVolume;
    uint8_t _activeVoices;
    char _contextHint[32];
    char _kitName[16];
    
    // Sequencer view state
    Pattern* _seqPattern;
    int _seqCurrentStep;
    bool _seqPlaying;
    int _seqActiveTrack;
    
    // Song mode state
    SongStep* _songSteps;
    uint8_t _songLength;
    uint8_t _songCursor;
    bool _songLoopEnabled;
    
    // Menu/navigation state
    uint8_t _menuCursor;
    uint8_t _currentPage;
    bool _shiftHeld;
    unsigned long _lastInteraction;
    
    // Button state
    int _pressedButtonId;
    unsigned long _lastButtonPressTime;
    
    // Transition animation
    unsigned long _lastModeChangeTime;
    const unsigned long MODE_TRANSITION_MS = 100;
    bool _isTransitioning;
    
    // Scope data
    volatile int16_t* _scopeData;
    
    // Parameter edit overlay
    bool _isEditingParam;
    unsigned long _paramEditStartTime;
    const unsigned long PARAM_EDIT_TIMEOUT = 1500;
    char _paramLabel[32];
    float _paramValue;
    float _paramMin;
    float _paramMax;
    int _paramType;
    char _paramUnit[16];
    
    // FX status
    bool _fxBC, _fxSR, _fxFL, _fxDL;
    
    // Step edit state
    uint8_t _stepEditIndex;
    uint8_t _stepEditParam;  // 0=velocity, 1=probability, 2=pitch, 3=filter
    
    // Settings data
    SettingItem _settings[12];
    uint8_t _settingsCount;
    
    // Kit selection
    const char* _availableKits[4];
    uint8_t _kitCount;
    uint8_t _selectedKit;
    
    // Loading animation
    bool _isLoading;
    unsigned long _loadingStartTime;
    uint8_t _loadingProgress;
    
    // =========================================================================================
    // DRAWING FUNCTIONS
    // =========================================================================================
    
    // Layout components
    void drawTopBar();
    void drawBottomBar();
    
    // Mode-specific screens
    void drawPerformance(int offsetX, int offsetY);
    void drawSequencer(int offsetX, int offsetY);
    void drawSong(int offsetX, int offsetY);
    void drawKit(int offsetX, int offsetY);
    void drawSettings(int offsetX, int offsetY);
    
    // Sub-mode screens
    void drawVolumeMode(int offsetX, int offsetY);
    void drawStepEdit(int offsetX, int offsetY);
    void drawSettingEdit(int offsetX, int offsetY);
    void drawFXEdit(int offsetX, int offsetY);
    
    // Overlays
    void drawParameterOverlay();
    void drawLoadingAnimation();
    
    // Primitives
    void drawProgressBar(int x, int y, int w, int h, float progress);
    void drawProgressBar(int x, int y, int w, int h, int value, int max);  // Overload
    void drawMenuList(int x, int y, const char* items[], int count, int selected);
    void drawGrid4x4(int x, int y, int w, int h, bool* states, int pressed);
    void drawStepGrid(int x, int y, int trackIdx, bool* steps, int currentStep, int length);
    void drawParameterMeter(int x, int y, int w, int h, const char* label, int value, int max);
    void drawConfirmation(const char* message);
    void drawError(const char* message);
    
    // Helper drawing utilities
    void drawBox(int x, int y, int w, int h, bool filled, bool inverted);
    void drawIcon(int x, int y, const unsigned char* icon);
    void drawCenteredText(int y, const char* text, const uint8_t* font);
    void drawRightAlignedText(int x, int y, const char* text);
    int centerX(int textWidth);
    int rightAlignX(int textWidth);
    
    // Pattern fills
    void fillPattern50(int x, int y, int w, int h);
    void fillPattern25(int x, int y, int w, int h);
    void fillPatternVelocity(int x, int y, int w, int h, uint8_t velocity);
    
    // Animation functions
    void animateTransition(Mode from, Mode to);
    void animateConfirmation(const char* message);
    void animateError(const char* message);
    void animateLoading(int progress);
    
    // Helpers
    void enterSubMode(SubMode sm);
    void exitSubMode();
    void resetMenuCursor();
    const char* getModeIcon(Mode m);
    const char* getBottomHint();
};

#endif
