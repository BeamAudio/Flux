#include "../src/interface/input_handler.hpp"
#include <iostream>
#include <memory>
#include <cassert>

namespace Beam {

class TestComponent : public Component {
public:
    bool clicked = false;
    void render(QuadBatcher& batcher, float dt, float screenW, float screenH) override {}
    bool onMouseDown(float x, float y, int button) override {
        clicked = true;
        return true;
    }
};

} // namespace Beam

int main() {
    Beam::InputHandler handler;
    auto comp = std::make_shared<Beam::TestComponent>();
    comp->setBounds(10, 10, 100, 100);
    handler.addComponent(comp);

    // Test miss
    handler.handleMouseDown(5, 5, 0);
    assert(!comp->clicked);
    std::cout << "UI Test: Miss handled correctly." << std::endl;

    // Test hit
    handler.handleMouseDown(50, 50, 0);
    assert(comp->clicked);
    std::cout << "UI Test: Hit handled correctly." << std::endl;

    return 0;
}




