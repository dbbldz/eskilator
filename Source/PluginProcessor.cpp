#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"

HelloWorldVST3AudioProcessor::HelloWorldVST3AudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Audio Input", juce::AudioChannelSet::disabled(), true)
        .withOutput("Audio Output", juce::AudioChannelSet::stereo(), true))
{
    // Initialize timing system
    clearTimingQueue();
    hostTimingValid = false;
    lastHostTime = 0.0;
    hostTimeOffset = 0.0;
    
    // Set up default quarter note pattern (steps 0, 4, 8, 12)
    stepActive[0] = true;   // Beat 1
    stepActive[4] = true;   // Beat 2
    stepActive[8] = true;   // Beat 3
    stepActive[12] = true;  // Beat 4
    
    // Load default click sample
    loadDefaultClickSample();
}

HelloWorldVST3AudioProcessor::~HelloWorldVST3AudioProcessor()
{
}

const juce::String HelloWorldVST3AudioProcessor::getName() const
{
    return "Beat Generator v2";
}

bool HelloWorldVST3AudioProcessor::acceptsMidi() const
{
    return true;  // Accept MIDI input for timing
}

bool HelloWorldVST3AudioProcessor::producesMidi() const
{
    return true;  // Output MIDI notes
}

bool HelloWorldVST3AudioProcessor::isMidiEffect() const
{
    return false;  // This is an instrument, not a MIDI effect
}

double HelloWorldVST3AudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int HelloWorldVST3AudioProcessor::getNumPrograms()
{
    return 1;
}

int HelloWorldVST3AudioProcessor::getCurrentProgram()
{
    return 0;
}

void HelloWorldVST3AudioProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String HelloWorldVST3AudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void HelloWorldVST3AudioProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

void HelloWorldVST3AudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Store sample rate for AU compatibility - getSampleRate() can be unreliable during AU initialization
    currentSampleRate = sampleRate;
    
    // Reset timing system for new sample rate
    clearTimingQueue();
    hostTimingValid = false;
    
    // Set default timing precision based on sample rate
    if (sampleRate >= 96000.0)
        timingPrecisionSamples = 1; // High sample rate = high precision
    else if (sampleRate >= 48000.0)
        timingPrecisionSamples = 2; // Standard sample rate = good precision
    else
        timingPrecisionSamples = 4; // Lower sample rate = lower precision
    
    juce::ignoreUnused(samplesPerBlock);
}

void HelloWorldVST3AudioProcessor::releaseResources()
{
}

bool HelloWorldVST3AudioProcessor::isBusesLayoutSupported(const BusesLayout& busesLayout) const
{
    // More flexible bus layout for AU compatibility
    auto outputChannels = busesLayout.getMainOutputChannelSet();
    
    // Accept stereo or mono output
    if (outputChannels != juce::AudioChannelSet::stereo() && 
        outputChannels != juce::AudioChannelSet::mono())
        return false;
        
    // Input can be disabled or any valid channel set for AU compatibility
    return true;
}

