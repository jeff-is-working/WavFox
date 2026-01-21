# CLAUDE.md - AI Assistant Guide for WavFox

This document provides comprehensive information about the WavFox codebase structure, development workflows, and conventions specifically for AI assistants working on this project.

## Project Overview

**WavFox** is an ESP32-based amateur radio fox hunt transmitter that plays custom WAV audio messages instead of traditional Morse code. It's designed for the HackerBox Fox Hunt kit using a TTGO ESP32 T-Display and SA818 VHF/UHF radio module.

### Key Information
- **License**: GNU General Public License v3.0 (GPL-3.0)
- **Platform**: ESP32 (TTGO T-Display)
- **Language**: Arduino C++ (firmware), Python 3 (tooling)
- **Hardware**: ESP32 + SA818 radio transceiver + ST7789 TFT display
- **Purpose**: Amateur radio direction finding (RDF) / fox hunt events

### Legal Requirements
- **CRITICAL**: Amateur radio license required to operate
- **FCC Part 97 Compliance**: All transmissions must include valid callsign
- **Frequency**: Default 146.565 MHz (verify legality for license class)
- **NO modifications** to transmission code without understanding FCC regulations

---

## Repository Structure

```
WavFox/
├── Fox_Hunt_TX_WAV.ino        # PRIMARY: Main sketch with WAV playback
├── Fox_Hunt_TX.ino            # LEGACY: Original Morse-only version
├── fox_wav_converter.py       # Audio conversion utility
├── README.md                  # User documentation
├── WAV_Upload_Guide.md        # WAV file preparation guide
├── LICENSE                    # GPL-3.0 license
├── CLAUDE.md                  # This file
├── .github/workflows/
│   └── codeql.yml            # Security scanning workflow
└── data/                      # (User-created) SPIFFS filesystem folder
    └── foxmessage.wav        # (User-provided) Audio file for transmission
```

### File Purposes

#### Fox_Hunt_TX_WAV.ino (PRIMARY)
- **Main firmware** with WAV file playback capability
- Includes SPIFFS filesystem support
- WAV header parsing and DAC playback
- Automatic fallback to Morse code if WAV not found
- Lines 48-50: User configuration (callsign, frequency)
- Lines 188-253: WAV playback implementation
- Lines 382-420: Morse code fallback

#### Fox_Hunt_TX.ino (LEGACY)
- Original Morse code-only implementation
- **DO NOT** modify unless specifically requested
- Kept for reference and backwards compatibility
- Use Fox_Hunt_TX_WAV.ino for new development

#### fox_wav_converter.py
- Audio conversion tool (requires pydub + ffmpeg)
- Converts any audio format to ESP32-compatible WAV
- Default: 22050 Hz, 8-bit mono, 5-second max
- Supports batch conversion
- Lines 24-115: Main conversion function
- Lines 118-166: Batch processing

---

## Hardware Architecture

### Pin Mapping (CRITICAL - DO NOT CHANGE)

| Component | GPIO Pin | Function | Notes |
|-----------|----------|----------|-------|
| **Audio Output** | GPIO 25 | DAC2 → SA818 AF_IN | Primary audio path |
| **PTT (Push-to-Talk)** | GPIO 13 | SA818 PTT control | LOW=TX, HIGH=RX |
| **SA818 Serial RX** | GPIO 21 | UART communication | AT commands |
| **SA818 Serial TX** | GPIO 22 | UART communication | AT commands |
| **SA818 Power** | GPIO 27 | Power control | HIGH=ON, LOW=OFF |
| **SA818 H/L** | GPIO 33 | Power level | LOW=Low power |
| **SA818 Audio In** | GPIO 17 | Audio detection | Input only |
| **Display DC** | GPIO 16 | ST7789 Data/Command | |
| **Display CS** | GPIO 5 | ST7789 Chip Select | |
| **Display SCL** | GPIO 18 | ST7789 Clock | |
| **Display SDA** | GPIO 19 | ST7789 Data | |
| **Display Reset** | GPIO 23 | ST7789 Reset | |
| **Display Backlight** | GPIO 4 | Backlight control | |
| **Button A** | GPIO 15 | User button | INPUT_PULLDOWN |
| **Button B** | GPIO 26 | User button | INPUT_PULLDOWN |
| **Button C** | GPIO 32 | User button | INPUT_PULLDOWN |

