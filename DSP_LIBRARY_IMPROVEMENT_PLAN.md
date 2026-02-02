# DSP Library Improvement Plan

**Goal:** Expand and optimize the BeamEngine DSP library to provide a professional-grade suite of audio processing tools.

## 1. Core Signal Processing
*   **Oversampling Framework**: Implement a standard `Oversampler` class (2x, 4x, 8x) with high-quality polyphase filtering to reduce aliasing in non-linear processors (Saturation, Compressors).
*   **Spectral Processing**:
    *   FFT/IFFT Wrapper: Optimize integration with FFT libraries (e.g., KissFFT or PFFFT) for use in `FluxNode`.
    *   Spectral Modifiers: Add base classes for spectral gating, convolution, and freeze effects.
*   **Delay Lines**:
    *   `InterpolatedDelay`: High-quality Hermite or Sinc interpolation for pitch-shifting delays.
    *   `MultiTapDelay`: Efficient implementation for reverbs and chorus.

## 2. New Algorithm Modules
*   **Analog Modeling**:
    *   **Circuit Components**: Base classes for `Diode`, `Transistor`, `OpAmp` (simplified non-linear equations).
    *   **Filter Topologies**: Implement Sallen-Key and Ladder filters using Zero-Delay Feedback (ZDF) topology for stability at modulation rates.
*   **Dynamics**:
    *   `LookaheadLimiter`: A true mastering limiter with latency compensation.
    *   `TransientShaper`: Envelope-independent attack/sustain modifier.

## 3. Optimization & Safety
*   **SIMD Vectorization**: Identify hot loops (biquads, gain application, mixing) and provide AVX2/NEON optimized paths in `simd_utils.hpp`.
*   **Denormal Protection**: Audit all recursive filters and feedback loops for denormal numbers (add DC offset or use FTZ flags).
*   **Memory Pool**: Implement a real-time safe memory allocator for temporary buffers needed during `process()`.

## 4. Execution Steps
1.  **Audit Existing DSP**: Benchmark current Biquad and Delay nodes.
2.  **Implement Oversampling**: Critical for the "Tube" and "Analog" suite quality.
3.  **Add ZDF Filters**: Replace standard Biquads in the Synthesizer/Filter nodes.
4.  **Optimize**: Profile with VTune/Instruments and apply SIMD where appropriate.
