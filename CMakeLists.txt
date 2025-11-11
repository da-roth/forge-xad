cmake_minimum_required(VERSION 3.15)
project(ForgeXADIntegration VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(FORGE_XAD_BUILD_EXAMPLES "Build example programs" ON)
option(FORGE_XAD_BUILD_TESTS "Build test suite" ON)
option(FORGE_XAD_FETCH_SUBMODULES "Automatically fetch/update submodules" ON)

# Automatically initialize submodules if requested
if(FORGE_XAD_FETCH_SUBMODULES)
    message(STATUS "Updating submodules...")
    execute_process(
        COMMAND git submodule update --init --recursive
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        RESULT_VARIABLE GIT_SUBMOD_RESULT
    )
    if(NOT GIT_SUBMOD_RESULT EQUAL "0")
        message(WARNING "git submodule update failed with ${GIT_SUBMOD_RESULT}")
    endif()
endif()

# Check that submodules exist
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/forge/CMakeLists.txt")
    message(FATAL_ERROR "Forge submodule not found. Please run:\n  git submodule update --init --recursive")
endif()

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/XAD/CMakeLists.txt")
    message(FATAL_ERROR "XAD submodule not found. Please run:\n  git submodule update --init --recursive")
endif()

# Configure Forge to use dynamic runtime (match XAD's default)
set(FORGE_USE_STATIC_RUNTIME OFF CACHE BOOL "Use dynamic runtime (/MD) to match XAD" FORCE)

# Add Forge library
add_subdirectory(extern/forge)

# Add XAD library
add_subdirectory(extern/XAD)

# Create the bridge library
add_library(forge_xad_bridge
    src/xad_tape_converter.cpp
    src/operation_inference.cpp
)

target_include_directories(forge_xad_bridge PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/forge/src
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/XAD/src
)

# Link against Forge and XAD
target_link_libraries(forge_xad_bridge PUBLIC
    forge
    xad
)

# Set compile options
target_compile_features(forge_xad_bridge PUBLIC cxx_std_17)

# Export targets for use in build tree (like XAD does)
# This allows other projects to use forge_xad::bridge via add_subdirectory
export(EXPORT ForgeXADTargets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/ForgeXADTargets.cmake"
    NAMESPACE forge_xad::
)

# Add examples
if(FORGE_XAD_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Add tests
if(FORGE_XAD_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation (optional)
install(TARGETS forge_xad_bridge
    EXPORT ForgeXADTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include
)

# Install the export set with namespace (for installed use)
install(EXPORT ForgeXADTargets
    FILE ForgeXADTargets.cmake
    NAMESPACE forge_xad::
    DESTINATION lib/cmake/ForgeXAD
)
