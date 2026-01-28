cmake_minimum_required(VERSION 3.22)

project(UDS VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# ==============================================================================
# External Libraries
# ==============================================================================

# JUCE - Audio Framework
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.0
)

# nlohmann/json - Modern JSON for C++ (presets)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)

# Signalsmith Stretch - High-quality interpolation (MIT License)
FetchContent_Declare(
    signalsmith_stretch
    GIT_REPOSITORY https://github.com/Signalsmith-Audio/signalsmith-stretch.git
    GIT_TAG        main
)

# chowdsp_wdf - Wave Digital Filters for circuit modeling (BSD-3 License)
FetchContent_Declare(
    chowdsp_wdf
    GIT_REPOSITORY https://github.com/Chowdhury-DSP/chowdsp_wdf.git
    GIT_TAG        main
)

# Make libraries available
message(STATUS "Fetching JUCE...")
FetchContent_MakeAvailable(juce)

message(STATUS "Fetching nlohmann/json...")
FetchContent_MakeAvailable(json)

message(STATUS "Fetching Signalsmith Stretch...")
FetchContent_MakeAvailable(signalsmith_stretch)

message(STATUS "Fetching chowdsp_wdf...")
FetchContent_MakeAvailable(chowdsp_wdf)

# ==============================================================================
# ASIO SDK (Optional - for low-latency Windows audio)
# ==============================================================================
if(DEFINED ENV{ASIO_SDK_DIR})
    set(ASIO_SDK_DIR $ENV{ASIO_SDK_DIR})
elseif(NOT DEFINED ASIO_SDK_DIR)
    set(ASIO_SDK_DIR "" CACHE PATH "Path to Steinberg ASIO SDK")
endif()

if(ASIO_SDK_DIR AND EXISTS "${ASIO_SDK_DIR}/common/iasiodrv.h")
    message(STATUS "ASIO SDK found at: ${ASIO_SDK_DIR}")
    set(JUCE_ASIO_ENABLED 1)
else()
    message(STATUS "ASIO SDK not found. Building without ASIO support.")
    set(JUCE_ASIO_ENABLED 0)
endif()

# ==============================================================================
# Plugin Configuration
# ==============================================================================
juce_add_plugin(UDS
    COMPANY_NAME "WorriedWalrus"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE "WwRs"
    PLUGIN_CODE "UDSd"
    FORMATS Standalone VST3
    PRODUCT_NAME "UDS - Universal Delay System"
)

# ==============================================================================
# Font Resources (Binary Data)
# ==============================================================================
juce_add_binary_data(UDS_BinaryData
    SOURCES
        Resources/Fonts/Inter-Regular.ttf
        Resources/Fonts/Inter-Medium.ttf
        Resources/Fonts/Inter-Bold.ttf
        Resources/Fonts/JetBrainsMono-Regular.ttf
        Resources/Fonts/SpaceGrotesk-Regular.ttf
        Resources/Fonts/SpaceGrotesk-Medium.ttf
        Resources/Fonts/SpaceGrotesk-Bold.ttf
)

target_link_libraries(UDS PRIVATE UDS_BinaryData)

# ==============================================================================
# Sources
# ==============================================================================
target_sources(UDS PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    
    # Core DSP (header-only)
    Source/Core/DelayAlgorithm.h
    Source/Core/DelayBandNode.h
    Source/Core/DelayMatrix.h
    Source/Core/FilterSection.h
    Source/Core/LFOModulator.h
    Source/Core/RoutingGraph.h
    Source/Core/SafetyLimiter.h
    
    # UI (header-only)
    Source/UI/Typography.h
    Source/UI/LookAndFeel.h
    Source/UI/BandParameterPanel.h
    Source/UI/NodeVisual.h
    Source/UI/BandNodeComponent.h
    Source/UI/NodeEditorCanvas.h
    Source/UI/MainComponent.h
)

# ==============================================================================
# Compile Definitions
# ==============================================================================
target_compile_definitions(UDS PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_ASIO=${JUCE_ASIO_ENABLED}
    JUCE_WASAPI=1
    JUCE_DIRECTSOUND=0
)

# Add ASIO SDK include path if available
if(JUCE_ASIO_ENABLED)
    target_include_directories(UDS PRIVATE "${ASIO_SDK_DIR}/common")
endif()

# ==============================================================================
# Link Libraries
# ==============================================================================
target_link_libraries(UDS PRIVATE
    # JUCE modules
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_basics
    juce::juce_graphics
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    
    # External libraries
    nlohmann_json::nlohmann_json
)

# Header-only DSP libraries (include paths only)
target_include_directories(UDS PRIVATE
    ${signalsmith_stretch_SOURCE_DIR}
    ${chowdsp_wdf_SOURCE_DIR}/include
)

# ==============================================================================
# Compile Options
# ==============================================================================
if(MSVC)
    target_compile_options(UDS PRIVATE /W4 /MP)
else()
    target_compile_options(UDS PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ==============================================================================
# Tests (Optional)
# ==============================================================================
option(UDS_BUILD_TESTS "Build the DSP test suite" OFF)
if(UDS_BUILD_TESTS)
    add_subdirectory(Tests)
endif()

