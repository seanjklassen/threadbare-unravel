# ==============================================================================
# THREADBARE SHARED CORE
# Common processor infrastructure, WebView bridge, and utilities
# ==============================================================================

set(THREADBARE_CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/WebViewBridge.cpp
)

add_library(threadbare_core STATIC ${THREADBARE_CORE_SOURCES})
target_include_directories(threadbare_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(threadbare_core PUBLIC cxx_std_20)
target_link_libraries(threadbare_core PUBLIC 
    juce::juce_audio_processors
    juce::juce_gui_extra
)
