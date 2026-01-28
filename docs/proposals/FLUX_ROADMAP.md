# Flux Engine Development Roadmap

## Phase 1: Graphics & UI Foundation
- [x] **Asset Manager**: Centralized caching for textures and fonts. (Completed: 2026-01-28)
- [x] **SDF Path Rendering**: Smooth anti-aliased curves for EQ/Compressor graphs. (Completed: 2026-01-28)
- [ ] **Vector Font Rendering**: Support for TrueType/OpenType fonts via stb_truetype.

## Phase 2: DSP Core & Performance
- [x] **SIMD Summing**: Use SSE/AVX for audio buffer mixing. (Completed: 2026-01-28)
- [x] **Denormal Protection**: Safety checks for IIR filters to prevent CPU spikes. (Completed: 2026-01-28)
- [ ] **Latency Compensation (PDC)**: Time-alignment for plugin-induced delay.

## Phase 3: DAW Features
- [x] **Automation System**: Time-stamped parameter changes. (Completed: 2026-01-28)
- [x] **MIDI Engine**: MIDI event buffers and basic synth node. (Completed: 2026-01-28)
- [ ] **Sample Rate Conversion**: High-quality resampling for mismatched files.

## Phase 4: Integration
- [ ] **VST3/CLAP Hosting**: Support for external plugins.
