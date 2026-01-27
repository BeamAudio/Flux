# Beam Audio Flux
## Proprietary Engine Architecture (No-Framework) v2.0

**Beam Audio Flux** is a next-generation, high-performance Digital Audio Workstation (DAW) built on a completely proprietary, "no-framework" C++20 engine known as **BeamEngine**.

### Core Pillars
- **No-Framework Design**: Built directly on SDL3 and OpenGL, avoiding JUCE/Qt overhead.
- **Game-Loop Pattern**: Continuous update/render cycle for fluid, reactive UI.
- **Node-Based DSP**: Modular signal flow with low-latency `AudioNode` architecture.
- **Disk-Streaming**: Efficient `dr_libs` integration for massive session handling.

### Tech Stack
- **Language**: C++20
- **Platform/Input**: SDL3 (Zlib)
- **Audio I/O**: miniaudio (MIT)
- **Graphics**: OpenGL 3.3+ (GLAD)
- **File I/O**: dr_libs (WAV/MP3/FLAC)

### Project Structure
- `src/core/`: Host shell, windowing, and event loop.
- `src/dsp/`: Audio engine, nodes, and signal graph.
- `src/graphics/`: OpenGL renderer, quad batcher, and shaders.
- `src/ui/`: Input handling and layout logic.
- `third_party/`: External header-only libraries.
