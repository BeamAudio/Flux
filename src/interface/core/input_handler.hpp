#ifndef INPUT_HANDLER_HPP
#define INPUT_HANDLER_HPP

#include "interface/core/component.hpp"
#include <vector>
#include <memory>

namespace Beam {

class InputHandler {
public:
    void addComponent(std::shared_ptr<Component> component);
    bool handleMouseDown(float x, float y, int button, bool shift = false);
    bool handleMouseUp(float x, float y, int button, bool shift = false);
    bool handleMouseMove(float x, float y, bool shift = false);
    bool handleMouseWheel(float x, float y, float delta, bool shift = false);
    void update(float dt);
    void render(QuadBatcher& batcher, float dt, float screenW, float screenH);

private:
    std::vector<std::shared_ptr<Component>> m_components;
    std::shared_ptr<Component> m_focusedComponent;
    std::shared_ptr<Component> m_capturedComponent;
};

} // namespace Beam

#endif // INPUT_HANDLER_HPP






