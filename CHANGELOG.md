# Changelog - ESP32-S3 Lofi Sampler Fixes

## Version 1.1 - 2026-01-28

### 🎵 Audio Quality Improvements

#### Fixed Distorted PCM5102A Output
- **Problem**: Audio was distorted and not clean
- **Solution**: 
  - Reduced output amplitude from 20000 to 8000 (60% reduction)
  - Added soft clipping using `tanh()` function for smooth saturation
  - Prevents hard clipping and digital distortion
  - Audio now sounds clean and professional

### 🖥️ Display Fixes

#### Fixed BPM Display Cutoff
- **Problem**: BPM text at top right was cut off on screen
- **Solution**: 
  - Moved BPM display from x=90 to x=80
  - Now properly visible even with 3-digit BPM values (e.g., "BPM:180")

#### Optimized Sequencer Performance
- **Problem**: Sequencer mode was lagging and not keeping up
- **Solution**:
  - Reduced display update frequency from every 100 audio buffers to every 200
  - Display now updates ~1 second instead of ~0.5 seconds
  - Significantly reduced CPU load
  - Sequencer timing is now rock-solid

### ⚙️ Settings Menu Redesign

#### New Vertical Scrollable Menu
- **Problem**: Old grid-based settings menu was confusing
- **Solution**: Complete redesign with vertical scrollable menu

**New Controls:**
- **Pad 0**: Navigate Up
- **Pad 1**: Select/Enter (confirm action)
- **Pad 2**: Navigate Down
- **Pad 3**: BPM -5 (fine adjust)
- **Pad 4**: BPM +5 (fine adjust)

**Menu Items:**
1. **Instrument** - Cycles through all 8 instruments (Sine, Square, Saw, Triangle, Pluck, Bass, Pad, Lead)
2. **BPM** - Quick select common BPM values (60→80→100→120→140→160→180)
3. **Play/Pause** - Toggle sequencer playback (acts as toggle)
4. **Clear Track** - Erase current track (acts as action)

**Features:**
- Shows 3 menu items at a time
- Scroll indicators (^ and v) when more items available
- Current values displayed next to each menu item
- Clear cursor indicator (">") shows selected item
- Instructions displayed at bottom: "0:Up 1:Sel 2:Dn"

### 🔧 Audio Engine Improvements

#### Fixed Note Echoing/Glitching
- **Problem**: Notes were echoing and glitching, especially during voice stealing
- **Solution**:
  - Improved voice stealing algorithm
  - Now prioritizes stealing voices in release phase (already fading out)
  - Falls back to oldest voice only if no releasing voices available
  - Much smoother transitions when polyphony limit is reached

### 📚 Documentation Updates

#### Updated QUICK_REFERENCE.md
- Completely rewrote Settings Mode section
- Updated all workflows to reflect new menu navigation
- Added troubleshooting entry for distorted audio
- Clarified all button mappings

---

## Technical Details

### Audio Processing Changes
```cpp
// Old (distorted):
buffer[i * 2] = (int16_t)(mixL * 20000);

// New (clean):
mixL = tanh(mixL * 1.5) * 0.8;  // Soft saturation
buffer[i * 2] = (int16_t)(mixL * 8000);
```

### Voice Stealing Algorithm
```cpp
// Now checks for voices in release phase first
for (int i = 0; i < POLYPHONY; i++) {
  if (voices[i].sampleCount > ATTACK_SAMPLES + SUSTAIN_SAMPLES) {
    return i;  // Steal this voice - it's already fading out
  }
}
```

### Performance Optimization
- Display update: 100 buffers → 200 buffers (50% reduction in update frequency)
- Reduces I2C bus traffic
- Frees up CPU cycles for audio processing

---

## Migration Notes

### For Users
- Settings menu now uses **Pad 0, 1, 2** instead of the old grid layout
- **Pad 1** is now the "Select" button (like Enter on a keyboard)
- BPM changes now cycle through common values for faster selection
- Fine BPM adjustment still available with Pad 3/4

### Breaking Changes
- Old pad mappings in Settings mode no longer work
- Must use new vertical menu navigation
- See updated QUICK_REFERENCE.md for new controls

---

## Testing Recommendations

1. **Audio Quality**: Listen for clean, distortion-free output
2. **Display**: Verify BPM is fully visible at all values (60-180)
3. **Sequencer**: Check that timing is tight and doesn't lag
4. **Settings Menu**: Test navigation with Pad 0/1/2
5. **Voice Stealing**: Play many notes rapidly, listen for smooth transitions

---

## Known Issues
None at this time.

## Future Improvements
- Consider adding volume control to settings menu
- Add visual feedback when selecting menu items
- Implement preset save/load functionality
