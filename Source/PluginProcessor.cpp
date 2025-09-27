/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
HARPyAudioProcessor::HARPyAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

HARPyAudioProcessor::~HARPyAudioProcessor()
{
}

//==============================================================================
const juce::String HARPyAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool HARPyAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool HARPyAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool HARPyAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double HARPyAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int HARPyAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int HARPyAudioProcessor::getCurrentProgram()
{
    return 0;
}

void HARPyAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String HARPyAudioProcessor::getProgramName (int index)
{
    return {};
}

void HARPyAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void HARPyAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    noteVels.clear();
    currentNote = 0;
    lastNoteValue = std::make_pair(-1, juce::uint8(0));
    time = 0;
    rate = static_cast<float> (sampleRate);
    absArpPos = 0;
    repeat = 0;
}

void HARPyAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool HARPyAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void HARPyAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    auto tmpBPM = getPlayHead()->getPosition()->getBpm();

    // NOTE: When you're running standalone, you won't get a value here, as there is no host environment
    if (tmpBPM.hasValue()) {
        // Set your module level variable to the retrieved value, for use elsewhere
        hostBPM = (int)*tmpBPM;
    }
    else {
        hostBPM = 120; // Default value
    }

    auto settings = getArpeggiatorSettings(apvts);

    auto rateCoefficient = std::powf(2.0f, settings.rate);

    // the audio buffer in a midi effect will have zero channels!
    jassert(buffer.getNumChannels() == 0);
    // however we use the buffer to get timing information
    auto numSamples = buffer.getNumSamples();
    // get noteVel duration
    const auto secondsInMinute = 60.0f;

    // Seriously can't figure what this is, but it works
    const auto magicFactor = 4.0f;

    auto ts = getPlayHead()->getPosition()->getTimeSignature();

    auto beatsInBar = 4.0f;
    auto beatLength = 4.0f;
    if (ts.hasValue()) {
        beatsInBar = float(ts->numerator);
        beatLength = float(ts->denominator);
    }

    auto noteDuration = static_cast<int> (rate * secondsInMinute * magicFactor * beatsInBar
        / beatLength / float(hostBPM) / rateCoefficient);

    for (const auto metadata : midiMessages)
    {
        const auto msg = metadata.getMessage();
        if (msg.isNoteOn())
            noteVels.add(std::make_pair(msg.getNoteNumber(), msg.getVelocity()));
        else if (msg.isNoteOff())
            for (juce::uint8 i = 0; i <= 127; ++i) {
                noteVels.removeValue(std::make_pair(msg.getNoteNumber(), i));

                if (noteVels.size() == 0) {
                    lastNoteValue = std::make_pair(-1, juce::uint8(0));
                    time = 0;
                    absArpPos = 0;
                    repeat = 0;
                }
            }
    }
    midiMessages.clear();

    if ((settings.repeats > 0) && (repeat >= settings.repeats)) {
        return;
    }

    if ((time + numSamples) >= int(noteDuration * settings.noteLength) && (lastNoteValue.first > 0)) {
        auto offset = juce::jmax(0, juce::jmin(int(int(noteDuration * settings.noteLength) - time), numSamples - 1));
        switch (settings.order) {
        default:
        case Up:
        case Down:
        case UpDown:
        case DownUp:
            
            midiMessages.addEvent(juce::MidiMessage::noteOff(1, lastNoteValue.first), offset);
            lastNoteValue = std::make_pair(-1, juce::uint8(0));
            break;
        case ChordRepeat:
            for (auto& noteVel : noteVels) {
                midiMessages.addEvent(juce::MidiMessage::noteOff(1, noteVel.first), offset);
            }
            break;
        }
    }
    if ((time + numSamples) >= noteDuration && noteVels.size() > 0) {
        auto offset = juce::jmax(0, juce::jmin(int(noteDuration - time), numSamples - 1));

        auto absArpLen = absoluteArpeggioLength();
        juce::uint8 finalVel = 0;

        switch (settings.order) {
        default:
        case Up:
        case Down:
        case UpDown:
        case DownUp:
        case UpAndDown:
        case DownAndUp:
            switch (settings.order)
            {
            default:
            case Up:
                currentNote = absArpPos;
                break;
            case Down:
                currentNote = absoluteArpeggioLength() - 1 - absArpPos;
                break;
            case UpDown:
                if (noteVels.size() <= 1) {
                    currentNote = 0;
                    break;
                }

                if (absArpPos < noteVels.size()) {
                    currentNote = absArpPos;
                }
                else {
                    currentNote = absoluteArpeggioLength() - absArpPos;
                }
                break;
            case DownUp:
                if (noteVels.size() <= 1) {
                    currentNote = 0;
                    break;
                }

                if (absArpPos < noteVels.size()) {
                    currentNote = noteVels.size() - 1 - absArpPos;
                }
                else {
                    currentNote = absArpPos - noteVels.size() + 1;
                }
                break;
            case UpAndDown:
                if (noteVels.size() <= 1) {
                    currentNote = 0;
                    break;
                }

                if (absArpPos < noteVels.size()) {
                    currentNote = absArpPos;
                }
                else {
                    currentNote = absoluteArpeggioLength() - 1 - absArpPos;
                }
                break;
            case DownAndUp:
                if (noteVels.size() <= 1) {
                    currentNote = 0;
                    break;
                }

                if (absArpPos < noteVels.size()) {
                    currentNote = noteVels.size() - 1 - absArpPos;
                }
                else {
                    currentNote = absArpPos - noteVels.size();
                }
                break;
            case Random:
                juce::Random rng;
                auto tmp = rng.nextInt(noteVels.size());
                if (currentNote != tmp) {
                    currentNote = tmp;
                }
                else {
                    currentNote = (currentNote == noteVels.size() - 1)
                        ? currentNote - 1
                        : currentNote + 1;
                }
                break;
            }

            lastNoteValue = noteVels[currentNote];

            finalVel = juce::uint8(float(lastNoteValue.second) * settings.velFineCtrl);
            midiMessages.addEvent(juce::MidiMessage::noteOn(1, lastNoteValue.first, finalVel), offset);

            ++absArpPos;
            if (absArpPos >= absArpLen) {
                absArpPos = 0;
            }

            if (settings.repeats > 0 && absArpPos == 0) {
                ++repeat;
            }
            break;
        case ChordRepeat:
            for (auto& noteVel : noteVels) {
                juce::uint8 finalVel = juce::uint8(float(noteVel.second) * settings.velFineCtrl);
                midiMessages.addEvent(juce::MidiMessage::noteOn(1, noteVel.first, finalVel), offset);
            }
            if (settings.repeats > 0) {
                ++repeat;
            }
            break;
        }
        
    }
    time = (time + numSamples) % noteDuration;
}

