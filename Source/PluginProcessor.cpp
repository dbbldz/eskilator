#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <map>
#include <cmath>

juce::AudioProcessorValueTreeState::ParameterLayout GliderAudioProcessor::createParameterLayout()
{
    return ParameterManager::createParameterLayout();
}

GliderAudioProcessor::GliderAudioProcessor()
    : AudioProcessor(getBusesLayout()),
      parameterManager(*this)
{
    // Register for parameter change notifications for ADSR parameters
    parameterManager.getAPVTS().addParameterListener("attack", this);
    parameterManager.getAPVTS().addParameterListener("decay", this);
    parameterManager.getAPVTS().addParameterListener("sustain", this);
    parameterManager.getAPVTS().addParameterListener("release", this);

    // Debug: Log plugin capabilities at construction
    PluginLogger::setLoggingEnabled(false);

    // Load default click sample (optional - plugin can work without it)
    loadDefaultSample(currentSampleRate);

    // Initialize sample gain to -6dB for safer starting level
    sampleGainDb = -6.0f;
}

GliderAudioProcessor::~GliderAudioProcessor()
{
    // Unregister parameter listeners to avoid dangling pointer
    parameterManager.getAPVTS().removeParameterListener("attack", this);
    parameterManager.getAPVTS().removeParameterListener("decay", this);
    parameterManager.getAPVTS().removeParameterListener("sustain", this);
    parameterManager.getAPVTS().removeParameterListener("release", this);

    // Mark plugin as not ready to prevent new background operations
    isPluginReady = false;
}

const juce::String GliderAudioProcessor::getName() const
{
    return "Glider";
}

bool GliderAudioProcessor::acceptsMidi() const
{
    return true;  // Accept MIDI input for sample triggering
}

bool GliderAudioProcessor::producesMidi() const
{
    return false;  // We don't output MIDI - we're a sample player
}

bool GliderAudioProcessor::isMidiEffect() const
{
    return false;  // This is an instrument, not a MIDI effect
}

bool GliderAudioProcessor::isSynth() const
{
    return true;  // This IS a synthesizer/instrument
}

double GliderAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int GliderAudioProcessor::getNumPrograms()
{
    return 1;
}

int GliderAudioProcessor::getCurrentProgram()
{
    return 0;
}

void GliderAudioProcessor::setCurrentProgram(int index)
{
    // No program support
}

const juce::String GliderAudioProcessor::getProgramName(int index)
{
    return "Default";
}

void GliderAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
    // No program support
}

void GliderAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Store sample rate for calculations
    currentSampleRate = sampleRate;
    
    // Reset all voices and initialize ADSR
    for (auto& voice : sampleVoices)
    {
        voice.isActive = false;
        voice.isGliding = false;
        voice.samplePosition = 0;
        voice.phaseAccumulator = 0.0;
        voice.glideCurrentStep = 0;
        voice.glideTotalSteps = 0;
        voice.glideSamplesPerStep = 0;
        voice.glideSampleCounter = 0;

        // Initialize ADSR envelope
        voice.adsr.setSampleRate(sampleRate);
        juce::ADSR::Parameters adsrParams;
        adsrParams.attack = getAttack();
        adsrParams.decay = getDecay();
        adsrParams.sustain = getSustain();
        adsrParams.release = getRelease();
        voice.adsr.setParameters(adsrParams);
    }
    
    // Mark plugin as ready
    isPluginReady = true;
}

void GliderAudioProcessor::releaseResources()
{
    // Mark plugin as not ready
    isPluginReady = false;
}

