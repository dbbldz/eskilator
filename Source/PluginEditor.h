#pragma once

#include <JuceHeader.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "PluginLogger.h"
#include "StyleSheet.h"

// Simple test component for debugging
class TestComponent : public juce::Component
{
public:
    TestComponent() { setOpaque(true); }
    ~TestComponent() override = default;
    
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colours::magenta); // Bright magenta for testing
        g.setColour(juce::Colours::white);
        g.drawText("TEST COMPONENT", getLocalBounds(), juce::Justification::centred);
    }
    
    void resized() override {}
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestComponent)
};

// Component for displaying and managing the sample bank
class SampleBankComponent : public juce::Component, public juce::FileDragAndDropTarget, public juce::Slider::Listener, public juce::Button::Listener, public juce::ChangeListener
{
public:
    SampleBankComponent(GliderAudioProcessor& processor);
    ~SampleBankComponent() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Update the display when samples change
    void updateSampleList();
    
    // Set callback for when samples are removed  
    void setSampleRemovedCallback(std::function<void(int)> callback) { onSampleRemoved = callback; }
    
    // Set callback for when sample count changes
    void setSampleCountChangedCallback(std::function<void()> callback) { onSampleCountChanged = callback; }
    
    // Drag and drop methods
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    
    // Slider and button listeners
    void sliderValueChanged(juce::Slider* slider) override;
    void buttonClicked(juce::Button* button) override;

    // ChangeListener for thumbnail updates
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    
private:
    GliderAudioProcessor& audioProcessor;
    std::function<void(int)> onSampleRemoved;
    std::function<void()> onSampleCountChanged;

    // Waveform display
    juce::AudioFormatManager formatManager;
    juce::AudioThumbnailCache thumbnailCache;
    juce::AudioThumbnail thumbnail;
    
    // Individual sample controls
    struct SampleControl : public juce::Component
    {
        juce::TextButton nameButton;
        juce::Slider gainKnob;
        juce::Label gainLabel;
        juce::Slider transposeKnob;
        juce::Label transposeLabel;
        juce::TextButton removeButton;
        int sampleIndex = -1;
        
        SampleControl();
        void resized() override;
    };
    
    juce::OwnedArray<SampleControl> sampleControls;
    
    // Group component for border
    juce::GroupComponent samplesGroup;
    
    // Drag and drop state
    bool isDragOver;
    
    // Helper methods
    void createSampleControl(int index);
    void removeSampleControl(int index);
    void updateSampleControl(int index);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleBankComponent)
};

class PluginEditor : public juce::AudioProcessorEditor
{
public:
    PluginEditor(GliderAudioProcessor&);
    ~PluginEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    GliderAudioProcessor& audioProcessor;
    PluginLogger& logger;

    // Plugin title label
    juce::Label titleLabel;

    // Test component for debugging
    std::unique_ptr<juce::Component> testComponent;
    
    // Parameter attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> decayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sustainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sampleGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> glideTimeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> glideStepsAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> transposeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fineTuneAttachment;
    
    // ADSR controls
    juce::Slider attackSlider;
    juce::Slider decaySlider;
    juce::Slider sustainSlider;
    juce::Slider releaseSlider;
    juce::Label attackLabel;
    juce::Label decayLabel;
    juce::Label sustainLabel;
    juce::Label releaseLabel;
    
    // Sample gain control
    juce::Slider sampleGainSlider;
    juce::Label sampleGainLabel;

    // Glide controls
    juce::Slider glideTimeSlider;
    juce::Label glideTimeLabel;
    juce::Slider glideStepsSlider;
    juce::Label glideStepsLabel;

    // Transpose control
    juce::Slider transposeSlider;
    juce::Label transposeLabel;

    // Fine tune control
    juce::Slider fineTuneSlider;
    juce::Label fineTuneLabel;

    // Sample bank component
    std::unique_ptr<SampleBankComponent> sampleBankComponent;
    
    // Groups
    juce::GroupComponent adsrGroup;
    juce::GroupComponent sampleGroup;
    juce::GroupComponent glideGroup;
    juce::GroupComponent sampleViewerGroup;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
