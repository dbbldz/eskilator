cmake_minimum_required(VERSION 3.15)
project(Eskilator)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE as a subdirectory
add_subdirectory(JUCE)

# Create the plugin
juce_add_plugin(Eskilator
    VERSION "1.0.0"
    COMPANY_NAME "Rare Earth Tones"
    COMPANY_WEBSITE "https://github.com/dbbldz/eskilator"
    COMPANY_EMAIL "dutchdubs@gmail.com"
    PRODUCT_NAME "Eskilator"
    PRODUCT_DESCRIPTION "Monophonic sampler with Triton-inspired stepped portamento glide effect."
    PRODUCT_WEBSITE "https://github.com/dbbldz/eskilator"
    PRODUCT_HELP_URL "https://github.com/dbbldz/eskilator"
    COMPANY_LEGAL_NAME "Rare Earth Tones"
    COMPANY_CODE "MGLA"
    PRODUCT_CODE "Eski"
    BUNDLE_ID "com.marcglasser.eskilator"
    VST3_CATEGORIES "Instrument|Sampler"
    AU_MAIN_TYPE "kAudioUnitType_MusicDevice"
    AU_SANDBOX_SAFE true
    FORMATS AU VST3 Standalone
    PRODUCT_ID "eski1234"
    MANUFACTURER_CODE "RETN"
    MANUFACTURER_NAME "Rare Earth Tones"
    NEEDS_AUDIO_INPUT FALSE
    NEEDS_AUDIO_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    PRODUCES_MIDI_OUTPUT FALSE
)

# Configure JUCE to generate JuceHeader.h
juce_generate_juce_header(Eskilator)

# Enable plugin copy step so it gets installed automatically
set_target_properties(Eskilator_VST3 PROPERTIES
    JUCE_ENABLE_PLUGIN_COPY_STEP ON
)

# Set source files
target_sources(Eskilator
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/PluginLogger.cpp
        Source/ParameterManager.cpp
        Source/SampleManager.cpp
        Source/StyleSheet.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.h
        Source/PluginLogger.h
        Source/ParameterManager.h
        Source/SampleManager.h
        Source/StyleSheet.h
)

# Set include directories
target_include_directories(Eskilator
    PRIVATE
        Source
        ${CMAKE_CURRENT_BINARY_DIR}/juce_binarydata_Eskilator_BinaryData/JuceLibraryCode
)

# Add binary data directly to the main target
juce_add_binary_data(Eskilator_BinaryData
    SOURCES
        DefaultSample.wav
)

# Link required JUCE modules and binary data
target_link_libraries(Eskilator PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_devices
    Eskilator_BinaryData
)

# Set compile definitions
target_compile_definitions(Eskilator
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_VST3_EMULATE_MIDI_CC_WITH_PARAMETERS=0
)
