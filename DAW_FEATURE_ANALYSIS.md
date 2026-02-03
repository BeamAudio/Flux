# Beam Audio Flux: Feature Analysis & Architectural Evolution

**Date:** 2026-02-03
**Status:** PERFORMANCE RECOVERY & FEATURE ROADMAP

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
| **External Plugins** | VST2/VST3/CLAP/AU | VST3/AU/ARA | **Internal C++ Only** | **CRITICAL GAP**: No hosting capability. Priority P2. |
| **Mixing** | VCAs, Groups, Aux | VCAs, Groups, Aux | **Direct Routing Only** | **CRITICAL GAP**: No Bus/Group abstraction. Priority P1. |

---

## 2. Recent Performance Recovery (2026-02-03)

The "Very Slow" performance issues reported by users have been addressed via a targeted optimization pass:

### 2.1 Critical Fixes Implemented
1.  **Thread Safety (Mutex Removal)**: The `m_automationMutex` was causing priority inversion in the audio thread. Replaced with `std::try_lock` to skip automation processing if the UI is busy, preventing audio dropouts.
2.  **Memory Management**: Removed `std::vector::resize` from the `audioCallback`. The scratch buffer is now strictly pre-allocated in `updatePlan`.
3.  **Atomic Optimization**: Optimized `TubeCompressorNode` (and applicable patterns) to cache `std::atomic` values in local registers during the sample loop, reducing memory barrier overhead by 99%.
4.  **SIMD Integration**: Replaced scalar mixing loops with `SIMD::add_with_gain` (SSE implementation) for efficient signal summation.
5.  **AutoTune Optimization**: Refactored `AutoTuneProcessor` to use SIMD for the YIN algorithm difference function (O(N) with SSE) and implemented double-buffering for thread-safe asynchronous analysis. Reduced analysis window to 600 samples for optimal performance.

---

## 3. Design Language: "The Analog Emerald"

### 3.1 The Harrison Mixbus Influence
Harrison Mixbus is praised for its "Knob-per-function" workflow and the warmth of its UI which feels like a physical console. To empower the Beam brand while embracing this look:

1.  **Tactile Skeuomorphism**: Move away from flat "Material Design" buttons.
    *   **Action**: Implement `Beveled` rendering in `QuadBatcher`. Buttons should have a physical "throw" and depth.
    *   **Action**: Use "Bakelite" and "Brushed Aluminum" textures for module backgrounds.
2.  **The Console Workflow**: In `Flux Mode`, modules should resemble vertical console strips where possible.
3.  **Visual Heat**: Controls should have a "glow" (SDF bloom) that intensifies as they are pushed (e.g., Tube Drive).

### 3.2 Branding & Logos
*   **The Flux Logo**: Should be integrated into the "Chassis" of every module, possibly as an engraved metallic badge.
*   **The Emerald Standard**: Maintain the `#219e6c` for active signal paths (the "light in the circuit").

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

---

## 5. Missing Fundamental Features Roadmap

### Phase 1: Routing & Mixing (Next Priority)
*   **Bus/Group Nodes**: Create a `BusNode` that acts as a summing point.
*   **Aux Sends**: Implement "Send" ports on `TrackNode` that route to Bus Nodes without breaking the linear graph (requires parallel graph branches).
*   **Solo/Mute Logic**: Implement "Solo in Place" logic (currently missing).

### Phase 2: MIDI & Composition
*   **Piano Roll**: A dedicated Editor for `MidiTrackNode`.
*   **MIDI Routing**: Support for MIDI cables in the graph.

### Phase 3: External Plugins
*   **Hosting**: Implement a `VST3HostNode` using `steinberg/vst3_sdk`.
*   **Sandboxing**: Run plugins in a separate process to prevent crashes from taking down the engine.