void HelloWorldVST3AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    
    // Get host timing information
    auto playHead = getPlayHead();
    if (playHead != nullptr)
    {
        juce::AudioPlayHead::CurrentPositionInfo posInfo;
        if (playHead->getCurrentPosition(posInfo))
        {
            if (posInfo.isPlaying)
            {
                // Update host timing state
                updateHostTiming(posInfo);
                
                // Calculate buffer timing using precise sample rate
                double bufferDuration = buffer.getNumSamples() / getSampleRate();
                double bufferStartTime = getPreciseTimeFromHost(posInfo);
                
                // Schedule timing events for this buffer - pass actual buffer size for precision
                scheduleTimingEvents(bufferStartTime, bufferDuration, posInfo.bpm, buffer.getNumSamples());
                
                // Process all scheduled timing events
                processTimingEvents(buffer, midiMessages);
            }
            else
            {
                // Transport stopped - clear timing queue
                clearTimingQueue();
                lastBeatClock = -1;
            }
        }
    }
    
    // Process audio for each sample
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        // Generate audio for each output channel
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);
            
            // Process all active sample voices
            float sampleValue = 0.0f;
            bool hasActiveVoices = false;
            
            for (int voiceIndex = 0; voiceIndex < MAX_VOICES; ++voiceIndex)
            {
                auto& voice = sampleVoices[voiceIndex];
                if (voice.isActive && voice.samplePosition < sampleBuffer.getNumSamples())
                {
                    hasActiveVoices = true;
                    int sourceChannel = juce::jmin(channel, sampleBuffer.getNumChannels() - 1);
                    float voiceSampleValue = sampleBuffer.getSample(sourceChannel, voice.samplePosition) * 0.5f;
                    
                    // Apply ADSR envelope
                    voiceSampleValue *= voice.currentEnvelopeValue;
                    sampleValue += voiceSampleValue;
                }
            }
            
            // Output audio from active voices
            if (hasActiveVoices)
            {
                channelData[sample] = sampleValue;
            }
            else
            {
                channelData[sample] = 0.0f;
            }
        }
        
        // Process all active sample voices
        for (int voiceIndex = 0; voiceIndex < MAX_VOICES; ++voiceIndex)
        {
            auto& voice = sampleVoices[voiceIndex];
            if (voice.isActive)
            {
                // Advance sample position
                if (voice.samplePosition < sampleBuffer.getNumSamples())
                {
                    voice.samplePosition++;
                }
                
                // Process ADSR envelope
                if (voice.currentEnvelopeState != EnvelopeState::Idle)
                {
                    voice.envelopeSampleCounter++;
                    float sampleRate = static_cast<float>(currentSampleRate);
                    
                    switch (voice.currentEnvelopeState)
                    {
                        case EnvelopeState::Attack:
                        {
                            float attackSamples = attackTimeSeconds * sampleRate;
                            if (voice.envelopeSampleCounter >= attackSamples)
                            {
                                voice.currentEnvelopeState = EnvelopeState::Decay;
                                voice.envelopeSampleCounter = 0;
                                voice.currentEnvelopeValue = 1.0f;
                            }
                            else
                            {
                                voice.currentEnvelopeValue = voice.envelopeSampleCounter / attackSamples;
                            }
                            break;
                        }
                        case EnvelopeState::Decay:
                        {
                            float decaySamples = decayTimeSeconds * sampleRate;
                            if (voice.envelopeSampleCounter >= decaySamples)
                            {
                                voice.currentEnvelopeState = EnvelopeState::Sustain;
                                voice.envelopeSampleCounter = 0;
                            }
                            else
                            {
                                float decayProgress = voice.envelopeSampleCounter / decaySamples;
                                voice.currentEnvelopeValue = 1.0f - (decayProgress * (1.0f - sustainLevelNormalized));
                            }
                            break;
                        }
                        case EnvelopeState::Sustain:
                        {
                            voice.currentEnvelopeValue = sustainLevelNormalized;
                            break;
                        }
                        case EnvelopeState::Release:
                        {
                            float releaseSamples = releaseTimeSeconds * sampleRate;
                            if (voice.envelopeSampleCounter >= releaseSamples)
                            {
                                voice.currentEnvelopeState = EnvelopeState::Idle;
                                voice.currentEnvelopeValue = 0.0f;
                                voice.isActive = false; // Mark voice as inactive
                            }
                            else
                            {
                                float releaseProgress = voice.envelopeSampleCounter / releaseSamples;
                                voice.currentEnvelopeValue = sustainLevelNormalized * (1.0f - releaseProgress);
                            }
                            break;
                        }
                        default:
                            break;
                    }
                }
                
                // Check if voice should start release
                if (voice.noteOffCountdown > 0)
                {
                    voice.noteOffCountdown--;
                    if (voice.noteOffCountdown <= 0 || voice.samplePosition >= sampleBuffer.getNumSamples())
                    {
                        voice.currentEnvelopeState = EnvelopeState::Release;
                        voice.envelopeSampleCounter = 0;
                    }
                }
            }
        }
    }
}

bool HelloWorldVST3AudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* HelloWorldVST3AudioProcessor::createEditor()
{
    return new HelloWorldVST3AudioProcessorEditor(*this);
}

