#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// Hardware-style knob: brass ring, knurled graphite cap, cream pointer
class HardwareLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos,
                          float startAngle, float endAngle, juce::Slider& s) override;
};

class Knob : public juce::Component
{
public:
    Knob(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId, const juce::String& title,
         juce::StringArray tickLabels, std::function<juce::String(double)> formatter, HardwareLookAndFeel& laf);
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::Slider slider { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };
    juce::Label name, value;
    juce::StringArray ticks;
    std::function<juce::String(double)> fmt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

// Chunky push button with a lamp window
class LedButton : public juce::Button
{
public:
    LedButton(const juce::String& text, juce::Colour lampColour) : juce::Button(text), lamp(lampColour) {}
    void paintButton(juce::Graphics& g, bool over, bool down) override;

private:
    juce::Colour lamp;
};

// Analog moving-coil style gain-reduction meter
class NeedleMeter : public juce::Component, private juce::Timer
{
public:
    explicit NeedleMeter(VcaBusCompAudioProcessor& p) : proc(p) { startTimerHz(30); }
    ~NeedleMeter() override { stopTimer(); }
    void paint(juce::Graphics& g) override;

private:
    void timerCallback() override;
    VcaBusCompAudioProcessor& proc;
    float shown = 0.0f;
};

class VcaBusCompAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit VcaBusCompAudioProcessorEditor(VcaBusCompAudioProcessor&);
    ~VcaBusCompAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    juce::Rectangle<int> cell(int col, int row) const { return { 20 + col * 108, 250 + row * 128, 108, 128 }; }

    VcaBusCompAudioProcessor& proc;
    HardwareLookAndFeel laf;
    NeedleMeter meter;
    std::vector<std::unique_ptr<Knob>> knobs;

    LedButton inButton { "IN", juce::Colour(0xffff4d3d) };
    LedButton gButton { "G", juce::Colour(0xffffb02e) };
    LedButton eButton { "E", juce::Colour(0xffffb02e) };

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> inAtt;
    std::unique_ptr<juce::ParameterAttachment> modeAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VcaBusCompAudioProcessorEditor)
};
