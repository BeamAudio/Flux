cmake_minimum_required(VERSION 3.20)
project(BeamAudioFlux VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source Files
file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(${PROJECT_NAME} ${SOURCES})

# Include Directories
target_include_directories(${PROJECT_NAME} PRIVATE 
    src 
    third_party
)

# Link Dependencies (Assumes libraries are installed or provided)
find_package(SDL3 REQUIRED)
find_package(OpenGL REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    SDL3::SDL3
    OpenGL::GL
)