### Critical Hardware Notes
1. **GPIO 25 is DAC2**: This is the ONLY pin that can do native DAC output for audio
2. **PTT Logic**: LOW = Transmit, HIGH = Receive (inverted logic!)
3. **Serial2**: Used for SA818 AT command communication (9600 baud)
4. **SPIFFS**: Requires partition scheme "Default 4MB with spiffs" in Arduino IDE

---

## Code Architecture

### Initialization Flow (setup())

```
1. Serial/Serial2 initialization (lines 74-75)
2. Morse code generation from callsign (line 77)
3. GPIO pin configuration (lines 79-106)
4. TFT display initialization (lines 100-112)
5. SA818 radio initialization via AT commands (lines 118-138)
6. SPIFFS filesystem mounting (lines 144-168)
7. WAV file detection and validation (lines 153-167)
8. Display status screen (lines 170-172)
```

### Main Loop (loop())

```
1. Assert PTT (transmit mode) - digitalWrite(SA818_PTT, LOW)
2. Wait 750ms for radio stabilization
3. Play WAV file OR Morse code
4. Release PTT (receive mode) - digitalWrite(SA818_PTT, HIGH)
5. Wait 15 seconds
6. Repeat
```

### WAV Playback Architecture (playWAV())

**Location**: Lines 188-253 in Fox_Hunt_TX_WAV.ino

**Flow**:
1. Open WAV file from SPIFFS
2. Read and validate 44-byte WAV header
3. Extract: sample rate, bit depth, channels
4. Calculate timing: `sampleDelay = 1000000 / sampleRate` (microseconds)
5. Read audio in 512-byte chunks
6. Convert samples to 8-bit DAC values (0-255)
7. Output via `dacWrite(SA818_AF_IN, value)`
8. Precise timing via `delayMicroseconds(sampleDelay)`
9. Return DAC to neutral (128) when complete

**Important**:
- 8-bit audio: Direct output (0-255)
- 16-bit audio: Converted to 8-bit: `(sample16 + 32768) >> 8`
- Timing is CRITICAL for proper playback speed

### WAV Header Structure

```cpp
struct WAVHeader {
  char riff[4];           // "RIFF" magic number
  uint32_t fileSize;      // File size - 8 bytes
  char wave[4];           // "WAVE" format
  char fmt[4];            // "fmt " chunk
  uint32_t fmtSize;       // Usually 16 for PCM
  uint16_t audioFormat;   // 1 = PCM (uncompressed)
  uint16_t numChannels;   // 1 = mono, 2 = stereo
  uint32_t sampleRate;    // Hz (e.g., 22050, 44100)
  uint32_t byteRate;      // Calculated field
  uint16_t blockAlign;    // Calculated field
  uint16_t bitsPerSample; // 8 or 16
  char data[4];           // "data" chunk marker
  uint32_t dataSize;      // Audio data size in bytes
};
// Total: 44 bytes
```

### SA818 AT Commands

The SA818 radio module is controlled via AT commands over Serial2:

| Command | Purpose | Expected Response |
|---------|---------|------------------|
| `AT+DMOCONNECT` | Establish connection | `+DMOCONNECT:0` |
| `AT+VERSION` | Get firmware version | Version string |
| `AT+DMOSETGROUP=0,freq,freq,0000,1,0000` | Set frequency | `+DMOSETGROUP:0` |
| `AT+DMOSETVOLUME=8` | Set volume (1-8) | `+DMOSETVOLUME:0` |

**Format**: All commands end with `\r\n` (CRLF)

