#include "audio_processor_value_tree_state.hpp"

namespace Beam {

AudioProcessorValueTreeState::AudioProcessorValueTreeState() {
}

AudioProcessorValueTreeState::~AudioProcessorValueTreeState() {
}

std::shared_ptr<Parameter> AudioProcessorValueTreeState::createAndAddParameter(
    const std::string& parameterID,
    const std::string& parameterName,
    const std::string& parameterLabel,
    float minValue,
    float maxValue,
    float defaultValue
) {
    auto param = std::make_shared<Parameter>(parameterName, minValue, maxValue, defaultValue);
    
    // Set up change notification
    param->onChanged = [this, parameterID](float newValue) {
        auto it = m_listeners.find(parameterID);
        if (it != m_listeners.end() && it->second) {
            it->second(newValue);
        }
    };
    
    m_parameters[parameterID] = param;
    return param;
}

std::shared_ptr<Parameter> AudioProcessorValueTreeState::getParameter(const std::string& parameterID) const {
    auto it = m_parameters.find(parameterID);
    if (it != m_parameters.end()) {
        return it->second;
    }
    return nullptr;
}

float AudioProcessorValueTreeState::getParameterAsFloat(const std::string& parameterID) const {
    auto param = getParameter(parameterID);
    if (param) {
        return param->getValue();
    }
    return 0.0f;
}

void AudioProcessorValueTreeState::setParameterAsFloat(const std::string& parameterID, float newValue) {
    auto param = getParameter(parameterID);
    if (param) {
        param->setValue(newValue);
    }
}

void AudioProcessorValueTreeState::addParameterListener(
    const std::string& parameterID,
    std::function<void(float)> listener
) {
    m_listeners[parameterID] = listener;
    
    // Also attach to the parameter's onChange event if it exists
    auto param = getParameter(parameterID);
    if (param) {
        auto oldCallback = param->onChanged;
        param->onChanged = [this, parameterID, listener, oldCallback](float newValue) {
            listener(newValue);
            if (oldCallback) {
                oldCallback(newValue);
            }
        };
    }
}

void AudioProcessorValueTreeState::removeParameterListener(
    const std::string& parameterID,
    std::function<void(float)> listener
) {
    m_listeners.erase(parameterID);
}

const std::map<std::string, std::shared_ptr<Parameter>>& AudioProcessorValueTreeState::getParameters() const {
    return m_parameters;
}

void AudioProcessorValueTreeState::resetToDefault() {
    // Note: Our Parameter class doesn't store default values, so we'd need to enhance it
    // For now, this is a placeholder implementation
    for (auto& pair : m_parameters) {
        // Reset to some default value - in a real implementation, we'd store the default
        pair.second->setValue(pair.second->getMin()); // Just reset to minimum for now
    }
}

} // namespace Beam
