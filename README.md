# ESP32-S3 LofiOS Sampler

## Overview
LofiOS is a minimal, functional, and aesthetically pleasing design language and firmware for an ESP32-S3 based lofi sampler. It features a custom 128x64 OLED interface, 16-pad matrix support, and a comprehensive navigation system inspired by modern music hardware like the OP-1 and Elektron devices.

---

## 🚀 Quick Start

### Hardware Setup
- **MCU**: ESP32-S3-WROOM-1U-N8R2
- **Display**: 128x64 monochrome OLED (I2C)
  - SDA: GPIO 48
  - SCL: GPIO 47
- **Input**:
  - 4x4 Button Matrix (Rows: 4,3,8,15 | Cols: 16,17,18,13)
  - F1 (SHIFT/BACK): GPIO 37
  - F2 (MODE/SELECT): GPIO 36
- **LEDs**: GPIO 9, 10, 11, 12

### Compilation
1. Install Arduino IDE with ESP32 support.
2. Install `U8g2` library.
3. Select board: `ESP32S3 Dev Module`.
4. Upload `SynthProject.ino`.

### First Use
1. **Power On**: Boots to **PERFORMANCE** mode.
2. **Play**: Tap pads 0-15 to trigger samples.
3. **Navigate**: 
   - **F2** moves forward (Next Mode / Confirm).
   - **F1** moves back (Previous Mode / Cancel).
   - **Hold F1 (800ms)** for sub-modes/modifiers.

---

## ⚡ Quick Reference

### Function Keys
| Key | Action | Function |
|-----|--------|----------|
| **F1** | Tap | BACK / EXIT / STOP |
| **F2** | Tap | NEXT / ENTER / CONFIRM |
| **F1** | Hold (800ms) | SHIFT / MODIFIER |

### Primary Modes (Cycle with F2)
1. **🎹 PERFORMANCE**: Live sample triggering.
2. **🎵 SEQUENCER**: Step programming (16 steps).
3. **🎼 SONG**: Pattern chaining.
4. **📦 KIT**: Load sample kits.
5. **⚙️ SETTINGS**: System configuration.

### Common Shortcuts
- **Octave Shift**: Tap F1 in Performance Mode.
- **Volume**: Hold F1 + Pad 15 (Performance Mode).
- **Edit Step**: Long Press Step Pad (Sequencer Mode).
- **FX Edit**: Hold F1 for 800ms (Performance Mode).
- **Mute Tracks**: Hold F1 + Pads 0-7 (Performance Mode).

---

## 📖 Comprehensive User Manual

### 1. PERFORMANCE MODE
**Purpose**: Live playing and performance.
- **Display**: Waveform scope (left), 4x4 pad grid (right).
- **Controls**:
  - **Pads 0-15**: Trigger samples.
  - **F1 Tap**: Cycle Octave (-1, 0, +1).
  - **F1 H + Pads 0-7**: Mute/Unmute tracks.
  - **F1 H + Pads 8-11**: Toggle FX (Crush, SR, Filt, Delay).
  - **F1 H + Pad 15**: Enter Volume Mode.

### 2. SEQUENCER MODE
**Purpose**: Step sequencing.
- **Display**: 16-step grid, track info.
- **Controls**:
  - **Pads 0-15**: Toggle steps.
  - **F1 Tap**: Play/Stop.
  - **F1 H + Pads 0-7**: Select Track (1-8).
  - **F1 H + Pads 8-11**: Select Pattern (1-4).
  - **F1 H + Pads 12-13**: BPM +/- 5.
  - **Long Press Step**: Enter **STEP EDIT** for detailed parameter locking (Velocity, Probability, Pitch, Filter).

### 3. SONG MODE
**Purpose**: Arrange patterns into a song.
- **Controls**:
  - **Pads 0-1**: Cursor Up/Down.
  - **Pads 2-5**: Select Pattern type for slot.
  - **Pads 6-7**: Adjust Repeat Count.
  - **Pads 8-9**: Insert/Delete Slot.
  - **F1 H + Pad 12**: Toggle Loop.

### 4. KIT MODE
**Purpose**: Select and load sample kits.
- **Controls**:
  - **Cursor**: Use Pads 0/1 to scroll.
  - **Load**: Press **F2** to load selected kit.
  - **Cancel**: Press **F1**.

### 5. SETTINGS MODE
**Purpose**: Global configuration.
- **Pages**:
  1. Audio (Volume, Tune, Attack, Release)
  2. Sequencer (BPM, Length, Metro, Swing)
  3. System (Auto-save, USB, Brightness, Reset)
- **Controls**:
  - **F2**: Edit selected setting.
  - **Pads 2-3**: Page Up/Down.

---

## 🔄 Navigation Flow & State Machine

### Mode Hierarchy
```
PERFORMANCE <-> SEQUENCER <-> SONG <-> KIT <-> SETTINGS
     ^              ^
     |              |
  [SUB-MODES]    [SUB-MODES]
  - Volume       - Step Edit
  - FX Edit
```

### Accessing Sub-Modes
- **Volume**: From Performance, hold F1 + Pad 15.
- **FX Edit**: From Performance, hold F1 (800ms).
- **Step Edit**: From Sequencer, long press any step (800ms).
- **Setting Edit**: From Settings, press F2 on an item.

All sub-modes auto-exit on completion or via **F1** (Back).

---

## 🛠 Troubleshooting

| Issue | Solution |
|-------|----------|
| **Stuck in validation?** | Press **F1** to back out of any menu. |
| **No Sound?** | Check Volume (Perf -> F1+Pad15). Ensure Kit is loaded. |
| **Can't Edit Step?** | Make sure to HOLD the pad for full 800ms. |
| **Display Glitch?** | Reset device (Hardware limitation of I2C sometimes). |

---
*Generated consolidated documentation.*
