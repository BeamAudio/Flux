# Beam Audio Flux
## Proprietary Engine Architecture (No-Framework) v2.0

**Beam Audio Flux** is a next-generation, high-performance Digital Audio Workstation (DAW) built on a completely proprietary, "no-framework" C++20 engine known as **BeamEngine**.

### Download & Installation
1.  Navigate to the [Releases](https://github.com/BeamAudio/Flux/releases) page.
2.  Download the latest zip for your platform (Windows x64 available).
3.  Extract and run `BeamAudioFlux.exe`.

---

## User Guide

### Basic Navigation
*   **Flux Mode (F1)**: The creative playground. Drag cables between nodes to route audio.
*   **Slice Mode (F2)**: The timeline view. Arrange clips, slice audio, and mix.
*   **Spacebar**: Play/Pause.
*   **Mouse Wheel**: Zoom in/out (centered on mouse).
*   **Right Click (Hold)**: Pan the view.

### Routing & FX
1.  **Add FX**: Open the Sidebar (left) and click on an effect category (e.g., EQUALIZERS). Click an effect to add it to the workspace.
2.  **Wiring**: Drag from a module's Output Port (Right) to another module's Input Port (Left).
3.  **Recording**:
    *   Add an **Empty Tape** from the UTILITY category.
    *   Wire your **Audio Input** node to the Tape's input.
    *   Press the **Record (O)** button in the top bar.
    *   Stop recording to save the file to disk.

### Metering
*   **Master Strip**: Located on the right. Shows peak levels (VU) and fader gain.
*   **Loudness Meter**: Add from UTILITY. Displays Momentary/ShortTerm LUFS and True Peak.
*   **Spectrum Analyzer**: Add from UTILITY. Displays a 10-band frequency breakdown.

---

## Developer Guide

### Architecture
*   **Engine**: C++20, SDL3, OpenGL 3.3.
*   **DSP**: Single-sample or Block-based processing via `FluxNode`.
*   **UI**: Immediate-mode inspired retained structure (`Component` tree).

### Creating New FX
To add a new DSP effect to Beam Audio Flux:

1.  **Define the Class**: Inherit from `FluxPlugin` in `src/engine/analog_suite.hpp`.
    ```cpp
    class MyNewReverb : public FluxPlugin {
    public:
        MyNewReverb(int buf, float sr) : FluxPlugin("My Reverb", buf, sr) {
            addParam("Decay", 0.1f, 5.0f, 2.0f);
        }
        void processBlock(const float* in, float* out, int total) override {
            // Your DSP logic here
        }
    };
    ```
2.  **Register the UI**:
    *   Add the name to `Sidebar` categories in `src/interface/sidebar.hpp`.
    *   Add the instantiation logic to `Workspace::addFX` in `src/interface/workspace.hpp`.

### Future: Scripting
We are working on a JIT-compiled scripting language to allow FX creation at runtime without recompiling the engine. Stay tuned for the `FluxScript` update.

---

### Building from Source
**Requirements**: CMake 3.20+, Visual Studio 2022 (Windows).

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```