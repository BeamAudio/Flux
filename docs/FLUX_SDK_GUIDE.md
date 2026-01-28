# Flux Abstraction Layer - Developer Guide

## 1. Introduction
The **Flux Abstraction Layer** is designed to let you build powerful audio plugins and custom DSP effects without needing to understand the underlying graphics engine (OpenGL/SDF) or the low-level audio driver (SDL3).

As a developer, your primary workspace is the `Beam::FluxPlugin` class.

## 2. Core Concepts

### 2.1 The FluxPlugin
Every custom effect you build will inherit from `Beam::FluxPlugin`. This base class handles:
- **Audio Routing**: Stereo input/output buffering.
- **Parameter Management**: Thread-safe communication between the UI and Audio threads.
- **GUI Generation**: Automatically creating a standardized, hardware-accelerated UI for your parameters.

### 2.2 Parameters
You define parameters in your plugin's constructor. The system supports floating-point values (knobs) by default.
```cpp
addParam("Drive", 0.0f, 10.0f, 1.0f); // Name, Min, Max, Default
```

### 2.3 Audio Processing
You implement the `processBlock` method. This is where your DSP math lives.
```cpp
void processBlock(const float* input, float* output, int totalSamples) {
    // Your code here
}
```

## 3. Step-by-Step: Creating a New Effect

### Step 1: Create the Class
Create a new header file (e.g., `src/dsp/my_effect.hpp`).

```cpp
#include "flux_plugin.hpp"

namespace Beam {

class MyEffect : public FluxPlugin {
public:
    MyEffect(int bufferSize, float sampleRate) 
        : FluxPlugin("My Effect", bufferSize, sampleRate) 
    {
        // Define your parameters here
        addParam("Intensity", 0.0f, 1.0f, 0.5f);
    }

    void processBlock(const float* input, float* output, int totalSamples) override {
        float intensity = getParam("Intensity");
        
        for (int i = 0; i < totalSamples; ++i) {
            // Simple example: Scale volume
            output[i] = input[i] * intensity;
        }
    }
};

}
```

### Step 2: Register the Effect
Open `src/ui/workspace.hpp` and add your include:
```cpp
#include "../dsp/my_effect.hpp"
```

Then, inside the `addFX` method, add a condition for your new effect string:
```cpp
else if (type == "MyEffect") fxNode = std::make_shared<MyEffect>(1024 * 4, 44100.0f);
```

### Step 3: Add a UI Button
Open `src/ui/sidebar.hpp` and add a button to spawn your effect:
```cpp
batcher.drawText("MY EFFECT", m_bounds.x + 20, yOff + 8, 14, 0.7f, 0.7f, 0.7f, 1.0f);
// ... inside onMouseDown ...
if (onAddFX) onAddFX("MyEffect");
```

## 4. Best Practices
- **Performance**: Avoid allocating memory (new/malloc) inside `processBlock`.
- **State**: Use member variables to store filter history (e.g., `m_lastSample`).
- **Thread Safety**: Always use `getParam()` to read control values; never read from UI variables directly.

## 5. Under the Hood (Optional)
If you *really* need to know:
- **Rendering**: Your parameters generate `Knob` components that use SDF shaders for smooth rendering.
- **Audio**: Your `FluxPlugin` is wrapped in a `FluxNode` which is sorted topologically in the `FluxGraph`.
