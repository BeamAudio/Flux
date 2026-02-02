# Beam Audio Flux: Definitive Master Specification & Roadmap

**Version:** 1.0.0-STABILIZATION
**Lead Architect:** Gemini CLI / BeamEngine Core Team
**Project Philosophy:** "Performance through Purity." Zero-framework, high-concurrency, skeuomorphic-modern hybrid.

---

## 1. Project Identity & Vision
Beam Audio Flux is not merely a DAW; it is a **Real-Time Audio Environment** built on the proprietary **BeamEngine**. It is designed to bridge the gap between rigid transform-based workstations and high-overhead neural vocoders. By utilizing **Parametric Wavefunction Collapse (PWC)** and **Analytic Matching Pursuit** (derived from the BVC codec ecosystem), Flux aims to provide a generative, semantic approach to audio manipulation.

### Core Mandates:
1.  **Zero Frameworks**: No JUCE, no Qt, no Boost. Every byte of the core engine (DSP, UI, Graphics) is native C++20.
2.  **Symmetric Multiprocessing**: Lock-free audio threading using atomic RenderPlan swaps.
3.  **Visual High-Fidelity**: 60fps+ UI rendering using SDF (Signed Distance Field) primitives and GPU-bound quad batching.

---

## 2. Technical Architecture Deep-Dive

### 2.1 The BeamEngine (DSP Core)
The engine operates on a directed acyclic graph (DAG) model where nodes represent atomic DSP operations.

*   **Topological Sorting**: On every connection change, the `FluxGraph` executes **Kahn’s Algorithm**. This ensures that nodes are processed in a linear schedule that respects signal dependency.
*   **RenderPlan**: The output of the compiler is a `RenderPlan`—a flattened execution list that the `AudioEngine` can iterate through without touching the graph's mutex.
*   **Memory Management**: Pre-allocation is mandatory. The `setupBuffers` method in `FluxNode` initializes interleaved 32-bit float buffers. No `new` or `malloc` is permitted in the `process()` loop.

### 2.2 The Graphics Subsystem (QuadBatcher)
The DAW uses a custom OpenGL 3.3+ renderer designed for thousands of updates per second.
*   **Vertex Structure**: Uses a packed `Vertex` struct (Position, TexCoord, Color).
*   **SDF Shaders**: Instead of textures for UI shapes, we use **Signed Distance Fields**. This allows for perfectly smooth rounded corners, anti-aliased lines, and glows at any zoom level with zero pixelation.
*   **Clipping (Scissor Stack)**: Implements a hierarchical `pushClip/popClip` system. This is critical for nested components and scrolling areas (like the Workspace).

### 2.3 The UI Toolkit (BeamUI)
A custom reactive framework built on the `Component` base class.
*   **Absolute vs. Flex**: Components store absolute bounds, but children are arranged via the **FlexBox** layout engine.
*   **Z-Order Propagation**: Mouse events are dispatched in reverse-child order (top-to-bottom) with immediate hit-testing.
*   **The Emerald Standard**: A centralized `LookAndFeel` class enforces the brand identity:
    *   **Primary**: #219e6c (Emerald Green) - Signal paths and active states.
    *   **Alert**: #8f0707 (Deep Red) - Recording and peak indicators.
    *   **Base**: #0d0d0f (Brand Black) - Main panels and backgrounds.

---

## 3. The "Node-Driven UI" Model
This is the heart of the latest refactor. It unifies how DSP developers interact with the GUI.

### 3.1 `createEditor` Workflow
Each node inherits `virtual std::shared_ptr<Component> createEditor(const NodeEditorContext& ctx)`.
*   **The Context**: `NodeEditorContext` provides pointers to `AudioDeviceManager`, `ProjectState`, and `GlobalTransport`.
*   **The Editor**: A node can return a `GenericNodeEditor` (automatic) or a specialized UI (like `TubeCompressorUI`).

### 3.2 `AudioModule` Orchestration
The `AudioModule` is the "Canvas Window" that hosts the Editor.
*   It handles the **Port Components** (Input/Output ports).
*   It handles the **Delete Button** and **Title Bar**.
*   It uses `FlexBox` to ensure the Editor content is perfectly padded and aligned.

---

## 4. Comprehensive Design Audit & Technical Debt

### 4.1 Threading Risks (The "P0" Priority)
*   **The Violation**: Currently, when a user moves a slider, it modifies a `Parameter`. That parameter calls `onChanged`. If automation is also moving that parameter, the call originates from the **Audio Thread**. If that callback triggers a UI refresh, the app will crash or stutter.
*   **The Solution**: A **Thread-Safe Command Queue**. `Parameter::setValue` must push a message to a lock-free queue, which the UI thread polls during `update()`.

### 4.2 Layout Deficiencies
*   **The Violation**: `FlexBox` lacks `wrap`. If a node has 20 parameters, they currently shrink to 1px height or overflow the window.
*   **The Solution**: Implement a multi-line wrap algorithm in `layout.hpp` and add container-level padding logic.

---

