#pragma once
#include <JuceHeader.h>
#include "ChannelStrip.h"

class ConsoleStripProcessor : public juce::AudioProcessor
{
public:
    ConsoleStripProcessor();
    ~ConsoleStripProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Console Channel Strip"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    strip::Params readParams() const;
    juce::AudioProcessorValueTreeState apvts;

    // meters for the editor (linear peak, positive dB of reduction)
    std::atomic<float> inPeak { 0.0f }, outPeak { 0.0f }, compGr { 0.0f }, gateGr { 0.0f };

private:
    float p(const char* id) const { return apvts.getRawParameterValue(id)->load(); }
    bool b(const char* id) const { return p(id) > 0.5f; }

    strip::ChannelStrip dsp;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConsoleStripProcessor)
};
