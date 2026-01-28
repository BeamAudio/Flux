# Beam Audio Flux: UI/UX & Architecture Proposal v3.0 (Revised)

## 1. Executive Vision
Beam Audio Flux bridges the gap between **Harrison Mixbus** (analog feel) and **REAPER** (routing power). 
**Status:** [CORE ARCHITECTURE DONE]

## 2. Core UI Modes

### A. Splicing Mode (Editing View) [NEW CONCEPT]
*   **Timeline View:** The Workspace transforms into a multi-track linear timeline.
*   **Tape Splices:** Visual representations of audio data that can be cut, moved, and cross-faded.
*   **Alignment Tools:** Snapping to grid, nudge controls, and time-stretching (Elastic Tape).
*   **Interaction:** Focus on "Cutting and Pasting" the actual signal.

### B. Flux Mode (Mixing View)
*   **Modular Canvas:** The current 2D spatialized area for routing. [DONE]
*   **Bezier Routing:** Visualizing audio flow with physical cables. [IN PROGRESS]
*   **Interaction:** Focus on FX chains and master output.

## 3. Core UI Components

### A. The Command Center (Top Bar)
*   **Mode Switcher:** A prominent toggle between "SPLICING" and "FLUX" modes.
*   **Transport:** Global Play/Stop/Rec linked to the timeline.
*   **Persistence:** JSON-based Save/Load. [DONE]

### B. The Flux Browser (Left Sidebar)
*   **Asset/FX Library:** Drag modules into Flux mode, or Audio into Splicing mode.

### C. The Master Tape (Right Sidebar)
*   **Master Strip:** Fixed output with Saturation/VU Meters. [DONE UI]

## 4. System Features

### A. Dynamic Routing & Sync
*   **Timeline Logic:** Every TrackNode has a `timePosition` property used in Splicing Mode.
*   **Graph Traversal:** Kahn's Algorithm for processing order. [PENDING]

### B. Design Principles
*   **Tactile Transition:** Switching modes should feel like "opening up" the tape machine to see the reels inside.
