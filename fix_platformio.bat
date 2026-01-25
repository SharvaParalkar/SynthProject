@echo off
echo Fixing PlatformIO framework corruption (missing FreeRTOS headers and bootloader)...
echo.

echo Step 1: Cleaning project build...
if exist ".pio" (
    echo Removing project build directory...
    rmdir /s /q ".pio"
)

echo.
echo Step 2: Cleaning PlatformIO packages cache...
if exist "%USERPROFILE%\.platformio\packages" (
    echo Removing packages cache...
    rmdir /s /q "%USERPROFILE%\.platformio\packages"
)

if exist "%USERPROFILE%\.platformio\.cache" (
    echo Removing build cache...
    rmdir /s /q "%USERPROFILE%\.platformio\.cache"
)

echo.
echo Step 3: Updating PlatformIO core...
"%USERPROFILE%\.platformio\penv\Scripts\platformio.exe" upgrade

echo.
echo Step 4: Uninstalling espressif32 platform (if exists)...
"%USERPROFILE%\.platformio\penv\Scripts\platformio.exe" platform uninstall espressif32

echo.
echo Step 5: Installing espressif32 platform (fresh install)...
"%USERPROFILE%\.platformio\penv\Scripts\platformio.exe" platform install espressif32

echo.
echo Step 6: Installing all required packages...
"%USERPROFILE%\.platformio\penv\Scripts\platformio.exe" pkg install

echo.
echo Step 7: Verifying installation...
"%USERPROFILE%\.platformio\penv\Scripts\platformio.exe" platform list

echo.
echo Done! Try building your project now with: pio run
pause
