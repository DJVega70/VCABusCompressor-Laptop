#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// Console-strip styling: colored plastic caps over a dark skirt, white silkscreen ticks
class StripLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos,
                          float startAngle, float endAngle, juce::Slider& s) override;
};

class Knob : public juce::Component
{
public:
    Knob(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId, const juce::String& title,
         juce::StringArray tickLabels, juce::Colour cap, bool large,
         std::function<juce::String(double)> formatter, StripLookAndFeel& laf);
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    bool isLarge;
    juce::Slider slider { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };
    juce::Label name, value;
    juce::StringArray ticks;
    std::function<juce::String(double)> fmt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

// Push button with a lit LED bar across the top
class LedButton : public juce::Button
{
public:
    LedButton(const juce::String& text, juce::Colour ledColour) : juce::Button(text), led(ledColour) {}
    void paintButton(juce::Graphics& g, bool over, bool down) override;

private:
    juce::Colour led;
};

// Vertical LED ladder, 0 dB at the top
class GrMeter : public juce::Component, private juce::Timer
{
public:
    explicit GrMeter(VcaBusCompAudioProcessor& p) : proc(p) { startTimerHz(30); }
    ~GrMeter() override { stopTimer(); }
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
    juce::Rectangle<int> cell(int col, int row) const { return { 20 + col * 120, 168 + row * 126, 120, 126 }; }

    VcaBusCompAudioProcessor& proc;
    StripLookAndFeel laf;
    GrMeter meter;
    std::vector<std::unique_ptr<Knob>> knobs;

    LedButton inButton { "IN", juce::Colour(0xffef4444) };
    LedButton gButton { "G", juce::Colour(0xfff5c518) };
    LedButton eButton { "E", juce::Colour(0xfff5c518) };

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> inAtt;
    std::unique_ptr<juce::ParameterAttachment> modeAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VcaBusCompAudioProcessorEditor)
};
