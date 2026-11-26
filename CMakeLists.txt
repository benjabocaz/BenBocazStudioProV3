cmake_minimum_required(VERSION 3.22)

project(LosAndesCapital VERSION 2.5.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Si usas JUCE via FetchContent o submodulo, ajusta esta ruta si corresponde
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
else()
    find_package(JUCE CONFIG REQUIRED)
endif()

juce_add_plugin(LosAndesCapital
    COMPANY_NAME "BenBocaz"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_HAS_MINI_GUI FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Bbcz
    PLUGIN_CODE Lac1
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "Los Andes Capital"
)

juce_generate_juce_header(LosAndesCapital)

target_sources(LosAndesCapital
    PRIVATE
        Source/PluginProcessor.h
        Source/PluginProcessor.cpp
        Source/PluginEditor.h
        Source/PluginEditor.cpp
        Source/DSP/AcousticEngine.h
        Source/DSP/AnalogModules.h
        Source/DSP/VocalMicModeler.h
)

# Recolecta todos los recursos visuales (.jpg, .jpeg y .png) sin tener que renombrar nada
file(GLOB_RECURSE CUSTOM_ASSETS_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Assets/*.jpg"
    "${CMAKE_CURRENT_SOURCE_DIR}/Assets/*.jpeg"
    "${CMAKE_CURRENT_SOURCE_DIR}/Assets/*.png"
)

if(CUSTOM_ASSETS_FILES)
    juce_add_binary_data(LosAndesCapitalAssets
        NAMESPACE BinaryData
        SOURCES ${CUSTOM_ASSETS_FILES}
    )

    target_compile_definitions(LosAndesCapital
        PRIVATE
            HAS_CUSTOM_ASSETS=1
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_VST3_CAN_REPLACE_VST2=0
    )

    target_link_libraries(LosAndesCapital
        PRIVATE
            LosAndesCapitalAssets
            juce::juce_audio_utils
            juce::juce_audio_processors
            juce::juce_dsp
            juce::juce_gui_basics
            juce::juce_gui_extra
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags
    )
else()
    target_compile_definitions(LosAndesCapital
        PRIVATE
            HAS_CUSTOM_ASSETS=0
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_VST3_CAN_REPLACE_VST2=0
    )

    target_link_libraries(LosAndesCapital
        PRIVATE
            juce::juce_audio_utils
            juce::juce_audio_processors
            juce::juce_dsp
            juce::juce_gui_basics
            juce::juce_gui_extra
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags
    )
endif()
