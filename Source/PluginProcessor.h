#pragma once

#include <JuceHeader.h>
#include <functional>
#include "PluginLogger.h"
#include "SampleManager.h"
#include "ParameterManager.h"

class GliderAudioProcessor : public juce::AudioProcessor,
                                     private juce::AudioProcessorValueTreeState::Listener
{
public:

    GliderAudioProcessor();
    ~GliderAudioProcessor() override;
    
    // Logger instance
    PluginLogger logger;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& busesLayout) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    bool isSynth() const;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;
    
    // Modern parameter management with APVTS
    juce::AudioProcessorValueTreeState& getAPVTS() { return parameterManager.getAPVTS(); }
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    // Sample loading functionality
    void loadSample(const juce::File& audioFile);
    void loadDefaultSample(double sampleRate);
    bool hasSample() const { return sampleManager.hasSample(); }
    juce::String getSampleName(int index = 0) const { return sampleManager.getSampleName(index); }
    double getOriginalSampleRate() const { return sampleManager.getOriginalSampleRate(); }
    
    // Sample bank management
    int getSampleCount() const { return sampleManager.getSampleCount(); }
    void removeSample(int index);
    void clearSampleBank();
    
    // Per-sample parameter management
    void setSampleGain(int index, float gainDb) { sampleManager.setSampleGain(index, gainDb); }
    float getSampleGain(int index) const { return sampleManager.getSampleGain(index); }
    void setSampleTranspose(int index, float semitones) { sampleManager.setSampleTranspose(index, semitones); }
    float getSampleTranspose(int index) const { return sampleManager.getSampleTranspose(index); }
    
    // Get current sample information
    int getCurrentSampleIndex() const { return sampleManager.getCurrentSampleIndex(); }
    juce::String getCurrentSampleName() const;
    double getSampleDuration(int index = 0) const; // Get duration in seconds
    const juce::AudioBuffer<float>* getSampleBufferForDisplay(int index = 0) const;
    
    // State change callback
    std::function<void()> onStateRestored;
    
    // Set state restored callback
    void setStateRestoredCallback(std::function<void()> callback) { onStateRestored = callback; }
    
    // Try to reload sample from path
    bool reloadSampleFromPath();
    
    // ADSR envelope controls - now use atomic parameter access
    float getAttack() const { return parameterManager.getAttack(); }
    float getDecay() const { return parameterManager.getDecay(); }
    float getSustain() const { return parameterManager.getSustain(); }
    float getRelease() const { return parameterManager.getRelease(); }
    
    // Sample volume gain control - now use atomic parameter access
    float getSampleGain() const { return parameterManager.getSampleGain(); }

    // Voice count control
    int getVoiceCount() const { return parameterManager.getVoiceCount(); }
    
    // Glide controls
    float getGlideTime() const { return parameterManager.getGlideTime(); }
    int getGlideSteps() const { return parameterManager.getGlideSteps(); }
    
    // Get current sample rate
    double getSampleRate() const { return currentSampleRate; }
    

    
    // Voice management methods
    int allocateVoice(); // Returns voice index, with proper voice stealing
    void startVoice(int voiceIndex, float velocity, float pitch = 0.0f); // Start a voice with current sample and velocity

    // AudioProcessorValueTreeState::Listener implementation
    void parameterChanged(const juce::String& parameterID, float newValue) override;