bool GliderAudioProcessor::isBusesLayoutSupported(const BusesLayout& busesLayout) const
{
    // Support mono and stereo
    if (busesLayout.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && busesLayout.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    
    return true;
}

void GliderAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Sample-accurate MIDI handling: split buffer at each MIDI event
    int startSample = 0;

    for (auto it = midiMessages.begin(); it != midiMessages.end(); ++it)
    {
        auto message = (*it).getMessage();
        int sampleOffset = (*it).samplePosition;

        // Render audio up to this MIDI event
        if (sampleOffset > startSample)
        {
            renderAudioSegment(buffer, startSample, sampleOffset);
        }
                    
        if (message.isNoteOn())
        {
            logger.log("MIDI Note ON: Note=" + juce::String(message.getNoteNumber()) + 
                      ", Velocity=" + juce::String(message.getVelocity()));
            
            // Trigger sample playback
            logger.log("hasSample() = " + juce::String(hasSample() ? "true" : "false"));
            if (hasSample())
            {
                // Convert MIDI note number to pitch offset (C4 = 60 = 0 semitones)
                int baseNoteNumber = 60; // Middle C
                float pitchOffset = static_cast<float>(message.getNoteNumber() - baseNoteNumber);
                // No pitch limit - allow full MIDI range
                
                // MONOPHONIC DESIGN: Always use voice 0, apply crossfade on every note
                auto& voice = sampleVoices[0];

                // Check if we should apply glide (different pitch)
                float glideTime = getGlideTime();
                int glideSteps = getGlideSteps();
                bool shouldGlide = (glideTime > 0.0f) && hasLastPitch && (lastMonophonicPitch != pitchOffset);

                logger.log("Note trigger - HasLastPitch=" + juce::String(hasLastPitch ? "true" : "false") +
                          ", LastPitch=" + juce::String(lastMonophonicPitch) +
                          ", NewPitch=" + juce::String(pitchOffset) +
                          ", ShouldGlide=" + juce::String(shouldGlide ? "true" : "false"));

                // Save old state for crossfade before resetting
                voice.glideOldPhaseAccumulator = voice.phaseAccumulator;
                voice.glideOldPitchRatio = voice.cachedPitchRatio > 0.0f ? voice.cachedPitchRatio : std::pow(2.0f, voice.pitch / 12.0f);

                if (shouldGlide)
                {
                    // Different pitch - apply glide
                    voice.isGliding = true;
                    voice.glideStartPitch = lastMonophonicPitch;
                    voice.glideTargetPitch = pitchOffset;
                    voice.glideCurrentStep = 0;
                    voice.glideTotalSteps = glideSteps;
                    voice.glideSamplesPerStep = static_cast<int>(glideTime * 0.001f * currentSampleRate) / glideSteps;
                    voice.glideSampleCounter = 0;
                    voice.cachedPitchRatio = 0.0f; // Force recalculation
                    voice.pitch = voice.glideStartPitch; // Start with beginning pitch

                    logger.log("Applied glide to voice 0");
                }
                else
                {
                    // Same pitch or first note - no glide, just set pitch directly
                    voice.isGliding = false;
                    voice.pitch = pitchOffset;
                    voice.cachedPitchRatio = 0.0f; // Force recalculation

                    logger.log("No glide - set voice 0 to pitch " + juce::String(pitchOffset));
                }

                // Always reset sample position and apply crossfade for clean restart
                voice.phaseAccumulator = 0.0;
                voice.samplePosition = 0;
                voice.isInGlideCrossfade = true;
                voice.glideCrossfadeSampleCount = 0;

                // Update velocity and activate voice
                voice.velocity = message.getVelocity() / 127.0f;
                voice.isActive = true;

                // ADSR ENVELOPE: Always restart envelope on every note
                voice.adsr.noteOn();
                logger.log("ADSR noteOn() triggered - envelope restarted");

                // Update monophonic pitch tracking
                lastMonophonicPitch = pitchOffset;
                hasLastPitch = true;
            }
        }
        else if (message.isNoteOff())
        {
            // Trigger release phase of ADSR envelope for monophonic voice
            auto& voice = sampleVoices[0];
            if (voice.isActive)
            {
                voice.adsr.noteOff();
                logger.log("ADSR noteOff() triggered - starting release phase");
            }
        }

        // Update start position for next segment
        startSample = sampleOffset;
    }

    // Render any remaining audio after last MIDI event
    if (startSample < buffer.getNumSamples())
    {
        renderAudioSegment(buffer, startSample, buffer.getNumSamples());
    }
}

