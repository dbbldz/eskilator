#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "StyleSheet.h"
#include <iostream>

// Plugin dimensions constants
const int PLUGIN_WIDTH = 500;
const int PLUGIN_HEIGHT = 555;

PluginEditor::PluginEditor(GliderAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), logger(p.logger)
{    
    // Set up the editor size
    setSize(PLUGIN_WIDTH, PLUGIN_HEIGHT);
    
    // Create sample bank component FIRST (before any JUCE automatic calls)
    sampleBankComponent = std::make_unique<SampleBankComponent>(audioProcessor);
    sampleBankComponent->setSampleRemovedCallback([this](int index) {
        audioProcessor.removeSample(index);
        sampleBankComponent->updateSampleList();
    });
    sampleBankComponent->setSampleCountChangedCallback([this]() {
        // Update UI if needed
    });
    
    // Force the component to be visible and on top
    addAndMakeVisible(sampleBankComponent.get());
    sampleBankComponent->setAlwaysOnTop(true);
    sampleBankComponent->toFront(true);
    
    // Debug: Log that component was created
    logger.log("SampleBankComponent created and added to editor FIRST");
    
    // Uniform color for all controls and borders
    auto uniformGreen = juce::Colour(0xff5af542);

    // Configure plugin title label
    titleLabel.setText("ESKILATOR", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(24.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    titleLabel.setColour(juce::Label::textColourId, uniformGreen);
    addAndMakeVisible(titleLabel);

    // Configure and add GroupComponents (so they render behind controls)
    adsrGroup.setText("ADSR Envelope");
    adsrGroup.setColour(juce::GroupComponent::outlineColourId, uniformGreen);
    adsrGroup.setColour(juce::GroupComponent::textColourId, juce::Colours::white);
    addAndMakeVisible(adsrGroup);

    sampleGroup.setText("Controls");
    sampleGroup.setColour(juce::GroupComponent::outlineColourId, uniformGreen);
    sampleGroup.setColour(juce::GroupComponent::textColourId, juce::Colours::white);
    addAndMakeVisible(sampleGroup);

    // Glide group removed - controls now in sample/controls group

    sampleViewerGroup.setText("Sample");
    sampleViewerGroup.setColour(juce::GroupComponent::outlineColourId, uniformGreen);
    sampleViewerGroup.setColour(juce::GroupComponent::textColourId, juce::Colours::white);
    addAndMakeVisible(sampleViewerGroup);

    // Configure ADSR sliders
    attackSlider.setSliderStyle(juce::Slider::LinearBarVertical);
    attackSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 20);
    attackSlider.setRange(ParameterManager::ADSR_ATTACK_MIN, ParameterManager::ADSR_ATTACK_MAX, ParameterManager::ADSR_ATTACK_INCREMENT);
    attackSlider.setValue(ParameterManager::ADSR_ATTACK_DEFAULT);
    attackSlider.setColour(juce::Slider::trackColourId, uniformGreen);
    attackSlider.setColour(juce::Slider::backgroundColourId, juce::Colours::darkgrey);
    addAndMakeVisible(attackSlider);

    attackLabel.setText("Attack", juce::dontSendNotification);
    attackLabel.setFont(juce::Font(12.0f));
    attackLabel.setJustificationType(juce::Justification::centred);
    attackLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(attackLabel);

    decaySlider.setSliderStyle(juce::Slider::LinearBarVertical);
    decaySlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 20);
    decaySlider.setRange(ParameterManager::ADSR_DECAY_MIN, ParameterManager::ADSR_DECAY_MAX, ParameterManager::ADSR_DECAY_INCREMENT);
    decaySlider.setValue(ParameterManager::ADSR_DECAY_DEFAULT);
    decaySlider.setColour(juce::Slider::trackColourId, uniformGreen);
    decaySlider.setColour(juce::Slider::backgroundColourId, juce::Colours::darkgrey);
    addAndMakeVisible(decaySlider);

    decayLabel.setText("Decay", juce::dontSendNotification);
    decayLabel.setFont(juce::Font(12.0f));
    decayLabel.setJustificationType(juce::Justification::centred);
    decayLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(decayLabel);

    sustainSlider.setSliderStyle(juce::Slider::LinearBarVertical);
    sustainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 20);
    sustainSlider.setRange(ParameterManager::ADSR_SUSTAIN_MIN, ParameterManager::ADSR_SUSTAIN_MAX, ParameterManager::ADSR_SUSTAIN_INCREMENT);
    sustainSlider.setValue(ParameterManager::ADSR_SUSTAIN_DEFAULT);
    sustainSlider.setColour(juce::Slider::trackColourId, uniformGreen);
    sustainSlider.setColour(juce::Slider::backgroundColourId, juce::Colours::darkgrey);
    addAndMakeVisible(sustainSlider);

    sustainLabel.setText("Sustain", juce::dontSendNotification);
    sustainLabel.setFont(juce::Font(12.0f));
    sustainLabel.setJustificationType(juce::Justification::centred);
    sustainLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(sustainLabel);

    releaseSlider.setSliderStyle(juce::Slider::LinearBarVertical);
    releaseSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 20);
    releaseSlider.setRange(ParameterManager::ADSR_RELEASE_MIN, ParameterManager::ADSR_RELEASE_MAX, ParameterManager::ADSR_RELEASE_INCREMENT);
    releaseSlider.setValue(ParameterManager::ADSR_RELEASE_DEFAULT);
    releaseSlider.setColour(juce::Slider::trackColourId, uniformGreen);
    releaseSlider.setColour(juce::Slider::backgroundColourId, juce::Colours::darkgrey);
    addAndMakeVisible(releaseSlider);

    releaseLabel.setText("Release", juce::dontSendNotification);
    releaseLabel.setFont(juce::Font(12.0f));
    releaseLabel.setJustificationType(juce::Justification::centred);
    releaseLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(releaseLabel);
    
    // Configure sample gain slider
    sampleGainSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    sampleGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    sampleGainSlider.setRange(ParameterManager::SAMPLE_GAIN_MIN, ParameterManager::SAMPLE_GAIN_MAX, ParameterManager::SAMPLE_GAIN_INCREMENT);
    sampleGainSlider.setValue(ParameterManager::SAMPLE_GAIN_DEFAULT);
    sampleGainSlider.setTextValueSuffix(" dB");
    sampleGainSlider.setColour(juce::Slider::thumbColourId, uniformGreen);
    sampleGainSlider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::darkgrey);
    sampleGainSlider.setColour(juce::Slider::rotarySliderFillColourId, uniformGreen);
    addAndMakeVisible(sampleGainSlider);

    sampleGainLabel.setText("Master Gain", juce::dontSendNotification);
    sampleGainLabel.setFont(juce::Font(12.0f));
    sampleGainLabel.setJustificationType(juce::Justification::centred);
    sampleGainLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(sampleGainLabel);

    // Configure glide time slider
    glideTimeSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    glideTimeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    glideTimeSlider.setRange(ParameterManager::GLIDE_TIME_MIN, ParameterManager::GLIDE_TIME_MAX, ParameterManager::GLIDE_TIME_INCREMENT);
    glideTimeSlider.setValue(ParameterManager::GLIDE_TIME_DEFAULT);
    glideTimeSlider.setTextValueSuffix("ms");
    glideTimeSlider.setColour(juce::Slider::thumbColourId, uniformGreen);
    glideTimeSlider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::darkgrey);
    glideTimeSlider.setColour(juce::Slider::rotarySliderFillColourId, uniformGreen);
    addAndMakeVisible(glideTimeSlider);

    glideTimeLabel.setText("Glide Time", juce::dontSendNotification);
    glideTimeLabel.setFont(juce::Font(12.0f));
    glideTimeLabel.setJustificationType(juce::Justification::centred);
    glideTimeLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(glideTimeLabel);

    // Configure glide steps slider
    glideStepsSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    glideStepsSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    glideStepsSlider.setRange(ParameterManager::GLIDE_STEPS_MIN, ParameterManager::GLIDE_STEPS_MAX, ParameterManager::GLIDE_STEPS_INCREMENT);
    glideStepsSlider.setValue(ParameterManager::GLIDE_STEPS_DEFAULT);
    glideStepsSlider.setTextValueSuffix(" Steps");
    glideStepsSlider.setColour(juce::Slider::thumbColourId, uniformGreen);
    glideStepsSlider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::darkgrey);
    glideStepsSlider.setColour(juce::Slider::rotarySliderFillColourId, uniformGreen);
    addAndMakeVisible(glideStepsSlider);

    glideStepsLabel.setText("Glide Steps", juce::dontSendNotification);
    glideStepsLabel.setFont(juce::Font(12.0f));
    glideStepsLabel.setJustificationType(juce::Justification::centred);
    glideStepsLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(glideStepsLabel);

    // Create parameter attachments
    auto& apvts = audioProcessor.getAPVTS();
    attackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "attack", attackSlider);
    decayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "decay", decaySlider);
    sustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "sustain", sustainSlider);
    releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "release", releaseSlider);
    sampleGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "sampleGain", sampleGainSlider);
    glideTimeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "glideTime", glideTimeSlider);
    glideStepsAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "glideSteps", glideStepsSlider);
    
    // Now that all components are created, manually trigger the layout
    logger.log("All components created, manually calling resized()");
    resized();
}

