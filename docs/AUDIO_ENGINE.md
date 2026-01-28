# Beam Audio Engine - Architecture & SDK

## 1. Overview
The **Beam Audio Engine** is a modular, node-based DSP system designed for low-latency real-time audio processing. It powers the DAW's ability to chain effects, mix tracks, and process signal flows dynamically.

## 2. Core Architecture

### 2.1 The Flux Graph (`src/dsp/flux_graph.hpp`)
The backbone of the engine is a **Directed Acyclic Graph (DAG)**.
- **Topological Sorting**: The graph automatically sorts nodes so that audio signals flow correctly from sources (Tracks) to processors (Filters/Effects) to sinks (Master Output).
- **Buffer Management**: Handles the allocation and clearing of intermediate audio buffers between nodes.
- **Processing Loop**: Iterates through the sorted nodes and calls `process()` on each, summing outputs into connected inputs.

### 2.2 Audio Nodes (`src/dsp/flux_node.hpp`)
Every audio processor inherits from `FluxNode`.
- **Ports**: Defines Input and Output points (Stereo by default).
- **Parameters**: A thread-safe parameter system allows the UI (Main Thread) to control DSP variables (Audio Thread) without race conditions.
- **Process Kernel**: The core loop where samples are manipulated.

### 2.3 Audio Engine (`src/dsp/audio_engine.hpp`)
The bridge between the abstract Graph and the hardware driver (SDL3).
- **Callback**: Feeds the hardware buffer by ticking the `FluxGraph`.
- **Transport**: Manages global Play/Pause/Rewind states.
- **Thread Safety**: Uses mutexes to swap graphs or update connections safely during playback.

## 3. Flux Plugin SDK (`src/dsp/flux_plugin.hpp`)
To simplify the creation of new effects, we provide a high-level SDK. Developers do not need to manage buffers or graph logic manually.

### Key Features
- **Automatic GUI**: Parameters defined in the plugin automatically generate UI knobs.
- **Thread-Safe Parameters**: Get/Set is handled atomically.
- **Simplified Interface**: Just implement `processBlock`.

### Developer Guide: Creating a Custom Effect

1. **Inherit from `FluxPlugin`**:
   ```cpp
   #include "../dsp/flux_plugin.hpp"

   class MyDistortion : public Beam::FluxPlugin {
   public:
       MyDistortion(int bufSize, float sampleRate) 
           : FluxPlugin("Super Distortion", bufSize, sampleRate) 
       {
           // Define parameters (Name, Min, Max, Default)
           addParam("Drive", 0.0f, 10.0f, 1.0f);
           addParam("Mix", 0.0f, 1.0f, 0.5f);
       }

       void processBlock(const float* input, float* output, int totalSamples) override {
           float drive = getParam("Drive");
           
           for (int i = 0; i < totalSamples; ++i) {
               // Simple hard clipping
               float x = input[i] * drive;
               output[i] = std::max(-1.0f, std::min(1.0f, x));
           }
       }
   };
   ```

2. **Register the Effect**:
   Add your new class to the factory in `src/ui/workspace.hpp` inside `addFX`.

3. **Use It**:
   The new effect will appear in the UI, complete with knobs for "Drive" and "Mix".

## 4. Signal Flow
1. **Source**: `FluxTrackNode` reads from disk (`DiskStreamer`) via `WavReader`.
2. **Process**: Samples pass through user-defined chains (Gain -> Filter -> Delay).
3. **Mix**: `FluxGraph` sums signals at connection points.
4. **Master**: `MasterNode` applies final volume and metering analysis.
5. **Output**: `AudioEngine` pushes the final buffer to the SDL3 stream.
