# Proposal: The Analog Flux Expansion - Physics-Based Engine & FX Suite

## 1. Executive Summary
This expansion transforms Beam Audio Flux from a digital router into a living, breathing analog ecosystem. We will implement **Real-Time Physics-Based Modeling (PBM)** for the entire signal path (Tapes, Cables, Master) and introduce 30 high-fidelity analog-emulated processors.

---

## 2. Core Physics: The "Analog Warmth" Layer

### 2.1 Tape Machine Simulation (The Tape Node)
Instead of simple clipping, we will model the physical properties of magnetic tape:
- **Magnetic Hysteresis**: Using the **Langevin function** approximation to model how iron oxide particles retain charge. This creates non-linear saturation that is level-dependent.
- **Tape Speed & Wow/Flutter**: Implementing fractional delay lines modulated by a dual-component noise source (deterministic periodicity for Wow, stochastic high-frequency jitter for Flutter).
- **Head Bump**: A low-frequency resonance simulation based on the physical distance between the playback and record heads.

### 2.2 Cable Capacitance (The Routing Layer)
Cables in the Flux view will no longer be "perfect" digital connections:
- **Impedance Modeling**: Every virtual cable will have a "Length" and "Quality" parameter.
- **Low-Pass Roll-off**: High-frequency loss modeled via a 1-pole RC filter, simulating real-world capacitance where longer cables subtly dim the top end.

### 2.3 Master Transformer Saturation (The Master Node)
The Master Node will receive a "Console Grade" transformer simulation:
- **Odd-Harmonic Distortion**: Modeling the core saturation of a Nickel/Steel transformer.
- **Crosstalk**: A tiny percentage of Left-channel signal leaking into the Right-channel (and vice versa) based on frequency, enhancing the "3D" analog image.

---

## 3. The FX Suite (30 Analog Processors)

### 3.1 Equalizers (The Tone Shapers)
1. **Tube-P**: Passive Pultec-style (Inductor-based curves, simultaneous Boost/Attenuate).
2. **Console-E**: British 4000-series style (Aggressive, surgical, saturated).
3. **Vintage-G**: German 1950s discrete EQ (Broad, musical shelves).
4. **Graphic-10**: Inductor-based 10-band (Fixed frequency, proportional Q).
5. **Air-Lift**: Mastering EQ focused on 20kHz+ "Air" bands using high-shelf oversampling.

### 3.2 Compressors (The Dynamic Sculptors)
1. **Opto-2A**: Light-dependent resistor model (Slow, multi-stage release).
2. **FET-76**: Field-Effect Transistor model (Ultra-fast attack, "All-buttons-in" grit).
3. **VCA-Bus**: The "Glue" compressor (Punchy, linear, bus-focused).
4. **Vari-Mu**: Remote-cutoff tube model (Creamy compression, gain-dependent ratio).
5. **Level-Master**: 1940s style broadcast leveler (Smooth, automatic gain riding).

### 3.3 Multi-Band Compressors (The Precision Control)
1. **Tri-Flux**: 3-band VCA with Linkwitz-Riley crossovers.
2. **Dyn-Air**: 4-band focusing on transient control in high-frequencies.
3. **Warm-Sub**: 2-band optimized for separating sub-harmonics from punch.
4. **Linear-MB**: Multi-band with zero phase shift (Modern/Transparent).
5. **Valve-MB**: Multi-band where each crossover has its own tube saturation stage.

### 3.4 Limiters (The Ceiling)
1. **Tube-Peak**: Soft-knee tube limiter for "warm" loudness.
2. **Brick-Wall**: Zero-tolerance digital look-ahead limiter.
3. **Clip-Iron**: Hard-clipper simulating transformer core saturation.
4. **Soft-Ceil**: Mastering limiter with variable knee and spectral balancing.
5. **Vintage-Stop**: 1176-style limiter mode (Aggressive, fast).

### 3.5 Reverbs (The Space)
1. **Steel-Plate**: Physical model of a 2x3 meter vibrating steel sheet.
2. **Golden-Hall**: Algorithmic reverb with randomized delay modulation.
3. **Copper-Spring**: Dual-spring tank simulation (Boingy, tactile).
4. **Cathedral-SDF**: Ray-traced algorithmic reverb using the DAW's SDF logic for room shapes.
5. **Grain-Verb**: Granular-based pitch-shifting reverb for ethereal textures.

### 3.6 Delays (The Time)
1. **Echo-Plex**: Tape-based delay with motor-speed wow and motor-noise.
2. **BBD-Bucket**: Bucket Brigade Device model (Dark, filtered repeats with clock-noise).
3. **Ping-Pong-Pro**: Stereo-spread delay with cross-feedback filters.
4. **Reverse-Flow**: Real-time buffer reversal with analog "pitch-flip" artifacts.
5. **Space-Shifter**: Delay with integrated pitch-shifting in the feedback loop.

---

## 4. Visual Identity & UI Strategy
- **SDF-Driven Hardware**: Each module will have a unique faceplate rendered via SDF (brushed aluminum, rusted iron, bakelite plastic).
- **Physical Meters**: Needle ballistics will follow actual VU/PPM standards (Standard BS 60268-17).
- **Interactive Graphs**: EQs and Compressors will feature real-time frequency/transfer-function curves using smooth anti-aliased paths.

---

## 5. Implementation Roadmap
1. **Step 1**: Implement the `AnalogComponent` base class for shared saturation/noise logic.
2. **Step 2**: Vectorize (SIMD) the core DSP kernels to ensure 30+ instances can run simultaneously.
3. **Step 3**: Rewrite the `Sidebar` to categorize these 30 nodes into sub-menus.
4. **Step 4**: Apply the "Global Warmth" physics to the `AudioEngine` processing loop.
