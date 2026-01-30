#include "spectrum_module.hpp"
#include "look_and_feel.hpp"

namespace Beam {

void SpectrumModule::paint(QuadBatcher& batcher) {
    getLookAndFeel().drawSpectrumAnalyzer(batcher, *this);
    
    drawSpectrumGraph(batcher);

    // Ports
    if (m_inputPort) m_inputPort->paint(batcher);
    if (m_outputPort) m_outputPort->paint(batcher);
    
    Component::paint(batcher);
}

} // namespace Beam
