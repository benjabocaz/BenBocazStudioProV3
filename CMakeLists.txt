cmake_minimum_required(VERSION 3.22)
project(BenBocazStudioPro VERSION 2.5.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(BenBocazStudioPro
    COMPANY_NAME "Ben Bocaz Audio"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Bbau
    PLUGIN_CODE Bbsp
    FORMATS AU
    PRODUCT_NAME "Ben Bocaz Studio Pro"
)

target_sources(BenBocazStudioPro
    PRIVATE
        Source/DSP/AnalogModules.h
        Source/DSP/VocalMicModeler.h
        Source/DSP/AcousticEngine.h
        Source/PluginProcessor.h
        Source/PluginProcessor.cpp
        Source/PluginEditor.h
        Source/PluginEditor.cpp
)

file(GLOB PLUGIN_ASSETS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Assets/*.jpg"
    "${CMAKE_CURRENT_SOURCE_DIR}/Assets/*.jpeg"
    "${CMAKE_CURRENT_SOURCE_DIR}/Assets/*.png"
)

if(PLUGIN_ASSETS)
    list(LENGTH PLUGIN_ASSETS ASSET_COUNT)
    message(STATUS "=== INCRUSTANDO ${ASSET_COUNT} ASSETS EN BINARYDATA ===")
    foreach(asset ${PLUGIN_ASSETS})
        message(STATUS "  -> Asset: ${asset}")
    endforeach()

    juce_add_binary_data(BenBocazAssets SOURCES ${PLUGIN_ASSETS})
    set_target_properties(BenBocazAssets PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
    target_link_libraries(BenBocazStudioPro PRIVATE BenBocazAssets)
    target_compile_definitions(BenBocazStudioPro PRIVATE HAS_CUSTOM_ASSETS=1)
else()
    message(WARNING "No se encontraron archivos de imagen (.jpg/.jpeg/.png) en Assets/. Se usaran los graficos de respaldo.")
endif()

target_compile_definitions(BenBocazStudioPro
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_MODAL_LOOPS_PERMITTED=0
)

target_link_libraries(BenBocazStudioPro
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_formats
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