// Render audio for a segment of the buffer
void GliderAudioProcessor::renderAudioSegment(juce::AudioBuffer<float>& buffer, int startSample, int endSample)
{
    // Get current sample buffer
    int currentSampleIndex = sampleManager.getCurrentSampleIndex();
    const juce::AudioBuffer<float>& currentBuffer = sampleManager.getCurrentSampleBuffer();
    
    // Safety check: if no valid sample is loaded, output silence
    if (currentSampleIndex < 0 || currentBuffer.getNumSamples() <= 1) {
        buffer.clear();
        return;
    }
    
    // Cache parameter values to avoid repeated function calls
    // Note: Per-sample gain is applied per-voice in the voice processing loop
    float attackSamples = getAttack() * currentSampleRate;
    float decaySamples = getDecay() * currentSampleRate;
    float sustainValue = getSustain();
    float releaseSamples = getRelease() * currentSampleRate;
    
    // PERFORMANCE: Cache expensive gain calculations outside the sample loop
    float masterGainLinear = juce::Decibels::decibelsToGain(getSampleGain());
    float perSampleGainLinear = juce::Decibels::decibelsToGain(getSampleGain(currentSampleIndex));
    
    // ROBUST BUFFER VALIDATION: Use comprehensive validation like vst-test2
    bool bufferValid = (currentBuffer.getNumSamples() > 0 && currentBuffer.getNumChannels() > 0);
    int maxSamples = bufferValid ? currentBuffer.getNumSamples() : 0;
    int maxChannels = bufferValid ? currentBuffer.getNumChannels() : 0;
    
    // THREAD-SAFE PERFORMANCE OPTIMIZATION: Cache expensive operations outside the sample loop (like vst-test2)
    int currentVoiceCount = getVoiceCount();
    
    // PERFORMANCE: Early exit if no valid audio to process
    if (!bufferValid || currentVoiceCount == 0 || maxSamples == 0 || maxChannels == 0) {
        buffer.clear();
        return;
    }
    
    // Process audio for each sample in the segment
    for (int sample = startSample; sample < endSample; ++sample)
    {
        // Generate audio for each output channel
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);
            
            // Process the single monophonic voice (voice 0)
            float sampleValue = 0.0f;
            bool hasActiveVoices = false;

            int sourceChannel = juce::jmin(channel, maxChannels - 1);

            // MONOPHONIC: Only process voice 0
            auto& voice = sampleVoices[0];

            if (voice.isActive)
                {
                    // MULTIPLE SAFETY CHECKS: Comprehensive bounds validation (like vst-test2)
                    // Since we use phaseAccumulator (not samplePosition), validate based on current phase
                    float currentSampleIndex = static_cast<float>(voice.phaseAccumulator);
                    int currentSampleIndexInt = static_cast<int>(currentSampleIndex);

                    // Check if sample has ended (don't deactivate on envelope completion - allows sequential notes)
                    if (currentSampleIndexInt < 0 || currentSampleIndexInt >= maxSamples)
                    {
                        // Deactivate voice only when sample ends
                        voice.isActive = false;
                        voice.isGliding = false;
                        continue;
                    }
                    
                    // PARANOID VALIDATION: Double-check buffer state before access (like vst-test2)
                    if (maxSamples == 0 || maxChannels == 0 || sourceChannel >= maxChannels)
                    {
                        continue; // Skip this voice entirely if buffer is invalid
                    }
                    
                    hasActiveVoices = true;
                    
                    // For monophonic synth, we don't need noteOffCountdown - voices are managed by voice stealing
                    // Process glide for stepped portamento (Triton-style) - RE-ENABLED
                    if (voice.isGliding)
                    {
                        voice.glideSampleCounter++;
                        
                        // Check if it's time to move to the next step
                        if (voice.glideSampleCounter >= voice.glideSamplesPerStep)
                        {
                            voice.glideCurrentStep++;
                            voice.glideSampleCounter = 0;
                            
                            // Calculate the stepped pitch (discrete steps, not smooth)
                            if (voice.glideCurrentStep >= voice.glideTotalSteps)
                            {
                                // Glide complete - set final pitch (NO PHASE COMPENSATION)
                                voice.pitch = voice.glideTargetPitch;
                                voice.cachedPitchRatio = 0.0f; // Force recalculation on next sample
                                voice.isGliding = false;
                            }
                            else
                            {
                                // Calculate stepped pitch - this creates the "cheap" Triton sound
                                float stepProgress = static_cast<float>(voice.glideCurrentStep) / static_cast<float>(voice.glideTotalSteps);
                                float pitchDifference = voice.glideTargetPitch - voice.glideStartPitch;
                                float newPitch = voice.glideStartPitch + (pitchDifference * stepProgress);

                                // Update pitch directly (NO PHASE COMPENSATION)
                                // The discrete pitch jump is intentional for Triton-style stepped glide
                                voice.pitch = newPitch;
                                voice.cachedPitchRatio = 0.0f; // Force recalculation on next sample
                            }
                        }
                    }
                    
                    // Apply pitch modification
                    float voiceSampleValue = 0.0f;
                    
                    // Calculate pitch ratio with smoothing for glide
                    float pitchRatio = voice.cachedPitchRatio;
                    if (pitchRatio == 0.0f)
                    {
                        pitchRatio = std::pow(2.0f, voice.pitch / 12.0f);
                        voice.cachedPitchRatio = pitchRatio;
                        
                        
                        // Debug logging for pitch calculation
                        if (sample == 0 && channel == 0) { // Only log once per buffer
                            logger.log("Voice 0 - Pitch: " + juce::String(voice.pitch) +
                                      ", Pitch Ratio: " + juce::String(pitchRatio) +
                                      ", Phase: " + juce::String(voice.phaseAccumulator));
                        }
                    }
                    
                    // Phase-continuous sample reading
                    float oldPhase = voice.phaseAccumulator;
                    voice.phaseAccumulator += pitchRatio;

                    // Log suspicious phase jumps during glide
                    if (voice.isGliding && sample == 0 && channel == 0) {
                        float phaseJump = voice.phaseAccumulator - oldPhase;
                        if (phaseJump > pitchRatio * 2.0f || phaseJump < pitchRatio * 0.5f) {
                            logger.log("PHASE JUMP - Voice 0" +
                                      juce::String(" - Expected: ") + juce::String(pitchRatio) +
                                      " - Actual: " + juce::String(phaseJump) +
                                      " - From: " + juce::String(oldPhase) +
                                      " - To: " + juce::String(voice.phaseAccumulator));
                        }
                    }

                    // GLIDE CROSSFADE: Read from both old and new positions during crossfade
                    float pitchedSampleValue = 0.0f;

                    if (voice.isInGlideCrossfade && voice.glideCrossfadeSampleCount < voice.GLIDE_CROSSFADE_LENGTH)
                    {
                        // Read from NEW position (restarted sample)
                        float sampleIndex = static_cast<float>(voice.phaseAccumulator);
                        int sampleIndexInt = static_cast<int>(sampleIndex);
                        float sampleIndexFrac = sampleIndex - sampleIndexInt;

                        float newSample = 0.0f;
                        if (sampleIndexInt >= 0 && sampleIndexInt + 1 < maxSamples)
                        {
                            try {
                                float sample1 = currentBuffer.getSample(sourceChannel, sampleIndexInt);
                                float sample2 = currentBuffer.getSample(sourceChannel, sampleIndexInt + 1);
                                newSample = sample1 + (sample2 - sample1) * sampleIndexFrac;
                            } catch (...) {
                                newSample = 0.0f;
                            }
                        }
                        else if (sampleIndexInt >= 0 && sampleIndexInt < maxSamples)
                        {
                            try {
                                newSample = currentBuffer.getSample(sourceChannel, sampleIndexInt);
                            } catch (...) {
                                newSample = 0.0f;
                            }
                        }

                        // Read from OLD position (continuation)
                        float oldSampleIndex = static_cast<float>(voice.glideOldPhaseAccumulator);
                        int oldSampleIndexInt = static_cast<int>(oldSampleIndex);
                        float oldSampleIndexFrac = oldSampleIndex - oldSampleIndexInt;

                        float oldSample = 0.0f;
                        if (oldSampleIndexInt >= 0 && oldSampleIndexInt + 1 < maxSamples)
                        {
                            try {
                                float sample1 = currentBuffer.getSample(sourceChannel, oldSampleIndexInt);
                                float sample2 = currentBuffer.getSample(sourceChannel, oldSampleIndexInt + 1);
                                oldSample = sample1 + (sample2 - sample1) * oldSampleIndexFrac;
                            } catch (...) {
                                oldSample = 0.0f;
                            }
                        }
                        else if (oldSampleIndexInt >= 0 && oldSampleIndexInt < maxSamples)
                        {
                            try {
                                oldSample = currentBuffer.getSample(sourceChannel, oldSampleIndexInt);
                            } catch (...) {
                                oldSample = 0.0f;
                            }
                        }

                        // Crossfade between old and new
                        float blend = static_cast<float>(voice.glideCrossfadeSampleCount) / static_cast<float>(voice.GLIDE_CROSSFADE_LENGTH);
                        pitchedSampleValue = (oldSample * (1.0f - blend)) + (newSample * blend);

                        // Increment old phase accumulator
                        voice.glideOldPhaseAccumulator += voice.glideOldPitchRatio;

                        // Increment crossfade counter
                        voice.glideCrossfadeSampleCount++;

                        // Check if crossfade is complete
                        if (voice.glideCrossfadeSampleCount >= voice.GLIDE_CROSSFADE_LENGTH)
                        {
                            voice.isInGlideCrossfade = false;
                        }
                    }
                    else
                    {
                        // Normal sample reading (no crossfade)
                        float sampleIndex = static_cast<float>(voice.phaseAccumulator);
                        int sampleIndexInt = static_cast<int>(sampleIndex);
                        float sampleIndexFrac = sampleIndex - sampleIndexInt;

                        // Debug logging for sample reading
                        if (sample == 0 && channel == 0 && voice.isGliding) {
                            logger.log("Voice 0 - Sample Index: " + juce::String(sampleIndex) +
                                      " (int: " + juce::String(sampleIndexInt) +
                                      ", frac: " + juce::String(sampleIndexFrac) +
                                      ") - Max Samples: " + juce::String(maxSamples));
                        }

                        // ULTRA-SAFE pitched sample value with comprehensive bounds checking (like vst-test2)
                        if (sampleIndexInt >= 0 && sampleIndexInt + 1 < maxSamples)
                        {
                            try {
                                float sample1 = currentBuffer.getSample(sourceChannel, sampleIndexInt);
                                float sample2 = currentBuffer.getSample(sourceChannel, sampleIndexInt + 1);
                                pitchedSampleValue = sample1 + (sample2 - sample1) * sampleIndexFrac;
                            } catch (...) {
                                pitchedSampleValue = 0.0f; // Safe fallback
                            }
                        }
                        else if (sampleIndexInt >= 0 && sampleIndexInt < maxSamples)
                        {
                            try {
                                pitchedSampleValue = currentBuffer.getSample(sourceChannel, sampleIndexInt);
                            } catch (...) {
                                pitchedSampleValue = 0.0f; // Safe fallback
                            }
                        }
                        else if (sampleIndexInt >= maxSamples)
                        {
                            // ENVELOPE DISABLED - one-shot sample has ended, deactivate voice
                            voice.isActive = false;
                            voice.isGliding = false;
                            pitchedSampleValue = 0.0f;
                        }
                    }
                    
                    // Apply cached gain values instead of calculating per sample
                    voiceSampleValue = pitchedSampleValue * masterGainLinear * perSampleGainLinear * voice.velocity;

                    // Apply ADSR envelope
                    float envelopeValue = voice.adsr.getNextSample();
                    voiceSampleValue *= envelopeValue;

                    sampleValue = voiceSampleValue;
                }

            // Write output (like vst-test2)
            if (hasActiveVoices) {
                channelData[sample] = sampleValue;
            } else {
                channelData[sample] = 0.0f;
            }
        }
    }

    // Clear any remaining channels
    for (int channel = buffer.getNumChannels(); channel < buffer.getNumChannels(); ++channel)
    {
        buffer.clear(channel, 0, buffer.getNumSamples());
    }
}

