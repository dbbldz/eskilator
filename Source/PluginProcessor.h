#pragma once

#include <JuceHeader.h>

class HelloWorldVST3AudioProcessor : public juce::AudioProcessor
{
public:
    HelloWorldVST3AudioProcessor();
    ~HelloWorldVST3AudioProcessor() override;

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
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;
    
    // Step sequencer functionality
    void setStepActive(int stepIndex, bool isActive);
    bool isStepActive(int stepIndex) const;
    
    // Sample loading functionality
    void loadSample(const juce::File& audioFile);
    void loadDefaultClickSample();
    bool hasSample() const { return sampleBuffer.getNumSamples() > 0; }
    juce::String getSampleName() const { return currentSampleName; }
    
    // ADSR envelope controls
    void setAttack(float attackTime) { attackTimeSeconds = attackTime; }
    void setDecay(float decayTime) { decayTimeSeconds = decayTime; }
    void setSustain(float sustainLevel) { sustainLevelNormalized = sustainLevel; }
    void setRelease(float releaseTime) { releaseTimeSeconds = releaseTime; }
    
    float getAttack() const { return attackTimeSeconds; }
    float getDecay() const { return decayTimeSeconds; }
    float getSustain() const { return sustainLevelNormalized; }
    float getRelease() const { return releaseTimeSeconds; }
    
    // Step sequencer timing information
    int getLastTriggeredStep() const { return lastTriggeredStep; }
    double getLastTriggeredStepTime() const { return lastTriggeredStepTime; }

private:
    // Precise timing system
    struct TimingEvent
    {
        int sampleOffset;      // Sample offset within the buffer
        int stepNumber;        // Which step to trigger
        double preciseTime;    // Precise time in samples (can be fractional)
        bool isProcessed;      // Whether this event has been processed
    };
    
    // Timing queue for sample-accurate scheduling
    std::vector<TimingEvent> timingQueue;
    double lastProcessedTime = 0.0;
    double currentBufferStartTime = 0.0;
    
    // Global timing state to prevent duplicates and track timing across buffers
    double lastTriggeredStepTime = -1.0;
    int lastTriggeredStep = -1;
    double lastTriggeredStepPPQ = -1.0;  // Track PPQ position for host sync
    double globalTimingOffset = 0.0;
    
    // Host timing state
    double lastHostTime = 0.0;
    double hostTimeOffset = 0.0;
    bool hostTimingValid = false;
    double lastBPM = 0.0;
    
    // MIDI Clock synchronization variables
    int midiClockCounter = 0;       // Counts MIDI clock pulses (24 per quarter note)
    bool isTransportRunning = false;
    bool shouldGenerateBeat = false;
    int lastBeatClock = -1;
    
    // Audio synthesis variables
    bool noteIsPlaying = false;
    int noteOffCountdown = 0;
    int noteStartSample = 0; // Track when within the buffer the note should start
    double currentSampleRate = 44100.0; // Store sample rate for AU compatibility
    
    // ADSR envelope state
    enum class EnvelopeState { Idle, Attack, Decay, Sustain, Release };
    
    // Step sequencer state
    std::array<bool, 16> stepActive = {{false, false, false, false, false, false, false, false,
                                       false, false, false, false, false, false, false, false}}; // All steps disabled by default
    
    // Sample playback
    juce::AudioBuffer<float> sampleBuffer;
    int samplePosition = 0;
    juce::String currentSampleName;
    
    // Multiple sample voices for overlapping playback
    struct SampleVoice
    {
        int samplePosition = 0;
        int noteOffCountdown = 0;
        EnvelopeState currentEnvelopeState = EnvelopeState::Idle;
        float currentEnvelopeValue = 0.0f;
        int envelopeSampleCounter = 0;
        bool isActive = false;
    };
    
    static constexpr int MAX_VOICES = 8; // Allow up to 8 overlapping samples
    std::array<SampleVoice, MAX_VOICES> sampleVoices;
    
    // ADSR envelope parameters
    float attackTimeSeconds = 0.01f;      // 10ms default
    float decayTimeSeconds = 0.05f;       // 50ms default
    float sustainLevelNormalized = 0.7f;  // 70% default
    float releaseTimeSeconds = 0.1f;      // 100ms default
    
    // Timing precision configuration
    int timingPrecisionSamples = 1;       // Sample-accurate timing by default
    
    // ADSR envelope state (legacy - now handled per voice)
    EnvelopeState currentEnvelopeState = EnvelopeState::Idle;
    float currentEnvelopeValue = 0.0f;
    int envelopeSampleCounter = 0;
    
    // Private timing methods
    void updateHostTiming(const juce::AudioPlayHead::CurrentPositionInfo& posInfo);
    void scheduleTimingEvents(double bufferStartTime, double bufferDuration, double bpm, int actualBufferSamples);
    void processTimingEvents(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);
    double getPreciseTimeFromHost(const juce::AudioPlayHead::CurrentPositionInfo& posInfo);
    void clearTimingQueue();
    
    // Timing configuration
    void setTimingPrecision(int samples) { timingPrecisionSamples = samples; }
    int getTimingPrecision() const { return timingPrecisionSamples; }
    void calibrateTiming(double bpm);
    
    // Debug timing information
    int getQueuedEventsCount() const { return static_cast<int>(timingQueue.size()); }
    double getLastHostTime() const { return lastHostTime; }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HelloWorldVST3AudioProcessor)
};
