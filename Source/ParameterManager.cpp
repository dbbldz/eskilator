#include "ParameterManager.h"

ParameterManager::ParameterManager(juce::AudioProcessor& processor)
    : apvts(processor, nullptr, "Parameters", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout ParameterManager::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;
    
    // ADSR parameters using shared constants
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        "attack", "Attack", 
        ADSR_ATTACK_MIN, 
        ADSR_ATTACK_MAX, 
        ADSR_ATTACK_DEFAULT));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        "decay", "Decay", 
        ADSR_DECAY_MIN, 
        ADSR_DECAY_MAX, 
        ADSR_DECAY_DEFAULT));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        "sustain", "Sustain", 
        ADSR_SUSTAIN_MIN, 
        ADSR_SUSTAIN_MAX, 
        ADSR_SUSTAIN_DEFAULT));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        "release", "Release", 
        ADSR_RELEASE_MIN, 
        ADSR_RELEASE_MAX, 
        ADSR_RELEASE_DEFAULT));
    
    // Sample gain parameter using shared constants
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        "sampleGain", "Master Gain", 
        SAMPLE_GAIN_MIN, 
        SAMPLE_GAIN_MAX, 
        SAMPLE_GAIN_DEFAULT));
    
    // Voice count parameter using shared constants
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        "voiceCount", "Voice Count", 
        VOICE_COUNT_MIN, 
        VOICE_COUNT_MAX, 
        VOICE_COUNT_DEFAULT));
    
    // Glide parameter using shared constants
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        "glideTime", "Glide Time",
        GLIDE_TIME_MIN,
        GLIDE_TIME_MAX,
        GLIDE_TIME_DEFAULT));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        "glideSteps", "Glide Steps",
        GLIDE_STEPS_MIN,
        GLIDE_STEPS_MAX,
        GLIDE_STEPS_DEFAULT));

    // Global transpose parameter using shared constants with discrete steps
    juce::NormalisableRange<float> transposeRange(TRANSPOSE_MIN, TRANSPOSE_MAX, TRANSPOSE_INCREMENT);
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        "transpose", "Transpose",
        transposeRange,
        TRANSPOSE_DEFAULT));

    // Fine tune (cents) parameter using shared constants
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        "finetune", "Fine Tune",
        FINETUNE_MIN,
        FINETUNE_MAX,
        FINETUNE_DEFAULT));

    return { parameters.begin(), parameters.end() };
}

float ParameterManager::getAttack() const
{
    if (auto* param = apvts.getParameter("attack")) 
        return param->getValue(); 
    return ADSR_ATTACK_DEFAULT; 
}

float ParameterManager::getDecay() const
{
    if (auto* param = apvts.getParameter("decay")) 
        return param->getValue(); 
    return ADSR_DECAY_DEFAULT; 
}

float ParameterManager::getSustain() const
{
    if (auto* param = apvts.getParameter("sustain")) 
        return param->getValue(); 
    return ADSR_SUSTAIN_DEFAULT; 
}

float ParameterManager::getRelease() const
{
    if (auto* param = apvts.getParameter("release")) 
        return param->getValue(); 
    return ADSR_RELEASE_DEFAULT; 
}

float ParameterManager::getSampleGain() const
{
    if (auto* param = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("sampleGain")))
        return param->get();
    return SAMPLE_GAIN_DEFAULT; 
}

int ParameterManager::getVoiceCount() const
{
    if (auto* param = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("voiceCount"))) {
        // AudioParameterFloat already returns the actual value in the range, not normalized
        float rawValue = param->get();
        return static_cast<int>(rawValue);
    }
    return static_cast<int>(VOICE_COUNT_DEFAULT); 
}

float ParameterManager::getGlideTime() const
{
    if (auto* param = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("glideTime"))) {
        return param->get();
    }
    return GLIDE_TIME_DEFAULT; 
}

int ParameterManager::getGlideSteps() const
{
    if (auto* param = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("glideSteps"))) {
        return static_cast<int>(param->get());
    }
    return static_cast<int>(GLIDE_STEPS_DEFAULT);
}

float ParameterManager::getTranspose() const
{
    if (auto* param = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("transpose"))) {
        return param->get();
    }
    return TRANSPOSE_DEFAULT;
}

float ParameterManager::getFineTune() const
{
    if (auto* param = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("finetune"))) {
        return param->get();
    }
    return FINETUNE_DEFAULT;
}
