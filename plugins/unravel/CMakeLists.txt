# ==============================================================================
# THREADBARE UNRAVEL PLUGIN
# Algorithmic reverb with Ghost Engine granular synthesis
# ==============================================================================

set(UNRAVEL_SOURCE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/Source)
set(UNRAVEL_DSP_DIR ${UNRAVEL_SOURCE_ROOT}/DSP)
set(UNRAVEL_PROCESSORS_DIR ${UNRAVEL_SOURCE_ROOT}/Processors)
set(UNRAVEL_UI_DIR ${UNRAVEL_SOURCE_ROOT}/UI)
set(UNRAVEL_CONFIG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/config)

# ==============================================================================
# PARAMETER GENERATION (params.json -> C++ header + JS module)
# ==============================================================================
set(PARAMS_JSON ${UNRAVEL_CONFIG_DIR}/params.json)
set(PARAMS_CPP_OUTPUT ${UNRAVEL_SOURCE_ROOT}/UnravelGeneratedParams.h)
set(PARAMS_JS_OUTPUT ${UNRAVEL_UI_DIR}/frontend/src/generated/params.js)
set(PARAMS_GENERATOR ${CMAKE_SOURCE_DIR}/shared/scripts/generate_params.js)

# Find Node.js
find_program(NODE_EXECUTABLE node HINTS /usr/local/bin /opt/homebrew/bin)
if(NOT NODE_EXECUTABLE)
    message(WARNING "Node.js not found - parameter generation will be skipped")
else()
    add_custom_command(
        OUTPUT ${PARAMS_CPP_OUTPUT} ${PARAMS_JS_OUTPUT}
        COMMAND ${NODE_EXECUTABLE} ${PARAMS_GENERATOR} ${PARAMS_JSON} ${PARAMS_CPP_OUTPUT} ${PARAMS_JS_OUTPUT}
        DEPENDS ${PARAMS_JSON} ${PARAMS_GENERATOR}
        COMMENT "Generating parameter definitions from params.json"
        VERBATIM
    )
    
    # Custom target to trigger generation
    add_custom_target(UnravelGenerateParams
        DEPENDS ${PARAMS_CPP_OUTPUT} ${PARAMS_JS_OUTPUT}
    )
endif()

# ==============================================================================
# DSP LIBRARY
# ==============================================================================
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

# ==============================================================================
# FRONTEND RESOURCES
# ==============================================================================
file(GLOB_RECURSE UI_RESOURCES CONFIGURE_DEPENDS "${UNRAVEL_UI_DIR}/frontend/dist/*")
juce_add_binary_data(UnravelResources
    NAMESPACE UnravelResources
    SOURCES ${UI_RESOURCES}
)

# Preserve folder structure for resources
set_source_files_properties(${UI_RESOURCES} PROPERTIES
    HEADER_FILE_ONLY FALSE
)

# ==============================================================================
# PLUGIN TARGET
# ==============================================================================
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

# Include generated header in sources
target_sources(ThreadbareUnravel PRIVATE 
    ${UNRAVEL_PROCESSOR_SOURCES} 
    ${UNRAVEL_UI_SOURCES}
    ${PARAMS_CPP_OUTPUT}
)
target_include_directories(ThreadbareUnravel PRIVATE ${UNRAVEL_SOURCE_ROOT})
target_compile_features(ThreadbareUnravel PUBLIC cxx_std_20)

# Ensure parameters are generated before building
if(TARGET UnravelGenerateParams)
    add_dependencies(ThreadbareUnravel UnravelGenerateParams)
endif()

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