---

## Development Workflows

### Making Code Changes

#### 1. User Configuration Changes (SAFE)
**Lines 48-50 in Fox_Hunt_TX_WAV.ino**

```cpp
String callmessage = "N0CALL";     // User's FCC callsign
float frequency = 146.565;         // TX frequency in MHz
const char* wavFileName = "/foxmessage.wav";  // WAV filename
```

**AI Assistant Guidelines**:
- ALWAYS validate callsign format (e.g., K1ABC, WA2XYZ)
- WARN about frequency legality (check license class)
- Verify WAV filename starts with `/` (SPIFFS requirement)

#### 2. Timing Adjustments
**Line 176 (pre-transmission delay)**: Currently 750ms
**Line 185 (inter-transmission delay)**: Currently 15000ms (15 seconds)

**AI Assistant Guidelines**:
- Pre-TX delay should be 500-1000ms (radio stabilization)
- Inter-TX delay should be 10-60 seconds (typical fox hunt)
- WARN if inter-TX < 5000ms (annoying for operators)

#### 3. Audio/WAV Changes
**DO NOT** modify playWAV() unless specifically requested
**Complex**: Timing-sensitive, hardware-specific

**If modifications needed**:
1. Test thoroughly with multiple WAV formats
2. Verify sample rate calculations
3. Check for buffer overruns
4. Validate DAC output range (0-255)

#### 4. Display Changes
**Text size**: Line 102 `tft.setTextSize(2)`
**Colors**: ST77XX_RED, ST77XX_YELLOW, ST77XX_BLUE, ST77XX_GREEN, ST77XX_BLACK, ST77XX_WHITE
**Screen**: 240x135 pixels (landscape with rotation 1)

**AI Assistant Guidelines**:
- Keep text readable (size 2 recommended)
- Use high-contrast colors
- Test on actual display size (small screen!)

### Adding New Features

#### Multiple WAV Files
**User Request**: "Support multiple WAV files with button selection"

**Implementation Pattern**:
```cpp
// In setup(), after button pinMode declarations (after line 81)
if(digitalRead(Button_A) == HIGH) {
  wavFileName = "/fox1.wav";
} else if(digitalRead(Button_B) == HIGH) {
  wavFileName = "/fox2.wav";
} else if(digitalRead(Button_C) == HIGH) {
  wavFileName = "/fox3.wav";
}
```

**AI Assistant Checklist**:
- [ ] Read buttons during setup() only (not in loop)
- [ ] Update display to show selected file
- [ ] Document which button = which file
- [ ] Remind user to upload all WAV files to data/

#### Volume Control
**User Request**: "Add volume adjustment"

**Implementation Pattern**:
```cpp
// Add global variable after line 54
float volumeScale = 0.8;  // 0.0 to 1.0

// In playWAV(), after reading sample (line 233 for 8-bit)
uint8_t sample8 = buffer[i] * volumeScale;
dacWrite(SA818_AF_IN, sample8);
```

**AI Assistant Checklist**:
- [ ] Clamp volume to 0.0-1.0 range
- [ ] Consider button control for runtime adjustment
- [ ] WARN about clipping if volume > 1.0

### Python Tool Modifications (fox_wav_converter.py)

**Common Requests**:
1. Change default sample rate
2. Add new output formats
3. Batch processing improvements

**AI Assistant Guidelines**:
- Maintain pydub dependency structure
- Keep command-line interface consistent
- Preserve error handling patterns
- Update help text if adding options

---

## Testing Guidelines

### Pre-Upload Checklist

When assisting with code changes, ensure user:

1. **Verified Configuration**:
   - [ ] Callsign is their valid FCC callsign
   - [ ] Frequency is legal for their license class
   - [ ] Timing values are reasonable

2. **For WAV Changes**:
   - [ ] WAV file exists in data/ folder
   - [ ] File is exactly `foxmessage.wav` (or matches code)
   - [ ] Format: Mono, 8/16-bit PCM, 11025-44100 Hz
   - [ ] Duration: < 10 seconds (< 5 seconds recommended)
   - [ ] File size: < 500 KB

