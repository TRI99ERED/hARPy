/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

enum ArpeggioOrder {
    Up,
    Down,
    UpDown,
    DownUp,
    UpAndDown,
    DownAndUp,
    Random,
    ChordRepeat,
};

struct ArpeggiatorSettings {
    float rate{ 0.f };
    ArpeggioOrder order{ Up };
    float velFineCtrl{ 1.f };
    float noteLength{ 1.f };
    int repeats{ 0 };
    int delta{ 0 };
    int offsets{ 0 };
};

ArpeggiatorSettings getArpeggiatorSettings(juce::AudioProcessorValueTreeState& apvts);

//==============================================================================
/**
*/
class HARPyAudioProcessor  : public juce::AudioProcessor, juce::AudioProcessorValueTreeState::Listener
{
public:
    //==============================================================================
    HARPyAudioProcessor();
    ~HARPyAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts{ *this, nullptr, "Parameters", createParameterLayout() };

    int hostBPM = 120;

private:
    //==============================================================================
    int currentNote;
    std::pair<int, juce::uint8> lastNoteValue;
    int time;
    float rate;
    juce::SortedSet<std::pair<int, juce::uint8>> noteVels;
    int absArpPos = 0;
    int repeat = 0;

    int getAbsoluteArpeggioLength();
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HARPyAudioProcessor)
};
