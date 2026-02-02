#include "engine/session/parameter_queue.hpp"
#include "engine/session/parameter.hpp"

namespace Beam {

void ParameterQueue::dispatch() {
    std::vector<ParameterMessage> localMessages;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_messages.empty()) return;
        localMessages.swap(m_messages);
    }

    for (auto& msg : localMessages) {
        if (msg.parameter) {
            msg.parameter->triggerCallback(msg.newValue);
        }
    }
}

} // namespace Beam
