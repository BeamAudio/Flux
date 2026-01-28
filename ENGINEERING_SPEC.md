# Beam Audio Flux - Engineering Specification v1.0

## 1. Executive Vision
Beam Audio Flux is a high-performance, "no-framework" Digital Audio Workstation (DAW) built with C++20, SDL3, and OpenGL 3.3. It is designed for creative, non-traditional workflows with a focus on manual recording, experimental FX generation, and a tactile analog feel.

## 2. Technical Architecture

### 2.1 Engine (src/engine)
- **AudioEngine**: Manages SDL3 playback and capture streams.
- **FluxGraph**: Node-based DSP graph using Kahn's algorithm for topological sorting.
- **TrackNode**: Handles disk-streaming (playback/record) for the timeline.
- **FluxNode**: Base class for all DSP modules (Effects, Synths, Utilities).

### 2.2 Rendering (src/render)
- **QuadBatcher**: High-performance OpenGL batch renderer minimizing draw calls.
- **SDF Shaders**: Used for smooth anti-aliased UI elements, curves, and waveforms.

### 2.3 Interface (src/interface)
- **Hybrid View**:
  - **Splicing Mode**: Linear timeline for traditional editing.
  - **Flux Mode**: Modular spatial canvas for routing and FX design.
- **Custom UI System**: Built from scratch to ensure maximum reactivity and stylized "analog" aesthetics.

## 3. Development Roadmap

### Phase 1: Core Consolidation (Current)
- [ ] Unify folder structure (In Progress).
- [ ] Implement robust Audio Input/Recording.
- [ ] Link Splicing Timeline with TrackNodes.

### Phase 2: Creative Tools
- [ ] Tape-style saturation and pitch-warping nodes.
- [ ] Bezier-based visual routing with "physical" cable properties.
- [ ] Integrated Waveform Editor with SDF-based precision.

### Phase 3: Expansion
- [ ] VST3/CLAP Hosting support.
- [ ] Collaborative session syncing (UDP-based).

## 4. Design Principles
1. **No-Framework**: Keep dependencies minimal (SDL3, GLAD, miniaudio, nlohmann/json).
2. **Tactile Feedback**: UI should respond with "physics" where appropriate (cables, meters).
3. **Non-Destructive**: All timeline edits are metadata-driven on top of raw audio files.
