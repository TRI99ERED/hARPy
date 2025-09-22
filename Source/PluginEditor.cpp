/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
HARPyAudioProcessorEditor::HARPyAudioProcessorEditor (HARPyAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.

    for (auto* comp : getComps()) {
        addAndMakeVisible(comp);
    }

    setSize (400, 300);
}

HARPyAudioProcessorEditor::~HARPyAudioProcessorEditor()
{
}

//==============================================================================
void HARPyAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (15.0f));
    juce::String bpm = juce::String(audioProcessor.hostBPM);
    g.drawFittedText (bpm, getLocalBounds(), juce::Justification::centred, 1);
}

void HARPyAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    auto bounds = getLocalBounds();

    auto rateArea = bounds.removeFromLeft(bounds.getWidth() * 0.5f);
    auto orderArea = bounds;

    rateSlider.setBounds(rateArea);
    orderSlider.setBounds(orderArea);
}

std::vector<juce::Component*> HARPyAudioProcessorEditor::getComps()
{
    return {
        &rateSlider,
        &orderSlider,
    };
}
