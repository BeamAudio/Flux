cmake_minimum_required(VERSION 3.20)
project(BeamAudioFlux VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Fetch SDL3
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE) # Disable tests to speed up
FetchContent_Declare(
  SDL3
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG main 
)
FetchContent_MakeAvailable(SDL3)

# Source Files
file(GLOB_RECURSE SESSION_SOURCES "src/session/*.cpp")
file(GLOB_RECURSE ENGINE_SOURCES "src/engine/*.cpp")
file(GLOB_RECURSE INTERFACE_SOURCES "src/interface/*.cpp")
file(GLOB_RECURSE RENDER_SOURCES "src/render/*.cpp")
file(GLOB_RECURSE APPLICATION_SOURCES "src/application/*.cpp")
set(THIRD_PARTY_SOURCES "third_party/glad.c")

# Include Directories
include_directories(src)
include_directories(third_party)

# Main Executable
add_executable(${PROJECT_NAME} src/main.cpp ${SESSION_SOURCES} ${ENGINE_SOURCES} ${INTERFACE_SOURCES} ${RENDER_SOURCES} ${APPLICATION_SOURCES} ${THIRD_PARTY_SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES 
    WIN32_EXECUTABLE FALSE
    LINK_FLAGS "/SUBSYSTEM:CONSOLE"
)

# SDL3 defines its own targets
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3-static opengl32 user32 gdi32 winmm imm32 ole32 oleaut32 version uuid advapi32 shell32 setupapi)
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3-static OpenGL::GL)
endif()

# Standalone Tests
add_executable(test_persistence tests/test_persistence.cpp)
add_executable(test_juce_like_api tests/test_juce_like_api.cpp ${ENGINE_SOURCES} ${APPLICATION_SOURCES})

target_include_directories(test_juce_like_api PRIVATE src)

if(WIN32)
    target_link_libraries(test_juce_like_api PRIVATE SDL3::SDL3-static)
else()
    target_link_libraries(test_juce_like_api PRIVATE SDL3::SDL3-static)
endif()

# target_link_libraries(test_persistence PRIVATE ${TEST_LIBS})
