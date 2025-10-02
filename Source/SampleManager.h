#pragma once

#include <JuceHeader.h>
#include <functional>
#include <vector>
#include <random>
#include <atomic>
#include <mutex>

struct SampleInfo
{
    juce::AudioBuffer<float> buffer;
    juce::String name;
    juce::String path;
    double originalSampleRate = 44100.0;
    bool isDefault = false;
    
    // Per-sample parameters
    float gain = 0.0f;        // Gain in dB (-24 to +24)
    float transpose = 0.0f;   // Transpose in semitones (-12 to +12)
};

class SampleManager
{
public:
    SampleManager();
    ~SampleManager() = default;
    
    // Load sample from file
    bool loadSample(const juce::File& audioFile, double currentSampleRate);
    
    // Load default click sample from binary data
    bool loadDefaultSample(double currentSampleRate);
    
    // Reload sample from previously stored path
    bool reloadSampleFromPath(double currentSampleRate);
    
    // Check if any samples are loaded
    bool hasSample() const { return !sampleBank.empty(); }
    
    // Check if a specific sample exists
    bool hasSampleAtIndex(int index) const { return index >= 0 && index < static_cast<int>(sampleBank.size()); }
    
    // Get sample information
    juce::String getSampleName(int index = 0) const;
    double getOriginalSampleRate(int index = 0) const;
    const juce::String& getSamplePath(int index = 0) const;
    
    // Get sample buffers
    const juce::AudioBuffer<float>& getSampleBuffer(int index = 0) const;
    const juce::AudioBuffer<float>& getProcessedSampleBuffer(int index = 0) const;
    
    // Set processed sample buffer (for timestretch) - thread-safe
    void setProcessedSampleBuffer(const juce::AudioBuffer<float>& buffer, int index = 0);
    
    // Thread-safe buffer copy method
    bool copyBufferSafely(const juce::AudioBuffer<float>& source, juce::AudioBuffer<float>& dest) const;
    
    // Clear processed buffer
    void clearProcessedBuffer(int index = 0);
    
    // Set sample rate (for resampling)
    void setSampleRate(double sampleRate) { currentSampleRate = sampleRate; }
    
    // Get current sample rate
    double getSampleRate() const { return currentSampleRate; }
    
    // Set callback for when sample needs timestretch update
    void setTimestretchUpdateCallback(std::function<void()> callback) { onTimestretchUpdate = callback; }
    
    // Sample bank management
    int getSampleCount() const { return static_cast<int>(sampleBank.size()); }
    void removeSample(int index);
    void clearSampleBank();
    
    // Chain selection and randomization
    void setChainSelector(int selector) { 
        // Keep full 0-63 range, mapping happens in getCurrentSampleIndex()
        chainSelector = juce::jlimit(0, 63, selector); 
        
        // Reset randomization cache so the new selection takes effect immediately
        resetRandomizationCache();
    }
    int getChainSelector() const { return chainSelector; }
    
    void setRandomizationAmount(float amount) { randomizationAmount = juce::jlimit(0.0f, 1.0f, amount); }
    float getRandomizationAmount() const { return randomizationAmount; }
    
    // Reset randomization cache for new triggers
    void resetRandomizationCache() const { randomizationDecisionMade = false; cachedSampleIndex = -1; }
    
    // Get the currently selected sample based on chain selector and randomization
    int getCurrentSampleIndex() const;
    
    // Get current sample buffer considering chain selection and randomization
    const juce::AudioBuffer<float>& getCurrentSampleBuffer() const;
    
    // Per-sample parameter management
    void setSampleGain(int index, float gainDb);
    float getSampleGain(int index) const;
    void setSampleTranspose(int index, float semitones);
    float getSampleTranspose(int index) const;

private:
    std::vector<SampleInfo> sampleBank;
    std::vector<juce::AudioBuffer<float>> processedBuffers;
    
    // Thread synchronization for buffer access
    mutable juce::CriticalSection bufferLock;
    mutable std::atomic<bool> bufferInUse{false};
    
    double currentSampleRate = 44100.0;
    
    // Chain selection and randomization
    int chainSelector = 0;           // Which sample to play (0 = first sample)
    float randomizationAmount = 0.0f; // 0.0 = no randomization, 1.0 = full random
    
    // Random number generation for sample selection
    mutable std::mt19937 randomGenerator;
    mutable std::uniform_int_distribution<int> randomDistribution;
    
    // Cache for randomization decision (to prevent rapid switching within a single trigger)
    mutable int cachedSampleIndex = -1;
    mutable bool randomizationDecisionMade = false;
    
    std::function<void()> onTimestretchUpdate;
    
    // Thread safety for sample bank modifications
    mutable std::mutex sampleBankMutex;
    
    // Helper methods
    bool performSampleRateConversion(const juce::AudioBuffer<float>& sourceBuffer, 
                                   double sourceSampleRate, 
                                   double targetSampleRate,
                                   juce::AudioBuffer<float>& destBuffer);
    
    // Internal version without mutex lock (for use within already-locked methods)
    int getCurrentSampleIndexInternal() const;
    
    void updateSampleInfo(const juce::String& name, const juce::String& path, double sampleRate);
    
    void triggerTimestretchUpdate();
    
    // Initialize random generator
    void initializeRandomGenerator();
};
