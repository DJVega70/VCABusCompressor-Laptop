#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class KnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos,
                          float startAngle, float endAngle, juce::Slider& s) override;
};

// Rotary knob with tick labels around it (stepped or continuous), name and value readout
class Knob : public juce::Component
{
public:
    Knob(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId, const juce::String& title,
         juce::StringArray tickLabels, bool stepped, juce::Colour accent,
         std::function<juce::String(double)> formatter, KnobLookAndFeel& laf);
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::Slider slider { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };
    juce::Label name, value;
    juce::StringArray ticks;
    std::function<juce::String(double)> fmt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

// LED-style gain reduction meter (0, 2, 4, 6, 8, 10, 14, 20 dB)
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
    VcaBusCompAudioProcessor& proc;
    KnobLookAndFeel laf;
    GrMeter meter;
    std::vector<std::unique_ptr<Knob>> knobs;

    juce::ComboBox modeBox, hpfBox;
    juce::Label modeLabel, hpfLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAtt, hpfAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VcaBusCompAudioProcessorEditor)
};
