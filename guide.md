# 🎛️ ESP32-S3 Lofi Sampler - User Guide

Welcome to the official guide for the **LofiOS Sampler**. This manual will help you master every feature of your device, from making your first beat to arranging full songs.

---

## 🗺️ Device Overview

### Hardware Layout

```text
       ┌──────────────────────────────┐
       │      OLED DISPLAY (128x64)   │
       │                              │
       │ [▶] PERF  120  [██░░] VOL    │
       └──────────────────────────────┘
      
       [ F1 ]                    [ F2 ]
    (Back/Shift)              (Next/Enter)

    ┌─────┐  ┌─────┐  ┌─────┐  ┌─────┐
    │  12 │  │  13 │  │  14 │  │  15 │  <- Row 4
    └─────┘  └─────┘  └─────┘  └─────┘
    ┌─────┐  ┌─────┐  ┌─────┐  ┌─────┐
    │   8 │  │   9 │  │  10 │  │  11 │  <- Row 3
    └─────┘  └─────┘  └─────┘  └─────┘
    ┌─────┐  ┌─────┐  ┌─────┐  ┌─────┐
    │   4 │  │   5 │  │   6 │  │   7 │  <- Row 2
    └─────┘  └─────┘  └─────┘  └─────┘
    ┌─────┐  ┌─────┐  ┌─────┐  ┌─────┐
    │   0 │  │   1 │  │   2 │  │   3 │  <- Row 1
    └─────┘  └─────┘  └─────┘  └─────┘
```

### The Golden Rules of Navigation

| Button | Action | Result |
| :---: | :--- | :--- |
| **`F2`** | **Tap** | **Go Forward** (Next Mode, Enter Menu, Confirm) |
| **`F1`** | **Tap** | **Go Back** (Previous Mode, Exit, Cancel) |
| **`F1`** | **Hold (800ms)** | **Shift / Modifier** (Access secondary functions) |

---

## 🎹 Mode 1: PERFORMANCE
**The Home Screen.** Play samples live and control global effects.

### Screen Layout
```text
┌──────────────────────────────────────┐
│ ▶ PERF      120 BPM       [████░░]   │ <- Top Bar
├──────────────────────────────────────┤
│ ┌────┐   ┌─┐ ┌─┐ ┌─┐ ┌─┐             │
│ │~~~~│   │█│ │ │ │ │ │ │             │
│ │SCOPE   └─┘ └─┘ └─┘ └─┘             │
│ └────┘   ┌─┐ ┌─┐ ┌─┐ ┌─┐             │
│          │ │ │ │ │ │ │█│             │
│          └─┘ └─┘ └─┘ └─┘    PADS     │
├──────────────────────────────────────┤
│ F1:SHIFT                    F2:SEQ   │ <- Context Hints
└──────────────────────────────────────┘
```

### 🎮 Controls
*   **Play**: Tap **`Pads 0-15`** to trigger samples.
*   **Octave**: Tap **`F1`** to cycle octaves (Lower -> Normal -> Higher).
*   **Mute Tracks**: Hold **`F1`** + **`Pads 0-7`**.
*   **Global FX**: Hold **`F1`** + **`Pads 8-11`** to toggle:
    *   `Pad 8`: Bitcrush
    *   `Pad 9`: Sample Rate
    *   `Pad 10`: Filter
    *   `Pad 11`: Delay

### 🔊 Volume Control
Hold **`F1`** + **`Pad 15`** to open the Volume Overlay.
```text
      VOLUME: 80%
   [████████░░░░]
```
*   Use **`Pads 0-9`** to set volume 0-100% instantly.
*   Wait 2s or tap `F1` to exit.

---

## 🎵 Mode 2: SEQUENCER
**The Brain.** Program 16-step patterns for 8 separate tracks.

### Screen Layout
```text
┌──────────────────────────────────────┐
│ ■ SEQ 1     120 BPM                  │
├──────────────────────────────────────┤
│ TRK1  1   2   3   4                  │
│      [█] [ ] [ ] [█]  <- 1-4         │
│       5   6   7   8                  │
│      [ ] [█] [ ] [ ]  <- 5-8         │
│         ... (16 steps total)         │
├──────────────────────────────────────┤
│ F1:PLAY                     F2:SONG  │
└──────────────────────────────────────┘
```

### 🎮 Controls
*   **Add/Remove Steps**: Tap **`Pads 0-15`** to toggle steps 1-16.
*   **Play/Stop**: Tap **`F1`**.
*   **Change Track**: Hold **`F1`** + **`Pads 0-7`** (Track 1-8).
    *   *Tip: The screen will scroll to show the active track.*
