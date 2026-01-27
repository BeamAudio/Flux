# Beam Audio Flux - Technical Architecture

## 1. Overview
Beam Audio Flux is a hybrid DAW (Digital Audio Workstation) designed for real-time audio manipulation and "Splicing" (time-domain editing). It uses a modular architecture where the DSP (Digital Signal Processing) logic is decoupled from the UI through a graph-based abstraction layer.

## 2. Audio Engine & DSP Abstraction (`FluxGraph`)

### 2.1 The `FluxNode`
All audio processing entities must inherit from `FluxNode`. A node defines:
- **Input Ports**: Buffers where incoming audio is summed.
- **Output Ports**: Buffers where processed audio is stored.
- **Process Method**: The core DSP loop.

### 2.2 `FluxGraph`
The `FluxGraph` manages the lifecycle and connectivity of nodes.
- **Topological Sorting**: On every connection change, the graph rebuilds a "schedule" using Kahn's algorithm to ensure nodes are processed in the correct order.
- **Propagation**: The graph handles the transfer of data from a source node's output buffer to the destination node's input buffer.

### 2.3 `AudioEngine`
Uses **SDL3** for low-latency audio I/O. It holds a reference to the `FluxGraph` and triggers the `process()` chain inside the SDL audio callback (via `SDL_PutAudioStreamData`).

## 3. Rendering Engine (`QuadBatcher`)

To achieve high performance for complex UI and waveforms, the project uses a custom OpenGL batch renderer.

- **Batching**: Instead of one draw call per rectangle, `QuadBatcher` accumulates vertices into a large buffer and issues a single `glDrawElements` call when full or when `flush()` is called.
- **Shaders**: A simple GLSL vertex/fragment shader pair handles orthographic projection and vertex colors.
- **Coordinate System**: Uses pixel-perfect screen space coordinates (Top-Left is 0,0).

## 4. UI Component System

### 4.1 `Component` Base Class
Every UI element inherits from `Component`. It provides:
- **Bounds**: A `Rect` defining the position and size.
- **Event Callbacks**: `onMouseDown`, `onMouseUp`, `onMouseMove`.
- **Visibility/Draggable Flags**: Built-in support for UI interaction.

### 4.2 `InputHandler`
The `InputHandler` translates SDL events into component-specific events. It supports:
- **Focus**: Tracking which component is currently being dragged or interacted with.
- **Z-Order**: Iterating components in reverse order for correct hit testing (top-most first).

## 5. Project Management (`FluxProject`)
`FluxProject` is the central state container. It holds the `FluxGraph` and is responsible for serialization/deserialization via `nlohmann::json`. This allows for a clean "Save/Load" implementation by capturing the entire graph state and UI positions.

## 6. Current Limitations & Roadmap
- **Interactivity**: Many UI components are currently "view-only" and need their `onMouseDown` handlers implemented to link back to the `FluxGraph` parameters.
- **Waveform Visualization**: Waveform data needs to be pre-calculated by `WavReader` and stored in a vertex buffer for the `QuadBatcher` to render efficiently.
- **Threading**: The `FluxGraph` processing currently runs on the same thread as the SDL audio callback. Future versions should consider parallel node processing for complex graphs.
