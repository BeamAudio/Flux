# Beam Audio Flux
## Proprietary Engine Architecture (No-Framework) v2.0

**Beam Audio Flux** is a next-generation, high-performance Digital Audio Workstation (DAW) built on a completely proprietary, "no-framework" C++20 engine known as **BeamEngine**. It avoids heavy frameworks like JUCE or Qt, opting for a lightweight stack of SDL3 and OpenGL for maximum efficiency and full control over the DSP and UI pipelines.

## Version 0.2: "Giant Steps"
The **0.2 Giant Steps** release marks a significant milestone in Beam Audio Flux's evolution from a node-based experiment to a production-capable DAW. This version bridges the gap between creative sound design and traditional mixing workflows.

**Key Features in 0.2:**
*   **Analog Mixing Console**: A dedicated Mix View (F3) featuring vertical channel strips with high-resolution Luminous metering, 0dB-centered faders, and analog-inspired panners.
*   **Engine Summing 2.0**: Global Solo/Mute logic and constant-power panners integrated directly into the core SIMD summing stage for zero-latency performance.
*   **Precision Timeline**: Fixed clip timing synchronization in Slice Mode. Audio processors are now globally frame-aware, ensuring sample-accurate region playback regardless of clip movement or slicing.
*   **PDC (Plugin Delay Compensation)**: Automatic latency matching across complex parallel signal chains, ensuring all tracks stay perfectly in phase.
*   **FlexBox UI Engine**: A revolutionary CSS-inspired layout system that allows the UI to adapt fluidly to any window size.
*   **FluxScript AOT**: Near-native performance for custom DSP scripts via Ahead-of-Time compilation using the bundled MinGW toolchain.

---

## User Manual

### 1. Basic Navigation
*   **Flux Mode (F1)**: The creative playground. Drag cables between nodes to route audio.
*   **Slice Mode (F2)**: The timeline view. Arrange clips, slice audio, and manage vertical track lanes.
*   **Mix View (F3)**: The analog console. Fine-tune levels, panning, and solo/mute states for the final sum.
*   **Spacebar**: Global Play/Pause.
*   **Mouse Wheel**: Zoom in/out on the canvas or timeline.
*   **Right Click (Hold)**: Pan the view in any mode.

### 2. Splicing & Arrangement
*   **Importing Audio**: Drag and drop `.wav` files directly from your OS into the Workspace or Timeline.
*   **Region Movement**: Click and drag clips in Slice Mode. Hold **Shift** while dragging to toggle grid-snapping (1-second intervals).
*   **Slicing**: Select the **Scissors Tool** (or press 'S' in Slice Mode) and click on a region to split it at the cursor position.
*   **Timing Integrity**: v0.2 ensures that when you slice a region, the second half maintains its absolute timing relative to the source file, allowing for seamless non-destructive editing.

### 3. Mixing & Routing
*   **Solo Logic**: Clicking **S** on a channel strip isolates that track. The engine intelligently scans the graph to mute all non-soloed paths leading to the Master.
*   **Panning**: The Mixer panners use a constant-power curve ($sin/cos$) to ensure the perceived volume remains identical as you move a sound across the stereo field.
*   **Unity Gain**: Mixer faders feature a prominent red tick at 0dB. Double-click any fader to snap it back to unity.

---

## SDK & Developer Tutorial

Beam Audio Flux is designed for extensibility. Developers can create native C++ plugins by extending two core classes.

### 1. Creating a DSP Processor
Extend `FluxProcessor` for the audio-thread logic. This class must be real-time safe (no allocations, no locks).
```cpp
class MyGainProcessor : public FluxProcessor {
public:
    void updateParameters(const float* params) override {
        m_gain = params[0]; // Parameters are mapped in order
    }
    void process(const float** inputs, float** outputs, int frames) override {
        for (int i = 0; i < frames * 2; ++i) {
            outputs[0][i] = inputs[0][i] * m_gain;
        }
    }
private:
    float m_gain = 1.0f;
};
```

### 2. Creating a Node
Extend `FluxNode` to define the metadata, parameters, and ports.
```cpp
class MyGainNode : public FluxNode {
public:
    MyGainNode() {
        addParameter(std::make_shared<Parameter>("Gain", 0.0f, 2.0f, 1.0f));
    }
    std::shared_ptr<FluxProcessor> createProcessor() override {
        return std::make_shared<MyGainProcessor>();
    }
    std::string getName() const override { return "My Gain"; }
    std::vector<Port> getInputPorts() const override { return {{"In", 2}}; }
    std::vector<Port> getOutputPorts() const override { return {{"Out", 2}}; }
};
```

### 3. UI Unification
BeamEngine uses a "Node-Driven UI" architecture. By default, the engine generates a slider-based interface using `GenericNodeEditor`. To create a specialized interface (like the `TapeReel`), override `createEditor` and return a custom `Component` using the `FlexBox` layout engine for responsive positioning.

---

### Building from Source
**Requirements**: CMake 3.20+, Visual Studio 2022 (Windows).

```bash
mkdir build
cd build
cmake ..
cmake --build build --config Release
```

### Packaging (Windows)
To create a distribution-ready package, use the provided PowerShell script:
```powershell
.\package_windows.ps1
```
This script performs a full CMake rebuild before bundling the executable, DLLs, assets, and SDK headers into the `dist` folder.

---
© 2026 Beam Audio. Built with BeamEngine.