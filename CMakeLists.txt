cmake_minimum_required(VERSION 3.22)

project(LosAndesCapital VERSION 3.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version" FORCE)

# Si tienes JUCE como submódulo en la carpeta JUCE/ o extern/JUCE
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/JUCE/CMakeLists.txt")
    add_subdirectory(extern/JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(BenBocazStudioPro
    COMPANY_NAME "Los Andes Capital"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_CREATES_GUI TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Lac1
    PLUGIN_CODE Lacp
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "Los Andes Capital"
)

juce_generate_juce_header(BenBocazStudioPro)

target_sources(BenBocazStudioPro
    PRIVATE
        Source/PluginProcessor.h
        Source/PluginProcessor.cpp
        Source/PluginEditor.h
        Source/PluginEditor.cpp
        Source/DSP/AnalogModules.h
        Source/DSP/VocalMicModeler.h
        Source/DSP/AcousticEngine.h
)

target_include_directories(BenBocazStudioPro
    PRIVATE
        Source
        Source/DSP
)

# Recursos gráficos desde Assets/
file(GLOB_RECURSE CUSTOM_ASSET_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Assets/*.jpg"
    "${CMAKE_CURRENT_SOURCE_DIR}/Assets/*.jpeg"
    "${CMAKE_CURRENT_SOURCE_DIR}/Assets/*.png"
)

if(CUSTOM_ASSET_FILES)
    juce_add_binary_data(LosAndesCapitalAssets
        NAMESPACE BinaryData
        SOURCES ${CUSTOM_ASSET_FILES}
    )

    target_link_libraries(BenBocazStudioPro PRIVATE LosAndesCapitalAssets)
    target_compile_definitions(BenBocazStudioPro PRIVATE HAS_CUSTOM_ASSETS=1)
else()
    target_compile_definitions(BenBocazStudioPro PRIVATE HAS_CUSTOM_ASSETS=0)
endif()

target_link_libraries(BenBocazStudioPro
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
