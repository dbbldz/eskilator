#include "PluginProcessor.h"
#include "PluginEditor.h"

HelloWorldVST3AudioProcessorEditor::HelloWorldVST3AudioProcessorEditor(HelloWorldVST3AudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // Configure the title label  
    titleLabel.setText("16-Step Sequencer", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(16.0f));
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(titleLabel);
    
    // Configure sample label
    sampleLabel.setText("Drag audio file here", juce::dontSendNotification);
    sampleLabel.setFont(juce::Font(12.0f));
    sampleLabel.setJustificationType(juce::Justification::centred);
    sampleLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(sampleLabel);
    
    // Configure ADSR sliders with more practical ranges
    setupSlider(attackSlider, attackLabel, "Attack", 0.0f, 0.25f, 0.01f);       // 0ms to 250ms
    setupSlider(decaySlider, decayLabel, "Decay", 0.001f, 0.5f, 0.05f);         // 1ms to 500ms
    setupSlider(sustainSlider, sustainLabel, "Sustain", 0.0f, 1.0f, 0.7f);      // 0% to 100% (keep this range)
    setupSlider(releaseSlider, releaseLabel, "Release", 0.001f, 1.0f, 0.1f);    // 1ms to 1 second
    
    // Create 16 simple TextButtons FIRST
    for (int i = 0; i < 16; ++i)
    {
        auto* button = new juce::TextButton();
        button->setButtonText(juce::String(i + 1));
        button->addListener(this);
        button->setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);
        button->setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        addAndMakeVisible(button);
        stepButtons.add(button);
    }
    
    // Set size LAST - this triggers resized() which needs buttons to exist
    setSize(800, 300);
    
    // Update UI to reflect the processor's default state
    updateStepButtonStates();
    
    // Start timer to update the current step indicator
    startTimerHz(30); // Update 30 times per second for smooth animation
}

HelloWorldVST3AudioProcessorEditor::~HelloWorldVST3AudioProcessorEditor()
{
    // Stop the timer
    stopTimer();
    // Remove listeners to prevent crashes
    for (int i = 0; i < stepButtons.size(); ++i)
    {
        if (stepButtons[i] != nullptr)
        {
            stepButtons[i]->removeListener(this);
        }
    }
}

void HelloWorldVST3AudioProcessorEditor::paint(juce::Graphics& g)
{
    // Fill background
    g.fillAll(juce::Colour(0xff1e1e1e));
    
    // Draw drop area
    if (!dropArea.isEmpty())
    {
        if (isDragOver)
        {
            g.setColour(juce::Colours::orange.withAlpha(0.3f));
            g.fillRect(dropArea);
        }
        
        g.setColour(isDragOver ? juce::Colours::orange : juce::Colours::grey);
        g.drawRect(dropArea, 2);
    }
}

void HelloWorldVST3AudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    
    // Title at the top
    auto titleArea = area.removeFromTop(30);
    titleLabel.setBounds(titleArea);
    
    // Sample info area
    auto sampleArea = area.removeFromTop(30);
    sampleLabel.setBounds(sampleArea);
    dropArea = sampleArea.reduced(5); // Store drop area for drag feedback
    
    // ADSR controls area
    auto adsrArea = area.removeFromTop(80);
    int sliderWidth = adsrArea.getWidth() / 4;
    
    attackSlider.setBounds(adsrArea.getX(), adsrArea.getY(), sliderWidth, 60);
    attackLabel.setBounds(adsrArea.getX(), adsrArea.getY() + 60, sliderWidth, 20);
    
    decaySlider.setBounds(adsrArea.getX() + sliderWidth, adsrArea.getY(), sliderWidth, 60);
    decayLabel.setBounds(adsrArea.getX() + sliderWidth, adsrArea.getY() + 60, sliderWidth, 20);
    
    sustainSlider.setBounds(adsrArea.getX() + sliderWidth * 2, adsrArea.getY(), sliderWidth, 60);
    sustainLabel.setBounds(adsrArea.getX() + sliderWidth * 2, adsrArea.getY() + 60, sliderWidth, 20);
    
    releaseSlider.setBounds(adsrArea.getX() + sliderWidth * 3, adsrArea.getY(), sliderWidth, 60);
    releaseLabel.setBounds(adsrArea.getX() + sliderWidth * 3, adsrArea.getY() + 60, sliderWidth, 20);
    
    // Safety check and arrange 16 buttons in a single row
    if (stepButtons.size() == 16 && area.getWidth() > 0)
    {
        auto buttonWidth = area.getWidth() / 16;
        for (int i = 0; i < 16; ++i)
        {
            if (stepButtons[i] != nullptr)
            {
                stepButtons[i]->setBounds(i * buttonWidth, area.getY(), buttonWidth, area.getHeight());
            }
        }
    }
}