private:
    // Sample-accurate audio rendering
    void renderAudioSegment(juce::AudioBuffer<float>& buffer, int startSample, int endSample);


    // Modern parameter management
    ParameterManager parameterManager;
    
    // Audio synthesis variables
    bool noteIsPlaying = false;
    int noteOffCountdown = 0;
    int noteStartSample = 0; // Track when within the buffer the note should start
    double currentSampleRate = 44100.0; // Store sample rate for AU compatibility
    
    // ADSR envelope state (legacy enum kept for compatibility)
    enum class EnvelopeState { Idle, Attack, Decay, Sustain, Release };
    
    SampleManager sampleManager;
    
    std::atomic<bool> isPluginReady{false};
    
    // Multiple sample voices for overlapping playback
    struct SampleVoice
    {
        int samplePosition = 0;
        int noteOffCountdown = 0;
        EnvelopeState currentEnvelopeState = EnvelopeState::Idle;
        float currentEnvelopeValue = 0.0f;
        int envelopeSampleCounter = 0;
        bool isActive = false;
        juce::uint64 voiceStartTime = 0; // For voice stealing - track when voice started
        float velocity = 1.0f; // Velocity value for this voice (0.0 to 1.0)
        float pitch = 0.0f; // Pitch value for this voice (-12.0 to +12.0 semitones)
        float cachedPitchRatio = 0.0f; // Cache pitch ratio to avoid repeated std::pow() calls
        float releaseMultiplier = 0.0f; // Cached release multiplier for smooth fadeouts
        
        // Voice stealing crossfade
        bool isBeingStolen = false; // Whether this voice is being faded out due to stealing
        float stolenFadeOutValue = 1.0f; // Fadeout multiplier for stolen voices
        int stolenFadeOutSamples = 0; // Counter for stolen voice fadeout
        int stolenFadeOutDuration = 512; // Duration in samples for stolen voice fadeout (~11.6ms at 44.1kHz)
        
        // Glide state for stepped portamento
        bool isGliding = false;           // Whether this voice is currently gliding
        float glideStartPitch = 0.0f;     // Starting pitch for glide
        float glideTargetPitch = 0.0f;    // Target pitch for glide
        int glideCurrentStep = 0;         // Current step in the glide process
        int glideTotalSteps = 0;          // Total number of steps for the glide
        int glideSamplesPerStep = 0;      // Samples per step
        int glideSampleCounter = 0;       // Counter for current step duration

        // Phase continuity for stepped portamento (prevents clicks)
        double phaseAccumulator = 0.0;  // Continuous phase position for sample reading

        // Glide crossfade state (prevents clicks when restarting sample)
        bool isInGlideCrossfade = false;       // Whether voice is crossfading at glide start
        int glideCrossfadeSampleCount = 0;     // Counter for crossfade progress
        double glideOldPhaseAccumulator = 0.0; // Old phase position to crossfade from
        float glideOldPitchRatio = 0.0f;       // Old pitch ratio for old position playback
        static constexpr int GLIDE_CROSSFADE_LENGTH = 256; // ~5.8ms at 44.1kHz (increased from 64)

        // ADSR envelope using JUCE's built-in class
        juce::ADSR adsr;  // Exponential envelope with proper legato support
    };
    
    static constexpr int MAX_VOICES = 64; // Allow up to 64 overlapping samples (like vst-test2)
    std::array<SampleVoice, MAX_VOICES> sampleVoices;
    juce::uint64 voiceAllocationCounter = 0; // For tracking voice allocation order
    
    // All parameters are now managed by APVTS
    
    // Parameter update callback
    void updateParametersFromUI();
    
    // ADSR envelope state (legacy - now handled per voice)
    EnvelopeState currentEnvelopeState = EnvelopeState::Idle;
    float currentEnvelopeValue = 0.0f;
    int envelopeSampleCounter = 0;
    
    // ADSR envelope parameters
    float attackTimeSeconds = 0.01f;      // 10ms default
    float decayTimeSeconds = 0.05f;       // 50ms default
    float sustainLevelNormalized = 0.7f;  // 70% default
    float releaseTimeSeconds = 0.1f;      // 100ms default
    
    // Sample volume gain control
    float sampleGainDb = -6.0f; // Default gain
    
    // Glide state for monophonic mode
    float lastMonophonicPitch = 0.0f;     // Last pitch used in monophonic mode
    bool hasLastPitch = false;            // Whether we have a previous pitch to glide from
    
    // Bus layout configuration
    static juce::AudioProcessor::BusesProperties getBusesLayout();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GliderAudioProcessor)
};