PluginEditor::~PluginEditor()
{
    // Explicitly destroy parameter attachments before base class destructor
    // This prevents crashes when attachments try to access destroyed sliders
    attackAttachment.reset();
    decayAttachment.reset();
    sustainAttachment.reset();
    releaseAttachment.reset();
    sampleGainAttachment.reset();
    glideTimeAttachment.reset();
    glideStepsAttachment.reset();
}

void PluginEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
}

void PluginEditor::resized()
{
    // Debug: Log that resized is being called
    logger.log("PluginEditor::resized() called");

    auto bounds = getLocalBounds();
    int margin = 20; // Uniform margin between sections
    int knobSize = 80;
    int labelHeight = 20;
    int groupHeight = 140; // Reduced height for more compact layout

    // Apply outer margins (top and sides)
    bounds.removeFromTop(margin);
    bounds.removeFromLeft(margin);
    bounds.removeFromRight(margin);
    bounds.removeFromBottom(margin);

    // Position title label in top left corner
    auto titleArea = bounds.removeFromTop(30);
    titleLabel.setBounds(titleArea.removeFromLeft(200));
    bounds.removeFromTop(10); // Small spacing after title

    // Sample viewer section at top (fixed height)
    int sampleViewerHeight = 150;
    auto sampleViewerArea = bounds.removeFromTop(sampleViewerHeight);
    bounds.removeFromTop(10); // Reduced margin after sample viewer

    // Now allocate the remaining space for control groups
    auto controlsArea = bounds.removeFromTop(groupHeight);
    bounds.removeFromTop(10); // Reduced margin after controls

    auto adsrArea = bounds; // ADSR fills remaining space

    // Set group bounds
    sampleViewerGroup.setBounds(sampleViewerArea);
    sampleGroup.setBounds(controlsArea);
    adsrGroup.setBounds(adsrArea);

    // Controls Group: Sample gain + Glide controls in a single row
    auto controlsContentArea = controlsArea.reduced(10);
    controlsContentArea.removeFromTop(15); // Top margin for group title

    int controlSpacing = 20;

    // Sample gain knob
    auto sampleGainArea = controlsContentArea.removeFromLeft(knobSize + controlSpacing);
    sampleGainSlider.setBounds(sampleGainArea.removeFromTop(knobSize));
    sampleGainLabel.setBounds(sampleGainSlider.getX(), sampleGainSlider.getBottom() + 5, knobSize, labelHeight);

    // Glide Time knob
    auto glideTimeArea = controlsContentArea.removeFromLeft(knobSize + controlSpacing);
    glideTimeSlider.setBounds(glideTimeArea.removeFromTop(knobSize));
    glideTimeLabel.setBounds(glideTimeSlider.getX(), glideTimeSlider.getBottom() + 5, knobSize, labelHeight);

    // Glide Steps knob
    auto glideStepsArea = controlsContentArea.removeFromLeft(knobSize + controlSpacing);
    glideStepsSlider.setBounds(glideStepsArea.removeFromTop(knobSize));
    glideStepsLabel.setBounds(glideStepsSlider.getX(), glideStepsSlider.getBottom() + 5, knobSize, labelHeight);
    
    // ADSR Group controls
    auto adsrControlsArea = adsrArea.reduced(10);
    adsrControlsArea.removeFromTop(15); // Top margin for group title

    // ADSR controls in a single horizontal row - ensure enough space for sliders and labels
    auto adsrRow = adsrControlsArea; // Use all available space
    int adsrBarWidth = 40; // Bar width for ADSR controls - wide enough for text
    int adsrSliderSpacing = 15; // Tight spacing between ADSR sliders

    // Ensure each slider gets moderate height for better control with lots of padding
    auto attackArea = adsrRow.removeFromLeft(adsrBarWidth + adsrSliderSpacing);
    attackSlider.setBounds(attackArea.removeFromTop(knobSize * 1.2).withWidth(adsrBarWidth));
    attackLabel.setBounds(attackSlider.getX(), attackSlider.getBottom() + 15, adsrBarWidth, labelHeight);

    auto decayArea = adsrRow.removeFromLeft(adsrBarWidth + adsrSliderSpacing);
    decaySlider.setBounds(decayArea.removeFromTop(knobSize * 1.2).withWidth(adsrBarWidth));
    decayLabel.setBounds(decaySlider.getX(), decaySlider.getBottom() + 15, adsrBarWidth, labelHeight);

    auto sustainArea = adsrRow.removeFromLeft(adsrBarWidth + adsrSliderSpacing);
    sustainSlider.setBounds(sustainArea.removeFromTop(knobSize * 1.2).withWidth(adsrBarWidth));
    sustainLabel.setBounds(sustainSlider.getX(), sustainSlider.getBottom() + 15, adsrBarWidth, labelHeight);

    auto releaseArea = adsrRow.removeFromLeft(adsrBarWidth + adsrSliderSpacing);
    releaseSlider.setBounds(releaseArea.removeFromTop(knobSize * 1.2).withWidth(adsrBarWidth));
    releaseLabel.setBounds(releaseSlider.getX(), releaseSlider.getBottom() + 15, adsrBarWidth, labelHeight);

    // Sample Viewer Component content
    if (sampleBankComponent) {
        // Padding inside group matching other sections (reduced by 10 for border spacing)
        auto sampleContentArea = sampleViewerArea.reduced(10);
        sampleContentArea.removeFromTop(15); // Top margin for group title
        sampleBankComponent->setBounds(sampleContentArea);
    }
    

}

