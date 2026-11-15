// Smoke test of the real plugin processor (parameter mapping + latency). Not shipped in the plugin.
#include <JuceHeader.h>
#include "../Source/PluginProcessor.h"
int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    ConsoleStripProcessor proc;
    proc.setPlayConfigDetails(2, 2, 48000, 512);
    proc.prepareToPlay(48000, 512);
    auto set = [&](const char* id, float v) { auto* p = proc.apvts.getParameter(id); p->setValueNotifyingHost(p->convertTo0to1(v)); };
    auto run = [&](float amp, int blocks) {
        juce::AudioBuffer<float> b(2, 512); juce::MidiBuffer m; double ph = 0; float pk = 0;
        for (int k = 0; k < blocks; ++k) {
            for (int i = 0; i < 512; ++i, ph += 2 * juce::MathConstants<double>::pi * 1000 / 48000) { b.setSample(0, i, amp * (float) std::sin(ph)); b.setSample(1, i, amp * (float) std::sin(ph)); }
            proc.processBlock(b, m);
            if (k > blocks / 2) pk = std::max(pk, b.getMagnitude(0, 512));
        }
        return pk; };
    std::printf("latency reported: %d samples\n", proc.getLatencySamples());
    std::printf("default, 0.5 in -> peak %.4f (expect 0.5)\n", run(0.5f, 40));
    set("fader", -6.0f);  std::printf("fader -6 dB -> peak %.4f (expect 0.2506)\n", run(0.5f, 60));
    set("fader", 0.0f); set("pre", 20.0f); std::printf("preamp +20 dB -> peak %.4f (expect 5.0)\n", run(0.5f, 60));
    set("pre", 0.0f); set("lmg", 10.0f); set("lmf", 1000.0f); std::printf("LMF +10 dB @1k -> peak %.4f (expect 1.581)\n", run(0.5f, 80));
    set("lmg", 0.0f); set("compon", 1.0f); set("cthr", -21.0f); set("cratio", 4.0f);
    std::printf("comp thr -21 4:1 -> peak %.4f (expect ~0.178)\n", run(0.5f, 120));
    juce::MemoryBlock mb; proc.getStateInformation(mb); std::printf("state bytes: %zu\n", mb.getSize());
    return 0;
}