void HelloWorldVST3AudioProcessorEditor::buttonClicked(juce::Button* button)
{
    // Safety check
    if (button == nullptr) return;
    
    // Find which step button was clicked and toggle it
    for (int i = 0; i < stepButtons.size() && i < 16; ++i)
    {
        if (stepButtons[i] == button)
        {
            bool currentState = audioProcessor.isStepActive(i);
            audioProcessor.setStepActive(i, !currentState);
            
            // Update button color to show state, but preserve current step indicator
            if (i == currentStepPosition)
            {
                // This is the current step - keep it highlighted but update the base color
                if (!currentState) // Just turned ON
                {
                    button->setColour(juce::TextButton::buttonColourId, juce::Colours::orange);
                    button->setColour(juce::TextButton::textColourOffId, juce::Colours::black);
                }
                else // Just turned OFF
                {
                    button->setColour(juce::TextButton::buttonColourId, juce::Colours::orange);
                    button->setColour(juce::TextButton::textColourOffId, juce::Colours::black);
                }
            }
            else
            {
                // Regular button update
                if (!currentState) // Just turned ON
                {
                    button->setColour(juce::TextButton::buttonColourId, juce::Colours::limegreen);
                    button->setColour(juce::TextButton::textColourOffId, juce::Colours::black);
                }
                else // Just turned OFF
                {
                    button->setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);
                    button->setColour(juce::TextButton::textColourOffId, juce::Colours::white);
                }
            }
            button->repaint();
            break;
        }
    }
}

void HelloWorldVST3AudioProcessorEditor::timerCallback()
{
    // Update the current step indicator
    updateCurrentStepIndicator();
}

bool HelloWorldVST3AudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    // Check if any of the files are audio files
    for (const auto& file : files)
    {
        juce::File audioFile(file);
        if (audioFile.hasFileExtension("wav;aiff;mp3;flac;ogg;m4a"))
            return true;
    }
    return false;
}

void HelloWorldVST3AudioProcessorEditor::fileDragEnter(const juce::StringArray& files, int x, int y)
{
    juce::ignoreUnused(files, x, y);
    isDragOver = true;
    repaint();
}

void HelloWorldVST3AudioProcessorEditor::fileDragExit(const juce::StringArray& files)
{
    juce::ignoreUnused(files);
    isDragOver = false;
    repaint();
}

void HelloWorldVST3AudioProcessorEditor::filesDropped(const juce::StringArray& files, int x, int y)
{
    juce::ignoreUnused(x, y);
    isDragOver = false;
    
    // Load the first audio file found
    for (const auto& filePath : files)
    {
        juce::File audioFile(filePath);
        if (audioFile.hasFileExtension("wav;aiff;mp3;flac;ogg;m4a"))
        {
            audioProcessor.loadSample(audioFile);
            
            // Update the UI to show the loaded sample
            if (audioProcessor.hasSample())
            {
                sampleLabel.setText("Sample: " + audioProcessor.getSampleName(), juce::dontSendNotification);
            }
            break; // Only load the first valid audio file
        }
    }
    
    repaint();
}

