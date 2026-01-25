# Lofi Beat Maker - ESP32-S3

A hardware lofi beat maker inspired by PO-33 and EP-133, built on ESP32-S3.

## Hardware

- **MCU**: ESP32-S3-WROOM-1U-N8R2 (8MB Flash, 2MB PSRAM)
- **DAC**: PCM5102A (I2S)
- **Display**: SSD1306 OLED 128x64 (I2C)
- **Input**: 4x4 Button Matrix (16 keys) + 2 Function Buttons
- **Output**: 4 Status LEDs

## Features

### Core Features (Implemented)

1. **Sample Playback Engine**
   - 16 one-shot sample slots (one per key)
   - Chromatic pitch shifting (±12 semitones)
   - Sample start/end point trimming
   - Basic amplitude envelope (attack/release)

2. **Pattern Sequencer**
   - 16-step sequencer
   - 4 patterns
   - Per-step note entry (which of 16 samples to trigger)
   - Tempo control (60-180 BPM range)
   - Pattern chaining for song mode

3. **Built-in Lofi Sound Bank**
   - Pre-load 8-12 essential samples into flash
   - *(User needs to add sample data)*

4. **Effects**
   - **Bit crusher** (4→8→12→16 bit) - essential for lofi aesthetic
   - **Low-pass filter** with resonance - warmth/muffling

5. **UI/Controls**
   - **Mode button**: Switch between Play/Pattern/Settings modes
   - **Function button combinations**:
     - Hold + key = adjust pitch/sample params
     - Hold + mode = save/load pattern
   - **LED feedback**: Step indicator (which of 16 steps is playing)
   - **OLED display**: Current pattern/BPM, active effect settings

## Building

This project uses PlatformIO.

1. Install PlatformIO
2. Open project in PlatformIO
3. Build: `pio run`
4. Upload: `pio run -t upload`

## Adding Samples

To add the built-in lofi sound bank, you need to:

1. Convert your samples to:
   - Format: 16-bit signed PCM
   - Sample rate: 22kHz (for lofi aesthetic) or 44.1kHz
   - Mono

2. Include them in your code. Example:

```cpp
// In loadDefaultSamples() function in main.cpp
#include "samples.h" // Your sample data header

// Load samples
sampleEngine.loadSample(0, kick1_data, kick1_length, 22050);
sampleEngine.loadSample(1, kick2_data, kick2_length, 22050);
sampleEngine.loadSample(2, snare1_data, snare1_length, 22050);
// ... etc
```

3. Create sample data header. You can use tools like:
   - `xxd -i sample.wav > sample.h` (after extracting raw PCM)
   - Online WAV to C array converters
   - Custom Python script to convert WAV to C array

## Documentation

- **[USER_GUIDE.md](USER_GUIDE.md)** - Comprehensive user manual with tutorials, troubleshooting, and advanced techniques
- **[QUICK_REFERENCE.md](QUICK_REFERENCE.md)** - Quick reference card for easy access while using the device

## Quick Usage

### Play Mode
- Press any key (0-15) to trigger the corresponding sample
- Press OCTAVE to start/stop the sequencer
- Hold OCTAVE + key to adjust pitch (when sequencer not playing)

### Pattern Mode
- Press key (0-15) to select/edit that step
- Press OCTAVE + key to assign sample to step
- Press MODE + key (0-3) to select pattern
- Sequencer starts/stops automatically when pattern is playing

### Settings Mode
- Press key 0-2 to select setting (BPM, BitCrusher, Filter)
- Press key 4-15 to adjust value

**For detailed instructions, see [USER_GUIDE.md](USER_GUIDE.md)**

## Memory Strategy

- **5MB**: Reserved for samples in SPIFFS partition (~30-60 seconds at 22kHz/16-bit)
- **2MB**: Application code/UI (factory partition)
- **2MB PSRAM**: Audio buffer/playback (runtime)
- **~1MB**: Bootloader, NVS, PHY init data

The custom partition table (`partitions.csv`) allocates:
- `factory`: 2MB for application code
- `samples`: 5MB SPIFFS filesystem for sample storage

## Audio Architecture

```
16 voices → Mixer → Bit crusher → Filter → I2S DAC
           (polyphonic)    ↓          ↓
                      Effect chain
```

## Future Enhancements

- Sample recording
- Live effects per pad
- Swing/groove quantization
- Multiple tracks
- SD card sample loading

## License

MIT
