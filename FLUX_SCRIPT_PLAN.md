# FluxScript Improvement Plan

**Goal:** Evolve FluxScript from a basic scripting capability into a robust, high-performance DSL (Domain Specific Language) for real-time audio synthesis and effect creation.

## 1. Language Features
*   **Type System**:
    *   Strict Typing: Introduce `float`, `int`, `bool` types to enable better optimization.
    *   Arrays/Buffers: First-class support for `buffer[i]` access (essential for delays/granular).
*   **Control Flow**: Add `while` loops and `switch` statements (currently limited/missing).
*   **Standard Library**:
    *   Math: `sin()`, `cos()`, `tanh()`, `exp()`, `log()`.
    *   DSP: `delay()`, `filter()`, `phasor()`, `sample()` (for playback).

## 2. Compiler & Runtime (AOT/JIT)
*   **JIT Compilation**: Move beyond the current interpreter. Investigate LLVM integration or a custom bytecode JIT to achieve near-C++ performance for `process()` loops.
*   **AOT (Ahead-of-Time)**: Improve the transpiler (FluxScript -> C++) to generate completely standalone C++ `FluxNode` classes that can be compiled into the main binary or DLLs.

## 3. Integration & DX
*   **Hot Reloading**: The ability to edit a `.flux` script and hear changes instantly without restarting the DAW (requires graph lock/swap safety).
*   **Editor Support**: A basic LSP (Language Server Protocol) implementation or syntax highlighting rules for VS Code / Notepad++.
*   **Debugger**: A simple way to `print()` or inspect variables frame-by-frame.

## 4. Execution Steps
1.  **Language Spec**: Formalize the FluxScript grammar.
2.  **StdLib Implementation**: Bind C++ DSP functions to the script runtime.
3.  **JIT Investigation**: Prototype a simple JIT for arithmetic operations.
4.  **Hot Reload**: Implement file watching for script nodes in the engine.
