cmake_minimum_required(VERSION 3.20)
project(BeamAudioFlux VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source Files
file(GLOB_RECURSE CORE_SOURCES "src/core/*.cpp")
file(GLOB_RECURSE DSP_SOURCES "src/dsp/*.cpp")
file(GLOB_RECURSE UI_SOURCES "src/ui/*.cpp")
file(GLOB_RECURSE GRAPHICS_SOURCES "src/graphics/*.cpp")

# Include Directories
include_directories(src)
include_directories(third_party)

# Main Executable (Requires SDL3 and OpenGL)
find_package(SDL3 QUIET)
find_package(OpenGL QUIET)

if(SDL3_FOUND AND OPENGL_FOUND)
    add_executable(${PROJECT_NAME} src/main.cpp ${CORE_SOURCES} ${DSP_SOURCES} ${UI_SOURCES} ${GRAPHICS_SOURCES})
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 OpenGL::GL)
else()
    message(WARNING "SDL3 or OpenGL not found. Skipping main executable.")
endif()

# Standalone Tests (No SDL3 required)
add_executable(test_audio_engine tests/test_audio_engine.cpp ${DSP_SOURCES})
add_executable(test_ui tests/test_ui.cpp ${UI_SOURCES})
add_executable(test_complex_dsp tests/test_complex_dsp.cpp ${DSP_SOURCES})
add_executable(test_integration tests/test_integration.cpp ${DSP_SOURCES} ${UI_SOURCES})
