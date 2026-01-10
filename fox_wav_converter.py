#!/usr/bin/env python3
"""
Fox Hunt WAV Converter
Converts audio files to the proper format for ESP32 Fox Hunt transmitter

Requirements: pip install pydub
(pydub requires ffmpeg to be installed on your system)
"""

import sys
import os
from pathlib import Path

try:
    from pydub import AudioSegment
    from pydub.effects import normalize
except ImportError:
    print("ERROR: pydub not installed")
    print("Install with: pip install pydub")
    print("Also ensure ffmpeg is installed on your system")
    sys.exit(1)


def convert_to_fox_wav(input_file, output_file="foxmessage.wav", 
                       sample_rate=22050, bit_depth=8, max_duration=5000):
    """
    Convert any audio file to ESP32-compatible WAV format
    
    Args:
        input_file: Path to input audio file (mp3, wav, m4a, etc.)
        output_file: Output filename (default: foxmessage.wav)
        sample_rate: Sample rate in Hz (default: 22050)
        bit_depth: 8 or 16 bit (default: 8)
        max_duration: Maximum duration in milliseconds (default: 5000 = 5 seconds)
    """
    
    if not os.path.exists(input_file):
        print(f"ERROR: Input file '{input_file}' not found")
        return False
    
    print(f"Loading: {input_file}")
    
    try:
        # Load audio file
        audio = AudioSegment.from_file(input_file)
        
        # Convert to mono
        if audio.channels > 1:
            print("Converting to mono...")
            audio = audio.set_channels(1)
        
        # Change sample rate
        if audio.frame_rate != sample_rate:
            print(f"Resampling to {sample_rate} Hz...")
            audio = audio.set_frame_rate(sample_rate)
        
        # Trim if too long
        if len(audio) > max_duration:
            print(f"Trimming to {max_duration/1000} seconds...")
            audio = audio[:max_duration]
        
        # Normalize audio to prevent clipping
        print("Normalizing audio...")
        audio = normalize(audio, headroom=3.0)  # -3dB headroom
        
        # Set bit depth
        if bit_depth not in [8, 16]:
            print(f"WARNING: Invalid bit depth {bit_depth}, using 8-bit")
            bit_depth = 8
        
        # Export parameters
        export_params = {
            "format": "wav",
            "parameters": [
                "-ar", str(sample_rate),  # Sample rate
                "-ac", "1",               # Mono
            ]
        }
        
        if bit_depth == 8:
            export_params["parameters"].extend(["-acodec", "pcm_u8"])
            print("Using 8-bit unsigned PCM")
        else:
            export_params["parameters"].extend(["-acodec", "pcm_s16le"])
            print("Using 16-bit signed PCM")
        
        # Export
        print(f"Exporting to: {output_file}")
        audio.export(output_file, **export_params)
        
        # Display info
        file_size = os.path.getsize(output_file)
        duration = len(audio) / 1000.0
        
        print("\n" + "="*50)
        print("CONVERSION SUCCESSFUL!")
        print("="*50)
        print(f"Output file: {output_file}")
        print(f"File size: {file_size:,} bytes ({file_size/1024:.1f} KB)")
        print(f"Duration: {duration:.2f} seconds")
        print(f"Sample rate: {sample_rate} Hz")
        print(f"Bit depth: {bit_depth}-bit")
        print(f"Channels: Mono")
        print("="*50)
        print("\nNext steps:")
        print("1. Copy this file to your sketch's 'data' folder")
        print("2. Use 'ESP32 Sketch Data Upload' in Arduino IDE")
        print("3. Upload your sketch to the ESP32")
        print("="*50)
        
        return True
        
    except Exception as e:
        print(f"\nERROR during conversion: {e}")
        return False


