# Beam Audio Flux - Context & Guidelines

## Project Overview
**Beam Audio Flux** is a high-performance, proprietary Digital Audio Workstation (DAW) and audio engine built from the ground up using C++20. It follows a "no-framework" philosophy, eschewing heavy libraries like JUCE or Qt in favor of direct implementations on top of **SDL3** and **OpenGL 3.3+**.

The core engine, **BeamEngine**, features a game-loop rendering pattern, a node-based DSP graph with topological sorting, and an optimized quad-batching renderer for high-frequency UI updates and waveform visualization.

## Technical Architecture
- **`BeamHost` (src/core)**: The central application controller. Manages the SDL3 window, OpenGL context, and the main event/update/render loop.
- **`FluxGraph` & `FluxNode` (src/dsp)**: A modular DSP architecture. `FluxGraph` uses Kahn's algorithm for topological sorting to ensure correct signal flow order. Nodes process audio in stereo buffers.
- **`AudioEngine` (src/dsp)**: Bridges the `FluxGraph` with hardware audio I/O using SDL3's audio stream API.
- **`QuadBatcher` (src/graphics)**: A high-performance OpenGL batch renderer that minimizes draw calls by grouping UI elements and waveforms into single vertex buffer uploads.
- **`InputHandler` & `Component` (src/ui)**: A custom reactive UI system. `InputHandler` manages event propagation and hit-testing based on Z-order.
- **`FluxProject` (src/core)**: Handles session state and serialization to JSON using `nlohmann/json`.

## Tech Stack
- **Language**: C++20
- **Graphics**: OpenGL 3.3+ (GLAD)
- **Platform/Events/Audio**: SDL3
- **Audio Utilities**: miniaudio, dr_libs (WAV/MP3/FLAC)
- **Serialization**: nlohmann/json

## Building and Running
The project uses CMake (3.20+) and fetches SDL3 automatically via `FetchContent`.

### Build Commands
```powershell
# Create build directory
mkdir build
cd build

# Configure (Windows/MSVC)
cmake ..

# Build
cmake --build . --config Release
```

### Running the Application
The main executable is located in the `build/Release/` directory (or `build/Debug/` depending on the config).
```powershell
./build/Debug/BeamAudioFlux.exe
```

### Running Tests
Multiple test targets are available for verification:
- `test_persistence`: Validates JSON save/load logic.
- `test_audio_engine`: Validates DSP node processing.
- `test_ui`: Validates rendering and component logic.
- `test_integration`: Validates the full UI-to-DSP parameter link.

## Development Conventions
1. **Memory Management**: Use `std::shared_ptr` and `std::unique_ptr` for lifecycle management. Raw pointers are allowed for non-owning references (e.g., in `InputHandler`).
2. **DSP Performance**: Avoid memory allocations inside `FluxNode::process()`. Pre-allocate buffers during initialization or `rebuildSchedule()`.
3. **UI Coordination**: UI changes should update DSP parameters via callbacks (e.g., `onValueChanged`) to maintain decoupling.
4. **Coordinate System**: UI coordinates are pixel-based with (0,0) at the top-left.
5. **Stereo Default**: The engine currently assumes stereo (2 channels) interleaved audio for all internal processing.
6. **No-Framework**: Do not introduce JUCE, Qt, or other large frameworks. Keep the core engine dependencies minimal and lightweight.
