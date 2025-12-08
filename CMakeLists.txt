cmake_minimum_required(VERSION 3.22)

project(ThreadbareUnravel VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.10
    GIT_SHALLOW ON
)

FetchContent_MakeAvailable(JUCE)

set(SOURCE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/Source)
set(DSP_DIR ${SOURCE_ROOT}/DSP)
set(PROCESSORS_DIR ${SOURCE_ROOT}/Processors)
set(UI_DIR ${SOURCE_ROOT}/UI)

file(GLOB_RECURSE THREADBARE_DSP_SOURCES CONFIGURE_DEPENDS "${DSP_DIR}/*.cpp")
file(GLOB_RECURSE THREADBARE_PROCESSOR_SOURCES CONFIGURE_DEPENDS "${PROCESSORS_DIR}/*.cpp")
file(GLOB_RECURSE THREADBARE_UI_SOURCES CONFIGURE_DEPENDS "${UI_DIR}/*.cpp")

if(THREADBARE_DSP_SOURCES STREQUAL "")
    message(FATAL_ERROR "No DSP sources found in ${DSP_DIR}.")
endif()

add_library(threadbare_dsp STATIC ${THREADBARE_DSP_SOURCES})
target_include_directories(threadbare_dsp PUBLIC ${SOURCE_ROOT})
target_compile_features(threadbare_dsp PUBLIC cxx_std_20)
target_link_libraries(threadbare_dsp PUBLIC juce::juce_dsp)

# Add frontend resources as binary data
file(GLOB_RECURSE UI_RESOURCES CONFIGURE_DEPENDS "${UI_DIR}/frontend/dist/*")
juce_add_binary_data(UnravelResources
    NAMESPACE UnravelResources
    SOURCES ${UI_RESOURCES}
)

# Preserve folder structure for resources
set_source_files_properties(${UI_RESOURCES} PROPERTIES
    HEADER_FILE_ONLY FALSE
)

juce_add_plugin(ThreadbareUnravel
    COMPANY_NAME "Threadbare"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    VST3_CAN_REPLACE_VST2 FALSE
    PLUGIN_MANUFACTURER_CODE TrbR
    PLUGIN_CODE Unrv
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "ThreadbareUnravel"
)

juce_generate_juce_header(ThreadbareUnravel)

target_sources(ThreadbareUnravel PRIVATE ${THREADBARE_PROCESSOR_SOURCES} ${THREADBARE_UI_SOURCES})
target_include_directories(ThreadbareUnravel PRIVATE ${SOURCE_ROOT})
target_compile_features(ThreadbareUnravel PUBLIC cxx_std_20)

target_link_libraries(ThreadbareUnravel
    PRIVATE
        threadbare_dsp
        UnravelResources
        juce::juce_audio_utils
        juce::juce_recommended_warning_flags
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
)

target_compile_definitions(ThreadbareUnravel
    PRIVATE
        JUCE_VST3_CAN_REPLACE_VST2=0
)

