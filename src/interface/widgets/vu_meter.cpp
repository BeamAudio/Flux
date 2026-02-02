#include "interface/widgets/vu_meter.hpp"
#include "interface/core/look_and_feel.hpp"

namespace Beam {

void VUMeter::paint(QuadBatcher& g) {
    getLookAndFeel().drawVUMeter(g, *this, getLevel());
    
    Component::paint(g);
}

} // namespace Beam
