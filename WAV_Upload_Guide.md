# Fox Hunt WAV File Upload Guide

## Overview
This guide explains how to prepare and upload a WAV file to your Fox Hunt device for use as the transmission message.

## WAV File Requirements

### Recommended Settings
- **Sample Rate**: 22050 Hz (lower rates like 16000 Hz or 11025 Hz also work)
- **Bit Depth**: 8-bit or 16-bit PCM
- **Channels**: Mono (1 channel)
- **Format**: WAV (uncompressed PCM)
- **Duration**: Keep under 5 seconds for typical fox hunt use

### Why These Settings?
- Lower sample rates work better with the ESP32's processing speed
- Mono audio reduces file size and processing requirements
- 8-bit is easier to process but 16-bit has better quality
- Shorter duration prevents buffer overflow and keeps transmissions brief

## Preparing Your WAV File

### Using Audacity (Free, Cross-Platform)

1. **Open or Record Your Audio**
   - Open Audacity
   - Either record your message or import an existing audio file
   - File → Import → Audio...

2. **Convert to Mono**
   - If stereo, select Tracks → Mix → Mix Stereo Down to Mono

3. **Adjust Sample Rate**
   - Click on the audio track name (left side)
   - Select "Rate" → Choose 22050 Hz

4. **Trim and Adjust**
   - Select any silence at beginning/end and press Delete
   - Keep total duration under 5 seconds
   - Normalize: Effect → Volume and Compression → Normalize
   - Set to -3 dB to prevent clipping

5. **Export as WAV**
   - File → Export → Export Audio...
   - Format: WAV (Microsoft)
   - Encoding: 
     - "Unsigned 8-bit PCM" (smaller file, slightly lower quality)
     - OR "Signed 16-bit PCM" (better quality, larger file)
   - Save as: `foxmessage.wav`

### Using FFmpeg (Command Line)

```bash
# Convert any audio file to proper format (8-bit, 22050 Hz, mono)
ffmpeg -i input.mp3 -ar 22050 -ac 1 -acodec pcm_u8 foxmessage.wav

# For 16-bit version
ffmpeg -i input.mp3 -ar 22050 -ac 1 -acodec pcm_s16le foxmessage.wav

# If you need to trim (example: first 5 seconds)
ffmpeg -i input.mp3 -ar 22050 -ac 1 -acodec pcm_u8 -t 5 foxmessage.wav
```

## Uploading to ESP32 SPIFFS

### Method 1: Arduino IDE with ESP32 Filesystem Uploader

1. **Install the Plugin**
   - Download: https://github.com/me-no-dev/arduino-esp32fs-plugin
   - Close Arduino IDE
   - Extract to: `[Arduino_Sketchbook]/tools/ESP32FS/tool/esp32fs.jar`
   - Restart Arduino IDE

2. **Create Data Folder**
   - In your sketch folder, create a folder named `data`
   - Example: `Fox_Hunt_TX_WAV/data/`

3. **Add Your WAV File**
   - Copy `foxmessage.wav` into the `data` folder
   - The file MUST be named exactly `foxmessage.wav`

4. **Upload to SPIFFS**
   - Connect your ESP32
   - In Arduino IDE: Tools → ESP32 Sketch Data Upload
   - Wait for upload to complete (takes 30-60 seconds)
   - You should see "SPIFFS Image Uploaded"

### Method 2: ESP32-FS Upload Tool (VS Code/PlatformIO)

If using PlatformIO:

1. Create `data` folder in project root
2. Add `foxmessage.wav` to `data` folder
3. Run: `pio run --target uploadfs`

### Method 3: Web Upload (Alternative)

You can also create a simple web server on the ESP32 to upload files, but the above methods are recommended for initial setup.

## Testing Your Setup

### Upload and Test Procedure

1. **Upload the sketch**
   - Upload the modified Fox_Hunt_TX_WAV.ino sketch
   - Wait for compilation and upload

2. **Upload the WAV file**
   - Use ESP32 Sketch Data Upload (see above)

