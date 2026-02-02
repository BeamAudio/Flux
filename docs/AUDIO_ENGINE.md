# Beam Audio Engine - Architecture (v2.0)

## 1. Overview
The **Beam Audio Engine** is a high-performance, real-time-safe DSP engine. In version 2.0, it uses a compiled execution plan strategy to virtually eliminate overhead in the audio hot loop.

## 2. Core Architecture

### 2.1 Compiled Render Plans (`src/engine/core/render_plan.hpp`)
The backbone of Beam 2.0 is the **RenderPlan**.
- **Static Sequence**: Instead of traversing a graph, the engine executes a flat array of `ProcessorExecution` structs.
- **Buffer Pool**: All intermediate audio buffers are pre-allocated in a central pool, eliminating per-node allocations and reducing cache misses.
- **Zero-Copy Sink**: Final output buffers are directly identified during compilation so the engine can copy them to hardware without traversal.

### 2.2 Functional Decoupling: Node vs. Processor
- **FluxNode**: The high-level model. Manages connectivity, automation, and UI.
- **FluxProcessor**: The low-level worker. Contains only DSP code and member states (filters, delays). 

### 2.3 The Processing Loop (`src/engine/core/audio_engine.cpp`)
The engine runs the `RenderPlan` in the SDL3 audio callback:
1. **Fetch Plan**: Atomically swaps to the latest compiled plan.
2. **Snapshot Parameters**: Snaps all node parameters into a thread-safe floating-point array for the current frame.
3. **Execute Sequence**: Ticks each `FluxProcessor` with its assigned input/output buffer pointers from the pool.
4. **Copy to Hardware**: Moves final master data to the hardware buffer.

## 3. High-Performance Design Patterns

### 3.1 Pointer-Based Routing
In Beam 2.0, signal routing is handled by passing arrays of pointers (`const float** inputs`, `float** outputs`) to processors. 
- **Efficiency**: No audio data is copied between nodes.
- **Flexibility**: Supports N-channel and multi-input processing (e.g., sidechain compression).

### 3.2 Real-Time Constraint Enforcement
- **No `std::vector` resize** in processors.
- **No `std::shared_ptr` allocation** during processing.
- **No virtual calls** inside the inner sample loops.

## 4. Signal Flow in Beam 2.0
1. **Initialization**: `AudioEngine` starts hardware stream.
2. **Compilation**: `FluxGraph::compile()` transforms node topology into a `RenderPlan`.
3. **Activation**: `AudioEngine::setActivePlan()` safely swaps the hot plan.
4. **Processing**: `FluxProcessor::process()` handles the raw samples.
5. **Output**: Master output buffers are written to physical device.
