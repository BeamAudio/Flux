#include "slider_modular.hpp"
#include "look_and_feel.hpp"

namespace Beam {

void ModularSlider::paint(QuadBatcher& g) {
    getLookAndFeel().drawModularSlider(g, *this);
    
    Component::paint(g);
}

} // namespace Beam
