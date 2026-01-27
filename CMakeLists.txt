cmake_minimum_required(VERSION 3.20)
project(BeamAudioFlux VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Fetch SDL3
FetchContent_Declare(
  SDL3
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG main # Or a specific version like preview-3.1.6
)
FetchContent_MakeAvailable(SDL3)

# Source Files
file(GLOB_RECURSE CORE_SOURCES "src/core/*.cpp")
file(GLOB_RECURSE DSP_SOURCES "src/dsp/*.cpp")
file(GLOB_RECURSE UI_SOURCES "src/ui/*.cpp")
file(GLOB_RECURSE GRAPHICS_SOURCES "src/graphics/*.cpp")
set(THIRD_PARTY_SOURCES "third_party/glad.c" "third_party/miniaudio_impl.cpp")

# Include Directories
include_directories(src)
include_directories(third_party)

# Main Executable
add_executable(${PROJECT_NAME} src/main.cpp ${CORE_SOURCES} ${DSP_SOURCES} ${UI_SOURCES} ${GRAPHICS_SOURCES} ${THIRD_PARTY_SOURCES})

# SDL3 defines its own targets
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3-shared opengl32)
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3-shared OpenGL::GL)
endif()

# Standalone Tests
add_executable(test_audio_engine tests/test_audio_engine.cpp ${DSP_SOURCES})
add_executable(test_ui tests/test_ui.cpp ${UI_SOURCES})
add_executable(test_complex_dsp tests/test_complex_dsp.cpp ${DSP_SOURCES})
add_executable(test_integration tests/test_integration.cpp ${DSP_SOURCES} ${UI_SOURCES})