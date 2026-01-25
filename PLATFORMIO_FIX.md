# Fixing PlatformIO Package Manifest Error

If you're getting the error:
```
MissingPackageManifestError: Could not find one of 'package.json' manifest files in the package
```

## Solution 1: Update PlatformIO Platform (Recommended)

1. Open PlatformIO IDE or terminal
2. Run these commands:
   ```bash
   pio platform update espressif32
   pio pkg install
   ```

## Solution 2: Clean and Rebuild

1. In PlatformIO, go to: **Platform > Clean Build Files**
2. Or run in terminal:
   ```bash
   pio run --target clean
   pio pkg install
   ```

## Solution 3: Reinstall Platform (if above doesn't work)

1. Remove the platform cache:
   ```bash
   pio platform uninstall espressif32
   pio platform install espressif32
   ```

## Solution 4: Check PlatformIO Version

Make sure you're using a recent version of PlatformIO:
- PlatformIO Core: 6.1 or later
- Update PlatformIO: `pio upgrade`

## Alternative: Use Specific Board Variant

If the issue persists, try using a more specific board variant. Your hardware is ESP32-S3-WROOM-1U-N8R2 (8MB Flash, 2MB PSRAM).

You might need to use:
```ini
board = esp32-s3-devkitc-1-n8r2
```

But first, check available boards:
```bash
pio boards espressif32 | grep -i "s3.*8.*2"
```