juce::AudioProcessorEditor* GliderAudioProcessor::createEditor()
{
    return new PluginEditor(*this);
}

bool GliderAudioProcessor::hasEditor() const
{
    return true;
}

void GliderAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameterManager.getAPVTS().copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());

    // Save sample bank information
    auto* sampleBankElement = xml->createNewChildElement("SampleBank");
    int sampleCount = sampleManager.getSampleCount();
    sampleBankElement->setAttribute("count", sampleCount);

    for (int i = 0; i < sampleCount; ++i)
    {
        auto* sampleElement = sampleBankElement->createNewChildElement("Sample");
        sampleElement->setAttribute("path", sampleManager.getSamplePath(i));
        sampleElement->setAttribute("name", sampleManager.getSampleName(i));
        sampleElement->setAttribute("gain", sampleManager.getSampleGain(i));
        sampleElement->setAttribute("transpose", sampleManager.getSampleTranspose(i));
    }

    copyXmlToBinary(*xml, destData);
}

void GliderAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
    {
        if (xmlState->hasTagName(parameterManager.getAPVTS().state.getType()))
        {
            parameterManager.getAPVTS().replaceState(juce::ValueTree::fromXml(*xmlState));
        }

        // Restore sample bank information
        auto* sampleBankElement = xmlState->getChildByName("SampleBank");
        if (sampleBankElement != nullptr)
        {
            // Clear existing samples first
            sampleManager.clearSampleBank();

            // Load each saved sample
            for (auto* sampleElement : sampleBankElement->getChildIterator())
            {
                juce::String samplePath = sampleElement->getStringAttribute("path");

                // Only load samples that have a valid path and aren't the built-in sample
                if (samplePath.isNotEmpty() && samplePath != "Built-in")
                {
                    juce::File sampleFile(samplePath);
                    if (sampleFile.existsAsFile())
                    {
                        // Load the sample
                        if (sampleManager.loadSample(sampleFile, currentSampleRate))
                        {
                            // Restore per-sample parameters
                            int index = sampleManager.getSampleCount() - 1; // Just loaded sample is the last one
                            sampleManager.setSampleGain(index, sampleElement->getDoubleAttribute("gain", 0.0));
                            sampleManager.setSampleTranspose(index, sampleElement->getDoubleAttribute("transpose", 0.0));
                        }
                    }
                }
            }

            // If no samples were loaded, load the default sample
            if (!sampleManager.hasSample())
            {
                loadDefaultSample(currentSampleRate);
            }
        }
    }

    // Call state restored callback if set
    if (onStateRestored)
    {
        onStateRestored();
    }
}

