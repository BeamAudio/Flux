#ifndef AUDIO_PROCESSOR_VALUE_TREE_STATE_HPP
#define AUDIO_PROCESSOR_VALUE_TREE_STATE_HPP

#include "../session/parameter.hpp"
#include <map>
#include <string>
#include <functional>
#include <memory>

namespace Beam {

/**
 * @class AudioProcessorValueTreeState
 * @brief Manages parameters for an audio processor, similar to JUCE's AudioProcessorValueTreeState
 */
class AudioProcessorValueTreeState {
public:
    /**
     * @brief Constructor
     */
    AudioProcessorValueTreeState();

    /**
     * @brief Destructor
     */
    ~AudioProcessorValueTreeState();

    /**
     * @brief Creates a parameter and adds it to the state
     */
    std::shared_ptr<Parameter> createAndAddParameter(
        const std::string& parameterID,
        const std::string& parameterName,
        const std::string& parameterLabel,
        float minValue,
        float maxValue,
        float defaultValue
    );

    /**
     * @brief Gets a parameter by ID
     */
    std::shared_ptr<Parameter> getParameter(const std::string& parameterID) const;

    /**
     * @brief Gets the value of a parameter as a float
     */
    float getParameterAsFloat(const std::string& parameterID) const;

    /**
     * @brief Sets the value of a parameter
     */
    void setParameterAsFloat(const std::string& parameterID, float newValue);

    /**
     * @brief Adds a parameter listener
     */
    void addParameterListener(
        const std::string& parameterID,
        std::function<void(float)> listener
    );

    /**
     * @brief Removes a parameter listener
     */
    void removeParameterListener(
        const std::string& parameterID,
        std::function<void(float)> listener
    );

    /**
     * @brief Gets all parameters
     */
    const std::map<std::string, std::shared_ptr<Parameter>>& getParameters() const;

    /**
     * @brief Resets all parameters to their default values
     */
    void resetToDefault();

private:
    std::map<std::string, std::shared_ptr<Parameter>> m_parameters;
    std::map<std::string, std::function<void(float)>> m_listeners;
};

} // namespace Beam

#endif // AUDIO_PROCESSOR_VALUE_TREE_STATE_HPP
