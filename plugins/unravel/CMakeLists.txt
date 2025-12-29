# ==============================================================================
# THREADBARE UNRAVEL PLUGIN
# Algorithmic reverb with Ghost Engine granular synthesis
# ==============================================================================

set(UNRAVEL_SOURCE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/Source)
set(UNRAVEL_DSP_DIR ${UNRAVEL_SOURCE_ROOT}/DSP)
set(UNRAVEL_PROCESSORS_DIR ${UNRAVEL_SOURCE_ROOT}/Processors)
set(UNRAVEL_UI_DIR ${UNRAVEL_SOURCE_ROOT}/UI)

file(GLOB_RECURSE UNRAVEL_DSP_SOURCES CONFIGURE_DEPENDS "${UNRAVEL_DSP_DIR}/*.cpp")
file(GLOB_RECURSE UNRAVEL_PROCESSOR_SOURCES CONFIGURE_DEPENDS "${UNRAVEL_PROCESSORS_DIR}/*.cpp")
file(GLOB_RECURSE UNRAVEL_UI_SOURCES CONFIGURE_DEPENDS "${UNRAVEL_UI_DIR}/*.cpp")

if(UNRAVEL_DSP_SOURCES STREQUAL "")
    message(FATAL_ERROR "No DSP sources found in ${UNRAVEL_DSP_DIR}.")
endif()

# DSP library (pure C++ logic, no JUCE UI dependencies)
add_library(unravel_dsp STATIC ${UNRAVEL_DSP_SOURCES})
target_include_directories(unravel_dsp PUBLIC ${UNRAVEL_SOURCE_ROOT})
target_compile_features(unravel_dsp PUBLIC cxx_std_20)
target_link_libraries(unravel_dsp PUBLIC juce::juce_dsp)

# Add frontend resources as binary data
file(GLOB_RECURSE UI_RESOURCES CONFIGURE_DEPENDS "${UNRAVEL_UI_DIR}/frontend/dist/*")
juce_add_binary_data(UnravelResources
    NAMESPACE UnravelResources
    SOURCES ${UI_RESOURCES}
)

# Preserve folder structure for resources
set_source_files_properties(${UI_RESOURCES} PROPERTIES
    HEADER_FILE_ONLY FALSE
)

juce_add_plugin(ThreadbareUnravel
    COMPANY_NAME "threadbare"
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
    PRODUCT_NAME "unravel"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/assets/app-icon.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/assets/app-icon.png"
)

juce_generate_juce_header(ThreadbareUnravel)

target_sources(ThreadbareUnravel PRIVATE ${UNRAVEL_PROCESSOR_SOURCES} ${UNRAVEL_UI_SOURCES})
target_include_directories(ThreadbareUnravel PRIVATE ${UNRAVEL_SOURCE_ROOT})
target_compile_features(ThreadbareUnravel PUBLIC cxx_std_20)

target_link_libraries(ThreadbareUnravel
    PRIVATE
        unravel_dsp
        UnravelResources
        threadbare_core
        juce::juce_audio_utils
        juce::juce_gui_extra
        juce::juce_recommended_warning_flags
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
)

target_compile_definitions(ThreadbareUnravel
    PRIVATE
        JUCE_VST3_CAN_REPLACE_VST2=0
)

