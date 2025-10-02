# Eskilator

A monophonic sampler with stepped portamento glide inspired by the KORG Triton.

## Overview

Eskilator is a VST3/AU/Standalone sampler plugin built with JUCE. It recreates the KORG Triton's stepped glide effect - where pitch transitions happen in discrete steps rather than smoothly. Load your own samples and adjust the glide time and step count to dial in the sound.

## Features

- **Stepped Portamento Glide**: Recreates the KORG Triton's stepped glide effect with adjustable time and step count
- **Monophonic Playback**: Single-voice triggering with natural sample behavior
- **ADSR Envelope**: Attack, Decay, Sustain, Release envelope control
- **Drag & Drop**: Load samples by dragging audio files into the plugin
- **Cross-Platform**: VST3, AudioUnit (AU), and Standalone formats

## Building from Source

### Prerequisites

- CMake 3.15 or later
- C++17 compatible compiler
- macOS: Xcode command line tools (for code signing)
- JUCE framework (included as git submodule)

### Quick Build

```bash
# Clone the repository
git clone https://github.com/dbbldz/eskilator.git
cd eskilator

# Initialize JUCE submodule
git submodule update --init --recursive

# Build all formats (VST3, AU, Standalone)
./build_and_install.sh

# Or build standalone only for quick UI testing
./build_and_install.sh standalone
```

### Manual Build

```bash
# Configure
mkdir build
cd build
cmake ..

# Build
cmake --build . --config Release

# Plugins will be in:
# - build/Eskilator_artefacts/VST3/Eskilator.vst3
# - build/Eskilator_artefacts/AU/Eskilator.component
# - build/Eskilator_artefacts/Standalone/Eskilator.app
```

### Installation

The `build_and_install.sh` script automatically installs plugins to:
- **macOS AU**: `~/Library/Audio/Plug-Ins/Components/`
- **macOS VST3**: `~/Library/Audio/Plug-Ins/VST3/`

You may need to restart your DAW for the new plugins to appear.

## Usage

### Loading Samples

Drag and drop an audio file (WAV, AIFF, etc.) onto the plugin to load it.

### Parameters

#### ADSR Envelope
- **Attack**: Envelope attack time (0-2 seconds)
- **Decay**: Envelope decay time (0-2 seconds)
- **Sustain**: Sustain level (0-100%)
- **Release**: Envelope release time (0-5 seconds)

#### Sample Controls
- **Sample Gain**: Sample volume (-60dB to +12dB)

#### Glide System
- **Glide Time**: Total duration of the glide effect (0-2 seconds)
- **Glide Steps**: Number of discrete steps in the glide (1-24 steps)
  - 1 step = smooth glide
  - 2-24 steps = stepped Triton-style portamento

## Development

### Logging

The plugin includes a custom logging system (disabled by default for performance). To enable logging:

```cpp
// In PluginLogger.cpp, set:
bool PluginLogger::loggingEnabled = true;

// Then use:
logger.log("Your debug message here");
```

Logs are written to `plugin_debug.txt` on your desktop.

### Code Signing

The build script automatically handles code signing:
- Uses Developer ID Application certificate if available
- Falls back to Mac Developer certificate
- Uses ad-hoc signing with runtime hardening as last resort

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.

## License

This project is licensed under the Creative Commons Attribution-NonCommercial 4.0 International License (CC BY-NC 4.0) - see the [LICENSE](LICENSE) file for details.

**Non-Commercial Use Only**: This software may not be used for commercial purposes.

## Credits

- Built with [JUCE](https://juce.com/) audio framework
- Inspired by KORG Triton's stepped portamento glide effect
- Developed by dubbel dutch

## Contact

- GitHub: [https://github.com/dbbldz/eskilator](https://github.com/dbbldz/eskilator)
- Email: dutchdubs@gmail.com
- Website: https://dubbeldutch.com
