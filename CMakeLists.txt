cmake_minimum_required(VERSION 3.22)

project(WON_KNOBBER VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- JUCE via FetchContent (no submodule, no system install) ----
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.13
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# ---- Plugin formats: VST3 everywhere, AU + AUv3 on macOS ----
set(WK_FORMATS VST3)
if(APPLE)
    list(APPEND WK_FORMATS AU AUv3)
endif()

juce_add_plugin(WonKnobber
    PRODUCT_NAME              "WON-KNOBBER"
    COMPANY_NAME              "kalijmeeks-maker"
    PLUGIN_MANUFACTURER_CODE  Kjmk
    PLUGIN_CODE               Wknb
    FORMATS                   ${WK_FORMATS}
    IS_SYNTH                  FALSE
    NEEDS_MIDI_INPUT          FALSE
    NEEDS_MIDI_OUTPUT         FALSE
    IS_MIDI_EFFECT            FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD   FALSE
)

# ---- Sources (recursive under Source/) ----
file(GLOB_RECURSE WK_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h"
)
target_sources(WonKnobber PRIVATE ${WK_SOURCES})

# ---- Embedded binary assets (PRO faceplate chassis + all 7 gem filmstrips) ----
# Embedding all 7 (~43 MB total) keeps the plugin self-contained offline; the
# "choose your stone" picker swaps between them at runtime via KnobLookAndFeel.
juce_add_binary_data(WonKnobberBinaryData SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/faceplate_pro_960x600.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Fonts/Orbitron.ttf"
    "${CMAKE_CURRENT_SOURCE_DIR}/sprites/knob_diamond_256.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/sprites/knob_onyx_256.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/sprites/knob_sapphire_256.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/sprites/knob_emerald_256.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/sprites/knob_ruby_256.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/sprites/knob_amethyst_256.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/sprites/knob_citrine_256.png"
)

target_include_directories(WonKnobber PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")

target_compile_definitions(WonKnobber PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(WonKnobber
    PRIVATE
        WonKnobberBinaryData
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

# ---- Treat warnings as errors on OUR target only (never on JUCE itself) ----
if(MSVC)
    target_compile_options(WonKnobber PRIVATE /W4 /WX)
else()
    target_compile_options(WonKnobber PRIVATE -Wall -Wextra -Werror)
endif()
