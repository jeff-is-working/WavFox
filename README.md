# Fox Hunt WAV Player for ESP32

Play custom WAV audio messages on your [HackerBox Fox Hunt](https://www.instructables.com/HackerBox-Fox-Hunt/) radio transmitter instead of Morse code!

<p align="center">
  <img src="https://img.shields.io/badge/Platform-ESP32-blue" alt="ESP32">
  <img src="https://img.shields.io/badge/License-GPL--3.0-green" alt="License">
  <img src="https://img.shields.io/badge/Audio-WAV-orange" alt="WAV">
</p>

## Overview

This project enhances the HackerBox Fox Hunt transmitter by adding WAV file playback capability. Instead of transmitting Morse code, you can now play custom voice messages, tones, or any audio you want for radio direction finding (RDF) events.

**Features:**
- Play WAV files stored in ESP32 SPIFFS
- Hardware DAC output for clean audio
- Automatic fallback to Morse code if WAV file is missing
- Visual feedback on TFT display
- Supports multiple sample rates and bit depths
- Python tool included for easy audio conversion

## Table of Contents

- [Hardware Requirements](#hardware-requirements)
- [Quick Start](#quick-start)
- [Installation](#installation)
- [Preparing Audio Files](#preparing-audio-files)
- [Usage](#usage)
- [Configuration](#configuration)
- [Troubleshooting](#troubleshooting)
- [Legal Requirements](#legal-requirements)
- [Contributing](#contributing)
- [License](#license)

## Hardware Requirements

- **TTGO ESP32 T-Display** (with ST7789 TFT)
- **SA818 VHF/UHF Radio Module**
- **HackerBox Fox Hunt Kit** or equivalent hardware

### Pin Connections

| Function | ESP32 GPIO | Notes |
|----------|------------|-------|
| Audio Out | GPIO 25 | DAC2 → SA818 AF_IN |
| PTT | GPIO 13 | Push to Talk |
| Display DC | GPIO 16 | ST7789 TFT |
| Display CS | GPIO 5 | ST7789 TFT |
| Display SCL | GPIO 18 | ST7789 TFT |
| Display SDA | GPIO 19 | ST7789 TFT |
| Display RES | GPIO 23 | ST7789 TFT |

## Quick Start

### 1. Install Arduino IDE and Dependencies

```bash
# Install ESP32 board support in Arduino IDE
# Boards Manager URL: https://dl.espressif.com/dl/package_esp32_index.json

# Install required libraries via Library Manager:
# - Adafruit GFX Library
# - Adafruit ST7735 and ST7789 Library
```

### 2. Install ESP32 Filesystem Uploader

Download and install the [ESP32 Sketch Data Upload](https://github.com/me-no-dev/arduino-esp32fs-plugin) plugin for Arduino IDE.

### 3. Clone This Repository

```bash
git clone https://github.com/jeff-is-working/WavFox.git
cd WavFox
```

### 4. Prepare Your Audio

Use the included Python converter:

```bash
pip install pydub
python fox_wav_converter.py your_message.mp3
```

Or prepare manually using Audacity (see [Preparing Audio Files](#preparing-audio-files)).

### 5. Upload to ESP32

```bash
# 1. Create data folder and add your WAV file
mkdir data
cp foxmessage.wav data/

# 2. Update your callsign in Fox_Hunt_TX_WAV.ino (line 25)
# 3. Upload the sketch (Ctrl+U in Arduino IDE)
# 4. Upload filesystem: Tools → ESP32 Sketch Data Upload
```

Done! Your fox hunt transmitter will now play your custom audio message.

## Installation

### Arduino IDE Setup

1. **Install ESP32 Board Support**
   - File → Preferences
   - Add to "Additional Board Manager URLs":
     ```
     https://dl.espressif.com/dl/package_esp32_index.json
     ```
   - Tools → Board → Boards Manager
   - Search "ESP32" and install

2. **Install Required Libraries**
   - Sketch → Include Library → Manage Libraries
   - Install:
     - `Adafruit GFX Library`
     - `Adafruit ST7735 and ST7789 Library`

3. **Install ESP32FS Plugin**
   - Download [ESP32FS](https://github.com/me-no-dev/arduino-esp32fs-plugin/releases)
   - Extract to: `[Arduino]/tools/ESP32FS/tool/esp32fs.jar`
   - Restart Arduino IDE

4. **Configure Board Settings**
   - Board: "ESP32 Dev Module" (or "TTGO T-Display")
   - Upload Speed: 921600
   - Flash Frequency: 80MHz
   - **Partition Scheme: "Default 4MB with spiffs"** ← Important!

## Preparing Audio Files

### WAV File Requirements

| Parameter | Recommended | Supported Range |
|-----------|------------|-----------------|
| Sample Rate | 22050 Hz | 11025 - 44100 Hz |
| Bit Depth | 8-bit | 8-bit or 16-bit |
| Channels | Mono | Mono only |
| Format | WAV PCM | Uncompressed only |
| Duration | 3-5 seconds | Up to ~30 seconds |

### Method 1: Python Converter (Easiest)

```bash
# Basic conversion
python fox_wav_converter.py my_audio.mp3

# Custom settings
python fox_wav_converter.py input.mp3 -s 22050 -b 8 -d 5

# Batch convert entire folder
python fox_wav_converter.py --batch ./audio_files
```

**Options:**
- `-s, --sample-rate`: Sample rate in Hz (default: 22050)
- `-b, --bit-depth`: 8 or 16 bit (default: 8)
- `-d, --max-duration`: Maximum duration in seconds (default: 5)

### Method 2: Audacity

1. Open/record your audio in Audacity
2. **Tracks** → **Mix** → **Mix Stereo Down to Mono**
3. Click track name → **Rate** → **22050 Hz**
4. **Effect** → **Normalize** → Set to -3 dB
5. **File** → **Export** → **Export Audio**
   - Format: WAV (Microsoft)
   - Encoding: Unsigned 8-bit PCM
6. Save as `foxmessage.wav`

### Method 3: FFmpeg Command Line

```bash
# Convert to 8-bit, 22050 Hz, mono
ffmpeg -i input.mp3 -ar 22050 -ac 1 -acodec pcm_u8 foxmessage.wav

# For 16-bit version
ffmpeg -i input.mp3 -ar 22050 -ac 1 -acodec pcm_s16le foxmessage.wav
```

## Usage

### Project Structure

```
Fox_Hunt_TX_WAV/
├── Fox_Hunt_TX_WAV.ino    # Main sketch
├── data/                   # SPIFFS filesystem
│   └── foxmessage.wav     # Your audio file
└── README.md
```

### Upload Process

1. **Update Configuration**
   ```cpp
   String callmessage = "N0CALL";  // Change to YOUR callsign
   float frequency = 146.565;       // Verify this is legal for your license
   ```

2. **Upload Sketch**
   - Open `Fox_Hunt_TX_WAV.ino`
   - Click Upload (or Ctrl+U)

3. **Upload Audio File**
   - Place `foxmessage.wav` in the `data` folder
   - Tools → **ESP32 Sketch Data Upload**
   - Wait 30-60 seconds for completion

4. **Verify Operation**
   - Open Serial Monitor (115200 baud)
   - Look for "Mode: WAV" on TFT display
   - Check for these messages:
     ```
     SPIFFS Mounted
     WAV file size: XXXXX bytes
     Sample Rate: 22050
     Bits per Sample: 8
     Channels: 1
     ```

### Transmission Cycle

Default behavior:
1. PTT ON
2. 750ms delay
3. Play WAV file
4. PTT OFF
5. Wait 15 seconds
6. Repeat

## Configuration

### Basic Settings

```cpp
// In Fox_Hunt_TX_WAV.ino

String callmessage = "N0CALL";           // Your FCC callsign
float frequency = 146.565;                // TX frequency (MHz)
const char* wavFileName = "/foxmessage.wav";  // WAV filename in SPIFFS
bool useWavFile = true;                   // true=WAV, false=Morse
```

### Timing Adjustments

```cpp
void loop() {
  digitalWrite(SA818_PTT,LOW);
  delay(750);                   // Pre-transmission delay (ms)
  
  if(useWavFile) {
    playWAV();
  }
  
  digitalWrite(SA818_PTT,HIGH);
  delay(15000);                 // Wait between transmissions (ms)
}
```

### Multiple WAV Files with Button Selection

Add this to `setup()` after button pinMode declarations:

```cpp
// Select WAV file based on button pressed at startup
if(digitalRead(Button_A)) {
  wavFileName = "/fox1.wav";
} else if(digitalRead(Button_B)) {
  wavFileName = "/fox2.wav";
} else if(digitalRead(Button_C)) {
  wavFileName = "/fox3.wav";
}
```

Don't forget to upload all three WAV files to the `data` folder!

### Audio Volume Adjustment

Modify the `playWAV()` function:

```cpp
// After reading the sample, multiply by volume (0.0 - 1.0)
uint8_t sample8 = buffer[i] * 0.8;  // 80% volume
dacWrite(SA818_AF_IN, sample8);
```

## Troubleshooting

### "WAV not found" on Display

- Verify file is named exactly `foxmessage.wav`
- Confirm file is in the `data` folder
- Re-run "ESP32 Sketch Data Upload"
- Check partition scheme is set to include SPIFFS

### No Audio or Distorted Audio

- Verify sample rate is 22050 Hz (or adjust in code)
- Ensure audio is mono, not stereo
- Check bit depth is 8 or 16-bit PCM
- Normalize audio to -3dB to prevent clipping
- Verify WAV is uncompressed (no MP3/AAC encoding)

### SPIFFS Mount Failed

- Tools → Partition Scheme → "Default 4MB with spiffs"
- Try "Minimal SPIFFS" if default fails
- Re-upload filesystem data

### Audio Plays Too Fast/Slow

Sample rate mismatch. Either:
- Re-export WAV at exactly 22050 Hz, or
- Modify code to match your file's sample rate:
  ```cpp
  // In playWAV() function
  uint32_t sampleDelay = 1000000 / 44100;  // Change to match your file
  ```

### File Size Too Large

- Reduce duration (keep under 5 seconds)
- Use 8-bit instead of 16-bit
- Lower sample rate to 16000 or 11025 Hz
- Trim silence from beginning/end

### ESP32 Won't Upload

- Check correct COM port selected
- Press and hold BOOT button during upload
- Reduce upload speed to 115200
- Try different USB cable/port

## Legal Requirements

### Amateur Radio License Required

This device **requires a valid amateur radio license** to operate legally.

### FCC Part 97 Compliance (USA)

- **Station Identification**: Your callsign must be transmitted
- **Frequency**: Verify 146.565 MHz is authorized for your license class
- **Content**: Must be plain language (no encryption or obscured meaning)
- **ID Interval**: Station ID required every 10 minutes during operation

### Best Practices for Fox Hunt Messages

Good examples:
- "This is [CALLSIGN], fox hunt transmitter number one"
- "CQ Fox, CQ Fox, this is [CALLSIGN]"
- "[CALLSIGN] fox transmitter"

Include:
- Your FCC callsign clearly spoken
- Brief identification (3-5 seconds)
- Optional: continuous tone (800-1000 Hz) for direction finding

## File Size Reference

| Duration | Sample Rate | Bit Depth | File Size |
|----------|-------------|-----------|-----------|
| 3 sec | 16000 Hz | 8-bit | ~48 KB |
| 5 sec | 22050 Hz | 8-bit | ~110 KB |
| 5 sec | 22050 Hz | 16-bit | ~220 KB |
| 10 sec | 22050 Hz | 8-bit | ~220 KB |

ESP32 SPIFFS partition: 1-2 MB available (plenty of space!)

## Repository Contents

```
fox-hunt-wav-player/
├── Fox_Hunt_TX_WAV.ino        # Main Arduino sketch
├── fox_wav_converter.py       # Python audio conversion tool
├── Quick_Reference.md         # Quick reference card
├── WAV_Upload_Guide.md        # Detailed upload instructions
├── README.md                  # This file
├── LICENSE                    # GPL-3.0 License
└── examples/
    └── data/
        └── sample.wav         # Example WAV file
```

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

### Development Setup

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

### Ideas for Contributions

- [ ] Web-based WAV file uploader
- [ ] Additional audio effects (reverb, echo)
- [ ] DTMF tone generation
- [ ] Battery voltage monitoring
- [ ] GPS integration for hiding locations
- [ ] Multiple frequency support
- [ ] Contest mode (rotating through multiple foxes)

## Credits

This project builds upon:
- [rot13labs fox](https://github.com/c0ldbru/fox) by c0ldbru
- [Yet Another Foxbox (YAFB)](https://github.com/N8HR/YAFB) by Gregory Stoike (KN4CK)
- [HackerBox Fox Hunt](https://hackerboxes.com/products/hackerbox-fox-hunt) kit

## License

This project is free software licensed under the GNU General Public License v3.0 - see the [LICENSE](LICENSE) file for details.

```
Copyright (C) 2024 Fox Hunt WAV Player Contributors

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
```

## Support

- **Issues**: Please use the [GitHub Issues](https://github.com/yourusername/fox-hunt-wav-player/issues) page
- **Discussions**: Check the [Discussions](https://github.com/yourusername/fox-hunt-wav-player/discussions) tab
- **Documentation**: See `WAV_Upload_Guide.md` for detailed instructions

## Resources

- [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32)
- [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library)
- [Audacity Audio Editor](https://www.audacityteam.org/)
- [FFmpeg](https://ffmpeg.org/)
- [Amateur Radio License Info (ARRL)](https://www.arrl.org/getting-licensed)
- [FCC Part 97 Rules](https://www.ecfr.gov/current/title-47/chapter-I/subchapter-D/part-97)

## Show Your Support

Give a ⭐️ if this project helped you! Share your fox hunt experiences and audio creations!

---

**73** 🦊📻
