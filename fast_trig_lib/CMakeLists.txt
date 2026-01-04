cmake_minimum_required(VERSION 3.20)
project(FastTrig VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Option for building tests and examples
option(BUILD_TESTS "Build test programs" ON)
option(BUILD_EXAMPLES "Build example programs" ON)
option(ENABLE_BENCHMARKS "Enable benchmark code" ON)

# FastTrig is a header-only library
add_library(FastTrig INTERFACE)
target_include_directories(FastTrig INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Compiler flags for optimization
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(FastTrig INTERFACE
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Release>:-march=native>
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

# Build examples
if(BUILD_EXAMPLES)
    add_executable(examples examples/examples.cpp)
    target_link_libraries(examples PRIVATE FastTrig)
    
    if(ENABLE_BENCHMARKS)
        target_compile_definitions(examples PRIVATE ENABLE_BENCHMARKS)
    endif()
endif()

# Build tests
if(BUILD_TESTS)
    enable_testing()
    
    add_executable(test_fast_trig tests/test_fast_trig.cpp)
    target_link_libraries(test_fast_trig PRIVATE FastTrig)
    
    # Link math library for standard math functions in tests
    if(UNIX)
        target_link_libraries(test_fast_trig PRIVATE m)
    endif()
    
    add_test(NAME FastTrigTest COMMAND test_fast_trig)
endif()

# Installation
install(TARGETS FastTrig
    EXPORT FastTrigTargets
    INCLUDES DESTINATION include
)

install(FILES include/fast_trig.hpp
    DESTINATION include
)

install(EXPORT FastTrigTargets
    FILE FastTrigConfig.cmake
    NAMESPACE FastTrig::
    DESTINATION lib/cmake/FastTrig
)

# Package configuration
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    FastTrigConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/FastTrigConfigVersion.cmake
    DESTINATION lib/cmake/FastTrig
)

# Doxygen documentation (optional)
find_package(Doxygen)
if(DOXYGEN_FOUND)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
    
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
endif()

# Print configuration summary
message(STATUS "FastTrig Configuration:")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Build Tests: ${BUILD_TESTS}")
message(STATUS "  Build Examples: ${BUILD_EXAMPLES}")
message(STATUS "  Enable Benchmarks: ${ENABLE_BENCHMARKS}")
