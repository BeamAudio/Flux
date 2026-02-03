#include "interface/editors/spectrum_module.hpp"
#include "interface/core/look_and_feel.hpp"

namespace Beam {

void SpectrumModule::paint(QuadBatcher& batcher) {
    getLookAndFeel().drawSpectrumAnalyzer(batcher, *this);
    
    // drawSpectrumGraph(batcher); // Deprecated, using SpectrumDisplay child

    // Ports
    for (auto& p : m_inputPorts) p->paint(batcher);
    for (auto& p : m_outputPorts) p->paint(batcher);
    
    Component::paint(batcher);
}

} // namespace Beam
