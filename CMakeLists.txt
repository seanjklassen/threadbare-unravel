cmake_minimum_required(VERSION 3.22)

# ==============================================================================
# THREADBARE MONOREPO ROOT
# Supports multiple plugins: Unravel, Weave, etc.
# ==============================================================================
project(Threadbare VERSION 0.2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ==============================================================================
# JUCE DEPENDENCY (shared across all plugins)
# ==============================================================================
include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.10
    GIT_SHALLOW ON
)

FetchContent_MakeAvailable(JUCE)

# ==============================================================================
# SHARED MODULES
# ==============================================================================
add_subdirectory(shared/core)

# ==============================================================================
# PLUGINS
# ==============================================================================
add_subdirectory(plugins/unravel)