void GliderAudioProcessor::loadSample(const juce::File& audioFile)
{
    sampleManager.loadSample(audioFile, currentSampleRate);
}

void GliderAudioProcessor::loadDefaultSample(double sampleRate)
{
    sampleManager.loadDefaultSample(sampleRate);
}

juce::String GliderAudioProcessor::getCurrentSampleName() const
{
    int currentIndex = sampleManager.getCurrentSampleIndex();
    return sampleManager.getSampleName(currentIndex);
}

double GliderAudioProcessor::getSampleDuration(int index) const
{
    if (!sampleManager.hasSampleAtIndex(index))
        return 0.0;

    const auto& buffer = sampleManager.getSampleBuffer(index);
    double sampleRate = sampleManager.getOriginalSampleRate(index);

    if (sampleRate <= 0.0)
        return 0.0;

    return static_cast<double>(buffer.getNumSamples()) / sampleRate;
}

const juce::AudioBuffer<float>* GliderAudioProcessor::getSampleBufferForDisplay(int index) const
{
    if (!sampleManager.hasSampleAtIndex(index))
        return nullptr;

    return &sampleManager.getSampleBuffer(index);
}

void GliderAudioProcessor::removeSample(int index)
{
    sampleManager.removeSample(index);
}

void GliderAudioProcessor::clearSampleBank()
{
    sampleManager.clearSampleBank();
}

