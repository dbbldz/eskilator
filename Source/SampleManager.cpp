#include "SampleManager.h"

#include "PluginLogger.h"

SampleManager::SampleManager()
{
    // Constructor - default values are set in header
    initializeRandomGenerator();
}

void SampleManager::initializeRandomGenerator()
{
    std::random_device rd;
    randomGenerator.seed(rd());
    randomDistribution = std::uniform_int_distribution<int>(0, 0);
}

void SampleManager::setSampleGain(int index, float gainDb)
{
    std::lock_guard<std::mutex> lock(sampleBankMutex);
    if (index >= 0 && index < static_cast<int>(sampleBank.size())) {
        sampleBank[index].gain = juce::jlimit(-24.0f, 24.0f, gainDb);
    }
}

float SampleManager::getSampleGain(int index) const
{
    std::lock_guard<std::mutex> lock(sampleBankMutex);
    if (index >= 0 && index < static_cast<int>(sampleBank.size())) {
        return sampleBank[index].gain;
    }
    return 0.0f;
}

void SampleManager::setSampleTranspose(int index, float semitones)
{
    std::lock_guard<std::mutex> lock(sampleBankMutex);
    if (index >= 0 && index < static_cast<int>(sampleBank.size())) {
        sampleBank[index].transpose = juce::jlimit(-12.0f, 12.0f, semitones);
    }
}

float SampleManager::getSampleTranspose(int index) const
{
    std::lock_guard<std::mutex> lock(sampleBankMutex);
    if (index >= 0 && index < static_cast<int>(sampleBank.size())) {
        return sampleBank[index].transpose;
    }
    return 0.0f;
}

bool SampleManager::loadSample(const juce::File& audioFile, double currentSampleRate)
{
    std::lock_guard<std::mutex> lock(sampleBankMutex);
    this->currentSampleRate = currentSampleRate;
    
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats(); // WAV, AIFF, etc.
    
    auto reader = formatManager.createReaderFor(audioFile);
    if (reader != nullptr)
    {
        SampleInfo newSample;
        newSample.originalSampleRate = reader->sampleRate;
        newSample.name = audioFile.getFileNameWithoutExtension();
        newSample.path = audioFile.getFullPathName();
        newSample.isDefault = false;
        
        // Check if we need sample rate conversion
        if (std::abs(newSample.originalSampleRate - currentSampleRate) > 0.1) {
            // If sample rates differ significantly calculate the required buffer size after resampling
            double conversionRatio = currentSampleRate / newSample.originalSampleRate;
            int resampledLength = static_cast<int>(reader->lengthInSamples * conversionRatio);
            
            // Create temporary buffer for original data
            juce::AudioBuffer<float> tempBuffer(static_cast<int>(reader->numChannels), 
                                              static_cast<int>(reader->lengthInSamples));
            
            // Read original data
            reader->read(&tempBuffer, 0, static_cast<int>(reader->lengthInSamples), 0, true, true);
            
            // Perform resampling
            if (!performSampleRateConversion(tempBuffer, newSample.originalSampleRate, currentSampleRate, newSample.buffer)) {
                delete reader;
                return false;
            }
        }
        else
        {
            // No resampling needed - load directly
            newSample.buffer.setSize(static_cast<int>(reader->numChannels), 
                                   static_cast<int>(reader->lengthInSamples));
            reader->read(&newSample.buffer, 0, static_cast<int>(reader->lengthInSamples), 0, true, true);
        }
        
        // Add the new sample to the bank
        sampleBank.push_back(newSample);
        
        // Initialize processed buffer for this sample
        processedBuffers.emplace_back();
        
        // Update random distribution for new sample count
        if (sampleBank.size() > 1) {
            randomDistribution = std::uniform_int_distribution<int>(0, static_cast<int>(sampleBank.size()) - 1);
        }
        
        delete reader;
        
        // Trigger timestretch update for the new sample
        triggerTimestretchUpdate();
        
        return true;
    }
    
    return false;
}