## 5. DSP Library Expansion Roadmap

### 5.1 Analog Suite 2.0
*   **ZDF Filters**: Moving from standard Biquads (direct form II) to **Zero-Delay Feedback** models. This is essential for the "Filter" node to sound musical when modulating the cutoff at high resonance.
*   **Oversampling (FIR-based)**: High-quality 2x/4x oversampling using polyphase decimation filters. This will be integrated into the `AnalogBase` class to prevent aliasing in the `Tube Drive` and `Saturate` algorithms.

### 5.2 Spectral Suite
*   **BVC-Derived Encoders**: Integrating the Analytic Matching Pursuit algorithms for semantic "splicing" of audio regions.
*   **Phase Vocoder**: A native FFT-based time-stretching engine for the Splicing mode.

---

## 6. FluxScript: The Native DSL
FluxScript is intended to be the "GLSL for Audio."
*   **Stage 1 (Current)**: Interpreter-based. Nodes like `FluxScriptNode` run a simple VM.
*   **Stage 2 (Short-term)**: Transpiler. FluxScript will be converted to C++ at runtime and compiled via `cl.exe` (Windows) or `clang` (macOS/Linux) into a DLL for native performance.
*   **Stage 3 (Long-term)**: LLVM JIT. Direct machine code generation from the script editor.

---

## 7. Immediate 4-Week Implementation Plan

### Week 1: Core Robustness (The "Hardening" Phase)
*   [x] **Mon-Tue**: Implement `AsyncCallbackQueue`. Refactor `Parameter` to never call UI logic directly.
*   [x] **Wed**: Implement `AtomicTransportState`. Fix race conditions between `AudioEngine::seek` and `FluxNode::process`.
*   [x] **Thu-Fri**: Overhaul `QuadBatcher::pushClip`. Fix the Y-coordinate flip logic for OpenGL scissors. Ensure text clipping is pixel-perfect.
*   [x] **Bonus**: Implement Lock-Free Circular Buffer for Recording (Fix 10.3).

### Week 2: Layout Engine & Editor UX
*   [x] **Mon-Tue**: Refactor `layout.hpp`. Add `flexWrap` and `padding` support.
*   [x] **Wed**: Rewrite `GenericNodeEditor`. Implement the "Two-Column" layout (Left: Label, Right: Control).
*   [x] **Thu-Fri**: Standardize `Slider` and `Knob` behavior. Ensure logarithmic mapping (for Frequency) and skewed mapping are built-in to `Parameter`.

### Week 3: Metering & Design System
*   [x] **Mon-Tue**: Implement `MeterSource` API. A thread-safe way for nodes to publish Peak/RMS data without impacting the audio loop.
*   [x] **Wed**: Create the `Theme` class. Replace all hardcoded `0.13f, 0.62f...` with `Theme::Emerald`.
*   [x] **Thu-Fri**: Implement the `RotaryKnob` widget. Add support for "shift-drag" for fine-tuning.

### Week 4: The "Pro" Upgrade
*   [x] **Mon-Wed**: Port `Opto2A` and `FET76` to the new `DynamicsEditor` (v2). Add the new GR (Gain Reduction) meters using the `MeterSource` API.
*   [x] **Thu-Fri**: Implement `Plugin Delay Compensation (PDC)` in the `RenderPlan` compiler. This allows nodes like "Lookahead Limiter" to report latency and have the engine align all tracks.
*   [x] **Bonus**: Implement Sample-Accurate Parameter Ramping (Fix 10.4).
*   [x] **Bonus**: Implement "Quiet State" for Audio Device Hot-Swap (Fix 10.5).

---

## 8. Data Structures & Serialization
*   **File Format**: `.flux` (JSON-based).
*   **Nodes**: Serialized by ID, type, and parameter map.
*   **Cables**: Serialized as pairs of `(SourceNodeID, SourcePort) -> (TargetNodeID, TargetPort)`.
*   **Regions**: Stored as absolute file paths + start/duration frames.

---

---



## 10. Broken Features & Functional Bug Registry

This section tracks existing features that are currently non-functional, unstable, or behaving incorrectly.



### 10.1 Automation Drift & Sync Failure

*   **Symptoms**: Automation lanes do not stay aligned with audio regions after a transport seek (Rewind/Fast-Forward). Linear interpolation sometimes "jumps" at block boundaries.

*   **Technical Root Cause**: The `AutomationLane` uses an absolute sample counter that is not correctly reset or offset during `AudioEngine::seek`. Additionally, parameter updates happen once per block instead of sample-accurately.

*   **Resolution Strategy**: 

    1.  Refactor `AutomationLane::getValue(size_t frame)` to use the global transport clock.

    2.  Implement **Sample-Accurate Parameter Ramping**: Instead of `setParameter(val)`, nodes must use `parameter.getNextValue()` inside their sample loops.



### 10.2 Zoom-Relative Hit-Testing (Workspace)

*   **Symptoms**: At zoom levels < 0.8x, clicking on Ports or selecting Cables becomes nearly impossible or registers on the wrong component.