bool GliderAudioProcessor::reloadSampleFromPath()
{
    return sampleManager.reloadSampleFromPath(currentSampleRate);
}

int GliderAudioProcessor::allocateVoice()
{
    int currentVoiceCount = getVoiceCount();
    
    logger.log("allocateVoice() called - VoiceCount=" + juce::String(currentVoiceCount) + 
              ", Voice0 active=" + juce::String(sampleVoices[0].isActive ? "true" : "false"));
    
    // First, try to find an inactive voice (works for both mono and poly)
    for (int i = 0; i < currentVoiceCount && i < MAX_VOICES; ++i)
    {
        if (!sampleVoices[i].isActive)
        {
            logger.log("Found inactive voice " + juce::String(i) + " for allocation");
            return i;
        }
    }
    
    // If no inactive voices, steal the oldest one within the allowed voice count
    int oldestVoice = 0;
    juce::uint64 oldestTime = sampleVoices[0].voiceStartTime;
    
    for (int i = 1; i < currentVoiceCount && i < MAX_VOICES; ++i)
    {
        if (sampleVoices[i].voiceStartTime < oldestTime)
        {
            oldestTime = sampleVoices[i].voiceStartTime;
            oldestVoice = i;
        }
    }
    
    // CROSSFADE DISABLED - immediately deactivate stolen voice
    auto& stolenVoice = sampleVoices[oldestVoice];
    if (stolenVoice.isActive)
    {
        logger.log("Voice " + juce::String(oldestVoice) + " stolen and deactivated");
        stolenVoice.isActive = false;
        stolenVoice.isGliding = false;
    }

    return oldestVoice;
}