def batch_convert(input_folder, output_folder="converted", **kwargs):
    """
    Convert all audio files in a folder
    
    Args:
        input_folder: Folder containing audio files
        output_folder: Where to save converted files
        **kwargs: Additional arguments to pass to convert_to_fox_wav
    """
    
    # Supported audio formats
    audio_extensions = ['.mp3', '.wav', '.m4a', '.ogg', '.flac', '.aac', '.wma']
    
    input_path = Path(input_folder)
    output_path = Path(output_folder)
    
    if not input_path.exists():
        print(f"ERROR: Input folder '{input_folder}' not found")
        return
    
    # Create output folder
    output_path.mkdir(exist_ok=True)
    
    # Find all audio files
    audio_files = []
    for ext in audio_extensions:
        audio_files.extend(input_path.glob(f'*{ext}'))
        audio_files.extend(input_path.glob(f'*{ext.upper()}'))
    
    if not audio_files:
        print(f"No audio files found in {input_folder}")
        return
    
    print(f"Found {len(audio_files)} audio file(s)")
    print("="*50)
    
    successful = 0
    for audio_file in audio_files:
        output_name = audio_file.stem + ".wav"
        output_file = output_path / output_name
        
        print(f"\nProcessing: {audio_file.name}")
        if convert_to_fox_wav(str(audio_file), str(output_file), **kwargs):
            successful += 1
        print()
    
    print("="*50)
    print(f"Batch conversion complete: {successful}/{len(audio_files)} successful")
    print("="*50)


def show_usage():
    """Display usage information"""
    print("""
Fox Hunt WAV Converter
======================

Usage:
    python fox_wav_converter.py <input_file> [output_file]
    python fox_wav_converter.py --batch <input_folder> [output_folder]

Examples:
    # Convert single file
    python fox_wav_converter.py my_message.mp3
    python fox_wav_converter.py my_message.mp3 foxmessage.wav
    
    # Convert all files in a folder
    python fox_wav_converter.py --batch ./audio_files
    python fox_wav_converter.py --batch ./audio_files ./converted

Options:
    --sample-rate, -s    Sample rate in Hz (default: 22050)
    --bit-depth, -b      Bit depth: 8 or 16 (default: 8)
    --max-duration, -d   Max duration in seconds (default: 5)

Advanced Examples:
    # 16-bit, 44100 Hz, 10 second max
    python fox_wav_converter.py input.mp3 -s 44100 -b 16 -d 10
    
    # Lower quality for smaller file size
    python fox_wav_converter.py input.mp3 -s 11025 -b 8 -d 3

Recommended Settings:
    Sample Rate: 22050 Hz (good balance of quality and size)
    Bit Depth: 8-bit (smaller files, adequate quality)
    Duration: 3-5 seconds (keeps transmissions brief)
""")


def main():
    """Main function"""
    
    if len(sys.argv) < 2:
        show_usage()
        sys.exit(1)
    
    # Parse arguments
    args = sys.argv[1:]
    
    # Default settings
    settings = {
        'sample_rate': 22050,
        'bit_depth': 8,
        'max_duration': 5000  # milliseconds
    }
    
    # Parse options
    i = 0
    batch_mode = False
    while i < len(args):
        arg = args[i]
        
        if arg in ['--help', '-h']:
            show_usage()
            return
        elif arg == '--batch':
            batch_mode = True
            i += 1
            continue
        elif arg in ['--sample-rate', '-s']:
            settings['sample_rate'] = int(args[i+1])
            i += 2
            continue
        elif arg in ['--bit-depth', '-b']:
            settings['bit_depth'] = int(args[i+1])
            i += 2
            continue
        elif arg in ['--max-duration', '-d']:
            settings['max_duration'] = int(float(args[i+1]) * 1000)  # convert to ms
            i += 2
            continue
        
        break
    
    # Get remaining positional arguments
    remaining_args = args[i:]
    
    if batch_mode:
        input_folder = remaining_args[0] if remaining_args else "."
        output_folder = remaining_args[1] if len(remaining_args) > 1 else "converted"
        batch_convert(input_folder, output_folder, **settings)
    else:
        if not remaining_args:
            print("ERROR: No input file specified")
            show_usage()
            sys.exit(1)
        
        input_file = remaining_args[0]
        output_file = remaining_args[1] if len(remaining_args) > 1 else "foxmessage.wav"
        
        if convert_to_fox_wav(input_file, output_file, **settings):
            sys.exit(0)
        else:
            sys.exit(1)


if __name__ == "__main__":
    main()
