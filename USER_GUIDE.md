# Lofi Beat Maker - User Guide

## Table of Contents
1. [Getting Started](#getting-started)
2. [Hardware Overview](#hardware-overview)
3. [Understanding the Three Modes](#understanding-the-three-modes)
4. [Play Mode - Live Performance](#play-mode---live-performance)
5. [Pattern Mode - Step Sequencing](#pattern-mode---step-sequencing)
6. [Settings Mode - Effects & Tempo](#settings-mode---effects--tempo)
7. [Button Reference](#button-reference)
8. [Creating Your First Beat](#creating-your-first-beat)
9. [Advanced Techniques](#advanced-techniques)
10. [Troubleshooting](#troubleshooting)

---

## Getting Started

### First Power-On
When you first power on your Lofi Beat Maker:
1. The OLED display will show "PLAY MODE"
2. A test tone (C4, 261.63 Hz) will automatically play
3. All 16 sample slots are pre-loaded with test tones (chromatic scale)
4. The sequencer is stopped by default

### Quick Test
- **Press any button (0-15)** in Play Mode to hear the corresponding test tone
- **Press OCTAVE button** to start/stop the sequencer
- **Press MODE button** to cycle through: Play → Pattern → Settings → Play

---

## Hardware Overview

### Button Layout
```
┌─────────────────────────┐
│  [MODE]  [OCTAVE]       │  Function Buttons
├─────────────────────────┤
│  [0]  [1]  [2]  [3]     │
│  [4]  [5]  [6]  [7]     │  4x4 Button Matrix
│  [8]  [9]  [10] [11]    │  (16 keys total)
│  [12] [13] [14] [15]    │
└─────────────────────────┘
```

### Display
- **128x64 OLED** shows current mode, BPM, pattern info, and settings
- Updates every 50ms for smooth feedback

### LEDs
- **4 status LEDs** show the current step in binary (0-15)
- LED 0 = bit 0, LED 1 = bit 1, LED 2 = bit 2, LED 3 = bit 3
- Example: Step 5 = LEDs 0 and 2 on (binary 0101)

---

## Understanding the Three Modes

### Mode Indicators
- **PLAY MODE**: Live performance and sequencer playback
- **PATTERN MODE**: Step sequencer editing
- **SETTINGS**: Tempo and effects adjustment

**Switch modes**: Press the **MODE** button to cycle through modes

---

## Play Mode - Live Performance

Play Mode is for live performance and sequencer playback.

### Basic Operations

#### Trigger Samples
- **Press any button (0-15)** to instantly trigger that sample
- Samples play with a 10ms attack and 100ms release envelope
- Multiple samples can play simultaneously (polyphonic)

#### Start/Stop Sequencer
- **Press OCTAVE button** to start the sequencer
- **Press OCTAVE button again** to stop
- When stopped, the sequencer resets to step 0

#### While Sequencer is Playing
- You can still press buttons to trigger samples live
- Live triggers don't affect the sequencer pattern
- The LEDs show which step is currently playing

#### While Sequencer is Stopped
- **Hold OCTAVE + Press button (0-15)** = Adjust pitch (future feature)

### Display Information
- Shows "PLAY MODE" at the top
- Displays current BPM
- Shows current pattern number (0-3)
- Shows "PLAYING" or "STOPPED" status

---

## Pattern Mode - Step Sequencing

Pattern Mode is where you create and edit your beats.

### Understanding Patterns
- **4 patterns** available (Pattern 0, 1, 2, 3)
- Each pattern has **16 steps**
- Each step can trigger one of the 16 samples
- Steps can be enabled/disabled (toggled)

### Editing Steps

#### Select a Step
1. **Press any button (0-15)** to select that step
2. The display shows "P:X S:Y" (Pattern X, Step Y)
3. The step grid highlights the selected step

#### Toggle a Step On/Off
1. Select a step (press its button)
2. **Press the same button again** to toggle it on/off
3. Active steps show as filled boxes, inactive as outlines

#### Assign a Sample to a Step
1. Select a step (press button 0-15)
2. **Hold OCTAVE button**
3. **Press button (0-15)** to assign that sample to the step
4. The step is automatically enabled

#### Select a Pattern
1. **Hold MODE button**
2. **Press button 0, 1, 2, or 3** to select that pattern
3. The display updates to show the new pattern

### Visual Feedback
- **Step Grid**: 4x4 grid showing all 16 steps
  - Filled box = step is active
  - Outline = step is inactive
  - Highlighted = current step (when sequencer is playing)
- **Current Step**: Highlighted with a frame when playing

### Pattern Workflow
1. Enter Pattern Mode
2. Select pattern (MODE + 0-3)
3. Select step (press 0-15)
4. Assign sample (OCTAVE + 0-15)
5. Toggle steps on/off as needed
6. Return to Play Mode
7. Start sequencer (OCTAVE)

---

## Settings Mode - Effects & Tempo

Settings Mode lets you adjust tempo and effects.

### Available Settings
1. **BPM** (Tempo) - Button 0
2. **Bit Crusher** - Button 1
3. **Low-Pass Filter** - Button 2

### Adjusting Settings

#### Step 1: Select Setting
- **Press button 0** = Select BPM
- **Press button 1** = Select Bit Crusher
- **Press button 2** = Select Filter

#### Step 2: Adjust Value
Once a setting is selected, use buttons 4-15 to adjust:

**BPM (Button 0)**
- Buttons 4-15 set BPM from 60 to 170 in steps of 10
- Button 4 = 60 BPM
- Button 5 = 70 BPM
- ...
- Button 15 = 170 BPM

**Bit Crusher (Button 1)**
- Buttons 4-15 set bit depth from 4 to 15 bits
- Button 4 = 4 bits (most lofi)
- Button 5 = 5 bits
- ...
- Button 15 = 15 bits (less lofi)
- Button 4 = OFF (disables bit crusher)

**Filter Cutoff (Button 2)**
- Buttons 4-15 set filter cutoff from 200 Hz to 20 kHz
- Button 4 = 200 Hz (very muffled)
- Button 5-15 = Gradual increase
- Button 15 = 20 kHz (bright)
- Button 4 = OFF (disables filter)

### Display Information
- Shows "SETTINGS" at the top
- Displays current BPM
- Shows Bit Crusher bits and ON/OFF status
- Shows Filter cutoff frequency and ON/OFF status
- Highlights the selected setting with an underline

---

## Button Reference

### Function Buttons

| Button | Function |
|--------|----------|
| **MODE** | Cycle through modes (Play → Pattern → Settings → Play) |
| **MODE + 0-3** | (In Pattern Mode) Select pattern 0-3 |
| **OCTAVE** | (In Play Mode) Start/stop sequencer |
| **OCTAVE + 0-15** | (In Pattern Mode) Assign sample to selected step |
| **OCTAVE + 0-15** | (In Play Mode, stopped) Adjust pitch (future) |

### Matrix Buttons (0-15)

#### Play Mode
- **0-15**: Trigger sample 0-15

#### Pattern Mode
- **0-15**: Select step 0-15
- **0-15** (again): Toggle step on/off
- **OCTAVE + 0-15**: Assign sample to selected step

#### Settings Mode
- **0-2**: Select setting (BPM, BitCrusher, Filter)
- **4-15**: Adjust selected setting value

---

## Creating Your First Beat

### Step-by-Step Tutorial

1. **Power On**
   - Device boots into Play Mode
   - Test tones are loaded in all 16 slots

2. **Enter Pattern Mode**
   - Press **MODE** button until display shows "PATTERN MODE"

3. **Select Pattern 0**
   - Hold **MODE** button
   - Press button **0**

4. **Create a Kick Pattern**
   - Press button **0** (select step 0)
   - Hold **OCTAVE** and press **0** (assign sample 0 to step 0)
   - Press button **4** (select step 4)
   - Hold **OCTAVE** and press **0** (assign sample 0 to step 4)
   - Press button **8** (select step 8)
   - Hold **OCTAVE** and press **0** (assign sample 0 to step 8)
   - Press button **12** (select step 12)
   - Hold **OCTAVE** and press **0** (assign sample 0 to step 12)

5. **Add a Snare**
   - Press button **2** (select step 2)
   - Hold **OCTAVE** and press **1** (assign sample 1 to step 2)
   - Press button **6** (select step 6)
   - Hold **OCTAVE** and press **1** (assign sample 1 to step 6)
   - Press button **10** (select step 10)
   - Hold **OCTAVE** and press **1** (assign sample 1 to step 10)
   - Press button **14** (select step 14)
   - Hold **OCTAVE** and press **1** (assign sample 1 to step 14)

6. **Add Hi-Hats**
   - Press button **1** (select step 1)
   - Hold **OCTAVE** and press **2** (assign sample 2)
   - Repeat for steps 3, 5, 7, 9, 11, 13, 15

7. **Return to Play Mode**
   - Press **MODE** button until display shows "PLAY MODE"

8. **Start the Sequencer**
   - Press **OCTAVE** button
   - Your beat should start playing!

9. **Adjust Tempo (Optional)**
   - Press **MODE** to enter Settings Mode
   - Press button **0** (select BPM)
   - Press button **6** (set to 120 BPM)
   - Press **MODE** to return to Play Mode

---

## Advanced Techniques

### Pattern Chaining
Pattern chaining allows you to create longer sequences by linking patterns together.

**How it works:**
- When a pattern finishes, it automatically moves to the next pattern in the chain
- Chains loop continuously

**To use pattern chaining:**
- Currently implemented in code but needs UI enhancement
- You can programmatically set chains using the sequencer API

### Live Performance Tips

1. **Layer Live Samples**
   - Start the sequencer
   - Press buttons to add live samples on top
   - Great for fills and accents

2. **Quick Pattern Switching**
   - Create variations in different patterns
   - Switch patterns while sequencer is playing (MODE + 0-3 in Pattern Mode)

3. **Use Effects for Variation**
   - Adjust bit crusher for different sections
   - Use filter to create build-ups and breakdowns

### Sample Management

**Current Test Tones:**
- All 16 slots are loaded with chromatic scale tones (C4 to D#5)
- Key 0 = C4 (261.63 Hz)
- Key 1 = C#4 (277.18 Hz)
- Key 2 = D4 (293.66 Hz)
- ... and so on

**To Load Your Own Samples:**
1. Convert samples to 16-bit signed PCM, mono
2. Recommended: 22 kHz sample rate for lofi aesthetic
3. Edit `loadDefaultSamples()` in `main.cpp`
4. Use `sampleEngine.loadSample(index, data, length, sampleRate)`

---

## Troubleshooting

### No Audio Output
1. **Check I2S connections:**
   - BCLK → GPIO 6
   - LRC → GPIO 7
   - DOUT → GPIO 5

2. **Check Serial Monitor:**
   - Look for "I2S driver installed successfully"
   - Check for any error messages

3. **Verify test tone:**
   - Press button 0 in Play Mode
   - You should hear a tone

### Display Not Working
1. **Check I2C connections:**
   - SDA → GPIO 48
   - SCL → GPIO 47

2. **Check Serial Monitor:**
   - Look for "UI initialized"
   - Check for I2C errors

### Buttons Not Responding
1. **Check button matrix connections:**
   - Row pins: 4, 3, 8, 15
   - Column pins: 16, 17, 18, 13

2. **Check function buttons:**
   - MODE → GPIO 36
   - OCTAVE → GPIO 37

### Sequencer Not Playing
1. **Check if sequencer is started:**
   - Display should show "PLAYING" in Play Mode
   - Press OCTAVE to start

2. **Check if pattern has active steps:**
   - Enter Pattern Mode
   - Verify steps are enabled (filled boxes)

3. **Check BPM setting:**
   - Enter Settings Mode
   - Verify BPM is set (not 0)

### Samples Not Loading
1. **Check memory:**
   - Serial Monitor shows memory allocation status
   - Ensure PSRAM is detected

2. **Check sample format:**
   - Must be 16-bit signed PCM
   - Mono (not stereo)
   - Sample rate: 22050 or 44100 Hz recommended

### Effects Not Working
1. **Verify effects are enabled:**
   - Enter Settings Mode
   - Check that Bit Crusher or Filter shows "ON"

2. **Check effect values:**
   - Bit Crusher: Button 4 = OFF, Buttons 5-15 = ON
   - Filter: Button 4 = OFF, Buttons 5-15 = ON

---

## Quick Reference Card

### Mode Switching
- **MODE**: Cycle modes (Play → Pattern → Settings)

### Play Mode
- **0-15**: Trigger sample
- **OCTAVE**: Start/stop sequencer

### Pattern Mode
- **0-15**: Select step
- **0-15** (again): Toggle step
- **OCTAVE + 0-15**: Assign sample
- **MODE + 0-3**: Select pattern

### Settings Mode
- **0**: Select BPM
- **1**: Select Bit Crusher
- **2**: Select Filter
- **4-15**: Adjust value

### LED Indicators
- Binary display of current step (0-15)
- LED 0 = bit 0, LED 1 = bit 1, etc.

---

## Tips for Best Results

1. **Start Simple**
   - Create a basic kick-snare pattern first
   - Add complexity gradually

2. **Use Test Tones to Learn**
   - The chromatic scale helps you understand pitch relationships
   - Experiment with different step patterns

3. **Experiment with Effects**
   - Bit crusher adds character
   - Filter creates movement and dynamics

4. **Save Your Patterns**
   - Patterns are stored in memory while powered
   - Consider implementing save/load functionality

5. **Practice Live Performance**
   - Layer samples while sequencer plays
   - Use different patterns for verse/chorus

---

## Technical Specifications

- **Audio Rate**: 44.1 kHz
- **Bit Depth**: 16-bit
- **Max Voices**: 16 (polyphonic)
- **Max Samples**: 16 slots
- **Patterns**: 4 patterns
- **Steps per Pattern**: 16 steps
- **BPM Range**: 60-180 BPM
- **Effects**: Bit Crusher (4-15 bits), Low-Pass Filter (200 Hz - 20 kHz)

---

## Support & Development

For issues, feature requests, or contributions, please refer to the main README.md file.

**Happy Beat Making! 🎵**
