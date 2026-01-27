# Beam Audio Flux: UI/UX & Architecture Proposal v3.0

## 1. Executive Vision
Beam Audio Flux will bridge the gap between the **tactile, console-centric workflow of Harrison Mixbus** and the **unlimited routing flexibility of REAPER**. It maintains its proprietary, no-framework C++20 core for ultra-low latency and high-refresh-rate visuals.

## 2. Core UI Components

### A. The Infinite Floor (Main Workspace)
*   **Modular Canvas:** A 2D spatialized area where modules live.
*   **Auto-Strip Mode:** Toggle to align floating modules into vertical console strips.
*   **Bezier Routing:** Physical cable simulation with color-coding for signal paths.

### B. The Command Center (Top Bar)
*   **Transport:** Large, reactive buttons for Play/Stop/Rec.
*   **Persistence:** Quick icons for "New Project", "Open", and "Save".
*   **DSP Monitor:** Real-time feedback on audio thread performance.

### C. The Flux Browser (Left Sidebar)
*   **Drag-and-Drop Library:** FX modules and Audio assets ready to be pulled into the workspace.
*   **Search/Filter:** Quickly find high-pass filters or tape delays.

### D. The Master Tape (Right Sidebar)
*   **Fixed Output Strip:** Inspired by Mixbus master saturation.
*   **Analog Metering:** High-refresh-rate VU meters using the Game-Loop pattern.

## 3. System Features

### A. Project Persistence (.flux)
*   **Serialization:** Every module and connection is saved into a JSON structure.
*   **Asset Management:** Links to external WAV files with relative path support.

### B. Dynamic Routing Engine
*   **Graph Traversal:** The AudioEngine will now sort nodes topologically based on cable connections.
*   **Live Injection:** Interactively add/remove FX without stopping the audio stream.

## 4. Design Principles
*   **Tactile Feedback:** Every knob and slider must feel "heavy" and responsive.
*   **Zero Latency UI:** UI interactions must never block the audio thread (Lock-free queues).
*   **Modern Aesthetic:** Dark mode by default, with high-contrast "Flux Blue" accents.
