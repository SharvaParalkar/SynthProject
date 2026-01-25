# LofiOS Developer & Design Reference

## 🎨 LofiOS Design System

**LofiOS** is a cohesive design language for the ESP32-S3 monochrome OLED display. It prioritizes information density, high contrast, and immediate visual feedback.

### Core Principles
1.  **Information Density**: Maximize use of 128x64px space without clutter.
2.  **Visual Feedback**: Every interaction must have an immediate on-screen response (inverted pixels, border changes).
3.  **Monochrome "Colors"**: Use patterns to create hierarchy.
    *   **Solid (100%)**: Active, Selected, Primary.
    *   **Checkered (50%)**: Secondary background, borders.
    *   **Dotted (25%)**: Disabled, hints, tertiary.

### Typography System
| Font | Size | U8g2 Name | Usage |
|------|------|-----------|-------|
| **SMALL** | 5x7px | `u8g2_font_5x7_tf` | Hints, small labels, step counters. |
| **MEDIUM** | 6x10px| `u8g2_font_6x10_tf`| Body text, menu items, parameters. |
| **LARGE** | 7x13px| `u8g2_font_7x13B_tf`| Headers, modal values, large feedback. |

### Icon System
All icons are 8x8px bitmaps stored in `Icons.h` (PROGMEM).
- **Mode Icons**: Performance (▶), Sequencer (■), Song (♫), Kit (♪), Settings (⚙).
- **Function Icons**: Play, Stop, Loop, Mute.

---

## 📐 Visual Layout Reference

### Global Grid (128x64)
```
Y=0  ┌──────────────────────────────────────────────┐
     │ TOP BAR (12px)   [Icon] [Context]   [BPM]    │
Y=12 ├──────────────────────────────────────────────┤
     │                                              │
     │            MAIN CONTENT AREA (40px)          │
     │      (Mode specific layouts go here)         │
     │                                              │
Y=52 ├──────────────────────────────────────────────┤
     │ BOTTOM BAR (12px)  F1:Back        F2:Next    │
Y=64 └──────────────────────────────────────────────┘
```
**Margins**: 2px left/right.
**Element Spacing**: 2px small, 4px large.

### Component Specs
- **Top Bar**:
  - Icon: (2, 2)
  - Text: (14, 2)
  - BPM: (70, 2)
  - Volume Bar: (108, 3) - 18x6px
- **Performance Grid**: 4x4 matrix, 7x7px cells, 2px gap.
- **Sequencer Grid**: 4x4 matrix, 6x6px cells, 1px gap. Fill patterns indicate velocity.
- **Scroll Lists**: 4 items visible, 9px row height. Selected item inverted.

---

## 💻 Developer Reference

### Architecture
- **UI.h/cpp**: Main class `SynthUI`. Handles drawing and input state.
- **Icons.h**: Bitmap assets.
- **SynthProject.ino**: Main loop, dispatches inputs to `SynthUI`.

### Common Drawing Helpers
```cpp
// Draw a standard UI box (filled, outlined, or inverted)
void drawBox(int x, int y, int w, int h, bool filled, bool inverted);

// Draw a progress bar with consistent styling
void drawProgressBar(int x, int y, int w, int h, int value, int max);

// Draw centered text
void drawCenteredText(int y, const char* text, const uint8_t* font);

// Fill patterns for 'grayscale' effect
void fillPattern50(int x, int y, int w, int h); // Checkered
void fillPattern25(int x, int y, int w, int h); // Dotted
```

### Performance Tips
- **Frame Budget**: ~33ms (30 FPS).
- **Dirty Rects**: Only redraw main content if shifting modes.
- **PROGMEM**: Keep all icons/static data in flash.

---

## 🔮 Future Enhancements (Roadmap)
- [ ] **Custom Fonts**: Replace U8g2 fonts with pixel-perfect custom set.
- [ ] **Micro-animations**: Smooth transitions for value changes.
- [ ] **MIDI Integration**: Show MIDI activity in Top Bar.
- [ ] **Scene Saving**: Save full state to SD Card/Flash.
- [ ] **Accessibility**: High-contrast mode or large text option.

---
*Reference for styling and code structure.*