*   **Change Pattern**: Hold **`F1`** + **`Pads 8-11`** (Pattern 1-4).
*   **Change Tempo**: Hold **`F1`** + **`Pad 12`** (Slower) or **`Pad 13`** (Faster).

### 🛠️ Advanced: Step Edit
Want to change the pitch or probability of just **one** step?
1.  **Long Press (800ms)** the specific step pad you want to edit.
2.  The **STEP EDIT** menu appears:
    ```text
    ┌─────────────────────────┐
    │       STEP 5 EDIT       │
    │        VELOCITY         │
    │      [██████░░]         │
    │          75%            │
    └─────────────────────────┘
    ```
3.  **Edit**:
    *   **`Pads 0-7`**: Set Velocity.
    *   **`F2`** (Tap): Cycle to next parameter (**Probability**, **Pitch**, **Filter**).
4.  **`F1`**: Exit back to grid.

---

## 🎼 Mode 3: SONG
**The Arrangement.** Chain patterns together to make a full track.

### Screen Layout
```text
┌──────────────────────────────────────┐
│ ♫ SONG      LEN: 8       LOOP: ON    │
├──────────────────────────────────────┤
│ > 1. PATTERN 1  x4                   │
│   2. PATTERN 2  x2                   │
│   3. PATTERN 1  x2                   │
│   4. PATTERN 3  x4                   │
├──────────────────────────────────────┤
│ F1:PLAY                     F2:KIT   │
└──────────────────────────────────────┘
```

### 🎮 Controls
*   **Navigate**: **`Pad 0`** (Up), **`Pad 1`** (Down).
*   **Select Pattern**: **`Pads 2-5`** (Assign Pattern 1-4 to current slot).
*   **Repeats**: **`Pad 6`** (-1), **`Pad 7`** (+1).
*   **Edit Structure**:
    *   **`Pad 8`**: Insert new slot.
    *   **`Pad 9`**: Delete current slot.
*   **Loop Song**: Hold **`F1`** + **`Pad 12`**.

---

## 📦 Mode 4: KIT
**The Sounds.** Load different drum kits or synth patches.

### Screen Layout
```text
┌──────────────────────────────────────┐
│ ♪ KITS      4 TOTAL                  │
├──────────────────────────────────────┤
│   LOFI DRUMS    (16)                 │
│ > SYNTH WAVE    (12)                 │
│   GLITCH KIT    (16)                 │
│   VINTAGE KEYS  (10)                 │
├──────────────────────────────────────┤
│ F1:BACK                     F2:LOAD  │
└──────────────────────────────────────┘
```

### 🎮 Controls
1.  **Scroll**: Use **`Pads 0-1`** to find a kit.
2.  **Load**: Press **`F2`**.
    *   *Animation:* A loading bar will appear.
    *   *Result:* You are automatically returned to **PERFORMANCE** mode with new sounds loaded.

---

## ⚙️ Mode 5: SETTINGS
**The System.** Configure your device.

### Screen Layout
```text
┌──────────────────────────────────────┐
│ ⚙ SETTINGS   PAGE 1/3                │
├──────────────────────────────────────┤
│ > Master Vol:   80%                  │
│   Master Tune:  +0                   │
│   BPM:          120                  │
│   Metro:        OFF                  │
├──────────────────────────────────────┤
│ F1:BACK                     F2:EDIT  │
└──────────────────────────────────────┘
```

### 🎮 Controls
*   **Scroll**: **`Pad 0`** / **`Pad 1`**.
*   **Change Page**: **`Pad 2`** (Prev) / **`Pad 3`** (Next).
*   **Edit Value**:
    1.  Select item.
    2.  Press **`F2`**.
    3.  Use Pads to adjust value.
    4.  Press **`F2`** to Confirm (or **`F1`** to Cancel).

---

## 💡 Pro Tips

### 🚦 interpreting LED Feedback
*   **Performance Mode**: LEDs 1-4 light up when voices are playing.
*   **Sequencer Mode**: LED 1 flashes on the beat.
*   **Recording**: LED 4 turns RED (if RGB) or solid ON.

### ⚡ Shortcuts Cheat Sheet
| Gesture | Context | Action |
| :--- | :--- | :--- |
| **F1 + Pad 15** | Performance | Rapid Volume Change |
| **F1 + Pad 0-7** | Performance | Mute/Unmute Layer |
| **F1 + Pad 0-7** | Sequencer | Quick Track Select |
| **Long Press Step** | Sequencer | Parameter Locking (Edit step) |

---
*Created for the LofiOS Firmware v1.0*
