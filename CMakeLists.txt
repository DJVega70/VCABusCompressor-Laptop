cmake_minimum_required(VERSION 3.22)
project(ConsoleStrip VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.9
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(ConsoleStrip
    COMPANY_NAME "StudioTools"
    PLUGIN_MANUFACTURER_CODE Stlz
    PLUGIN_CODE Csn1
    FORMATS VST3 Standalone
    PRODUCT_NAME "Console Channel Strip"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    VST3_CATEGORIES Fx EQ
    COPY_PLUGIN_AFTER_BUILD FALSE)

juce_generate_juce_header(ConsoleStrip)

target_sources(ConsoleStrip PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(ConsoleStrip PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(ConsoleStrip
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags)

# DSP unit tests (plain C++, no JUCE):  cmake --build build --target strip_test
add_executable(strip_test EXCLUDE_FROM_ALL tests/strip_test.cpp)

# Optional: smoke test of the real processor (parameter mapping, latency)
add_executable(processor_smoke EXCLUDE_FROM_ALL
    tests/processor_smoke.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_include_directories(processor_smoke PRIVATE
    ${CMAKE_BINARY_DIR}/ConsoleStrip_artefacts/JuceLibraryCode ${juce_SOURCE_DIR}/modules)
target_compile_definitions(processor_smoke PRIVATE
    $<TARGET_PROPERTY:ConsoleStrip,COMPILE_DEFINITIONS> JUCE_STANDALONE_APPLICATION=0)
target_link_libraries(processor_smoke PRIVATE juce::juce_audio_utils juce::juce_recommended_config_flags)
