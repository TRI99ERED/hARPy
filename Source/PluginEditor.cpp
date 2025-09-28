/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

void LookAndFeel::drawRotarySlider(juce::Graphics& g,
    int x, int y, int width, int height,
    float sliderPosProportional,
    float rotaryStartAngle,
    float rotaryEndAngle,
    juce::Slider& slider)
{
    using namespace juce;

    auto bounds = Rectangle<float>(x, y, width, height);

    g.setColour(Colours::black);
    g.fillEllipse(bounds);

    g.setColour(Colours::white);
    g.drawEllipse(bounds, 1.f);

    if (auto* rswl = dynamic_cast<RotarySliderWithLabel*>(&slider)) {
        auto center = bounds.getCentre();

        Path p;

        Rectangle<float> r;
        r.setLeft(center.getX() - 3);
        r.setRight(center.getX() + 3);
        r.setTop(bounds.getY());
        r.setBottom(center.getY() - rswl->getTextHeight() * 1.5f);

        p.addRoundedRectangle(r, 3.f);

        jassert(rotaryStartAngle < rotaryEndAngle);

        auto sliderAngRad = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);

        p.applyTransform(AffineTransform().rotated(sliderAngRad, center.getX(), center.getY()));

        g.fillPath(p);

        g.setFont(rswl->getTextHeight());
        auto text = rswl->getDisplayString();
        auto strWidth = g.getCurrentFont().getStringWidth(text);

        r.setSize(strWidth + 3, rswl->getTextHeight() + 3);
        r.setCentre(bounds.getCentreX(), bounds.getY() + bounds.getHeight() + rswl->getTextHeight());

        g.setColour(Colours::black);
        g.fillRect(r);

        g.setColour(Colours::white);
        g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);
    }
}
//==============================================================================
void RotarySliderWithLabel::paint(juce::Graphics& g) {
    using namespace juce;

    g.fillAll(Colours::darkgrey);

    auto startAng = degreesToRadians(180.f + 45.f);
    auto endAng = degreesToRadians(180.f - 45.f) + MathConstants<float>::twoPi;

    auto range = getRange();

    auto sliderBounds = getSliderBounds();

    /*g.setColour(Colours::red);
    g.drawRect(getLocalBounds());
    g.setColour(Colours::yellow);
    g.drawRect(sliderBounds);*/

    g.setColour(Colours::white);
    g.setFont(getTextHeight());
    g.drawFittedText(title, getLocalBounds(), Justification::centredBottom, 1);

    getLookAndFeel().drawRotarySlider(
        g,
        sliderBounds.getX(),
        sliderBounds.getY(),
        sliderBounds.getWidth(),
        sliderBounds.getHeight(),
        jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0),
        startAng,
        endAng,
        *this
    );
}

juce::Rectangle<int> RotarySliderWithLabel::getSliderBounds() const {
    auto bounds = getLocalBounds();

    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());

    size -= getTextHeight() * 2;

    juce::Rectangle<int> r;
    r.setSize(size, size);
    r.setCentre(bounds.getCentreX(), 0);
    r.setY(3);

    return r;
}

juce::String RotarySliderWithLabel::getDisplayString() const {
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param)) {
        return choiceParam->getCurrentChoiceName();
    }

    if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param)) {
        return juce::String(getValue() * 100.f, 0) + "%";
    }

    if (auto* intParam = dynamic_cast<juce::AudioParameterInt*>(param)) {
        int value = getValue();
        return (title == "Repeats")
            ? juce::String((value == 0) ? juce::String("inf") : juce::String(value))
            : juce::String(value);
    }
}
//==============================================================================
HARPyAudioProcessorEditor::HARPyAudioProcessorEditor (HARPyAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
    rateSlider(*audioProcessor.apvts.getParameter("Rate"), "Rate"),
    orderSlider(*audioProcessor.apvts.getParameter("Order"), "Order"),
    velFineCtrlSlider(*audioProcessor.apvts.getParameter("Velocity Fine Control"), "Velocity"),
    noteLenSlider(*audioProcessor.apvts.getParameter("Note Length"), "Note Length"),
    repeatsSlider(*audioProcessor.apvts.getParameter("Repeats"), "Repeats"),
    deltaSlider(*audioProcessor.apvts.getParameter("Delta"), "Delta"),
    offsetsSlider(*audioProcessor.apvts.getParameter("Offsets"), "Offsets"),
    rateSliderAttachment(audioProcessor.apvts, "Rate", rateSlider),
    orderSliderAttachment(audioProcessor.apvts, "Order", orderSlider),
    velFineCtrlSliderAttachment(audioProcessor.apvts, "Velocity Fine Control", velFineCtrlSlider),
    noteLenSliderAttachment(audioProcessor.apvts, "Note Length", noteLenSlider),
    repeatsSliderAttachment(audioProcessor.apvts, "Repeats", repeatsSlider),
    deltaSliderAttachment(audioProcessor.apvts, "Delta", deltaSlider),
    offsetsSliderAttachment(audioProcessor.apvts, "Offsets", offsetsSlider)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.

    for (auto* comp : getComps()) {
        addAndMakeVisible(comp);
    }

    setSize (600, 125);
}

HARPyAudioProcessorEditor::~HARPyAudioProcessorEditor()
{
}

//==============================================================================
void HARPyAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    
    using namespace juce;

    g.fillAll(Colours::black);

    auto bounds = getLocalBounds();
    auto titleArea = bounds.removeFromBottom(bounds.getHeight() * 0.1f);
    auto h = titleArea.getHeight();

    g.setColour(Colours::white);
    g.setFont(h);
    g.drawFittedText("hARPy v0.0.2 by tri99er", titleArea, Justification::centredLeft, 1);

    /*g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (15.0f));
    juce::String bpm = juce::String(audioProcessor.hostBPM);
    g.drawFittedText (bpm, getLocalBounds(), juce::Justification::centred, 1);*/
}

void HARPyAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    auto bounds = getLocalBounds();

    auto titleArea = bounds.removeFromBottom(bounds.getHeight() * 0.1f);

    auto rateArea = bounds.removeFromLeft(bounds.getWidth() / 7.f);
    bounds.removeFromLeft(1);
    auto orderArea = bounds.removeFromLeft(bounds.getWidth() / 6.f);
    bounds.removeFromLeft(1);
    auto velFineCtrlArea = bounds.removeFromLeft(bounds.getWidth() / 5.f);
    bounds.removeFromLeft(1);
    auto noteLenArea = bounds.removeFromLeft(bounds.getWidth() / 4.f);
    bounds.removeFromLeft(1);
    auto repeatsArea = bounds.removeFromLeft(bounds.getWidth() / 3.f);
    bounds.removeFromLeft(1);
    auto deltaArea = bounds.removeFromLeft(bounds.getWidth() / 2.f);
    bounds.removeFromLeft(1);
    auto offsetsArea = bounds;

    rateSlider.setBounds(rateArea);
    orderSlider.setBounds(orderArea);
    velFineCtrlSlider.setBounds(velFineCtrlArea);
    noteLenSlider.setBounds(noteLenArea);
    repeatsSlider.setBounds(repeatsArea);
    deltaSlider.setBounds(deltaArea);
    offsetsSlider.setBounds(offsetsArea);
}

std::vector<juce::Component*> HARPyAudioProcessorEditor::getComps()
{
    return {
        &rateSlider,
        &orderSlider,
        &velFineCtrlSlider,
        &noteLenSlider,
        &repeatsSlider,
        &deltaSlider,
        &offsetsSlider,
    };
}