void HelloWorldVST3AudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::ignoreUnused(destData);
}

void HelloWorldVST3AudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    juce::ignoreUnused(data, sizeInBytes);
}

// Step sequencer functionality
void HelloWorldVST3AudioProcessor::setStepActive(int stepIndex, bool isActive)
{
    if (stepIndex >= 0 && stepIndex < 16)
    {
        stepActive[stepIndex] = isActive;
    }
}

bool HelloWorldVST3AudioProcessor::isStepActive(int stepIndex) const
{
    if (stepIndex >= 0 && stepIndex < 16)
        return stepActive[stepIndex];
    return false;
}

void HelloWorldVST3AudioProcessor::loadSample(const juce::File& audioFile)
{
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats(); // WAV, AIFF, etc.
    
    auto reader = formatManager.createReaderFor(audioFile);
    if (reader != nullptr)
    {
        // Resize buffer to fit the sample
        sampleBuffer.setSize(static_cast<int>(reader->numChannels), 
                           static_cast<int>(reader->lengthInSamples));
        
        // Read the audio file into our buffer
        reader->read(&sampleBuffer, 0, static_cast<int>(reader->lengthInSamples), 0, true, true);
        
        // Store the sample name
        currentSampleName = audioFile.getFileNameWithoutExtension();
        
        // Reset playback position
        samplePosition = 0;
        
        delete reader;
    }
}

void HelloWorldVST3AudioProcessor::loadDefaultClickSample()
{
    // Load the embedded click sample from binary data
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    
    // Get the embedded binary data
    const char* clickData = BinaryData::Click_808_wav;
    int clickDataSize = BinaryData::Click_808_wavSize;
    
    // Create a memory input stream from the binary data
    auto memoryStream = std::make_unique<juce::MemoryInputStream>(clickData, clickDataSize, false);
    
    // Create a reader for the audio data
    auto reader = formatManager.createReaderFor(std::move(memoryStream));
    
    if (reader != nullptr)
    {
        // Resize buffer to fit the sample
        sampleBuffer.setSize(static_cast<int>(reader->numChannels), 
                           static_cast<int>(reader->lengthInSamples));
        
        // Read the audio file into our buffer
        reader->read(&sampleBuffer, 0, static_cast<int>(reader->lengthInSamples), 0, true, true);
        
        // Store the sample name
        currentSampleName = "Click 808";
        
        // Reset playback position
        samplePosition = 0;
        
        delete reader;
    }
}



juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HelloWorldVST3AudioProcessor();
}

// Private timing methods implementation
void HelloWorldVST3AudioProcessor::updateHostTiming(const juce::AudioPlayHead::CurrentPositionInfo& posInfo)
{
    // Calculate precise time from host position
    double currentTime = getPreciseTimeFromHost(posInfo);
    
    // Update host timing state
    if (!hostTimingValid)
    {
        hostTimeOffset = 0.0;
        lastHostTime = currentTime;
        hostTimingValid = true;
        
        // Auto-calibrate timing for new BPM
        if (posInfo.bpm > 0.0)
        {
            calibrateTiming(posInfo.bpm);
        }
    }
    else
    {
        // Check for timing discontinuities (loops, jumps, etc.)
        static juce::int64 lastTimeInSamples = 0;
        
        // Only check for discontinuities if we have a valid previous time
        if (lastTimeInSamples > 0)
        {
            double expectedTime = lastHostTime + (posInfo.timeInSamples - lastTimeInSamples) / currentSampleRate;
            if (std::abs(currentTime - expectedTime) > 0.1) // More than 100ms jump
            {
                hostTimeOffset = currentTime - expectedTime;
            }
        }
        lastTimeInSamples = posInfo.timeInSamples;
        
        // Check for BPM changes and recalibrate if needed
        if (std::abs(posInfo.bpm - lastBPM) > 0.1) // BPM changed by more than 0.1
        {
            calibrateTiming(posInfo.bpm);
            lastBPM = posInfo.bpm;
        }
        
        lastHostTime = currentTime;
    }
}

