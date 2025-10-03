#pragma once

#include <JuceHeader.h>

class ParameterManager
{
public:
    // ADSR Parameter Constants - Shared between host and UI
    static constexpr float ADSR_ATTACK_MIN = 0.01f;      // 0.01 seconds (10ms minimum)
    static constexpr float ADSR_ATTACK_MAX = 1.0f;       // 1.0 seconds (matching original)
    static constexpr float ADSR_ATTACK_DEFAULT = 0.02f;  // 0.02 seconds (20ms default)
    static constexpr float ADSR_ATTACK_INCREMENT = 0.001f;
    
    static constexpr float ADSR_DECAY_MIN = 0.001f;      // 0.001 seconds (1ms, matching original)
    static constexpr float ADSR_DECAY_MAX = 1.0f;        // 1.0 seconds (matching original)
    static constexpr float ADSR_DECAY_DEFAULT = 1.00f;   // 0.05 seconds (matching original)
    static constexpr float ADSR_DECAY_INCREMENT = 0.001f;
    
    static constexpr float ADSR_SUSTAIN_MIN = 0.0f;      // 0.0 (0% volume)
    static constexpr float ADSR_SUSTAIN_MAX = 1.0f;      // 1.0 (100% volume)
    static constexpr float ADSR_SUSTAIN_DEFAULT = 1.0f;  // 0.7 (70% volume, matching original)
    static constexpr float ADSR_SUSTAIN_INCREMENT = 0.001f;
    
    static constexpr float ADSR_RELEASE_MIN = 0.001f;    // 0.001 seconds (1ms)
    static constexpr float ADSR_RELEASE_MAX = 10.0f;     // 10.0 seconds (increased for long samples)
    static constexpr float ADSR_RELEASE_DEFAULT = 4.0f;  // 0.5 seconds (increased from 0.1s)
    static constexpr float ADSR_RELEASE_INCREMENT = 0.001f;

    // Master Gain Parameter Constants
    static constexpr float SAMPLE_GAIN_MIN = -24.0f;
    static constexpr float SAMPLE_GAIN_MAX = 24.0f;
    static constexpr float SAMPLE_GAIN_DEFAULT = -6.0f;
    static constexpr float SAMPLE_GAIN_INCREMENT = 0.1f;
    
    // Voice Count Parameter Constants
    static constexpr float VOICE_COUNT_MIN = 1.0f;
    static constexpr float VOICE_COUNT_MAX = 8.0f;
    static constexpr float VOICE_COUNT_DEFAULT = 1.0f; // Monophonic by default
    static constexpr float VOICE_COUNT_INCREMENT = 1.0f;
    
    // Glide Parameter Constants
    static constexpr float GLIDE_TIME_MIN = 0.0f;        // 0ms = no glide
    static constexpr float GLIDE_TIME_MAX = 1000.0f;     // 1000ms = 1 second maximum
    static constexpr float GLIDE_TIME_DEFAULT = 100.0f;  // Start with 100ms glide
    static constexpr float GLIDE_TIME_INCREMENT = 1.0f;  // 1ms increments
    
    static constexpr float GLIDE_STEPS_MIN = 2.0f;       // Minimum 2 steps for a transition
    static constexpr float GLIDE_STEPS_MAX = 16.0f;      // Maximum 16 steps
    static constexpr float GLIDE_STEPS_DEFAULT = 2.0f;   // Default 2 steps for quick glide
    static constexpr float GLIDE_STEPS_INCREMENT = 1.0f; // 1 step increments

    // Global Transpose Parameter Constants
    static constexpr float TRANSPOSE_MIN = -24.0f;       // -2 octaves
    static constexpr float TRANSPOSE_MAX = 24.0f;        // +2 octaves
    static constexpr float TRANSPOSE_DEFAULT = 0.0f;     // No transpose
    static constexpr float TRANSPOSE_INCREMENT = 1.0f;   // 1 semitone increments

    // Fine Tune (Cents) Parameter Constants
    static constexpr float FINETUNE_MIN = -100.0f;       // -100 cents
    static constexpr float FINETUNE_MAX = 100.0f;        // +100 cents
    static constexpr float FINETUNE_DEFAULT = 0.0f;      // No fine tune
    static constexpr float FINETUNE_INCREMENT = 1.0f;    // 1 cent increments

    ParameterManager(juce::AudioProcessor& processor);
    ~ParameterManager() = default;

    // Create the parameter layout
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    // Get the APVTS instance
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    
    // Parameter access methods
    float getAttack() const;
    float getDecay() const;
    float getSustain() const;
    float getRelease() const;
    float getSampleGain() const;
    int getVoiceCount() const;
    float getGlideTime() const;
    int getGlideSteps() const;
    float getTranspose() const;
    float getFineTune() const;

private:
    juce::AudioProcessorValueTreeState apvts;
};
