# Quick Reference Guide

## Mode Switching
**BTN_MODE** cycles through:
1. LAUNCHPAD → 2. SEQUENCER → 3. SETTINGS → (back to 1)

---

## 🎹 LAUNCHPAD MODE

### Controls
- **Pads 1-16**: Play notes (polyphonic)
- **BTN_OCTAVE**: Cycle octave (2-6)
- **BTN_MODE**: Switch to Sequencer

### Display
```
LAUNCHPAD        BPM:120
[Instrument]     Oct:4
```

---

## 🎼 SEQUENCER MODE

### Controls
- **Pads 1-16**: Toggle step on/off
- **BTN_OCTAVE**: Select track (1→2→3→4→1)
- **BTN_MODE**: Switch to Settings

### Display
```
SEQUENCER        BPM:120
[Instrument] Trk:1  [>|||]
━━━━━━━━━━━━━━━━━━━━━━━━
[Step Grid - Current Track]
[All Tracks Overview]
```

### LED Indicators (Playing)
- LED 1: Steps 1-4
- LED 2: Steps 5-8
- LED 3: Steps 9-12
- LED 4: Steps 13-16

---

## ⚙️ SETTINGS MODE

### Navigation
- **Pad 0** (top-left): Navigate Up
- **Pad 1** (top-second): Select/Enter
- **Pad 2** (top-third): Navigate Down
- **Pad 3**: BPM -5 (fine adjust)
- **Pad 4**: BPM +5 (fine adjust)

### Menu Items

| Item | Function | Select Action |
|------|----------|---------------|
| **Instrument** | Change sound | Cycles through 8 instruments |
| **BPM** | Change tempo | Cycles: 60→80→100→120→140→160→180 |
| **Play/Pause** | Start/Stop sequencer | Toggles playback |
| **Clear Track** | Erase current track | Clears all steps |

### Display
```
SETTINGS         BPM:120
━━━━━━━━━━━━━━━━━━━━━━━━
Settings Menu:          ^
> Instrument      Sine
  BPM             120
  Play/Pause      Stopped
                      v
0:Up 1:Sel 2:Dn
```


---

## 🎵 Instrument Sounds

1. **Sine** - Pure, smooth tone
2. **Square** - Hollow, retro game sound
3. **Saw** - Bright, buzzy synth
4. **Triangle** - Soft, mellow tone
5. **Pluck** - Percussive, decaying sound
6. **Bass** - Deep, rich bass
7. **Pad** - Layered, atmospheric
8. **Lead** - Cutting, bright lead

---

## 📝 Quick Workflows

### Create a Beat (30 seconds)
1. **BTN_MODE** × 2 → SEQUENCER
2. Press pads 1, 5, 9, 13 (kick pattern)
3. **BTN_OCTAVE** (switch to track 2)
4. Press pads 3, 7, 11, 15 (snare pattern)
5. **BTN_MODE** → SETTINGS
6. Navigate to "Play/Pause" and press **Pad 1** (select)

### Change Instrument
1. **BTN_MODE** → SETTINGS
2. Navigate to "Instrument" (Pad 0/2)
3. Press **Pad 1** to cycle through instruments
4. **BTN_MODE** → Return to previous mode

### Adjust Tempo
1. **BTN_MODE** → SETTINGS
2. Navigate to "BPM" (Pad 0/2)
3. Press **Pad 1** to cycle common BPM values
4. Or use **Pad 3/4** for fine adjustment (-5/+5)
5. Range: 60-180 BPM

### Layer 4 Tracks
1. SEQUENCER mode
2. Program Track 1
3. **BTN_OCTAVE** → Track 2
4. **BTN_MODE** → SETTINGS, change instrument
5. **BTN_MODE** → SEQUENCER, program Track 2
6. Repeat for Tracks 3 & 4

---

## 🔧 Troubleshooting

| Problem | Solution |
|---------|----------|
| No sound | Check I2S wiring, verify PCM5102A power |
| Distorted audio | Reduce volume on amplifier/speakers |
| No display | Check I2C pins (SDA=48, SCL=47) |
| Buttons stuck | Check matrix wiring, verify pullups |
| Won't play | Go to Settings → Play/Pause → Press Pad 1 |

---

## 💡 Pro Tips

- **Layer rhythms**: Use different instruments per track
- **Octave tricks**: Change octave in Launchpad for different note ranges
- **Clear wisely**: Pad 16 only clears current track
- **Live jamming**: Use Launchpad mode while sequencer plays
- **BPM sweet spots**: 
  - 80-90: Hip-hop
  - 120-130: House
  - 140-150: Techno
  - 160-180: Drum & Bass

---

## 🎛️ Default Settings

- **Mode**: Launchpad
- **BPM**: 120
- **Octave**: 4
- **Instrument**: Sine
- **Track**: 1
- **Playing**: Stopped
- **All sequences**: Empty

---

**Press BTN_MODE to cycle modes anytime!**