3. **Arduino IDE Settings**:
   - [ ] Board: "ESP32 Dev Module" or "TTGO T-Display"
   - [ ] **Partition Scheme: "Default 4MB with spiffs"** ← CRITICAL
   - [ ] Upload Speed: 921600 (or 115200 if failing)

4. **Upload Process**:
   - [ ] Upload sketch first (Ctrl+U)
   - [ ] Then upload SPIFFS: Tools → ESP32 Sketch Data Upload

### Serial Monitor Debugging

**Baud Rate**: 9600 (line 74)

**Expected Output**:
```
SPIFFS Mounted
WAV file size: XXXXX bytes
Sample Rate: 22050
Bits per Sample: 8
Channels: 1
Playing WAV file
WAV playback complete
```

**Error Messages**:
- `SPIFFS Mount Failed` → Partition scheme wrong or SPIFFS corrupted
- `WAV file not found` → File missing or wrong name
- `Invalid WAV file` → File corrupted or wrong format

### Common Issues & Solutions

| Symptom | Cause | Solution |
|---------|-------|----------|
| "Mode: Morse" on display | WAV not found | Upload data/ folder via ESP32 Sketch Data Upload |
| Audio too fast/slow | Sample rate mismatch | Re-export WAV at 22050 Hz |
| Distorted audio | Wrong bit depth or clipping | Use 8-bit or 16-bit PCM, normalize to -3dB |
| No transmission | PTT not working | Check GPIO 13 wiring |
| Display blank | TFT wiring issue | Verify pins 4,5,16,18,19,23 |
| Won't upload | BOOT button needed | Press/hold BOOT during upload |

---

## Code Style & Conventions

### Existing Patterns (FOLLOW THESE)

1. **Pin Definitions**: Use `#define` (lines 26-46)
   ```cpp
   #define PIN_NAME 12
   ```

2. **Global Variables**: Declared after pin definitions (lines 48-54)
   ```cpp
   String callmessage = "N0CALL";
   ```

3. **Function Organization**:
   - `setup()` → initialization
   - `loop()` → main execution
   - Helper functions below (setfreq, disp_channel, playWAV, playMorse, etc.)

4. **Comments**:
   - Function purposes documented
   - Complex logic explained
   - Hardware notes included
   - Credit to original sources maintained

5. **Serial Output**:
   - Use `Serial.println()` for debugging
   - Use `Serial2.print()` for SA818 AT commands

6. **Error Handling**:
   - Check file existence before opening
   - Validate WAV headers
   - Fallback to Morse if WAV fails
   - Display errors on TFT

### Naming Conventions

| Type | Pattern | Example |
|------|---------|---------|
| Pins | COMPONENT_FUNCTION | `SA818_PTT`, `TFT_DC` |
| Functions | camelCase | `playWAV()`, `setfreq()` |
| Variables | camelCase | `wavFileName`, `sampleRate` |
| Constants | UPPER_SNAKE | (not widely used) |
| Structs | PascalCase | `WAVHeader` |

### DO NOT Change

1. **Pin definitions** (unless hardware changes)
2. **SA818 AT command format** (protocol-specific)
3. **WAV header structure** (standard format)
4. **DAC calculations** (timing-critical)
5. **GPL-3.0 license headers** (legal requirement)
6. **Credit comments** (attribution required)

---

## Dependencies & Build System

### Arduino Libraries Required

```cpp
#include <Adafruit_GFX.h>      // Graphics library
#include <Adafruit_ST7789.h>   // ST7789 TFT driver
#include <SPI.h>               // SPI communication
#include <SPIFFS.h>            // ESP32 filesystem
```

**Installation**: Arduino IDE → Sketch → Include Library → Manage Libraries
- Search: "Adafruit GFX"
- Search: "Adafruit ST7735 and ST7789"

