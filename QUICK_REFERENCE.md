# Lofi Beat Maker - Quick Reference Card

## Mode Switching
- **MODE** = Cycle: Play → Pattern → Settings → Play

---

## 🎵 PLAY MODE
**Live performance and sequencer playback**

| Action | Button |
|--------|--------|
| Trigger sample | Press **0-15** |
| Start/stop sequencer | Press **OCTAVE** |
| Adjust pitch (future) | Hold **OCTAVE** + Press **0-15** |

**Display shows:** BPM, Pattern number, PLAYING/STOPPED

---

## 🎛️ PATTERN MODE
**Step sequencer editing**

| Action | Button |
|--------|--------|
| Select step | Press **0-15** |
| Toggle step on/off | Press **0-15** (again) |
| Assign sample to step | Hold **OCTAVE** + Press **0-15** |
| Select pattern | Hold **MODE** + Press **0-3** |

**Display shows:** Pattern number, Step number, Step grid

---

## ⚙️ SETTINGS MODE
**Tempo and effects adjustment**

### Select Setting
| Setting | Button |
|---------|--------|
| BPM (Tempo) | Press **0** |
| Bit Crusher | Press **1** |
| Filter | Press **2** |

### Adjust Value (after selecting setting)
| Button | BPM | Bit Crusher | Filter |
|--------|-----|-------------|--------|
| **4** | 60 BPM | OFF / 4 bits | OFF / 200 Hz |
| **5** | 70 BPM | 5 bits | ~1.8 kHz |
| **6** | 80 BPM | 6 bits | ~3.4 kHz |
| **7** | 90 BPM | 7 bits | ~5.0 kHz |
| **8** | 100 BPM | 8 bits | ~6.6 kHz |
| **9** | 110 BPM | 9 bits | ~8.2 kHz |
| **10** | 120 BPM | 10 bits | ~9.8 kHz |
| **11** | 130 BPM | 11 bits | ~11.4 kHz |
| **12** | 140 BPM | 12 bits | ~13.0 kHz |
| **13** | 150 BPM | 13 bits | ~14.6 kHz |
| **14** | 160 BPM | 14 bits | ~16.2 kHz |
| **15** | 170 BPM | 15 bits | ~20.0 kHz |

---

## 💡 LED INDICATORS
**4 LEDs show current step in binary (0-15)**

| LEDs | Step | LEDs | Step |
|------|------|------|------|
| 0 | 0 | 0,2 | 5 |
| 1 | 1 | 1,2 | 6 |
| 0,1 | 2 | 0,1,2 | 7 |
| 2 | 4 | 3 | 8 |
| 0,2 | 4 | 0,3 | 9 |
| 1,2 | 5 | 1,3 | 10 |
| 0,1,2 | 6 | 0,1,3 | 11 |
| 0,3 | 8 | 2,3 | 12 |
| 1,3 | 9 | 0,2,3 | 13 |
| 0,1,3 | 10 | 1,2,3 | 14 |
| 2,3 | 12 | 0,1,2,3 | 15 |

---

## 🎹 SAMPLE SLOTS
**Default test tones (chromatic scale)**

| Slot | Note | Frequency |
|------|------|-----------|
| 0 | C4 | 261.63 Hz |
| 1 | C#4 | 277.18 Hz |
| 2 | D4 | 293.66 Hz |
| 3 | D#4 | 311.13 Hz |
| 4 | E4 | 329.63 Hz |
| 5 | F4 | 349.23 Hz |
| 6 | F#4 | 369.99 Hz |
| 7 | G4 | 392.00 Hz |
| 8 | G#4 | 415.30 Hz |
| 9 | A4 | 440.00 Hz |
| 10 | A#4 | 466.16 Hz |
| 11 | B4 | 493.88 Hz |
| 12 | C5 | 523.25 Hz |
| 13 | C#5 | 554.37 Hz |
| 14 | D5 | 587.33 Hz |
| 15 | D#5 | 622.25 Hz |

---

## 🚀 QUICK START

1. **Power on** → Boots to Play Mode
2. **Press 0-15** → Test samples
3. **Press MODE** → Enter Pattern Mode
4. **Press 0** → Select step 0
5. **Hold OCTAVE + Press 0** → Assign sample 0
6. **Press MODE** → Return to Play Mode
7. **Press OCTAVE** → Start sequencer
8. **Press MODE** → Enter Settings Mode
9. **Press 0** → Select BPM
10. **Press 6** → Set 120 BPM
11. **Press MODE** → Return to Play Mode

---

## 📋 CREATING A BASIC BEAT

### Kick Pattern (Steps 0, 4, 8, 12)
1. Pattern Mode → Select step 0 → OCTAVE+0
2. Select step 4 → OCTAVE+0
3. Select step 8 → OCTAVE+0
4. Select step 12 → OCTAVE+0

### Snare Pattern (Steps 2, 6, 10, 14)
1. Select step 2 → OCTAVE+1
2. Select step 6 → OCTAVE+1
3. Select step 10 → OCTAVE+1
4. Select step 14 → OCTAVE+1

### Hi-Hat Pattern (Steps 1, 3, 5, 7, 9, 11, 13, 15)
1. Select step 1 → OCTAVE+2
2. Repeat for steps 3, 5, 7, 9, 11, 13, 15

### Play Your Beat
1. Play Mode → Press OCTAVE → Start!

---

## 🔧 TROUBLESHOOTING

| Problem | Solution |
|---------|----------|
| No audio | Check I2S connections (BCLK=6, LRC=7, DOUT=5) |
| No display | Check I2C connections (SDA=48, SCL=47) |
| Buttons not working | Check button matrix connections |
| Sequencer not playing | Press OCTAVE to start, check pattern has active steps |
| Effects not working | Enable in Settings Mode (button 4 = OFF) |

---

**For detailed information, see USER_GUIDE.md**
