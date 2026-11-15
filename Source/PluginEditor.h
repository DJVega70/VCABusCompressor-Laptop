#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// Original console-style look: charcoal panel, coloured knob caps, lit push buttons, LED ladders.
class ConsoleLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h, float pos,
                          float startAngle, float endAngle, juce::Slider&) override;
    void drawLinearSlider(juce::Graphics&, int x, int y, int w, int h, float sliderPos,
                          float minPos, float maxPos, juce::Slider::SliderStyle, juce::Slider&) override;
};

class StripKnob : public juce::Component
{
public:
    StripKnob(juce::AudioProcessorValueTreeState&, const juce::String& id, const juce::String& title,
              juce::Colour cap, std::function<juce::String(double)> fmt, ConsoleLookAndFeel&);
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    juce::Slider slider { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };
    juce::String title;
    juce::Label value;
    std::function<juce::String(double)> fmt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

class LampButton : public juce::Button
{
public:
    LampButton(const juce::String& text, juce::Colour lamp) : juce::Button(text), lampColour(lamp) {}
    void paintButton(juce::Graphics&, bool over, bool down) override;
private:
    juce::Colour lampColour;
};

class Ladder : public juce::Component
{
public:
    enum Kind { Peak, Reduction };
    Ladder(Kind k, juce::String label) : kind(k), text(std::move(label)) {}
    void setValue(float v) { shown = v; repaint(); }
    void paint(juce::Graphics&) override;
private:
    Kind kind; juce::String text; float shown = 0.0f;   // dB (peak) or dB of reduction
};

class ConsoleStripEditor : public juce::AudioProcessorEditor
{
public:
    explicit ConsoleStripEditor(ConsoleStripProcessor&);
    ~ConsoleStripEditor() override;
    void resized() override;
    void paint(juce::Graphics& g) override { g.fillAll(juce::Colours::black); }

    static constexpr int kBaseW = 460, kBaseH = 996;
private:
    class Content;
    std::unique_ptr<Content> content;
    juce::ComponentBoundsConstrainer constrainer;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConsoleStripEditor)
};