### Python Dependencies

```python
# fox_wav_converter.py requirements
pydub          # Audio processing
# Also requires: ffmpeg (system-level)
```

**Installation**:
```bash
pip install pydub
# macOS: brew install ffmpeg
# Linux: apt-get install ffmpeg
# Windows: Download from ffmpeg.org
```

### ESP32 Board Support

**Board Manager URL**:
```
https://dl.espressif.com/dl/package_esp32_index.json
```

**Board Selection**: "ESP32 Dev Module" or "TTGO T-Display"

### Build Configuration

**Critical Settings**:
- **Partition Scheme**: "Default 4MB with spiffs" ← REQUIRED FOR WAV PLAYBACK
- Upload Speed: 921600 (or 115200)
- Flash Frequency: 80MHz
- Flash Size: 4MB

---

## AI Assistant Best Practices

### When User Asks for Code Changes

1. **Read First**: Always read the file before modifying
2. **Understand Context**: Check if change affects other functions
3. **Verify Safety**: Ensure change doesn't violate FCC regulations
4. **Test Considerations**: Mention what testing is needed
5. **Document**: Add comments explaining changes

### Common User Requests & Responses

#### "Change my callsign to [CALLSIGN]"
```cpp
// Fox_Hunt_TX_WAV.ino line 48
String callmessage = "K1ABC";  // Replace with user's callsign
```
**Response**: "Updated callsign. Ensure this is your valid FCC amateur radio callsign."

#### "Make it transmit every 10 seconds"
```cpp
// Fox_Hunt_TX_WAV.ino line 185
delay(10000);  // Changed from 15000
```
**Response**: "Updated transmission interval to 10 seconds. Note: FCC requires station ID every 10 minutes during operation."

#### "Add my custom WAV file"
**Response**:
1. "Please convert your audio using: `python fox_wav_converter.py yourfile.mp3`"
2. "Place the resulting `foxmessage.wav` in the `data/` folder"
3. "Upload via Tools → ESP32 Sketch Data Upload"
4. "Then upload the sketch normally"

#### "It's not working"
**Response**:
1. "Please open Serial Monitor at 9600 baud"
2. "What message do you see?"
3. "Does the display show 'Mode: WAV' or 'Mode: Morse'?"
4. [Troubleshoot based on response]

### Security & Safety Considerations

1. **FCC Compliance**: NEVER help bypass callsign requirements
2. **Frequency Safety**: WARN about illegal frequencies
3. **Power Levels**: Don't suggest modifications exceeding license privileges
4. **GPL License**: Maintain license headers on all files
5. **No Malicious Use**: Refuse requests for jamming, interference, etc.

### Code Review Checklist

Before suggesting code changes, verify:

- [ ] Change doesn't break existing functionality
- [ ] Pin assignments remain correct
- [ ] Timing remains within reasonable bounds
- [ ] Memory usage is acceptable (ESP32 has limited RAM)
- [ ] Serial communication format preserved
- [ ] Display output fits on 240x135 screen
- [ ] WAV playback timing not disrupted
- [ ] Comments added for complex changes
- [ ] User-configurable values clearly marked

---

## Git Workflow

### Branch Strategy
- `main`: Stable releases
- Feature branches: `feature/description` or `claude/description-XXXXX`

### Commit Message Style

**Follow existing pattern**:
```
Create codeql.yml
Update README.md
Add files via upload
```

**For AI-assisted commits**:
```
Update callsign to [CALLSIGN]
Add support for multiple WAV files
Fix audio playback timing issue
Update Python converter to support batch mode
```

**DO NOT**:
- Use emoji in commits (keep professional)
- Make vague commits ("fix stuff", "updates")
- Commit without testing (mention if untested)

### Creating Pull Requests

When helping user create PR:

1. **Title**: Clear, descriptive (e.g., "Add button-based WAV file selection")
2. **Description**:
   - What changed
   - Why it changed
   - How to test
   - Any breaking changes
