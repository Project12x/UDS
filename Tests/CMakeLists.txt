# Optional test executable
# Requires Catch2 to be available

find_package(Catch2 3 QUIET)

if(Catch2_FOUND)
    message(STATUS "Catch2 found - building tests")
    
    add_executable(UDS_Tests
        DSPTests.cpp
    )
    
    target_link_libraries(UDS_Tests PRIVATE
        Catch2::Catch2WithMain
    )
    
    target_include_directories(UDS_Tests PRIVATE
        ${CMAKE_SOURCE_DIR}/Source
    )
    
    # Enable testing
    include(CTest)
    include(Catch)
    catch_discover_tests(UDS_Tests)
else()
    message(STATUS "Catch2 not found - tests will not be built")
    message(STATUS "To build tests, install Catch2: vcpkg install catch2")
endif()
