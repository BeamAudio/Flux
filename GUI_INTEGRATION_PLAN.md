# GUI Integration & Design System Plan

**Goal:** Establish a unified, modular, and high-performance GUI framework ("BeamUI") that simplifies the design of both the DAW interface and individual FX plugins.

## 1. Unified Design System ("The Emerald Standard")
*   **Color Palette**: Define a centralized `Theme` class (singleton or context-injected) containing semantic colors (e.g., `Background`, `Surface`, `Accent`, `Warning`, `SignalHot`).
    *   *Implementation*: Refactor `LookAndFeel` to use these semantic keys instead of hardcoded RGB values.
*   **Typography**: Integrate a font manager (via `QuadBatcher`) that supports weights (Light, Regular, Bold) and sizes relative to a base scale.
*   **Spacing & Metrics**: Define standard grid units (e.g., 4px baseline) in `layout.hpp` to ensure consistent padding and margins across all modules.

## 2. Component Library Expansion
*   **Smart Containers**:
    *   `Panel`: A container with built-in support for borders, rounded corners, and background gradients (SDF-based).
    *   `GridContainer`: A 2D grid layout system for rack-like interfaces.
*   **Interactive Controls**:
    *   `RotaryKnob`: Enhanced knob with support for arcs, markers, and stepping (for selection parameters).
    *   `ToggleSwitch`: Skeuomorphic or flat toggle switches.
    *   `XYPad`: For 2D modulation sources.
*   **Visualization**:
    *   `WaveformDisplay`: Efficient, threaded waveform rendering for the timeline and sampler plugins.
    *   `SpectrumView`: FFT-based frequency visualizer using the existing `QuadBatcher` line drawing.

## 3. FX UI Framework ("FluxGUI")
*   **Standard Layouts**: Provide templates for common plugin types:
    *   *ChannelStripLayout*: Vertical stack (Input -> EQ -> Dyn -> Output).
    *   *RackLayout*: Horizontal slots (like 500-series modules).
*   **Auto-Binding**: Enhance `createEditor` to automatically map parameter ranges, units (Hz, dB), and skew (logarithmic sliders) to the UI controls without manual configuration.

## 4. Developer Experience (DX)
*   **Live Reloading**: (Stretch Goal) Allow tweaking UI layout (JSON/XML?) without recompiling C++.
*   **Debug Overlay**: A "Inspector" mode to see component bounds, padding, and Z-index on hover.

## 5. Execution Steps
1.  **Refactor LookAndFeel**: Centralize colors/shapes.
2.  **Build Core Widgets**: Knob, Switch, Panel.
3.  **Enhance GenericNodeEditor**: Use the new Layouts and Auto-Binding.
4.  **Update Analog Suite**: Port existing FX to use the new FluxGUI components.