// SampleBankComponent implementation
SampleBankComponent::SampleBankComponent(GliderAudioProcessor& processor)
    : audioProcessor(processor),
      isDragOver(false),
      thumbnailCache(5),
      thumbnail(512, formatManager, thumbnailCache)
{
    setOpaque(true);
    formatManager.registerBasicFormats();
    thumbnail.addChangeListener(this);
    updateSampleList();
}

SampleBankComponent::~SampleBankComponent()
{
    thumbnail.removeChangeListener(this);
}

void SampleBankComponent::paint(juce::Graphics& g)
{
    // Match app background (black)
    g.fillAll(juce::Colours::black);

    // Show current sample or drag prompt
    if (audioProcessor.hasSample())
    {
        auto bounds = getLocalBounds().reduced(10);

        // Uniform green color
        auto uniformGreen = juce::Colour(0xff5af542);

        // Display sample name
        juce::String sampleName = audioProcessor.getSampleName(0);
        g.setColour(uniformGreen);
        g.setFont(14.0f);
        auto nameArea = bounds.removeFromTop(20);
        g.drawText(sampleName, nameArea, juce::Justification::centredLeft);

        // Display duration
        double duration = audioProcessor.getSampleDuration(0);
        juce::String durationText = juce::String(duration, 2) + "s";
        g.setColour(juce::Colours::lightgrey);
        g.setFont(12.0f);
        auto durationArea = bounds.removeFromTop(18);
        g.drawText(durationText, durationArea, juce::Justification::centredLeft);

        // Draw waveform if available
        if (thumbnail.getNumChannels() > 0)
        {
            bounds.removeFromTop(5); // Small spacing
            auto thumbnailBounds = bounds.removeFromTop(bounds.getHeight() - 15); // Leave room for hint

            // Background for waveform
            g.setColour(juce::Colours::black);
            g.fillRect(thumbnailBounds);

            // Draw waveform as mono (just channel 0 for cleaner display)
            g.setColour(uniformGreen.withAlpha(0.8f));
            thumbnail.drawChannel(g, thumbnailBounds, 0.0, thumbnail.getTotalLength(), 0, 1.0f);
        }

        // Hint text at bottom
        g.setColour(juce::Colours::white);
        g.setFont(11.0f);
        g.drawText("Drag a new file to replace", bounds, juce::Justification::centredLeft);
    }
    else
    {
        // Empty state
        g.setColour(juce::Colours::white.withAlpha(0.7f));
        g.setFont(16.0f);
        g.drawText("Drag & Drop Audio File Here", getLocalBounds(), juce::Justification::centred);

        g.setColour(juce::Colours::lightgrey.withAlpha(0.5f));
        g.setFont(11.0f);
        g.drawText("WAV, AIFF, MP3, FLAC, OGG, M4A",
                   getLocalBounds().translated(0, 25),
                   juce::Justification::centred);
    }

    // Drag over highlight
    if (isDragOver)
    {
        g.setColour(juce::Colour(0xff00d9ff).withAlpha(0.2f));
        g.fillAll();
        g.setColour(juce::Colour(0xff00d9ff));
        g.drawRect(getLocalBounds(), 3);
    }
}

