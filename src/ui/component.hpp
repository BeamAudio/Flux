#ifndef COMPONENT_HPP
#define COMPONENT_HPP

#include <vector>
#include <memory>
#include <string>

namespace Beam {

struct Rect {
    float x, y, w, h;
    bool contains(float px, float py) const {
        return px >= x && px <= x + w && py >= y && py <= y + h;
    }
};

class Component {
public:
    virtual ~Component() = default;

    virtual void update(float dt) {}
    virtual void render() = 0;

    virtual bool onMouseDown(float x, float y, int button) { return false; }
    virtual bool onMouseUp(float x, float y, int button) { return false; }
    virtual bool onMouseMove(float x, float y) { return false; }

    void setBounds(float x, float y, float w, float h) { m_bounds = {x, y, w, h}; }
    const Rect& getBounds() const { return m_bounds; }

protected:
    Rect m_bounds{0, 0, 0, 0};
    bool m_isVisible = true;
    bool m_isEnabled = true;
};

} // namespace Beam

#endif // COMPONENT_HPP
