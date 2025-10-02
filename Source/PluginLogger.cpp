#include "PluginLogger.h"

// Initialize static member
// Disabled by default for performance
bool PluginLogger::loggingEnabled = false; 

PluginLogger::PluginLogger()
    : logFileInitialized(false)
{
    initializeLogFile();
}

PluginLogger::~PluginLogger()
{
    // Cleanup if needed
}

void PluginLogger::initializeLogFile()
{
    if (!logFileInitialized) {
        logFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory).getChildFile("plugin_debug.txt");
        
        // Clear the log file on first use
        clearLog();
        logFileInitialized = true;
    }
}

void PluginLogger::log(const juce::String& message)
{
    // Check global logging flag first
    if (!loggingEnabled) {
        return;
    }
    
    if (!logFileInitialized) {
        initializeLogFile();
    }
    
    writeToFile(message);
    writeToJUCE(message);
}

void PluginLogger::writeToFile(const juce::String& message)
{
    juce::FileOutputStream logStream(logFile);
    if (logStream.openedOk()) {
        juce::String timestamp = juce::Time::getCurrentTime().toString(true, true, true, true);
        
        // Add milliseconds for high precision
        int milliseconds = juce::Time::getCurrentTime().getMilliseconds();
        timestamp += "." + juce::String(milliseconds).paddedLeft('0', 3);
        logStream << "[" << timestamp << "] " << message << "\n";
        logStream.flush();
    }
}

void PluginLogger::writeToJUCE(const juce::String& message)
{
    // Write to JUCE logger for IDE/console output
    juce::Logger::writeToLog(message);
}

void PluginLogger::clearLog()
{
    if (logFile.existsAsFile()) {
        logFile.deleteFile();
    }
    logFile.create();
}

void PluginLogger::setLogFile(const juce::File& newLogFile)
{
    logFile = newLogFile;
    logFileInitialized = false;
    initializeLogFile();
}