3. **Testing**: Mention testing status
4. **License**: Confirm GPL-3.0 compatibility

---

## File Format Specifications

### WAV Files (SPIFFS)

**Required Specifications**:
- **Format**: WAV (RIFF/WAVE)
- **Encoding**: PCM (uncompressed)
- **Channels**: 1 (mono)
- **Bit Depth**: 8-bit unsigned OR 16-bit signed
- **Sample Rate**: 11025-44100 Hz (22050 recommended)
- **Duration**: < 10 seconds (< 5 seconds recommended)
- **File Size**: < 500 KB (SPIFFS space limited)

**Recommended**: 22050 Hz, 8-bit mono, ~3-5 seconds
**File Location**: `/data/foxmessage.wav` (uploaded to SPIFFS via ESP32 Sketch Data Upload)

### Python Script Arguments

```bash
# Basic usage
python fox_wav_converter.py input.mp3

# Custom settings
python fox_wav_converter.py input.mp3 output.wav -s 22050 -b 8 -d 5

# Batch mode
python fox_wav_converter.py --batch ./audio_files ./converted
```

**Options**:
- `-s, --sample-rate`: Sample rate in Hz (default: 22050)
- `-b, --bit-depth`: 8 or 16 (default: 8)
- `-d, --max-duration`: Max duration in seconds (default: 5)
- `--batch`: Process entire folder

---

## Hardware Limitations & Constraints

### ESP32 Constraints
- **Flash**: 4 MB total
- **SPIFFS**: ~1-2 MB (depends on partition scheme)
- **RAM**: 520 KB (limited for buffering)
- **CPU**: 240 MHz (fast enough for 44.1 kHz audio)
- **DAC Resolution**: 8-bit (0-255)
- **DAC Channels**: 2 (using DAC2/GPIO25)

### SA818 Radio Module
- **Frequency Range**: 134-174 MHz (VHF) or 400-470 MHz (UHF)
- **Power**: 0.5W (low) or 1W (high)
- **Bandwidth**: 12.5/25 kHz selectable
- **Control**: UART AT commands (9600 baud)

### Display (ST7789)
- **Resolution**: 240x135 pixels
- **Size**: ~1.14 inches
- **Colors**: 16-bit RGB (65K colors)
- **Interface**: SPI

### Practical Limits
- **WAV Duration**: Keep < 5 seconds (user experience)
- **Transmission Duty Cycle**: ~10% (5 sec TX / 50 sec total)
- **Battery Life**: Not specified (depends on battery size)
- **Range**: ~1 mile (depends on antenna, terrain, power)

---

## Troubleshooting Guide for AI Assistants

### User Reports "No Audio"

**Diagnostic Questions**:
1. "Does Serial Monitor show 'Playing WAV file'?"
2. "What's the file size shown in Serial Monitor?"
3. "Did you upload the SPIFFS data folder?"

**Common Causes**:
- WAV file not uploaded → Guide through ESP32 Sketch Data Upload
- Wrong sample rate → Recommend 22050 Hz
- Stereo instead of mono → Use converter tool
- File corrupted → Re-convert with Python tool

### User Reports "Wrong Speed"

**Cause**: Sample rate mismatch

**Solutions**:
1. **Re-export WAV**: `ffmpeg -i input.mp3 -ar 22050 -ac 1 -acodec pcm_u8 output.wav`
2. **OR modify code** (line 217): `uint32_t sampleDelay = 1000000 / 44100;` (change to match file)

### User Reports "SPIFFS Mount Failed"

**Solutions**:
1. Arduino IDE → Tools → Partition Scheme → "Default 4MB with spiffs"
2. Re-upload sketch
3. Upload SPIFFS data
4. If persistent: Try "Minimal SPIFFS" partition

### User Reports "Display Not Working"

**Diagnostic**:
1. Check wiring (pins 4, 5, 16, 18, 19, 23)
2. Verify TFT_BLK (backlight) is HIGH
3. Check if sketch compiles without errors
4. Ensure Adafruit libraries installed