void SampleBankComponent::resized()
{
    // No controls to layout - just display sample name in paint()
}

void SampleBankComponent::updateSampleList()
{
    // Update waveform thumbnail when sample changes
    if (audioProcessor.hasSample())
    {
        const auto* buffer = audioProcessor.getSampleBufferForDisplay(0);
        if (buffer != nullptr)
        {
            double sampleRate = audioProcessor.getOriginalSampleRate();
            thumbnail.reset(buffer->getNumChannels(), sampleRate);
            thumbnail.addBlock(0, *buffer, 0, buffer->getNumSamples());
        }
    }
    else
    {
        thumbnail.clear();
    }

    // Repaint to show current sample name and waveform
    repaint();
}

void SampleBankComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &thumbnail)
    {
        // Thumbnail has finished loading, trigger repaint
        repaint();
    }
}

bool SampleBankComponent::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (const auto& file : files)
    {
        if (juce::File(file).hasFileExtension("wav;aiff;mp3;flac;ogg;m4a"))
            return true;
    }
    return false;
}

void SampleBankComponent::fileDragEnter(const juce::StringArray& files, int x, int y)
{
    juce::ignoreUnused(x, y);
    isDragOver = true;
    repaint();
}

void SampleBankComponent::fileDragExit(const juce::StringArray& files)
{
    juce::ignoreUnused(files);
    isDragOver = false;
    repaint();
}

void SampleBankComponent::filesDropped(const juce::StringArray& files, int x, int y)
{
    juce::ignoreUnused(x, y);
    isDragOver = false;

    // Clear existing sample bank (single sample mode)
    audioProcessor.clearSampleBank();

    // Load the first audio file found
    for (const auto& filePath : files)
    {
        juce::File audioFile(filePath);
        if (audioFile.hasFileExtension("wav;aiff;mp3;flac;ogg;m4a"))
        {
            audioProcessor.loadSample(audioFile);
            break; // Only load one sample
        }
    }

    repaint();
}

// SampleControl - simplified for single sample mode (no individual controls)
SampleBankComponent::SampleControl::SampleControl()
{
}

void SampleBankComponent::SampleControl::resized()
{
}

void SampleBankComponent::sliderValueChanged(juce::Slider* slider)
{
    juce::ignoreUnused(slider);
}

void SampleBankComponent::buttonClicked(juce::Button* button)
{
    juce::ignoreUnused(button);
}
