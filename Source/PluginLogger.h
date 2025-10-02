#pragma once

#include <juce_core/juce_core.h>

class PluginLogger
{
public:
    PluginLogger();
    ~PluginLogger();
    
    // Log a message with timestamp
    void log(const juce::String& message);
    
    // Global logging control
    static void setLoggingEnabled(bool enabled) { loggingEnabled = enabled; }
    static bool isLoggingEnabled() { return loggingEnabled; }
    
    // Conditional logging helper for juce::Logger calls
    static void conditionalLog(const juce::String& message) {
        if (loggingEnabled) {
            juce::Logger::writeToLog(message);
        }
    }
    
    // Get the log file path
    juce::File getLogFile() const { return logFile; }
    
    // Clear the log file
    void clearLog();
    
    // Set custom log file location
    void setLogFile(const juce::File& newLogFile);

private:
    juce::File logFile;
    bool logFileInitialized;
    
    // Global logging control
    static bool loggingEnabled;
    
    // Initialize the log file
    void initializeLogFile();
    
    // Write message to file
    void writeToFile(const juce::String& message);
    
    // Write message to JUCE logger
    void writeToJUCE(const juce::String& message);
};
