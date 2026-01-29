#include "application_base.hpp"

namespace Beam {

ApplicationBase* ApplicationBase::s_instance = nullptr;

ApplicationBase::ApplicationBase() {
    s_instance = this;
}

ApplicationBase::~ApplicationBase() {
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

void ApplicationBase::suspended() {
    // Default implementation does nothing
}

void ApplicationBase::resumed() {
    // Default implementation does nothing
}

void ApplicationBase::systemRequestedQuit() {
    quit();
}

void ApplicationBase::quit() {
    m_shouldQuit = true;
}

ApplicationBase* ApplicationBase::getInstance() {
    return s_instance;
}

} // namespace Beam


