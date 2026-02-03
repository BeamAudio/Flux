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
1.  **Add FX**: Open the Sidebar (left) and click on an effect category.
2.  **VST3 Support**: Beam Audio Flux automatically scans `C:/Program Files/Common Files/VST3`. You can add custom scan paths in the Audio Configuration menu (Crank icon).
3.  **FluxScript**: Create custom DSP using our high-level language. Click "COMPILE AOT" on a script node to turn it into a native high-performance plugin.
4.  **Wiring**: Drag from a module's Output Port (Right) to another module's Input Port (Left).
5.  **Recording**:
    *   Add an **Audio Input** and an **Empty Tape**.
    *   Wire Input -> Tape.
    *   Press **Record (O)** in the top bar.
6.  **Rendering**:
    *   Click **RENDER** to export your project to high-quality WAV. The engine now uses a shared-plan architecture for 100% fidelity matching between playback and bounce.

---

## Developer Guide

### Architecture
*   **Engine**: C++20, SDL3, OpenGL 3.3.
*   **Hosting**: Native VST3 bridge with Planar-Interleaved conversion.
*   **Scripting**: AOT compilation from FluxScript to C++ DLLs.
*   **UI**: Modular `Component` system with `FlexBox` layout engine.

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

### Customizing the Logo
To add a custom logo that appears in the window title bar, taskbar, and as the executable icon:

1.  **Window/Taskbar Icon**:
    *   Place your icon image (e.g., `logo.bmp`) in `assets/images/`.
    *   In `src/session/beam_host.cpp`, inside `init()`:
        ```cpp
        SDL_Surface* icon = SDL_LoadBMP("assets/images/logo.bmp");
        if (icon) {
            SDL_SetWindowIcon(m_window, icon);
            SDL_DestroySurface(icon);
        }
        ```

2.  **Executable Icon (Windows)**:
    *   Create an `.rc` file (e.g., `resources.rc`) containing:
        ```rc
        IDI_ICON1 ICON "assets/images/logo.ico"
        ```
    *   Add this `.rc` file to your `CMakeLists.txt`:
        ```cmake
        add_executable(BeamAudioFlux src/main.cpp resources.rc ...)
        ```

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
