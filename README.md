# ESP32-S3 16-Step Sequencer & Launchpad

A powerful 16-step sequencer and launchpad for the ESP32-S3 with polyphonic audio synthesis.

## Features

### 🎹 Three Operating Modes

1. **LAUNCHPAD MODE** - Play notes live
   - 16 pads trigger notes in real-time
   - Polyphonic (up to 8 simultaneous voices)
   - Octave shifting with BTN_OCTAVE
   
2. **SEQUENCER MODE** - Create sequences
   - 16-step sequencer with 4 simultaneous tracks
   - Press pads to toggle notes on/off
   - Visual step grid on OLED
   - BPM range: 60-180 (default 120)
   
3. **SETTINGS MODE** - Configure instruments and playback
   - Select from 8 instrument presets
   - Adjust BPM
   - Start/stop playback
   - Clear tracks

### 🎵 8 Instrument Presets

1. **Sine** - Pure sine wave
2. **Square** - Classic square wave
3. **Saw** - Sawtooth wave
4. **Triangle** - Triangle wave
5. **Pluck** - Plucked string sound
6. **Bass** - Rich bass tone
7. **Pad** - Layered pad sound
8. **Lead** - Lead synth sound

## Hardware Setup

### Components
- ESP32-S3-WROOM-1U-N8R2 (8MB Flash, 2MB PSRAM)
- PCM5102A DAC (I2S audio output)
- SSD1306 OLED Display (128x64, I2C)
- 4x4 Button Matrix (16 pads)
- 2 Function Buttons
- 4 Status LEDs

### Pin Connections

#### I2S Audio (PCM5102A)
- **BCLK** → GPIO 6
- **LRC** → GPIO 7
- **DOUT** → GPIO 5

#### I2C Display (SSD1306)
- **SDA** → GPIO 48
- **SCL** → GPIO 47

#### Button Matrix
- **Rows** → GPIO 4, 3, 8, 15
- **Columns** → GPIO 16, 17, 18, 13

#### Function Buttons
- **BTN_MODE** → GPIO 36 (Mode cycling)
- **BTN_OCTAVE** → GPIO 37 (Octave/Track selection)

#### Status LEDs
- **LED 1-4** → GPIO 9, 10, 11, 12

## Controls

### Button Functions

| Button | Launchpad Mode | Sequencer Mode | Settings Mode |
|--------|---------------|----------------|---------------|
| **BTN_MODE** | Cycle to next mode | Cycle to next mode | Cycle to next mode |
| **BTN_OCTAVE** | Shift octave up | Select track (1-4) | - |
| **Pads 1-16** | Play notes | Toggle step on/off | Various settings |

### Settings Mode Pad Functions

| Pad | Function |
|-----|----------|
| 1-8 | Select instrument preset |
| 13 | Decrease BPM (-5) |
| 14 | Increase BPM (+5) |
| 15 | Toggle Play/Stop |
| 16 | Clear current track |

## OLED Display

The display shows different information based on the current mode:

### Launchpad Mode
```
LAUNCHPAD        BPM:120
Sine             Oct:4
```

### Sequencer Mode
```
SEQUENCER        BPM:120
Sine      Trk:1  >
━━━━━━━━━━━━━━━━━━━━
[■][□][■][□][■][□][■][□]...
> ····•·•·····•···
  ·•··············
  ················
  ·•·•·•·•·•·•·•·•
```

### Settings Mode
```
SETTINGS         BPM:120
Sine
━━━━━━━━━━━━━━━━━━━━
Instruments:
> Sine    Square
  Saw     Triangle
  Pluck   Bass
  Pad     Lead
```

## LED Indicators

- **Launchpad/Settings Mode**: Shows active track (1-4)
- **Sequencer Mode (Playing)**: Shows playback position (steps 1-4, 5-8, 9-12, 13-16)

## Workflow Examples

### Creating a Simple Beat

1. Press **BTN_MODE** twice to enter **SEQUENCER MODE**
2. Press **BTN_OCTAVE** to select Track 1
3. Press pads 1, 5, 9, 13 to create a kick pattern
4. Press **BTN_OCTAVE** to select Track 2
5. Press pads 3, 7, 11, 15 to create a snare pattern
6. Press **BTN_MODE** to enter **SETTINGS MODE**
7. Press pad 15 to start playback

### Live Performance

1. Ensure you're in **LAUNCHPAD MODE** (press BTN_MODE until it shows)
2. Press **BTN_MODE** then **SETTINGS MODE** to select an instrument
3. Return to **LAUNCHPAD MODE**
4. Press **BTN_OCTAVE** to change octaves
5. Play the 16 pads like a keyboard

### Layering Tracks

1. Enter **SEQUENCER MODE**
2. Select Track 1 (BTN_OCTAVE)
3. Program your melody
4. Press **BTN_MODE** → **SETTINGS MODE**
5. Select a different instrument for Track 2
6. Return to **SEQUENCER MODE**
7. Select Track 2 (BTN_OCTAVE)
8. Program a bassline
9. Repeat for Tracks 3 and 4

## Building & Uploading

### Using PlatformIO

```bash
# Build the project
pio run

# Upload to ESP32-S3
pio run --target upload

# Monitor serial output
pio device monitor
```

### Using Arduino IDE

1. Install ESP32 board support
2. Select board: "ESP32S3 Dev Module"
3. Install library: U8g2
4. Open `src/main.cpp`
5. Upload to board

## Technical Details

### Audio Specifications
- **Sample Rate**: 44.1 kHz
- **Bit Depth**: 16-bit
- **Polyphony**: 8 voices
- **Buffer Size**: 256 samples
- **Output**: Stereo I2S

### Sequencer Specifications
- **Steps**: 16 per track
- **Tracks**: 4 simultaneous
- **Resolution**: 16th notes
- **BPM Range**: 60-180
- **Note Range**: C2-C8 (7 octaves)

### Memory Usage
- **Flash**: ~200KB
- **RAM**: ~50KB
- **PSRAM**: Available for future expansion

## Troubleshooting

### No Audio Output
- Check PCM5102A connections (BCLK, LRC, DOUT)
- Verify I2S pins match your hardware
- Ensure PCM5102A has power (3.3V or 5V depending on module)

### Display Not Working
- Verify I2C connections (SDA=48, SCL=47)
- Check I2C address (default 0x3C)
- Ensure display has power

### Buttons Not Responding
- Check row/column pin connections
- Verify pullup resistors on column pins
- Test with multimeter for continuity

### Sequencer Not Playing
- Press pad 15 in Settings Mode to start
- Check that at least one step is programmed
- Verify BPM is set correctly

## Future Enhancements

- [ ] MIDI input/output
- [ ] Pattern save/load to PSRAM
- [ ] Effects (reverb, delay, filter)
- [ ] Swing/groove settings
- [ ] Song mode (chain patterns)
- [ ] CV/Gate output
- [ ] WiFi sync between multiple units

## License

MIT License - Feel free to modify and use in your projects!

## Credits

Created for ESP32-S3 hardware synthesizer project.
