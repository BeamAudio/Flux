# Beam Graphics Engine - Architecture & Usage

## 1. Overview
The **Beam Graphics Engine** is a high-performance, batch-rendering system built directly on top of OpenGL 3.3+. It is designed to mimic the aesthetic of high-end analog hardware (consoles, tape machines, rack units) while maintaining 60+ FPS performance even with hundreds of dynamic UI elements.

## 2. Core Components

### 2.1 QuadBatcher (`src/graphics/quad_batcher.hpp`)
The heart of the rendering system. Instead of issuing individual draw calls for every UI element, the `QuadBatcher` aggregates geometry into a single vertex buffer.

- **Batching Strategy**: Accumulates quads, text glyphs, and primitive shapes into a dynamic VBO. Flushes to the GPU only when the buffer is full, the texture state changes, or the shader mode changes.
- **Texture Management**: Handles a built-in 8x8 bitmap font atlas for high-performance text rendering.
- **Draw Commands**:
    - `drawQuad(...)`: Basic colored rectangle.
    - `drawRoundedRect(...)`: SDF-based rounded rectangle with soft edges.
    - `drawText(...)`: Renders strings using the internal font atlas.
    - `drawLine(...)`: Renders lines using thin quads.

### 2.2 Shader System (`src/graphics/ui_shaders.hpp`)
A unified shader architecture that handles multiple rendering modes via a single uber-shader.

- **Vertex Shader**: Handles orthographic projection and passes color/texture coordinates.
- **Fragment Shader**:
    - **Mode 0 (Solid/Gradient)**: Standard interpolated vertex color rendering.
    - **Mode 1 (Textured/Font)**: Samples the font atlas texture and uses the red channel as an alpha mask for sharp text.
    - **Mode 2 (SDF Rounded Rect)**: Uses Signed Distance Fields to procedurally generate resolution-independent rounded corners, borders, and soft shadows.

### 2.3 SDF Rendering (Signed Distance Fields)
To achieve the "analog hardware" look, we avoid using static textures for UI panels. Instead, we use mathematical SDFs in the pixel shader.
- **Advantages**: Infinite resolution (zoomable), tweakable parameters (corner radius, softness) at runtime, and extremely low memory usage.
- **Implementation**: The shader calculates the distance from the pixel to the mathematical edge of a rounded box. `smoothstep` is then used to antialias the edge, creating a buttery-smooth look.

## 3. UI Component System (`src/ui/component.hpp`)
A hierarchical, event-driven UI framework.

- **Lifecycle**: `update(dt)` for logic/animation, `render(batcher)` for drawing.
- **Events**: `onMouseDown`, `onMouseUp`, `onMouseMove`.
- **Propagation**: Events bubble down from the `InputHandler` to the focused component or the component under the mouse.
- **Coordinates**: Everything uses absolute pixel coordinates (Top-Left 0,0). `setBounds` ensures that parent movements propagate to children.

## 4. Usage Guide for Developers

### Creating a New Component
Inherit from `Beam::Component` and implement `render` and `update`.

```cpp
class MyAnalogMeter : public Beam::Component {
public:
    void update(float dt) override {
        // Smoothly decay the needle position
        m_value -= dt * 0.5f;
    }

    void render(Beam::QuadBatcher& batcher) override {
        // Draw the background panel
        batcher.drawRoundedRect(m_bounds.x, m_bounds.y, m_bounds.w, m_bounds.h, 
            10.0f, // Radius
            1.0f,  // Softness (Anti-aliasing)
            0.2f, 0.2f, 0.2f, 1.0f); // Color (Dark Grey)

        // Draw some text
        batcher.drawText("VU", m_bounds.x + 10, m_bounds.y + 10, 12, 1.0f, 1.0f, 1.0f, 1.0f);
    }
private:
    float m_value = 0.0f;
};
```

### Optimizing Performance
- **Minimize State Changes**: Group draw calls that use the same mode (Text vs Rects) together to reduce GPU flushing.
- **Use Batching**: Never call raw OpenGL commands inside a component. Always go through the `batcher`.