void HelloWorldVST3AudioProcessor::scheduleTimingEvents(double bufferStartTime, double bufferDuration, double bpm, int actualBufferSamples)
{
    // Clear previous timing queue
    timingQueue.clear();
    
    if (bpm <= 0.0) return;
    
    // Get current host position info for precise synchronization
    auto playHead = getPlayHead();
    if (playHead == nullptr) return;
    
    juce::AudioPlayHead::CurrentPositionInfo posInfo;
    if (!playHead->getCurrentPosition(posInfo) || posInfo.ppqPosition < 0.0) return;
    
    // Calculate timing for 16th notes (4 per quarter note)
    double sixteenthNoteDuration = 60.0 / (bpm * 4.0);
    double samplesPerSixteenth = sixteenthNoteDuration * currentSampleRate;
    
    // Use ultra-precise PPQ calculation based on host timing
    double bufferStartPPQ = posInfo.ppqPosition;
    double currentBPM = posInfo.bpm > 0.0 ? posInfo.bpm : bpm;
    
    // Verify we have valid PPQ position
    if (bufferStartPPQ < 0.0) 
    {
        // Fallback: use time-based calculation only if PPQ not available
        double samplesFromTimelineStart = static_cast<double>(posInfo.timeInSamples);
        bufferStartPPQ = (samplesFromTimelineStart / getSampleRate()) * (currentBPM / 60.0);
    }
    
    // Calculate PPQ per sample for drift-free precision
    double ppqPerSample = (currentBPM / 60.0) / getSampleRate();
    // Use actual buffer samples - no calculation to introduce errors
    double bufferEndPPQ = bufferStartPPQ + (actualBufferSamples * ppqPerSample);
    
    // Calculate 16th note positions in PPQ (0.25 PPQ per 16th note)
    double sixteenthNotePPQ = 0.25; // 1/4 of a quarter note
    
    // Add small buffer zone to catch events near boundaries (prevents timing gaps)
    double searchStartPPQ = bufferStartPPQ - (sixteenthNotePPQ * 0.05); // 5% buffer
    double searchEndPPQ = bufferEndPPQ + (sixteenthNotePPQ * 0.05);
    
    // Find the first 16th note that falls within the extended search range
    double firstSixteenthPPQ = std::ceil(searchStartPPQ / sixteenthNotePPQ) * sixteenthNotePPQ;
    
    // Schedule events for all 16th notes that fall within the extended search range
    double currentPPQ = firstSixteenthPPQ;
    while (currentPPQ <= searchEndPPQ)
    {
        // Calculate which step this represents (0-15) based on 16th note position
        // Each 4 quarter notes = 16 sixteenth notes = one complete 16-step cycle
        int totalSixteenths = static_cast<int>(std::round(currentPPQ / sixteenthNotePPQ));
        int stepNumber = totalSixteenths % 16;
        
        // Only schedule if this step is active
        if (stepActive[stepNumber])
        {
            // Ultra-precise sample calculation using PPQ-per-sample ratio
            // This eliminates floating-point accumulation errors
            double ppqFromBufferStart = currentPPQ - bufferStartPPQ;
            double exactSampleOffset = ppqFromBufferStart / ppqPerSample;
            
            // For ultra-precise timing, use fractional sample positioning
            // This prevents cumulative rounding errors
            double fractionalPart = exactSampleOffset - std::floor(exactSampleOffset);
            int sampleOffset = static_cast<int>(std::floor(exactSampleOffset));
            
            // If fractional part is >= 0.5, round up for better precision
            if (fractionalPart >= 0.5)
            {
                sampleOffset += 1;
            }
            
            // Ensure sample offset is within reasonable buffer bounds
            // Allow slight negative offsets for buffer boundary precision, but clamp to valid range
            int bufferSizeInSamples = actualBufferSamples;
            if (sampleOffset >= -2 && sampleOffset < bufferSizeInSamples + 2) // 2-sample tolerance
            {
                // Clamp to actual buffer bounds for processing
                sampleOffset = juce::jlimit(0, bufferSizeInSamples - 1, sampleOffset);
                // Check for duplicates using PPQ position with higher precision
                bool shouldSchedule = true;
                if (lastTriggeredStep == stepNumber)
                {
                    double ppqSinceLastTrigger = currentPPQ - lastTriggeredStepPPQ;
                    if (ppqSinceLastTrigger < sixteenthNotePPQ * 0.95) // 95% threshold for precision
                    {
                        shouldSchedule = false;
                    }
                }
                
                if (shouldSchedule)
                {
                    TimingEvent event;
                    event.stepNumber = stepNumber;
                    // Store precise PPQ position for drift-free timing
                    event.preciseTime = currentPPQ; // Store PPQ instead of time
                    event.sampleOffset = sampleOffset;
                    event.isProcessed = false;
                    
                    timingQueue.push_back(event);
                    
                    // Update last triggered info with precise PPQ
                    lastTriggeredStep = stepNumber;
                    lastTriggeredStepPPQ = currentPPQ;
                }
            }
        }
        
        // Move to next 16th note
        currentPPQ += sixteenthNotePPQ;
    }
}

