# VCA Bus Compressor (VST3)

Stereo-linked, feed-forward VCA-style bus compressor modeled on the published behavior of
classic 4000-series console bus compressors (a behavioral model, not a circuit emulation).

Controls: Threshold, Ratio (2/4/10:1), Attack (0.1/0.3/1/3/10/30 ms), Release
(0.1/0.2/0.4/0.8/1.2 s or Auto), Make-up, Mix (parallel), Voicing (G soft knee / E hard knee),
Sidechain high-pass (Off/60/90/150 Hz), LED gain-reduction meter.

DSP lives in Source/BusCompressor.h (no JUCE dependency). Test it with:
    g++ -O2 -std=c++17 tests/dsp_test.cpp -o dsp_test && ./dsp_test

Build (Windows): cmake -B build -G "Visual Studio 17 2022" -A x64
                 cmake --build build --config Release --target VCABusComp_VST3
Output: build\VCABusComp_artefacts\Release\VST3\VCA Bus Compressor.vst3

JUCE 8 is AGPLv3 / commercial and bundles the VST3 SDK: fine for personal use; distributing
requires AGPL compliance or a JUCE license.
