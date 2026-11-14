#pragma once
#include <JuceHeader.h>
#include "BusCompressor.h"

class VcaBusCompAudioProcessor : public juce::AudioProcessor
{
public:
    VcaBusCompAudioProcessor();
    ~VcaBusCompAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "VCA Bus Compressor"; }
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
    juce::AudioProcessorValueTreeState apvts;

    std::atomic<float> grMeterDb { 0.0f };   // positive dB of gain reduction, for the editor

    static const juce::StringArray& ratioLabels();
    static const juce::StringArray& attackLabels();
    static const juce::StringArray& releaseLabels();

private:
    vcacomp::BusCompressor comp;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VcaBusCompAudioProcessor)
};
