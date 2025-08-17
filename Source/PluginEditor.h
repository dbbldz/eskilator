#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class HelloWorldVST3AudioProcessorEditor : public juce::AudioProcessorEditor,
                                                    public juce::Button::Listener,
                                                    public juce::FileDragAndDropTarget,
                                                    public juce::Slider::Listener,
                                                    public juce::Timer
{
public:
    HelloWorldVST3AudioProcessorEditor(HelloWorldVST3AudioProcessor&);
    ~HelloWorldVST3AudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void buttonClicked(juce::Button* button) override;
    void timerCallback() override;
    
    // Drag and drop functionality
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    
    // ADSR slider callbacks
    void sliderValueChanged(juce::Slider* slider) override;
    
    // Helper method for setting up sliders
    void setupSlider(juce::Slider& slider, juce::Label& label, const juce::String& text, 
                    float minValue, float maxValue, float defaultValue);
    
    // Update step button visual states to match processor
    void updateStepButtonStates();
    
    // Update the current step indicator
    void updateCurrentStepIndicator();

private:
    HelloWorldVST3AudioProcessor& audioProcessor;
    
    // Simple array of TextButtons
    juce::OwnedArray<juce::TextButton> stepButtons;
    
    // Current step tracking for visual indicator
    int currentStepPosition = -1;
    
    juce::Label titleLabel;
    juce::Label sampleLabel;
    juce::Rectangle<int> dropArea;
    bool isDragOver = false;
    
    // ADSR controls
    juce::Slider attackSlider;
    juce::Slider decaySlider;
    juce::Slider sustainSlider;
    juce::Slider releaseSlider;
    juce::Label attackLabel;
    juce::Label decayLabel;
    juce::Label sustainLabel;
    juce::Label releaseLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HelloWorldVST3AudioProcessorEditor)
};