void HelloWorldVST3AudioProcessorEditor::setupSlider(juce::Slider& slider, juce::Label& label, 
                                                    const juce::String& text, float minValue, 
                                                    float maxValue, float defaultValue)
{
    slider.setSliderStyle(juce::Slider::LinearVertical);
    slider.setRange(minValue, maxValue, 0.001f);
    slider.setValue(defaultValue);
    slider.addListener(this);
    addAndMakeVisible(slider);
    
    label.setText(text, juce::dontSendNotification);
    label.setFont(juce::Font(10.0f));
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(label);
}

void HelloWorldVST3AudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &attackSlider)
        audioProcessor.setAttack(static_cast<float>(slider->getValue()));
    else if (slider == &decaySlider)
        audioProcessor.setDecay(static_cast<float>(slider->getValue()));
    else if (slider == &releaseSlider)
        audioProcessor.setRelease(static_cast<float>(slider->getValue()));
    else if (slider == &sustainSlider)
        audioProcessor.setSustain(static_cast<float>(slider->getValue()));
}

void HelloWorldVST3AudioProcessorEditor::updateStepButtonStates()
{
    // Update all step button colors to match the processor state
    for (int i = 0; i < stepButtons.size() && i < 16; ++i)
    {
        if (stepButtons[i] != nullptr)
        {
            // Don't update the current step indicator - preserve its highlight
            if (i == currentStepPosition)
                continue;
                
            bool isActive = audioProcessor.isStepActive(i);
            if (isActive)
            {
                // Active step - green
                stepButtons[i]->setColour(juce::TextButton::buttonColourId, juce::Colours::limegreen);
                stepButtons[i]->setColour(juce::TextButton::textColourOffId, juce::Colours::black);
            }
            else
            {
                // Inactive step - dark grey
                stepButtons[i]->setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);
                stepButtons[i]->setColour(juce::TextButton::textColourOffId, juce::Colours::white);
            }
            stepButtons[i]->repaint();
        }
    }
}

void HelloWorldVST3AudioProcessorEditor::updateCurrentStepIndicator()
{
    // Get the current step from the processor
    int newStepPosition = audioProcessor.getLastTriggeredStep();
    
    // Only update if the step position has changed
    if (newStepPosition != currentStepPosition)
    {
        // Clear previous step highlight
        if (currentStepPosition >= 0 && currentStepPosition < stepButtons.size())
        {
            if (stepButtons[currentStepPosition] != nullptr)
            {
                bool isActive = audioProcessor.isStepActive(currentStepPosition);
                if (isActive)
                {
                    // Restore active step color (green)
                    stepButtons[currentStepPosition]->setColour(juce::TextButton::buttonColourId, juce::Colours::limegreen);
                    stepButtons[currentStepPosition]->setColour(juce::TextButton::textColourOffId, juce::Colours::black);
                }
                else
                {
                    // Restore inactive step color (dark grey)
                    stepButtons[currentStepPosition]->setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);
                    stepButtons[currentStepPosition]->setColour(juce::TextButton::textColourOffId, juce::Colours::white);
                }
                stepButtons[currentStepPosition]->repaint();
            }
        }
        
        // Update current step position
        currentStepPosition = newStepPosition;
        
        // Highlight new current step
        if (currentStepPosition >= 0 && currentStepPosition < stepButtons.size())
        {
            if (stepButtons[currentStepPosition] != nullptr)
            {
                // Highlight current step with a bright color (orange/amber)
                stepButtons[currentStepPosition]->setColour(juce::TextButton::buttonColourId, juce::Colours::orange);
                stepButtons[currentStepPosition]->setColour(juce::TextButton::textColourOffId, juce::Colours::black);
                stepButtons[currentStepPosition]->repaint();
            }
        }
    }
}
