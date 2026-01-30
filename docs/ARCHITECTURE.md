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
Uses **SDL3** for low-latency audio I/O. It holds a reference to the `FluxGraph` and triggers the `process()` chain inside the SDL audio callback.

## 3. Rendering Engine (`QuadBatcher`)

To achieve high performance for complex UI and waveforms, the project uses a custom OpenGL batch renderer.

- **Batching**: Instead of one draw call per rectangle, `QuadBatcher` accumulates vertices into a large buffer and issues a single `glDrawElements` call when full or when `flush()` is called.
- **SDF Shaders**: Advanced shaders support High-Fidelity Rounded Rectangles and Anti-Aliased lines using Signed Distance Fields (SDF).
- **Gradients**: Supports vertex-color interpolation for smooth vertical gradients across all primitives.

## 4. Unified UI System

### 4.1 `Component` Base Class
The central building block of the UI. Inspired by JUCE, it provides:
- **Hierarchical Management**: `addChildComponent` allows building complex nested UIs.
- **Event Propagation**: Mouse events are automatically propagated through the hierarchy with correct hit-testing.
- **Paint/Resized Pattern**: Subclasses override `paint(QuadBatcher&)` for drawing and `resized()` for layout logic.
- **Absolute Coordinates**: All components use absolute screen coordinates for robust hit-testing and rendering.

### 4.2 `LookAndFeel` System
Rendering logic is decoupled from component logic.
- **Theming**: A global or component-specific `LookAndFeel` can be applied to change the entire DAW's aesthetic.
- **Brand Colors**: The system is pre-configured with the official brand identity:
    - **Emerald Green (#219e6c)**: Primary action color and signal paths.
    - **Deep Red (#8f0707)**: Recording and critical UI elements.
    - **Black/Dark Gray**: Foundations and background panels.
    - **White**: Text and high-contrast accents.

### 4.3 `InputHandler`
The `InputHandler` translates SDL events into component-specific events, managing focus and Z-order.

## 5. Project Management (`FluxProject`)
`FluxProject` is the central state container. It holds the `FluxGraph` and is responsible for serialization/deserialization via `nlohmann::json`.

## 6. Development Guidelines
- **UI First**: Always use `addChildComponent` to ensure children receive events.
- **Absolute Layout**: In `resized()`, set child bounds relative to your own `m_bounds.x` and `m_bounds.y`.
- **DSP Safety**: Never allocate memory inside a `FluxNode::process()` call.