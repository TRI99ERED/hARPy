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

    g.setColour(Colours::darkgrey);
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
        r.setCentre(bounds.getCentre());

        g.setColour(Colours::black);
        g.fillRect(r);

        g.setColour(Colours::white);
        g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);
    }
}
//==============================================================================
void RotarySliderWithLabel::paint(juce::Graphics& g) {
    using namespace juce;

    auto startAng = degreesToRadians(180.f + 45.f);
    auto endAng = degreesToRadians(180.f - 45.f) + MathConstants<float>::twoPi;

    auto range = getRange();

    auto sliderBounds = getSliderBounds();

    /*g.setColour(Colours::red);
    g.drawRect(getLocalBounds());
    g.setColour(Colours::yellow);
    g.drawRect(sliderBounds);*/

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
}
//==============================================================================
HARPyAudioProcessorEditor::HARPyAudioProcessorEditor (HARPyAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
    rateSlider(*audioProcessor.apvts.getParameter("Rate")),
    orderSlider(*audioProcessor.apvts.getParameter("Order")),
    rateSliderAttachment(audioProcessor.apvts, "Rate", rateSlider),
    orderSliderAttachment(audioProcessor.apvts, "Order", orderSlider)
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
    
    using namespace juce;

    g.fillAll(Colours::black);

    auto bounds = getLocalBounds();
    auto titleArea = bounds.removeFromTop(bounds.getHeight() * 0.1);
    auto h = titleArea.getHeight();

    g.setColour(Colours::white);
    g.setFont(h);
    g.drawFittedText("hARPy v0.0.1", titleArea, Justification::centredLeft, 1);

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

    auto titleArea = bounds.removeFromTop(bounds.getHeight() * 0.25);

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
