# Beam Audio Flux - Project Context & Architecture

## 1. System Overview
**Beam Audio Flux** is a high-performance C++20 DAW built on the proprietary **BeamEngine**. It eschews heavy frameworks (JUCE, Qt) in favor of a custom, lightweight architecture using SDL3 and OpenGL 3.3+.

## 2. Core Architecture: Engine & UI Unification
The system uses a robust "Node-Driven UI" architecture that decouples DSP logic from interface rendering while maintaining tight integration.

### 2.1 The Bridge: `createEditor`
*   **Concept**: Every DSP node (`FluxNode`) is responsible for defining its own UI.
*   **Mechanism**: `FluxNode::createEditor(const NodeEditorContext&)` returns a `std::shared_ptr<Component>`.
*   **Context Injection**: System services (like `AudioDeviceManager`) are passed via `NodeEditorContext`, keeping the engine decoupled from global state.

### 2.2 UI Generation Strategy
1.  **Automatic (Default)**: If a node does not implement `createEditor`, the base `FluxNode` implementation returns a `GenericNodeEditor`. This editor inspects the node's `Parameter` map and automatically generates a UI with Sliders and Labels using the FlexBox layout engine.
2.  **Custom (Specialized)**: Complex nodes (e.g., `TubeCompressor`) implement `createEditor` to return a specialized `Component` (e.g., `TubeCompressorUI`) with custom graphics, metering, and layout.

### 2.3 `AudioModule` Container
*   **Role**: Acts as the window/container for any node on the Workspace canvas.
*   **Responsibility**: Handles common DAW functionality:
    *   Input/Output Ports (Cable connections)
    *   Title Bar & Delete functionality
    *   Hosting the editor component returned by the node.
*   **Layout**: Uses `FlexBox` to dynamically size itself to fit the hosted editor.

## 3. Layout Engine: FlexBox
A modern, CSS-inspired layout engine located in `src/interface/layout.hpp`.
*   **Features**: Supports `Row`, `Column`, `AlignItems` (Stretch, Center, Start, End), `JustifyContent`.
*   **Integration**: Used heavily by `GenericNodeEditor` and `AudioModule` to ensure the UI adapts to different window sizes and parameter counts without hardcoded coordinates.

## 4. UI Toolkit (`src/interface/ui_toolkit.hpp`)
A central hub that consolidates all core UI widgets for easy access by DSP developers:
*   **Controls**: `Slider`, `Knob`, `Button`, `ComboBox`.
*   **Visuals**: `Label`, `Meter`, `VUMeter`.
*   **Layout**: `FlexBox`, `LayoutItem`.

## 5. Key File Structure
*   `src/engine/flux_node.hpp`: Base class defining the `createEditor` interface.
*   `src/interface/audio_module.hpp`: The container component for nodes.
*   `src/interface/generic_node_editor.hpp`: The auto-generated UI implementation.
*   `src/interface/layout.hpp`: The FlexBox layout engine.

## 6. Build & Run
The project uses CMake.
```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```