#include "audio_processor.hpp"

namespace Beam {

AudioProcessor::AudioProcessor() {
}

AudioProcessor::~AudioProcessor() {
}

int AudioProcessor::getNumParameters() const {
    return static_cast<int>(m_parameters.size());
}

std::string AudioProcessor::getParameterName(int parameterIndex) const {
    if (parameterIndex >= 0 && parameterIndex < m_parameters.size()) {
        return m_parameters[parameterIndex]->getName();
    }
    return "";
}

float AudioProcessor::getParameter(int parameterIndex) const {
    if (parameterIndex >= 0 && parameterIndex < m_parameters.size()) {
        return m_parameters[parameterIndex]->getValue();
    }
    return 0.0f;
}

void AudioProcessor::setParameter(int parameterIndex, float newValue) {
    if (parameterIndex >= 0 && parameterIndex < m_parameters.size()) {
        m_parameters[parameterIndex]->setValue(newValue);
    }
}

std::string AudioProcessor::getParameterText(int parameterIndex, float value) const {
    // Default implementation just returns the value as a string
    return std::to_string(value);
}

void AudioProcessor::addParameter(std::shared_ptr<Parameter> parameter) {
    m_parameters.push_back(parameter);
    m_namedParameters[parameter->getName()] = parameter;
}

std::shared_ptr<Parameter> AudioProcessor::getParameterByIndex(int index) const {
    if (index >= 0 && index < m_parameters.size()) {
        return m_parameters[index];
    }
    return nullptr;
}

std::shared_ptr<Parameter> AudioProcessor::getParameterByName(const std::string& name) const {
    auto it = m_namedParameters.find(name);
    if (it != m_namedParameters.end()) {
        return it->second;
    }
    return nullptr;
}

void AudioProcessor::setCurrentPlaybackState(bool isPlaying, double currentTimeSeconds, double tempo) {
    // Default implementation does nothing
}

} // namespace Beam


