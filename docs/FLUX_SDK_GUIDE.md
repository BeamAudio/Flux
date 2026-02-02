# Flux SDK - Developer Guide (v2.0)

## 1. Introduction
The **Flux SDK 2.0** uses a decoupled "Model/Processor" architecture. This ensures that your DSP logic runs on the high-priority audio thread without interruptions from the UI, while still providing a flexible and easy-to-use API.

## 2. Core Concepts: The Two Halves

### 2.1 The Processor (`FluxPluginProcessor`)
This is the real-time worker. It contains your DSP mathematical logic.
- **Rules**: No allocations, no deletions, no locking.
- **Interface**: Implements `processBlock(const float* input, float* output, int totalSamples)`.

### 2.2 The Node (`FluxPlugin`)
This is the persistent model that lives on the main thread.
- **Responsibility**: Manages parameters and UI.
- **Factory**: Implements `createProcessor()` to spawn its worker.

## 3. Step-by-Step: Creating a New Effect

### Step 1: Define the Processor
Create your DSP worker. Use `getParam(index)` to read values snapped by the engine for the current frame.

```cpp
class MyDistortionProcessor : public Beam::FluxPluginProcessor {
public:
    void processBlock(const float* input, float* output, int totalSamples) override {
        float drive = getParam(0); // "Drive" is our first param
        for (int i = 0; i < totalSamples; ++i) {
            output[i] = std::tanh(input[i] * drive);
        }
    }
};
```

### Step 2: Define the Node
Define the user-facing effect and its parameters.

```cpp
class MyDistortion : public Beam::FluxPlugin {
public:
    MyDistortion(int bufSize, float sr) : FluxPlugin("Distortion", bufSize, sr) {
        // Register params in order (indices 0, 1, 2...)
        addParam("Drive", 1.0f, 20.0f, 1.0f);
    }
    
    std::unique_ptr<FluxProcessor> createProcessor() override {
        return std::make_unique<MyDistortionProcessor>();
    }
};
```

### Step 3: Registration
Add your node to the factory in `src/interface/views/workspace.cpp` (or equivalent registry).

## 4. Best Practices for Beam 2.0

- **Parameter Order**: The indices in `getParam(index)` correspond to the order you called `addParam()` in the constructor.
- **Member State**: Always store filter states (z-1) or delay buffers inside the **Processor**, not the Node.
- **Interleaving**: By default, `processBlock` provides interleaved stereo samples (`L, R, L, R...`).

## 5. Advanced: Custom UI
If the auto-generated knobs aren't enough, override `createEditor()` in your `FluxPlugin` class to return a custom `Component`.
