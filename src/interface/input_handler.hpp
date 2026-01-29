#ifndef INPUT_HANDLER_HPP
#define INPUT_HANDLER_HPP

#include "component.hpp"
#include <vector>
#include <memory>

namespace Beam {

class InputHandler {
public:
    void addComponent(std::shared_ptr<Component> component);
    void handleMouseDown(float x, float y, int button);
    void handleMouseUp(float x, float y, int button);
    void handleMouseMove(float x, float y);
    void update(float dt);
    void render(QuadBatcher& batcher, float dt);

private:
    std::vector<std::shared_ptr<Component>> m_components;
    std::shared_ptr<Component> m_focusedComponent;
};

} // namespace Beam

#endif // INPUT_HANDLER_HPP