---

## Advanced Topics

### Custom Audio Effects

**Example: Fade In/Out**
```cpp
// In playWAV(), modify DAC output
uint8_t volume = 255;  // Start value
if (currentSample < fadeInSamples) {
  volume = (currentSample * 255) / fadeInSamples;
}
uint8_t adjustedSample = (buffer[i] * volume) / 255;
dacWrite(SA818_AF_IN, adjustedSample);
```

### DTMF Tone Generation

If user requests DTMF (touch-tone) support:
1. Use `ledcWriteTone()` like Morse code function
2. DTMF frequencies: 697-1633 Hz (dual-tone)
3. Requires two simultaneous tones (complex)

### GPS Integration

For "remember fox locations" feature:
1. Add GPS module (UART or I2C)
2. Log coordinates to SPIFFS
3. Display on TFT or serial
4. **DO NOT** transmit GPS data without proper APRS format

### Battery Monitoring

For "low battery warning":
1. Use ADC to read battery voltage
2. Display warning below threshold
3. Consider reducing TX power when low

---

## External Resources

### Documentation
- **ESP32 Arduino Core**: https://docs.espressif.com/projects/arduino-esp32/
- **Adafruit GFX**: https://learn.adafruit.com/adafruit-gfx-graphics-library
- **SA818 Datasheet**: Search "SA818 programming manual"
- **WAV Format**: http://soundfile.sapp.org/doc/WaveFormat/

### Tools
- **Arduino IDE**: https://www.arduino.cc/en/software
- **ESP32 SPIFFS Plugin**: https://github.com/me-no-dev/arduino-esp32fs-plugin
- **Audacity**: https://www.audacityteam.org/
- **FFmpeg**: https://ffmpeg.org/

### Legal/Regulatory
- **FCC Part 97**: https://www.ecfr.gov/current/title-47/chapter-I/subchapter-D/part-97
- **ARRL Licensing**: https://www.arrl.org/getting-licensed
- **Fox Hunting**: https://www.arrl.org/direction-finding

---

## Quick Reference

### Most Common Code Locations

| Task | File | Line(s) |
|------|------|---------|
| Change callsign | Fox_Hunt_TX_WAV.ino | 48 |
| Change frequency | Fox_Hunt_TX_WAV.ino | 50 |
| Change WAV filename | Fox_Hunt_TX_WAV.ino | 53 |
| TX timing (interval) | Fox_Hunt_TX_WAV.ino | 185 |
| Pre-TX delay | Fox_Hunt_TX_WAV.ino | 176 |
| WAV playback function | Fox_Hunt_TX_WAV.ino | 188-253 |
| Morse code function | Fox_Hunt_TX_WAV.ino | 382-420 |
| Display update | Fox_Hunt_TX_WAV.ino | 263-280 |
| Pin definitions | Fox_Hunt_TX_WAV.ino | 26-46 |
| Audio converter settings | fox_wav_converter.py | 218-222 |

### Quick Commands

```bash
# Convert audio file
python fox_wav_converter.py input.mp3

# Compile and upload (Arduino IDE)
Ctrl+U (Sketch) → Tools → ESP32 Sketch Data Upload (SPIFFS)

# Serial monitor
Ctrl+Shift+M (9600 baud)
```

---

## Version History

- **2024**: Initial WAV playback support added
- **2024**: Python converter tool created
- **2026-01**: CLAUDE.md documentation created

---

## Contact & Support

- **Issues**: GitHub Issues page
- **Original Projects**:
  - rot13labs fox: https://github.com/c0ldbru/fox
  - YAFB: https://github.com/N8HR/YAFB

---

**This document is maintained for AI assistants (Claude, etc.) to provide accurate, safe, and helpful support to WavFox users. When in doubt, prioritize user safety, FCC compliance, and code stability over feature additions.**

**73!** 🦊📻