void GliderAudioProcessor::startVoice(int voiceIndex, float velocity, float pitch)
{
    if (voiceIndex < 0 || voiceIndex >= MAX_VOICES)
        return;

    auto& voice = sampleVoices[voiceIndex];

    // Debug logging for pitch
    logger.log("startVoice - Input pitch: " + juce::String(pitch) +
              ", Velocity: " + juce::String(velocity));

    // Initialize voice parameters (like vst-test2)
    voice.samplePosition = 0;
    voice.phaseAccumulator = 0.0;  // Initialize phase accumulator for continuous reading
    voice.velocity = juce::jlimit(0.0f, 1.0f, velocity); // Store and clamp velocity
    
    // Get current sample index and apply per-sample transpose (like vst-test2)
    int currentSampleIndex = sampleManager.getCurrentSampleIndex();
    float sampleTranspose = 0.0f; // TODO: Implement getSampleTranspose if needed
    float finalPitch = pitch + sampleTranspose;  // No pitch limit - allow full range

    // Simple voice initialization - no glide logic here
    voice.isGliding = false;
    voice.pitch = finalPitch;
    voice.cachedPitchRatio = 0.0f; // Force recalculation
    
    // Use processed sample buffer if available, otherwise original (like vst-test2)
    int bufferLength = sampleManager.getCurrentSampleBuffer().getNumSamples();
    voice.noteOffCountdown = juce::jmin(bufferLength, static_cast<int>(currentSampleRate * 2.0));
    
    // Initialize voice and reset all glide state
    voice.isActive = true;
    voice.isGliding = false;
    voice.glideCurrentStep = 0;
    voice.glideSampleCounter = 0;
    voice.cachedPitchRatio = 0.0f; // Force recalculation

    // ENVELOPE DISABLED - no envelope initialization needed
    // CROSSFADE DISABLED - no crossfade state initialization needed

    // Debug logging for voice start
    logger.log("Voice " + juce::String(voiceIndex) + " STARTED - Velocity=" + juce::String(velocity) +
              ", Pitch=" + juce::String(finalPitch) +
              ", NoteOffCountdown=" + juce::String(voice.noteOffCountdown));
    
    // Track when this voice started for voice stealing (like vst-test2)
    voice.voiceStartTime = ++voiceAllocationCounter;
}

void GliderAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    // Handle parameter changes if needed
    logger.log("Parameter changed: " + parameterID + " = " + juce::String(newValue));

    // Update ADSR parameters when they change
    if (parameterID == "attack" || parameterID == "decay" ||
        parameterID == "sustain" || parameterID == "release")
    {
        // Update ADSR for voice 0 (monophonic)
        juce::ADSR::Parameters adsrParams;
        adsrParams.attack = getAttack();
        adsrParams.decay = getDecay();
        adsrParams.sustain = getSustain();
        adsrParams.release = getRelease();

        sampleVoices[0].adsr.setParameters(adsrParams);

        logger.log("ADSR updated - A:" + juce::String(adsrParams.attack) +
                  " D:" + juce::String(adsrParams.decay) +
                  " S:" + juce::String(adsrParams.sustain) +
                  " R:" + juce::String(adsrParams.release));
    }
}

void GliderAudioProcessor::updateParametersFromUI()
{
    // Update cached parameter values if needed
}

juce::AudioProcessor::BusesProperties GliderAudioProcessor::getBusesLayout()
{
    return BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true);
}

// This function is required for the standalone version
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GliderAudioProcessor();
}