3. **Monitor Serial Output**
   - Open Serial Monitor (115200 baud)
   - Watch for:
     ```
     SPIFFS Mounted
     WAV file size: XXXX bytes
     Sample Rate: 22050
     Bits per Sample: 8
     Channels: 1
     ```

4. **Check the Display**
   - The TFT should show "Mode: WAV" at the bottom
   - If it shows "Mode: Morse", the WAV file wasn't found

5. **Test Transmission**
   - **IMPORTANT**: Ensure you have an amateur radio license
   - Verify frequency is legal for your license class
   - The device will transmit every 15 seconds
   - Use a receiver to verify audio quality

## Troubleshooting

### "WAV not found" Message
- Verify file is named exactly `foxmessage.wav` (case-sensitive on some systems)
- Ensure file is in the `data` folder
- Re-run ESP32 Sketch Data Upload
- Check that SPIFFS partition is properly configured in Arduino IDE

### No Audio or Distorted Audio
- Check sample rate (22050 Hz recommended)
- Ensure mono (not stereo)
- Verify bit depth (8 or 16-bit PCM)
- Try normalizing audio to -3dB
- Check that audio isn't clipped (no flat tops on waveform)

### Audio Too Fast/Slow
- Sample rate mismatch
- Re-export with exact 22050 Hz (or adjust in code)

### File Too Large
- Reduce duration (keep under 5 seconds)
- Use 8-bit instead of 16-bit
- Lower sample rate to 16000 or 11025 Hz
- Ensure it's uncompressed WAV (no MP3/AAC compression)

### SPIFFS Mount Failed
- Check board partition scheme: Tools → Partition Scheme → "Default 4MB with spiffs"
- Try "Minimal SPIFFS" if you have a smaller flash chip

## Customizing Your Message

### Fox Hunt Best Practices
1. **Include your callsign** (FCC requirement)
2. **Keep it brief** (3-5 seconds)
3. **Speak clearly** at moderate pace
4. **Add tone or music** for better direction finding
5. **Test volume levels** - not too loud, not too quiet

### Example Messages
- "This is [CALLSIGN], fox hunt transmitter number [X]"
- "CQ Fox, CQ Fox, this is [CALLSIGN]"
- "[CALLSIGN] fox transmitter"

### Adding Background Tone
You can add a continuous tone in Audacity:
1. Generate → Tone...
2. Waveform: Sine
3. Frequency: 800-1000 Hz
4. Amplitude: 0.3
5. Mix with your voice (Tracks → Mix → Mix and Render)

## Advanced: Multiple WAV Files

To support multiple WAV files or selection via buttons, you would need to modify the code:

```cpp
// In setup(), check button states
if(digitalRead(Button_A)) wavFileName = "/fox1.wav";
if(digitalRead(Button_B)) wavFileName = "/fox2.wav";
if(digitalRead(Button_C)) wavFileName = "/fox3.wav";
```

Upload all WAV files to the `data` folder with appropriate names.

## File Size Limits

- ESP32 typically has 4MB flash
- SPIFFS partition is usually 1-2MB
- WAV file example sizes:
  - 5 sec, 22050 Hz, 8-bit mono: ~110 KB
  - 5 sec, 22050 Hz, 16-bit mono: ~220 KB
  - 3 sec, 16000 Hz, 8-bit mono: ~48 KB

You have plenty of space for several short messages!

## Legal Reminder

- **Amateur Radio License Required**: You must have a valid amateur radio license to transmit
- **Station Identification**: FCC Part 97 requires station identification
- **Frequency Selection**: Ensure 146.565 MHz is appropriate for your license class
- **Content**: Transmissions must be in plain language (no encryption/obscured meanings)

## Additional Resources

- Audacity: https://www.audacityteam.org/
- FFmpeg: https://ffmpeg.org/
- ESP32 SPIFFS Plugin: https://github.com/me-no-dev/arduino-esp32fs-plugin
- WAV Format Info: http://soundfile.sapp.org/doc/WaveFormat/
