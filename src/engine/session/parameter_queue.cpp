#include "engine/session/parameter_queue.hpp"
#include "engine/session/parameter.hpp"

namespace Beam {

void ParameterQueue::dispatch() {
    ParameterMessage msg;
    while (m_queue.read(&msg, 1) > 0) {
        if (msg.parameter) {
            msg.parameter->triggerCallback(msg.newValue);
        }
    }
}

} // namespace Beam
