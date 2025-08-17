# Hello World VST3 Plugin

A simple VST3 plugin built with JUCE that displays "Hello World" text in its GUI.

## Features

- VST3 format support
- AudioUnit (AU) support for macOS
- Standalone application
- Simple pass-through audio processing
- Clean, modern GUI displaying "Hello World"

## Prerequisites

- CMake 3.15 or higher
- C++17 compatible compiler
- JUCE framework

## Building the Plugin

### 1. Clone JUCE

First, you need to clone the JUCE framework into this directory:

```bash
git clone https://github.com/juce-framework/JUCE.git
```

### 2. Build the Project

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

### 3. Install the Plugin

The built plugin will be located in:
- **VST3**: `build/HelloWorldVST3_artefacts/Release/VST3/`
- **AudioUnit**: `build/HelloWorldVST3_artefacts/Release/AU/`
- **Standalone**: `build/HelloWorldVST3_artefacts/Release/Standalone/`

Copy the VST3 folder to your system's VST3 directory:
- **macOS**: `~/Library/Audio/Plug-Ins/VST3/`
- **Windows**: `C:\Program Files\Common Files\VST3\`
- **Linux**: `~/.vst3/`

## Project Structure

```
├── CMakeLists.txt          # Main build configuration
├── Source/
│   ├── PluginProcessor.h   # Audio processing class header
│   ├── PluginProcessor.cpp # Audio processing implementation
│   ├── PluginEditor.h      # GUI class header
│   └── PluginEditor.cpp    # GUI implementation
└── README.md               # This file
```

## Customization

To customize the plugin:

1. **Change the text**: Modify the label text in `Source/PluginEditor.cpp`
2. **Modify the GUI**: Edit the `paint()` and `resized()` methods in `PluginEditor.cpp`
3. **Add audio processing**: Implement your audio algorithm in `processBlock()` in `PluginProcessor.cpp`
4. **Change plugin metadata**: Update the `juce_add_plugin()` call in `CMakeLists.txt`

## License

This project is provided as-is for educational purposes. Modify the company and product information in `CMakeLists.txt` before distributing.
