cmake_minimum_required(VERSION 3.15)
project(HelloWorldVST3)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE as a subdirectory
add_subdirectory(JUCE)

# Create the VST3 plugin
juce_add_plugin(HelloWorldVST3
    VERSION "1.0.0"
    COMPANY_NAME "YourCompany"
    COMPANY_WEBSITE "https://yourcompany.com"
    COMPANY_EMAIL "contact@yourcompany.com"
    PRODUCT_NAME "Beat Generator v2"
    PRODUCT_DESCRIPTION "A MIDI beat generator that outputs C notes on every beat"
    PRODUCT_WEBSITE "https://yourcompany.com/helloworld"
    PRODUCT_HELP_URL "https://yourcompany.com/helloworld/help"
    COMPANY_LEGAL_NAME "Your Company Name"
    COMPANY_CODE "YCOM"
    PRODUCT_CODE "Hwld"
    VST3_CATEGORIES "Synth"
    AU_MAIN_TYPE "kAudioUnitType_MusicDevice"
    AU_SANDBOX_SAFE true
    FORMATS AU VST3 Standalone
    PRODUCT_ID "hwld1234"
    MANUFACTURER_CODE "YCOM"
    MANUFACTURER_NAME "YourCompany"
    NEEDS_AUDIO_INPUT FALSE
    NEEDS_AUDIO_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    IS_SYNTH TRUE
    WANTS_MIDI_INPUT TRUE
    PRODUCES_MIDI_OUTPUT TRUE
)

# Configure JUCE to generate JuceHeader.h
juce_generate_juce_header(HelloWorldVST3)

# Enable plugin copy step so it gets installed automatically
set_target_properties(HelloWorldVST3_VST3 PROPERTIES
    JUCE_ENABLE_PLUGIN_COPY_STEP ON
)

# Set source files
target_sources(HelloWorldVST3
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.h
)

# Add binary data (embed the click sample)
juce_add_binary_data(HelloWorldVST3_BinaryData
    SOURCES
        "Click 808.wav"
)

# Set include directories
target_include_directories(HelloWorldVST3
    PRIVATE
        Source
)

# Link required JUCE modules
target_link_libraries(HelloWorldVST3 PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_devices
    HelloWorldVST3_BinaryData
)

# Set compile definitions
target_compile_definitions(HelloWorldVST3
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_VST3_EMULATE_MIDI_CC_WITH_PARAMETERS=0
)
