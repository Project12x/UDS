# ==============================================================================
# UDS Test Suite
# ==============================================================================
# Uses FetchContent for Catch2 to avoid external dependencies

include(FetchContent)

# Fetch Catch2
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.4.0
)
FetchContent_MakeAvailable(Catch2)

message(STATUS "Building UDS DSP Tests")

add_executable(UDS_Tests
    DSPTests.cpp
)

target_link_libraries(UDS_Tests PRIVATE
    Catch2::Catch2WithMain
    juce::juce_dsp
    juce::juce_audio_basics
    juce::juce_core
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_data_structures
    juce::juce_events
)

target_include_directories(UDS_Tests PRIVATE
    ${CMAKE_SOURCE_DIR}/Source
)

target_compile_definitions(UDS_Tests PRIVATE
    JUCE_STANDALONE_APPLICATION=1
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
)

# Enable testing
include(CTest)
include(Catch)
catch_discover_tests(UDS_Tests)
