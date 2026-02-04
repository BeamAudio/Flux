# Beam Audio Flux
## Proprietary Engine Architecture (No-Framework) v2.0

**Beam Audio Flux** is a next-generation, high-performance Digital Audio Workstation (DAW) built on a completely proprietary, "no-framework" C++20 engine known as **BeamEngine**. It avoids heavy frameworks like JUCE or Qt, opting for a lightweight stack of SDL3 and OpenGL for maximum efficiency and full control over the DSP and UI pipelines.

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
*   **Mouse Wheel**: Zoom in/out and scroll through the Plugin Library.
*   **Right Click (Hold)**: Pan the view.
*   **Marquee Selection**: Left-click and drag on empty space in Flux Mode to select multiple nodes.
*   **Hover Tooltips**: Hover over any knob or slider to see its real-time value in a floating overlay.

### Routing & FX
1.  **Add FX**: Open the Sidebar (left) and click on an effect category. The library features a robust, hierarchical navigation system.
2.  **Global Bypass**: Every plugin module has a dedicated **B** button in its header for immediate processing control.
3.  **Advanced Auto-Tune**: Our specialized Auto-Tune module features a circular chromatic tuner, key/scale selection, and a "Humanize" algorithm for natural pitch correction.
4.  **Bulk Actions**: With nodes selected, press **Delete** to remove them all, or **Ctrl+D** to duplicate your selection.
5.  **Professional Sliders**: 
    *   **Double-Click**: Reset any slider (including mixer faders) to its unity or default position.
    *   **0dB Marks**: Visual tick marks at 70% height/width indicate the standard unity gain position.
6.  **VST3 Support**: Automatically scans `C:/Program Files/Common Files/VST3`. Add custom paths in the Audio Configuration menu (Crank icon).
7.  **FluxScript**: Create custom DSP using our high-level language. Click "COMPILE AOT" on a script node to turn it into a native high-performance plugin.

---

## Developer Guide

### Architecture
*   **Engine**: C++20, SDL3, OpenGL 3.3.
*   **Dynamics Metering**: New `isGRMeter` flag in `RackStyle` supports hardware-standard top-down metering for compressors like the FET-76.
*   **Project Stability**: Thread-safe project loading with automatic audio engine suspension during graph reconstruction.
*   **UI System**: A modular `Component` system built from scratch featuring:
    *   **AutoFlexContainer**: A CSS-like flexbox layout engine for dynamic, responsive UIs.
    *   **ScrollableContainer**: High-performance scrolling for large lists.
    *   **QuadBatcher**: An optimized OpenGL renderer that batches UI draw calls.

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
This script now performs a full CMake rebuild before bundling the executable, DLLs, assets, and SDK headers into the `dist` folder.

---
© 2026 Beam Audio. Built with BeamEngine.
