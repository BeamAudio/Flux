# Beam Audio Flux: Feature Analysis & Architectural Evolution

**Date:** 2026-01-31
**Task:** Comparative Analysis vs. Modern DAWs & Analog Design Language Implementation

---

## 1. Feature Comparison: Flux vs. SOTA (State of the Art)

| Feature | Reaper | Cubase/Logic | Beam Audio Flux | Gap / Action Plan |
| :--- | :--- | :--- | :--- | :--- |
| **Engine Architecture** | Anticipative FX, extreme routing | Hybrid buffers, tight MIDI integration | **Node-Based DAG (Kahn's Sort)** | Flux is more modular (like Bitwig/MaxMSP) but lacks multi-core load balancing for single chains. |
| **Plugin Compensation** | Advanced PDC | Frame-accurate PDC | **Compiler-level PDC (implemented)** | We have the foundation; need to verify with multi-parallel paths. |
| **Recording I/O** | Direct-to-disk, multi-threaded | High-buffer stability | **Lock-Free Circular Buffer** | Flux matches the "Pro" recording stability after the latest hardening. |
| **Automation** | Spline-based envelopes | Region-based + Track-based | **Sample-Accurate Ramping** | **BIG GAP:** We need a visual Spline Editor for envelopes. |
| **UI Rendering** | CPU/GDI/Bitmaps | Metal/Direct2D/Bitmap hybrid | **GPU-Bound QuadBatcher + SDF** | Flux is technically superior in rendering speed/aliasing via SDFs. |
| **Analog Integration** | Via Plugins | Stock channel strips | **Native Tape Physics in TrackNode** | Flux is "Analog-First" by design, mimicking tape behavior at the core. |

---

## 2. Design Language: "The Analog Emerald"

### 2.1 The Harrison Mixbus Influence
Harrison Mixbus is praised for its "Knob-per-function" workflow and the warmth of its UI which feels like a physical console. To empower the Beam brand while embracing this look:

1.  **Tactile Skeuomorphism**: Move away from flat "Material Design" buttons.
    *   **Action**: Implement `Beveled` rendering in `QuadBatcher`. Buttons should have a physical "throw" and depth.
    *   **Action**: Use "Bakelite" and "Brushed Aluminum" textures for module backgrounds.
2.  **The Console Workflow**: In `Flux Mode`, modules should resemble vertical console strips where possible.
3.  **Visual Heat**: Controls should have a "glow" (SDF bloom) that intensifies as they are pushed (e.g., Tube Drive).

### 2.2 Branding & Logos
*   **The Flux Logo**: Should be integrated into the "Chassis" of every module, possibly as an engraved metallic badge.
*   **The Emerald Standard**: Maintain the `#219e6c` for active signal paths (the "light in the circuit").

---

## 3. Implementation State Assessment

### 3.1 Strengths (Competitive)
- **High-Performance Rendering**: Our SDF-based UI is sharper than Reaper's bitmap-based themes and more responsive than Logic's heavy frameworks.
- **Generative Core**: Integration of BVC (Analytic Matching Pursuit) gives us a "semantic" audio advantage that traditional DAWs lack.
- **Embedded Scripting**: `FluxScript` allows for rapid prototyping of DSP without leaving the app.

### 3.2 Weaknesses (The "Must-Fix" List)
- **Undo/Redo (P0)**: Industry standard since 1990. We are currently "destructive-only" in the session state.
- **MIDI Piano Roll (P1)**: Essential for creative composition.
- **The "Tape Chassis" UI (P1)**: Current modules look like "Generic Windows". They need to look like "Hardware Units".

---

## 4. Design Update Roadmap (Mixbus Style)

### Week 1: The "Chassis" Overhaul
- [x] Implement `drawBevelRect` in `QuadBatcher`.
- [x] Implement "Console" color palette in `Theme`.
- [ ] Create `ConsoleChannelStrip` component to replace `GenericNodeEditor` for primary tracks.
- [ ] Add "Texture Support" to `Component` to allow for brushed metal/wood grain overlays.

### Week 2: Tactile Feedback
- [x] Refactor `RotaryKnob` to use a 3D-shaded look with realistic shadows.
- [x] Implement "LED Segment" meters (Mixbus style) instead of smooth bars.
- [x] Add "Engraved" text rendering style (Dark text with a light "inner shadow" offset) in `AudioModule`.

### Week 3: Brand Integration
- [ ] Add the Flux Logo badge to the `TopBar` and `MasterStrip`.
- [ ] Implement the "Emerald Glow" shader for active cables.
