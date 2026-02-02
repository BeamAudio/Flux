# Beam Audio Flux: Functional Maturity Assessment

**Version:** 1.0.0-FUNCTIONAL
**Benchmarked Against:** Reaper (Routing), Cubase (Precision), Logic (Workflow)

---

## 1. Core Engine & DSP Performance

| Capability | SOTA Standard | Flux Current State | Analysis |
| :--- | :--- | :--- | :--- |
| **Routing Topology** | Matrix / Arbitrary | **Node-Based DAG** | **SUPERIOR:** Flux allows Bitwig-style modular routing at the top level. Reaper is close, but Flux is visually explicit. |
| **Latency (PDC)** | Sample-Accurate | **Implemented (v1.0)** | **MATCHED:** PDC now injects `PDCDelayNodes` during graph compilation. Handles chain offsets accurately. |
| **Multicore Utility** | High (Anticipative) | **Single-Threaded Chain** | **INFERIOR:** While the Audio Engine is lock-free, the `RenderPlan` is a linear list. We need "Branch Parallelism" for parallel tracks. |
| **Sample Accuracy** | Industry Standard | **Ramped Parameters** | **MATCHED:** Recent implementation of one-pole smoothing in `Parameter::getNextValue()` eliminates zipper noise. |

---

## 2. Mixing & Signal Flow

| Capability | SOTA Standard | Flux Current State | Analysis |
| :--- | :--- | :--- | :--- |
| **Channel Strips** | Fixed/Templates | **Modular Modules** | **STRENGTH:** The "Mixbus" look is applied, but the layout is still modular. We lack a "Mixer View" (Consolidated Faders). |
| **Bussing/Groups** | Folder Tracks | **Manual Patching** | **FLEXIBLE:** You can create a "Bus" by routing multiple outputs to one Gain Node. Lacks "Folder" ergonomics. |
| **Sidechaining** | Dedicated Inputs | **Ad-hoc Patching** | **GAP:** `FluxNode` API needs a `getSidechainInput()` to distinguish control signals from carrier signals. |
| **Metering** | LUFS / True-Peak | **Pro-Grade API** | **MATCHED:** `MeterSource` + `LoudnessModule` provide EBU R128 data. |

---

## 3. Editing & Timeline (The "Splicing" Mode)

| Capability | SOTA Standard | Flux Current State | Analysis |
| :--- | :--- | :--- | :--- |
| **Non-Linear Edit** | Destructive/Non-Dest | **Metadata-Driven** | **MATCHED:** Regions use offsets into source files. |
| **Tools** | Smart Tools | **Scissors / Glue** | **BASIC:** Lacks "Slip" editing, "Time-Stretch" (Phase Vocoder), and "Fade" handles. |
| **Crossfades** | Auto-Linear/Log | **None** | **CRITICAL GAP:** Cutting regions currently causes clicks. Need 5ms auto-crossfade. |
| **Recording** | Background I/O | **Lock-Free Thread** | **MATCHED:** High-stability recording implemented via `LockFreeBuffer`. |

---

## 4. Automation & Creative Control

| Capability | SOTA Standard | Flux Current State | Analysis |
| :--- | :--- | :--- | :--- |
| **Automation Envelopes**| Visual Splines | **Underlying Lanes** | **INCOMPLETE:** The engine supports it, but there is no UI to draw the curves. |
| **MIDI Integration** | MPE / VST3 | **Basic Internal** | **GAP:** No Piano Roll, no external plugin hosting. Flux is currently a "Closed Environment." |
| **Scripting** | Lua / Python | **FluxScript (C++)** | **SUPERIOR:** Native-performance DSP scripting via runtime compilation. |

---

## 5. Critical Functional Roadmap (Next 3 Tasks)

### 1. The "Click-Free" Edit (Crossfades)
*   **Implementation**: Update `TrackNode` to apply a small gain ramp (fade-in/out) at the start/end of every region execution.
*   **Goal**: Professional transparency during splicing.

### 2. Sidechain Infrastructure
*   **Implementation**: Update `FluxNode` to support `PortType::Sidechain`. Update dynamics nodes (`FET76`, `Opto2A`) to listen to this port if connected.
*   **Goal**: Professional mixing capabilities (Duckers, De-essers).

### 3. The "Infinite Memory" (Undo/Redo)
*   **Implementation**: `Command` pattern for `Parameter::setValue` and `FluxGraph::connect`.
*   **Goal**: Move from "Experimental Tool" to "Reliable Workstation."

---

## 6. Competitive Advantage Summary
Beam Audio Flux is currently a **High-Performance Analog Sandbox**. It outperforms Reaper in visual routing ergonomics and Logic in raw DSP transparency (no hidden "smart" processing). It matches Harrison Mixbus in aesthetic warmth. Its primary functional deficit is **Composition Ergononomics** (Undo, Piano Roll, Fades).