void HelloWorldVST3AudioProcessor::processTimingEvents(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    for (auto& event : timingQueue)
    {
        if (!event.isProcessed && event.sampleOffset < buffer.getNumSamples())
        {
            // Mark event as processed
            event.isProcessed = true;
            
            // Update global timing state to prevent duplicates
            // Note: event.preciseTime now stores PPQ position for precision
            lastTriggeredStepPPQ = event.preciseTime; // This is now PPQ position
            lastTriggeredStep = event.stepNumber;
            
            // Generate MIDI note
            int noteNumber = 60 + (event.stepNumber % 12);
            int velocity = 100;
            midiMessages.addEvent(juce::MidiMessage::noteOn(1, noteNumber, (juce::uint8)velocity), event.sampleOffset);
            
            // Trigger sample playback if available
            if (hasSample())
            {
                // Find an available voice
                int voiceIndex = -1;
                for (int i = 0; i < MAX_VOICES; ++i)
                {
                    if (!sampleVoices[i].isActive)
                    {
                        voiceIndex = i;
                        break;
                    }
                }
                
                if (voiceIndex >= 0)
                {
                    // Start new sample voice
                    auto& voice = sampleVoices[voiceIndex];
                    voice.samplePosition = 0;
                    voice.noteOffCountdown = juce::jmin(sampleBuffer.getNumSamples(), static_cast<int>(currentSampleRate * 2.0));
                    voice.currentEnvelopeState = EnvelopeState::Attack;
                    voice.currentEnvelopeValue = 0.0f;
                    voice.envelopeSampleCounter = 0;
                    voice.isActive = true;
                }
            }
        }
    }
}

double HelloWorldVST3AudioProcessor::getPreciseTimeFromHost(const juce::AudioPlayHead::CurrentPositionInfo& posInfo)
{
    // Use timeInSamples for highest precision
    if (posInfo.timeInSamples >= 0)
    {
        return posInfo.timeInSamples / currentSampleRate;
    }
    
    // Fallback to PPQ position if timeInSamples is not available
    if (posInfo.ppqPosition >= 0.0 && posInfo.bpm > 0.0)
    {
        return (posInfo.ppqPosition * 60.0) / posInfo.bpm;
    }
    
    // Last resort fallback
    return 0.0;
}

void HelloWorldVST3AudioProcessor::clearTimingQueue()
{
    timingQueue.clear();
    lastProcessedTime = 0.0;
    currentBufferStartTime = 0.0;
    
    // Reset global timing state
    lastTriggeredStepTime = -1.0;
    lastTriggeredStep = -1;
    lastTriggeredStepPPQ = -1.0;
    globalTimingOffset = 0.0;
}

// Additional timing utility method
void HelloWorldVST3AudioProcessor::calibrateTiming(double bpm)
{
    if (bpm <= 0.0) return;
    
    // With PPQ-based timing, we no longer need the old precision samples approach
    // The new system uses fractional sample positioning for drift-free timing
    timingPrecisionSamples = 1; // Always sample-accurate now
    
    // Reset timing state for new BPM
    lastTriggeredStepTime = -1.0;
    lastTriggeredStep = -1;
    lastTriggeredStepPPQ = -1.0;
    globalTimingOffset = 0.0;
}

