# Beam Audio Flux - Design Audit & Robustness Report

**Date:** 2026-01-31
**Status:** DRAFT / EXPERIMENTAL

This document tracks identified architectural flaws, missing features, and robustness issues in the current BeamEngine implementation. It serves as a roadmap for moving from "Prototype" to "Production" quality.

## 1. Critical Robustness Issues

### 1.1 Thread Safety Violations
*   **UI Updates on Audio Thread**: The `Parameter` system triggers `onChanged` callbacks on the calling thread. If the Audio Engine updates a parameter (e.g., via Automation), the callback executes on the high-priority Audio Thread. If that callback touches the UI (e.g., repainting a Knob), it causes undefined behavior/crashes in OpenGL/SDL.
    *   *Fix Required*: Implement a message queue (RingBuffer) to defer UI updates to the main thread.
*   **Race Conditions in Transport**: `AudioEngine::seek` modifies node state (`m_currentFrame`) from the UI thread while the Audio Thread reads it.
    *   *Fix Required*: Atomicize transport state or use a command queue for transport operations.
*   **Automation Threading**: `m_automationLanes` in `AudioEngine` is accessed without a lock during processing but modified by the UI.

### 1.2 Memory Management
*   **Graph Lifecycle**: While `std::shared_ptr` prevents immediate crashes, the `FluxGraph` compilation step is heavy and blocks the UI thread.
*   **RenderPlan Swapping**: The atomic swap of `RenderPlan` is technically safe for the pointer, but if the old plan is destroyed immediately on the UI thread while the Audio Thread was *just* about to use a node from it (in a race where it grabbed the pointer but hadn't finished), it's risky. (Current implementation seems okay due to shared_ptr ref-counting, but needs verification).

## 2. Functional Gaps

### 2.1 Core DAW Features
*   **Undo/Redo System**: Completely missing. Any change to the graph (adding nodes, moving knobs) is destructive and irreversible.
    *   *Requirement*: Implement a Command Pattern (`FluxCommand`) stack.
*   **Plugin Delay Compensation (PDC)**: The engine assumes all nodes have 0 latency. Processing chains with inherent latency (FFT, Lookahead Limiters) will cause phase issues.
    *   *Requirement*: Add `getLatency()` to `FluxNode` and implement delay compensation in `RenderPlan`.
*   **State Serialization**: Only basic parameter values are saved. Internal DSP state (e.g., Filter history, Delay buffers) is lost on save/load.
    *   *Requirement*: `FluxNode::serializeState()` / `deserializeState()` virtual methods.

### 2.2 UI/UX Flaws
*   **No Metering Infrastructure**: There is no standard way for a Node to report metering data (RMS/Peak) to the UI efficiently. Currently relies on ad-hoc side-channels or unsafe polling.
*   **Rigid Windowing**: `AudioModule` uses a basic FlexBox layout but doesn't handle complex resizing or "folding" (collapsing) well.
*   **Accessibility**: No keyboard navigation or screen reader support.

## 3. DSP Engine Limitations
*   **No Sidechaining**: The `FluxGraph` topology supports multiple inputs, but the `FluxNode` API is rigid regarding "Main Input" vs "Sidechain".
*   **Sample Rate Locking**: Changing sample rate requires a full engine restart/re-allocation.
*   **No Offline Rendering**: The `OfflineRenderer` exists but is basic and may not match real-time processing exactly due to state resetting.

## 4. Next Steps (Prioritized)
1.  **Fix Threading**: Implement `AsyncParameterUpdate` system to decouple Audio Thread from UI.
2.  **Implement Undo/Redo**: Essential for usability.
3.  **Standardize Metering**: Create a thread-safe `MeterSource` API.
4.  **Add Latency Compensation**: Critical for professional mixing.
