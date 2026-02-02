#include "interface/widgets/meter.hpp"
#include "interface/core/look_and_feel.hpp"

namespace Beam {

void LuminousMeter::paint(QuadBatcher& g) {
    getLookAndFeel().drawLuminousMeter(g, *this, getLevel(), getPeak());
    
    Component::paint(g);
}

} // namespace Beam
