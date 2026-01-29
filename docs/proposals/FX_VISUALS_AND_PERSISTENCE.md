# Engineering Spec: FX Visual Feedback, Project Persistence, and Export Engine

## 1. FX Visual Feedback (The "Pro" Interface)

To reach industry standards, each analog emulation must provide real-time visual telemetry.

### 1.1 Dynamics (Compressors/Limiters)
- **Gain Reduction (GR) Meter**: A dedicated reverse-acting meter (standard BS 60268-10) showing how many dB the signal is being attenuated.
- **Transfer Function Graph**: A 2D plot showing Input vs. Output level, visualizing the "Knee" and "Ratio" of the compression curve.
- **Models Affected**: Opto-2A, FET-76, VCA-Bus, Vari-Mu, Tube Limiter.

### 1.2 Equalizers
- **Spectrum Overlay**: A real-time FFT-based analyzer showing the signal spectrum *before* and *after* the EQ.
- **Composite Curve**: A single, smooth SDF-rendered path representing the combined effect of all active EQ bands.
- **Models Affected**: Tube-P EQ, Console-E, Vintage-G.

### 1.3 Space & Time (Reverb/Delay)
- **Stereo Field Visualizer (Goniometer)**: Visualizing the width and phase correlation of the wet signal.
- **Decay Envelope**: A visual representation of the RT60 decay time.

---

## 2. Project Persistence (.flux files)

We will implement a robust JSON-based serialization engine using `nlohmann/json`.

### 2.1 Schema Definition
- **Metadata**: Project name, Sample Rate, Tempo.
- **Graph Topology**: 
    - Node list (Type, ID, Position, Parameter Values).
    - Connection list (Source Node/Port to Destination Node/Port).
- **Timeline State**:
    - Track list (Name, Volume, Pan).
    - Region list (Source File Path, Start Frame, Duration, Offset).

### 2.2 Atomic Saving
To prevent file corruption, we will save to a temporary file first and then rename it to the target `.flux` file.

---

## 3. The Rendering Engine (Offline Export)

The "Export to File" feature will use a specialized high-speed processing loop.

### 3.1 Architecture
- **OfflineContext**: A non-real-time audio callback that pulls data from the `FluxGraph` as fast as the CPU allows.
- **Encoder Bridge**:
    - **WAV**: Direct PCM write via `dr_wav`.
    - **MP3/FLAC**: Integration with `miniaudio` encoding capabilities.
- **Normalization**: Optional pass to ensure the peak level hits 0.0dB or a user-defined ceiling.

---

## 4. Implementation Strategy
1. **Phase 1**: Add telemetry getters to `FluxNode` (e.g., `getLatestGR()`).
2. **Phase 2**: Create specialized UI components (`GRMeter`, `ResponseGraph`).
3. **Phase 3**: Implement `ProjectManager::serialize` and `deserialize`.
4. **Phase 4**: Develop the `OfflineRenderer` class and add an "EXPORT" button to the Top Bar.
