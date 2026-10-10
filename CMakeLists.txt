cmake_minimum_required(VERSION 3.22)

project(MB11 VERSION 0.1.0 LANGUAGES C CXX)

option(MB11_BUILD_TESTS "Build the offline tests" ON)
option(MB11_COPY_PLUGIN "Copy the built plugins into the user's plugin folders" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

# Dependencies are fetched into the build tree, never committed.
include(FetchContent)

FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        9.0.3
    GIT_SHALLOW    TRUE
    EXCLUDE_FROM_ALL)

FetchContent_Declare(clap_juce_extensions
    GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
    GIT_TAG        7adee3a1bd4684d4caa5601100e364abccff4b4d
    EXCLUDE_FROM_ALL)

FetchContent_MakeAvailable(JUCE clap_juce_extensions)

# ---------------------------------------------------------------------------
# Plugin
# ---------------------------------------------------------------------------
juce_add_plugin(MB11
    PRODUCT_NAME             "MB-11"
    COMPANY_NAME             "sscheppert-blip"
    COMPANY_WEBSITE          "https://github.com/sscheppert-blip"
    BUNDLE_ID                "io.github.sscheppert-blip.mb11"
    PLUGIN_MANUFACTURER_CODE Sbli
    PLUGIN_CODE              Mb11
    FORMATS                  AU VST3 Standalone
    IS_SYNTH                 TRUE
    NEEDS_MIDI_INPUT         TRUE
    NEEDS_MIDI_OUTPUT        TRUE
    IS_MIDI_EFFECT           FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD  ${MB11_COPY_PLUGIN}
    VST3_CATEGORIES          Instrument Drum Synth
    AU_MAIN_TYPE             kAudioUnitType_MusicDevice
    DESCRIPTION              "Analog-style bass drum synthesizer")

set(MB11_SOURCES
    Source/dsp/Engine.cpp
    Source/Presets.cpp
    Source/Parameters.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/ui/FrontPanelModel.cpp
    Source/ui/Widgets.cpp)

target_sources(MB11 PRIVATE ${MB11_SOURCES})

target_include_directories(MB11 PRIVATE Source)

target_compile_definitions(MB11
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(MB11
    PRIVATE
        clap_juce_extensions
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

clap_juce_extensions_plugin(TARGET MB11
    CLAP_ID       "io.github.sscheppert-blip.mb11"
    CLAP_FEATURES instrument drum synthesizer mono)

# JUCE writes moduleinfo.json into the VST3 after signing and the CLAP bundle is
# not signed at all; Apple Silicon hosts refuse to load bundles without a valid signature.
if(APPLE)
    # The copy into ~/Library/Audio/Plug-Ins happens before this step, so the
    # installed copy is re-signed as well.
    foreach(format VST3 CLAP)
        set(installed "$ENV{HOME}/Library/Audio/Plug-Ins/${format}/$<TARGET_BUNDLE_DIR_NAME:MB11_${format}>")
        add_custom_command(TARGET MB11_${format} POST_BUILD
            COMMAND codesign --force --deep --sign - "$<TARGET_BUNDLE_DIR:MB11_${format}>"
            COMMAND test ! -d "${installed}" || codesign --force --deep --sign - "${installed}"
            VERBATIM)
    endforeach()
endif()

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
if(MB11_BUILD_TESTS)
    juce_add_console_app(MB11Tests PRODUCT_NAME "MB11Tests")

    target_sources(MB11Tests PRIVATE ${MB11_SOURCES} tests/Tests.cpp)
    target_include_directories(MB11Tests PRIVATE Source)
    target_compile_definitions(MB11Tests
        PRIVATE
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_MODAL_LOOPS_PERMITTED=1
            JucePlugin_Name="MB-11")

    target_link_libraries(MB11Tests
        PRIVATE
            clap_juce_extensions
            juce::juce_audio_utils
            juce::juce_dsp
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)

    enable_testing()
    add_test(NAME MB11Tests COMMAND MB11Tests)
endif()
