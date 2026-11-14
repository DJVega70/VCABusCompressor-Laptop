#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    const float kRatios[]  = { 2.0f, 4.0f, 10.0f };
    const float kAttacks[] = { 0.1f, 0.3f, 1.0f, 3.0f, 10.0f, 30.0f };      // ms
    const float kReleases[] = { 100.0f, 200.0f, 400.0f, 800.0f, 1200.0f };  // ms, index 5 = Auto
    const float kHpf[] = { 0.0f, 60.0f, 90.0f, 150.0f };
}

const juce::StringArray& VcaBusCompAudioProcessor::ratioLabels()
{
    static const juce::StringArray a { "2", "4", "10" };
    return a;
}
const juce::StringArray& VcaBusCompAudioProcessor::attackLabels()
{
    static const juce::StringArray a { ".1", ".3", "1", "3", "10", "30" };
    return a;
}
const juce::StringArray& VcaBusCompAudioProcessor::releaseLabels()
{
    static const juce::StringArray a { ".1", ".2", ".4", ".8", "1.2", "AUTO" };
    return a;
}

juce::AudioProcessorValueTreeState::ParameterLayout VcaBusCompAudioProcessor::createLayout()
{
    using F = juce::AudioParameterFloat;
    using C = juce::AudioParameterChoice;
    using R = juce::NormalisableRange<float>;
    juce::AudioProcessorValueTreeState::ParameterLayout l;

    l.add(std::make_unique<F>(juce::ParameterID{ "threshold", 1 }, "Threshold", R(-30.0f, 0.0f, 0.1f), -12.0f));
    l.add(std::make_unique<C>(juce::ParameterID{ "ratio", 1 },   "Ratio",   juce::StringArray { "2:1", "4:1", "10:1" }, 1));
    l.add(std::make_unique<C>(juce::ParameterID{ "attack", 1 },  "Attack",  juce::StringArray { "0.1 ms", "0.3 ms", "1 ms", "3 ms", "10 ms", "30 ms" }, 3));
    l.add(std::make_unique<C>(juce::ParameterID{ "release", 1 }, "Release", juce::StringArray { "0.1 s", "0.2 s", "0.4 s", "0.8 s", "1.2 s", "Auto" }, 2));
    l.add(std::make_unique<F>(juce::ParameterID{ "makeup", 1 },  "Make-up",  R(-5.0f, 15.0f, 0.1f), 0.0f));
    l.add(std::make_unique<F>(juce::ParameterID{ "mix", 1 },     "Mix",      R(0.0f, 100.0f, 1.0f), 100.0f));
    l.add(std::make_unique<C>(juce::ParameterID{ "mode", 1 },    "Voicing",  juce::StringArray { "G-Series (soft knee)", "E-Series (hard knee)" }, 0));
    l.add(std::make_unique<C>(juce::ParameterID{ "schpf", 1 },   "SC High-Pass", juce::StringArray { "Off", "60 Hz", "90 Hz", "150 Hz" }, 1));
    return l;
}

VcaBusCompAudioProcessor::VcaBusCompAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "STATE", createLayout())
{
}

bool VcaBusCompAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void VcaBusCompAudioProcessor::prepareToPlay(double sampleRate, int)
{
    comp.prepare(sampleRate);
    grMeterDb = 0.0f;
}

void VcaBusCompAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    if (buffer.getNumChannels() < 2 || buffer.getNumSamples() == 0)
        return;

    auto idx = [this](const char* id) { return juce::jlimit(0, 99, juce::roundToInt(apvts.getRawParameterValue(id)->load())); };

    vcacomp::Params p;
    p.thresholdDb = apvts.getRawParameterValue("threshold")->load();
    p.ratio       = kRatios[juce::jmin(idx("ratio"), 2)];
    p.attackMs    = kAttacks[juce::jmin(idx("attack"), 5)];
    const int rel = juce::jmin(idx("release"), 5);
    p.autoRelease = (rel == 5);
    p.releaseMs   = p.autoRelease ? 400.0f : kReleases[rel];
    p.makeupDb    = apvts.getRawParameterValue("makeup")->load();
    p.mix         = apvts.getRawParameterValue("mix")->load() / 100.0f;
    p.scHpfHz     = kHpf[juce::jmin(idx("schpf"), 3)];
    p.kneeDb      = idx("mode") == 0 ? 6.0f : 2.0f;   // G: softer knee, E: harder knee

    comp.setParams(p);
    comp.process(buffer.getWritePointer(0), buffer.getWritePointer(1), buffer.getNumSamples());
    grMeterDb = comp.getBlockMaxGainReductionDb();
}

juce::AudioProcessorEditor* VcaBusCompAudioProcessor::createEditor()
{
    return new VcaBusCompAudioProcessorEditor(*this);
}

void VcaBusCompAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary(*xml, destData);
}

void VcaBusCompAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VcaBusCompAudioProcessor();
}
