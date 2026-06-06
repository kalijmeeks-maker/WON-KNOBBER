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

# ---- RTNeural (BSD-3) via FetchContent — neural character stage. STL backend = header-only,
#      no Eigen submodule, smallest dep graph for CI (per the RTNeural-vs-ONNX spike). ----
set(RTNEURAL_STL ON CACHE BOOL "" FORCE)
FetchContent_Declare(
    RTNeural
    GIT_REPOSITORY https://github.com/jatinchowdhury18/RTNeural.git
    GIT_TAG        1fb1f075a5d66e85bfc8f488c3f3626840cb3a1d
)
FetchContent_MakeAvailable(RTNeural)

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
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/faceplate_pro_960x612.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/rear-panel-background-960x612.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Fonts/Orbitron.ttf"
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Fonts/Hultog_Engraved.ttf"
    "${CMAKE_CURRENT_SOURCE_DIR}/sprites/knob_diamond_256.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/sprites/knob_onyx_256.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/sprites/knob_sapphire_256.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/sprites/knob_emerald_256.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/sprites/knob_ruby_256.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/sprites/knob_amethyst_256.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/sprites/knob_citrine_256.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/factory_presets/tape_head.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/factory_presets/console_glue.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/factory_presets/furnace.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/factory_presets/velvet.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/factory_presets/sunday_drive.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/factory_presets/tube_warm.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/factory_presets/diode_bite.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/factory_presets/transformer.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/IRs/ir_flat.wav"
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/IRs/ir_studio_ribbon.wav"
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/IRs/ir_vintage_4x12.wav"
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/IRs/ir_console_box.wav"
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/IRs/ir_old_radio.wav"
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/IRs/ir_iron_core.wav"
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Models/model_tape.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Models/model_valve.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Models/model_transistor.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Models/model_iron.json"
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
        RTNeural
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

# RTNeural's own headers must not trip OUR -Werror — treat its includes as SYSTEM.
get_target_property(_rtneural_inc RTNeural INTERFACE_INCLUDE_DIRECTORIES)
if(_rtneural_inc)
    target_include_directories(WonKnobber SYSTEM PRIVATE ${_rtneural_inc})
endif()

# ---- Treat warnings as errors on OUR target only (never on JUCE itself) ----
if(MSVC)
    target_compile_options(WonKnobber PRIVATE /W4 /WX)
else()
    target_compile_options(WonKnobber PRIVATE -Wall -Wextra -Werror)
endif()

# ---- Headless GUI render harness (front faceplate -> PNG, no host/window) ----
# OFF by default so normal/CI plugin builds aren't slowed; enable with
# -DWK_BUILD_RENDER_HARNESS=ON. Build target: WonKnobberRender.
option(WK_BUILD_RENDER_HARNESS "Build the headless GUI render harness (WonKnobberRender)" OFF)
if(WK_BUILD_RENDER_HARNESS)
    juce_add_console_app(WonKnobberRender PRODUCT_NAME "WonKnobberRender")

    file(GLOB_RECURSE WK_HARNESS_SRC CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp")
    list(FILTER WK_HARNESS_SRC EXCLUDE REGEX "/PluginProcessor\\.cpp$")
    list(FILTER WK_HARNESS_SRC EXCLUDE REGEX "/PluginEditor\\.cpp$")
    target_sources(WonKnobberRender PRIVATE ${WK_HARNESS_SRC} "${CMAKE_CURRENT_SOURCE_DIR}/tools/RenderHarness.cpp")

    target_include_directories(WonKnobberRender PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")
    target_compile_definitions(WonKnobberRender PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_STANDALONE_APPLICATION=1
    )
    target_link_libraries(WonKnobberRender
        PRIVATE
            WonKnobberBinaryData
            juce::juce_audio_utils
            juce::juce_dsp
            RTNeural
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags
    )
    get_target_property(_rtneural_inc_h RTNeural INTERFACE_INCLUDE_DIRECTORIES)
    if(_rtneural_inc_h)
        target_include_directories(WonKnobberRender SYSTEM PRIVATE ${_rtneural_inc_h})
    endif()
endif()