bool SampleManager::loadDefaultSample(double currentSampleRate)
{
    // Load the default sample from binary data
    const char* sampleData = BinaryData::DefaultSample_wav;
    int sampleDataSize = BinaryData::DefaultSample_wavSize;
    
    if (sampleData != nullptr && sampleDataSize > 0)
    {
        // Create a memory input stream from the binary data
        juce::MemoryInputStream inputStream(sampleData, sampleDataSize, false);
        
        // Create an audio format reader
        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(std::make_unique<juce::MemoryInputStream>(sampleData, sampleDataSize, false)));
        
        if (reader != nullptr)
        {
            // Create sample info
            SampleInfo info;
            info.name = "Gliding Squares";
            info.path = "Built-in";
            info.originalSampleRate = reader->sampleRate;
            info.isDefault = true;
            
            // Create a buffer for the audio data
            info.buffer.setSize(static_cast<int>(reader->numChannels), 
                               static_cast<int>(reader->lengthInSamples));
            
            // Read the audio data
            if (reader->read(&info.buffer, 0, static_cast<int>(reader->lengthInSamples), 0, true, true))
            {
                // Add to sample bank
                sampleBank.push_back(info);
                
                // Initialize processed buffer for this sample
                processedBuffers.emplace_back();
                
                // Set current sample rate
                this->currentSampleRate = currentSampleRate;
                
                // Resample if necessary
                if (std::abs(currentSampleRate - reader->sampleRate) > 0.1)
                {
                    if (!performSampleRateConversion(info.buffer, info.originalSampleRate, currentSampleRate, processedBuffers.back())) {
                        // If resampling fails, use original buffer
                        processedBuffers.back() = info.buffer;
                    }
                }
                else
                {
                    // No resampling needed
                    processedBuffers.back() = info.buffer;
                }
                
                // Update random distribution for new sample count
                if (sampleBank.size() > 1) {
                    randomDistribution = std::uniform_int_distribution<int>(0, static_cast<int>(sampleBank.size()) - 1);
                }
                
                // Trigger timestretch update for the new sample
                triggerTimestretchUpdate();
                
                return true;
            }
        }
    }
    
    return false;
}

bool SampleManager::reloadSampleFromPath(double currentSampleRate)
{
    if (sampleBank.empty()) {
        return false;
    }
    
    // Try to reload the first sample from its path
    juce::File sampleFile(sampleBank[0].path);
    if (!sampleFile.existsAsFile()) {
        return false;
    }
    
    // Try to reload the sample
    return loadSample(sampleFile, currentSampleRate);
}

void SampleManager::setProcessedSampleBuffer(const juce::AudioBuffer<float>& buffer, int index)
{
    std::lock_guard<std::mutex> lock(sampleBankMutex);
    if (index >= 0 && index < static_cast<int>(processedBuffers.size())) {
        processedBuffers[index] = buffer;
    }
}

void SampleManager::clearProcessedBuffer(int index)
{
    std::lock_guard<std::mutex> lock(sampleBankMutex);
    if (index >= 0 && index < static_cast<int>(processedBuffers.size())) {
        processedBuffers[index].clear();
    }
}

void SampleManager::removeSample(int index)
{
    std::lock_guard<std::mutex> lock(sampleBankMutex);
    if (index >= 0 && index < static_cast<int>(sampleBank.size())) {
        sampleBank.erase(sampleBank.begin() + index);
        processedBuffers.erase(processedBuffers.begin() + index);
        
        // Update random distribution for new sample count
        if (sampleBank.size() > 1) {
            randomDistribution = std::uniform_int_distribution<int>(0, static_cast<int>(sampleBank.size()) - 1);
        } else {
            randomDistribution = std::uniform_int_distribution<int>(0, 0);
        }
        
        // Adjust chain selector if it's now out of bounds
        if (chainSelector >= static_cast<int>(sampleBank.size())) {
            chainSelector = static_cast<int>(sampleBank.size()) - 1;
            if (chainSelector < 0) chainSelector = 0;
        }
    }
}

void SampleManager::clearSampleBank()
{
    std::lock_guard<std::mutex> lock(sampleBankMutex);
    sampleBank.clear();
    processedBuffers.clear();
    chainSelector = 0;
    randomDistribution = std::uniform_int_distribution<int>(0, 0);
}

juce::String SampleManager::getSampleName(int index) const
{
    std::lock_guard<std::mutex> lock(sampleBankMutex);
    if (index >= 0 && index < static_cast<int>(sampleBank.size())) {
        return sampleBank[index].name;
    }
    return "";
}

double SampleManager::getOriginalSampleRate(int index) const
{
    std::lock_guard<std::mutex> lock(sampleBankMutex);
    if (index >= 0 && index < static_cast<int>(sampleBank.size())) {
        return sampleBank[index].originalSampleRate;
    }
    return 44100.0;
}

const juce::String& SampleManager::getSamplePath(int index) const
{
    std::lock_guard<std::mutex> lock(sampleBankMutex);
    if (index >= 0 && index < static_cast<int>(sampleBank.size())) {
        return sampleBank[index].path;
    }
    static const juce::String emptyString;
    return emptyString;
}

const juce::AudioBuffer<float>& SampleManager::getSampleBuffer(int index) const
{
    std::lock_guard<std::mutex> lock(sampleBankMutex);
    if (index >= 0 && index < static_cast<int>(sampleBank.size())) {
        return sampleBank[index].buffer;
    }
    static const juce::AudioBuffer<float> emptyBuffer;
    return emptyBuffer;
}

