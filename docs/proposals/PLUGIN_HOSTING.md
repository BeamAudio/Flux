# Plugin Hosting Strategy: VST3 & CLAP

**Date:** 2026-02-03
**Status:** PROPOSAL

## Executive Summary
This document outlines the strategy for adding external plugin support to **Beam Audio Flux** without incurring licensing fees. The focus is on **VST3** (via GPLv3 or standard free license) and **CLAP** (MIT License).

---

## 1. Licensing & Fees

### 1.1 VST3 (Steinberg)
*   **Cost**: Free. Steinberg does not charge for the VST3 SDK.
*   **Open Source (GPLv3)**: If we release Beam Audio Flux under GPLv3, we can use the SDK without signing agreements.
*   **Proprietary**: Since Beam Audio Flux is "Proprietary", we must register as a developer on the Steinberg website and sign the **VST3 License Agreement**. This is **free of charge**, but legally binding.
*   **VST2**: Discontinued. We cannot legally add new VST2 hosting support.

### 1.2 CLAP (Clever Audio Plugin)
*   **Cost**: Free.
*   **License**: MIT.
*   **Constraint**: None. We can use it in proprietary software without registration.
*   **Adoption**: Supported by Bitwig, u-he, and a growing list of developers.

---

## 2. Implementation Strategy

### 2.1 Architecture
We will create a `PluginNode` abstract base class that adapts external plugins into the Flux graph.

```cpp
class PluginNode : public FluxNode {
    // Common interface for VST3 and CLAP
    virtual void showEditor() = 0;
    virtual void hideEditor() = 0;
    virtual void processInterleaved(float* buffer, int frames) = 0;
};
```

### 2.2 Hosting Wrappers
1.  **VST3 Host**:
    *   Requires `vst3_sdk` (C++).
    *   We need to implement `IComponentHandler`, `IPlugFrame`, and `IEditController`.
    *   **Recommendation**: Use a lightweight single-header VST3 host wrapper if available, or implement a minimal host to keep the "No-Framework" philosophy.

2.  **CLAP Host**:
    *   Requires `clap-helpers` (C) or just the headers.
    *   Much simpler ABI (Application Binary Interface) than VST3 (COM-like).
    *   **Recommendation**: Implement CLAP first as a proof-of-concept due to its simplicity.

---

## 3. Roadmap

1.  **Phase 1**: **CLAP Support**.
    *   Add `clap` headers to `third_party/`.
    *   Implement `ClapHostNode` that loads a `.clap` DLL.
    *   Verify audio processing.

2.  **Phase 2**: **VST3 Support**.
    *   Register with Steinberg (if proprietary) or switch to GPLv3.
    *   Add `vst3` headers.
    *   Implement `Vst3HostNode`.

3.  **Phase 3**: **Sandbox / Bridging**.
    *   Plugins crash. Running them in the main process crashes the DAW.
    *   Future goal: Run plugins in a child process (like Bitwig/Reaper).

---

## 4. Action Items
*   [ ] Download CLAP headers (`third_party/clap`).
*   [ ] Create `src/engine/hosting/clap_host.cpp`.
*   [ ] Decide on licensing path for VST3 (Register vs. GPL).