*   **Technical Root Cause**: `Workspace::onMouseDown` applies the zoom transform to the mouse coordinates, but the "Hit-Testing Padding" (margin of error) remains constant in pixels. At low zoom, a 5px padding represents a tiny physical area.

*   **Resolution Strategy**: Scale hit-testing margins inversely with zoom: `padding = 15.0f / m_zoom`.



### 10.3 Recording Buffer Instability

*   **Symptoms**: Long recordings (> 1 minute) intermittently drop samples or cause the DAW to "hiccup."

*   **Technical Root Cause**: `FluxTrackNode::startRecording` writes directly to disk in the processing thread or via a simple blocking call in `pushData`. High disk latency stalls the audio callback.

*   **Resolution Strategy**: Implement a **Lock-Free Circular Buffer** for the `WavWriter`. The Audio Thread pushes data to the buffer; a dedicated low-priority "Disk Thread" drains the buffer to the `.wav` file.



### 10.4 Parameter "Pops" (Lack of Smoothing)

*   **Symptoms**: Rapidly moving a Gain or Cutoff slider causes audible clicking/zipper noise.

*   **Technical Root Cause**: Parameters update their values instantly at the start of a block. This causes a step-discontinuity in the waveform.

*   **Resolution Strategy**: Implement a **One-Pole Smoothing Filter** (Low-pass) on all parameter changes. Target value is approached exponentially over 10-20ms.



### 10.5 Audio Device "Hot-Swap" Crashes

*   **Symptoms**: Changing the output device in `ConfigView` often causes a hard crash or an infinite hang.

*   **Technical Root Cause**: `AudioDeviceManager` attempts to close the SDL Audio Stream while the `AudioEngine` callback is still active. 

*   **Resolution Strategy**: Implement a "Quiet State." Before a device change, the Engine must enter a `Muted` state, wait for the current callback to finish, and then release the hardware handle.



### 10.6 Splicing Mode: Zero-Cross & Clicks

*   **Symptoms**: Cutting audio regions (`Scissors Tool`) or moving them causes loud clicks at the boundaries.

*   **Technical Root Cause**: Regions are hard-cut. There is no automatic fade-in/out or zero-crossing detection.

*   **Resolution Strategy**: 

    1.  Implement a mandatory 5ms **Linear Crossfade** at every region boundary.

    2.  Add "Snap to Zero Crossing" logic when using the Scissors tool.



---







## 11. Visual & DSP Refinement of Current Modules



This section details the specific upgrades for existing components to align them with the new "Emerald Standard" and the Node-Driven UI architecture.







### 11.1 Analog Suite: The "Heritage" Refactor



*   **Visual Standard**: Every plugin in the Analog Suite (Opto-2A, Tube-P EQ, etc.) will receive a standardized high-fidelity faceplate.



    *   *Emerald Accents*: Active lamps and meters will use the official Emerald Green (#219e6c).



    *   *Real-Time Feedback*: Every compressor will include a "GR History" mini-graph, and every EQ will show a real-time curve overlay.



*   **DSP "Golden" Path**: Implement **2x Internal Oversampling** as a toggleable feature for all saturation stages to eliminate the "digital harshness" in the current Tube-P and Opto-2A nodes.







### 11.2 Workspace: Dynamic Cables & Interaction



*   **Cable Physics**: Replace the static Bezier curves with a lightweight **Catenary Physics** model. Cables should "hang" naturally and react to module movement with smooth interpolation.



*   **Color-Coded Paths**: 



    *   Emerald: Stereo Audio.



    *   White: MIDI Data.



    *   Red: Feedback Loops (Warning).



*   **Smart Selection**: Implement rubber-band selection and grouping for modules to allow moving entire signal chains at once.







### 11.3 Timeline: Splicing & Waveform Rendering



*   **Live Waveform Drawing**: Currently, waveforms are only visible after a recording finishes. Implement a **Streaming Waveform Cache** that draws the signal in real-time during the recording process.



*   **Non-Destructive Splicing**: Every "Cut" made with the Scissors tool must be a virtual offset, allowing the user to "pull back" the edge of a region to reveal the original audio (Standard DAW behavior).







### 11.4 Sidebar & Navigation



*   **Instant Search**: Add a fuzzy-search filter to the Sidebar to navigate the plugin library.



*   **Drag-and-Drop 2.0**: Allow dragging plugins directly onto an existing Cable to "insert" the effect between two nodes automatically.







### 11.5 Master Strip: Safety & Analysis



*   **Safety Limiter**: Add a built-in, non-optional, ultra-fast "Soft Clipper" to the Master Strip to protect the user's hardware from sudden signal bursts during experimentation.



*   **Phase Correlation Meter**: A new visualization component to detect mono-compatibility issues in the final mix.







## 12. Conclusion



Beam Audio Flux is moving from a collection of modules to a **Unified Platform**. By enforcing the "createEditor" pattern, hardening the threading model, and systematically repairing the broken functional core, we ensure that the DAW remains stable while enabling extreme flexibility for DSP innovation.