//==============================================================================
bool HARPyAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* HARPyAudioProcessor::createEditor()
{
    return new HARPyAudioProcessorEditor (*this);
}

//==============================================================================
void HARPyAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.

    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void HARPyAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.

    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);

    if (tree.isValid()) {
        apvts.replaceState(tree);
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout HARPyAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    juce::StringArray rateChoices {
        "1/1",
        "1/2",
        "1/4",
        "1/8",
        "1/16",
        "1/32",
        "1/64",
    };

    layout.add(std::make_unique<juce::AudioParameterChoice>("Rate", "Rate", rateChoices, 0));

    juce::StringArray orderChoices{
        "Up",
        "Down",
        "Up/Down",
        "Down/Up",
        "Up & Down",
        "Down & Up",
        "Random",
        "Chord Repeat"
    };

    layout.add(std::make_unique<juce::AudioParameterChoice>("Order", "Order", orderChoices, 0));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Velocity Fine Control", "Velocity Fine Control", 0.01f, 1.f, 1.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Note Length", "Note Length", 0.01f, 2.f, 1.f));

    layout.add(std::make_unique<juce::AudioParameterInt>("Repeats", "Repeats", 0, 16, 0));

    return layout;
}

int HARPyAudioProcessor::absoluteArpeggioLength()
{
    auto settings = getArpeggiatorSettings(apvts);
    switch (ArpeggioOrder(settings.order))
    {
    default:
    case Up:
    case Down:
    case Random:
        return noteVels.size();
        break;
    case UpDown:
    case DownUp:
        return (noteVels.size() * 2) - 2;
    case UpAndDown:
    case DownAndUp:
        return (noteVels.size() * 2);
    case ChordRepeat:
        return 1;
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HARPyAudioProcessor();
}

ArpeggiatorSettings getArpeggiatorSettings(juce::AudioProcessorValueTreeState& apvts)
{
    ArpeggiatorSettings settings;

    settings.rate = apvts.getRawParameterValue("Rate")->load();
    settings.order = ArpeggioOrder(int(apvts.getRawParameterValue("Order")->load()));
    settings.velFineCtrl = apvts.getRawParameterValue("Velocity Fine Control")->load();
    settings.noteLength = apvts.getRawParameterValue("Note Length")->load();
    settings.repeats = apvts.getRawParameterValue("Repeats")->load();

    return settings;
}
