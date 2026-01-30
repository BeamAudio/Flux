#include "knob.hpp"
#include "look_and_feel.hpp"

namespace Beam {

void Knob::paint(QuadBatcher& g) {
    float valueNorm = (getValue() - getMin()) / (getMax() - getMin());
    getLookAndFeel().drawKnob(g, *this, valueNorm);
    
    Component::paint(g);
}

} // namespace Beam
