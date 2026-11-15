#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout ConsoleStripProcessor::createLayout()
{
    using F = juce::AudioParameterFloat;
    using B = juce::AudioParameterBool;
    using C = juce::AudioParameterChoice;
    using R = juce::NormalisableRange<float>;
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    auto id = [](const char* s) { return juce::ParameterID { s, 1 }; };
    auto flt = [&](const char* i, const char* n, R r, float d, const char* unit = "") {
        l.add(std::make_unique<F>(id(i), n, r, d, juce::AudioParameterFloatAttributes().withLabel(unit))); };
    auto logR = [](float lo, float hi) { R r(lo, hi, 0.0f); r.setSkewForCentre(std::sqrt(lo * hi)); return r; };

    // preamp
    flt("pre", "Preamp Gain", R(0.0f, 72.0f, 0.1f), 0.0f, "dB");
    l.add(std::make_unique<B>(id("polarity"), "Polarity Invert", false));
    l.add(std::make_unique<C>(id("satmode"), "Saturation Type", juce::StringArray { "Transformer", "Class-A" }, 0));
    flt("drive", "Saturation Drive", R(0.0f, 100.0f, 0.1f), 0.0f, "%");

    // filters
    l.add(std::make_unique<B>(id("hpfon"), "HPF In", false));
    flt("hpf", "HPF Frequency", logR(20.0f, 400.0f), 80.0f, "Hz");
    l.add(std::make_unique<B>(id("lpfon"), "LPF In", false));
    flt("lpf", "LPF Frequency", logR(3000.0f, 20000.0f), 16000.0f, "Hz");

    // gate
    l.add(std::make_unique<B>(id("gateon"), "Gate In", false));
    flt("gthr", "Gate Threshold", R(-70.0f, 0.0f, 0.1f), -50.0f, "dB");
    flt("grange", "Gate Range", R(-70.0f, 0.0f, 0.1f), -40.0f, "dB");
    flt("ghold", "Gate Hold", R(0.0f, 500.0f, 1.0f), 20.0f, "ms");
    flt("grel", "Gate Release", logR(20.0f, 2000.0f), 150.0f, "ms");

    // compressor
    l.add(std::make_unique<B>(id("compon"), "Comp In", false));
    l.add(std::make_unique<B>(id("comppost"), "Comp Post-EQ", false));
    flt("cthr", "Comp Threshold", R(-40.0f, 0.0f, 0.1f), -18.0f, "dB");
    flt("cratio", "Comp Ratio", R(1.0f, 10.0f, 0.1f, 0.6f), 4.0f, ":1");
    flt("catk", "Comp Attack", logR(0.1f, 100.0f), 10.0f, "ms");
    flt("crel", "Comp Release", logR(50.0f, 2000.0f), 200.0f, "ms");
    flt("cmakeup", "Comp Make-up", R(0.0f, 20.0f, 0.1f), 0.0f, "dB");

    // EQ
    l.add(std::make_unique<B>(id("eqon"), "EQ In", true));
    l.add(std::make_unique<B>(id("propq"), "EQ Proportional Q", false));
    flt("lfg", "LF Gain", R(-16.0f, 16.0f, 0.1f), 0.0f, "dB");
    flt("lff", "LF Frequency", logR(30.0f, 450.0f), 100.0f, "Hz");
    l.add(std::make_unique<B>(id("lfbell"), "LF Bell", false));
    flt("lmg", "LMF Gain", R(-16.0f, 16.0f, 0.1f), 0.0f, "dB");
    flt("lmf", "LMF Frequency", logR(200.0f, 2500.0f), 600.0f, "Hz");
    flt("lmq", "LMF Q", logR(0.4f, 4.0f), 1.0f);
    flt("hmg", "HMF Gain", R(-16.0f, 16.0f, 0.1f), 0.0f, "dB");
    flt("hmf", "HMF Frequency", logR(600.0f, 7000.0f), 3000.0f, "Hz");
    flt("hmq", "HMF Q", logR(0.4f, 4.0f), 1.0f);
    flt("hfg", "HF Gain", R(-16.0f, 16.0f, 0.1f), 0.0f, "dB");
    flt("hff", "HF Frequency", logR(1500.0f, 16000.0f), 8000.0f, "Hz");
    l.add(std::make_unique<B>(id("hfbell"), "HF Bell", false));

    // output
    flt("fader", "Fader", R(-80.0f, 12.0f, 0.1f, 2.2f), 0.0f, "dB");
    flt("pan", "Balance", R(-100.0f, 100.0f, 1.0f), 0.0f);
    return l;
}

strip::Params ConsoleStripProcessor::readParams() const
{
    strip::Params s;
    s.preGainDb = p("pre"); s.polarity = b("polarity");
    s.satMode = juce::roundToInt(p("satmode")); s.satAmount = p("drive") / 100.0f;
    s.hpfHz = b("hpfon") ? p("hpf") : 0.0f;
    s.lpfHz = b("lpfon") ? p("lpf") : 0.0f;
    s.gateOn = b("gateon"); s.gateThrDb = p("gthr"); s.gateRangeDb = p("grange");
    s.gateHoldMs = p("ghold"); s.gateReleaseMs = p("grel");
    s.compOn = b("compon"); s.compPostEq = b("comppost");
    s.compThrDb = p("cthr"); s.compRatio = p("cratio"); s.compAttackMs = p("catk");
    s.compReleaseMs = p("crel"); s.compMakeupDb = p("cmakeup");
    s.eqOn = b("eqon"); s.eqPropQ = b("propq");
    s.lfGainDb = p("lfg"); s.lfFreq = p("lff"); s.lfBell = b("lfbell");
    s.lmfGainDb = p("lmg"); s.lmfFreq = p("lmf"); s.lmfQ = p("lmq");
    s.hmfGainDb = p("hmg"); s.hmfFreq = p("hmf"); s.hmfQ = p("hmq");
    s.hfGainDb = p("hfg"); s.hfFreq = p("hff"); s.hfBell = b("hfbell");
    s.outDb = p("fader"); s.pan = p("pan") / 100.0f;
    return s;
}

ConsoleStripProcessor::ConsoleStripProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "STATE", createLayout())
{
}

bool ConsoleStripProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void ConsoleStripProcessor::prepareToPlay(double sampleRate, int)
{
    dsp.prepare(sampleRate);
    dsp.setParams(readParams());
    setLatencySamples(strip::kLatencySamples);
}

void ConsoleStripProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    if (buffer.getNumChannels() < 2 || buffer.getNumSamples() == 0)
        return;

    dsp.setParams(readParams());
    dsp.process(buffer.getWritePointer(0), buffer.getWritePointer(1), buffer.getNumSamples());

    inPeak = dsp.inPeak; outPeak = dsp.outPeak; compGr = dsp.compGrDb; gateGr = dsp.gateGrDb;
}

juce::AudioProcessorEditor* ConsoleStripProcessor::createEditor()
{
    return new ConsoleStripEditor(*this);
}

void ConsoleStripProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary(*xml, destData);
}

void ConsoleStripProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ConsoleStripProcessor();
}
