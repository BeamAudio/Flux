#include "audio_module.hpp"
#include "look_and_feel.hpp"

namespace Beam {

void AudioModule::paint(QuadBatcher& g) {
    getLookAndFeel().drawAudioModule(g, *this);
    
    // Render Ports
    if (m_inputPort) m_inputPort->paint(g); 
    if (m_outputPort) m_outputPort->paint(g);
    
    // Note: m_children_legacy is handled by Component::render calling render() on them.
    // If they are in m_children_legacy but not in m_children, they won't be hit-testable by the new system.
}

} // namespace Beam