const juce::AudioBuffer<float>& SampleManager::getProcessedSampleBuffer(int index) const
{
    std::lock_guard<std::mutex> lock(sampleBankMutex);
    if (index >= 0 && index < static_cast<int>(processedBuffers.size())) {
        return processedBuffers[index];
    }
    static const juce::AudioBuffer<float> emptyBuffer;
    return emptyBuffer;
}

int SampleManager::getCurrentSampleIndexInternal() const
{
    if (sampleBank.empty()) {
        return -1;
    }
    
    // If we haven't made a randomization decision for this trigger yet, make it now
    if (!randomizationDecisionMade) {
        randomizationDecisionMade = true;
        
        // If randomization is enabled, decide whether to use random or chain selector
        if (randomizationAmount > 0.0f) {
            // Use randomization amount to determine probability of random selection
            float randomValue = static_cast<float>(randomGenerator()) / static_cast<float>(std::mt19937::max());
            if (randomValue < randomizationAmount) {
                // Use random selection
                cachedSampleIndex = randomDistribution(randomGenerator);
                return cachedSampleIndex;
            }
        }
        
        // Use chain selector
        int sampleCount = static_cast<int>(sampleBank.size());
        if (sampleCount <= 1) {
            cachedSampleIndex = 0;
        } else {
            // Map chain selector value (0-63) to sample index (0 to sampleCount-1)
            float normalizedSelector = static_cast<float>(chainSelector) / 63.0f;
            cachedSampleIndex = static_cast<int>(normalizedSelector * (sampleCount - 1) + 0.5f);
            cachedSampleIndex = juce::jlimit(0, sampleCount - 1, cachedSampleIndex);
        }
    }
    
    return cachedSampleIndex;
}

int SampleManager::getCurrentSampleIndex() const
{
    std::lock_guard<std::mutex> lock(sampleBankMutex);
    return getCurrentSampleIndexInternal();
}

const juce::AudioBuffer<float>& SampleManager::getCurrentSampleBuffer() const
{
    std::lock_guard<std::mutex> lock(sampleBankMutex);
    int currentIndex = getCurrentSampleIndexInternal();
    if (currentIndex >= 0 && currentIndex < static_cast<int>(sampleBank.size())) {
        return sampleBank[currentIndex].buffer;
    }
    static const juce::AudioBuffer<float> emptyBuffer(1, 1); // Initialize with 1 channel, 1 sample
    return emptyBuffer;
}

bool SampleManager::performSampleRateConversion(const juce::AudioBuffer<float>& sourceBuffer, 
                                               double sourceSampleRate, 
                                               double targetSampleRate,
                                               juce::AudioBuffer<float>& destBuffer)
{
    // Calculate the required buffer size after resampling
    double conversionRatio = targetSampleRate / sourceSampleRate;
    int resampledLength = static_cast<int>(sourceBuffer.getNumSamples() * conversionRatio);
    
    // Resize destination buffer for resampled data
    destBuffer.setSize(sourceBuffer.getNumChannels(), resampledLength);
    
    // Perform simple linear interpolation resampling
    for (int channel = 0; channel < sourceBuffer.getNumChannels(); ++channel) {
        auto* sourceData = sourceBuffer.getReadPointer(channel);
        auto* destData = destBuffer.getWritePointer(channel);
        
        for (int destSample = 0; destSample < resampledLength; ++destSample) {
            // Calculate source position with linear interpolation
            double sourcePos = destSample / conversionRatio;
            int sourceSample = static_cast<int>(sourcePos);
            double fraction = sourcePos - sourceSample;
            
            if (sourceSample < sourceBuffer.getNumSamples() - 1) {
                // Linear interpolation between two samples
                float sample1 = sourceData[sourceSample];
                float sample2 = sourceData[sourceSample + 1];
                destData[destSample] = sample1 + fraction * (sample2 - sample1);
            } else if (sourceSample < sourceBuffer.getNumSamples()) {
                // Use last sample if at the end
                destData[destSample] = sourceData[sourceSample];
            } else {
                // Silence if beyond source
                destData[destSample] = 0.0f;
            }
        }
    }
    
    return true;
}

void SampleManager::updateSampleInfo(const juce::String& name, const juce::String& path, double sampleRate)
{
    // This method is no longer used in the new implementation
    // Sample info is now stored in the SampleInfo struct
}

void SampleManager::triggerTimestretchUpdate()
{
    if (onTimestretchUpdate) {
        onTimestretchUpdate();
    }
}
