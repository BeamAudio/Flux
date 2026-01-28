#ifndef AUDIO_PROCESSOR_HPP
#define AUDIO_PROCESSOR_HPP

#include "../session/parameter.hpp"
#include "../engine/midi_event.hpp"
#include <string>
#include <vector>
#include <memory>
#include <map>

namespace Beam {

/**
 * @class AudioProcessor
 * @brief Base class for audio processing units, similar to JUCE's AudioProcessor
 */
class AudioProcessor {
public:
    AudioProcessor();
    virtual ~AudioProcessor();

    /**
     * @brief Called before audio processing begins
     */
    virtual void prepareToPlay(double sampleRate, int samplesPerBlock) = 0;

    /**
     * @brief Called after audio stops
     */
    virtual void releaseResources() = 0;

    /**
     * @brief Main processing function
     */
    virtual void processBlock(float** audioInputOutput, int numInputChannels, int numOutputChannels, int numSamples, const MIDIBuffer& midiMessages) = 0;

    /**
     * @brief Returns the name of this processor
     */
    virtual std::string getName() const = 0;

    /**
     * @brief Returns the number of input channels
     */
    virtual int getNumInputChannels() const = 0;

    /**
     * @brief Returns the number of output channels
     */
    virtual int getNumOutputChannels() const = 0;

    /**
     * @brief Returns the total number of parameters
     */
    int getNumParameters() const;

    /**
     * @brief Returns the name of a parameter
     */
    virtual std::string getParameterName(int parameterIndex) const;

    /**
     * @brief Returns the value of a parameter
     */
    virtual float getParameter(int parameterIndex) const;

    /**
     * @brief Sets the value of a parameter
     */
    virtual void setParameter(int parameterIndex, float newValue);

    /**
     * @brief Returns the text representation of a parameter's value
     */
    virtual std::string getParameterText(int parameterIndex, float value) const;

    /**
     * @brief Adds a parameter to this processor
     */
    void addParameter(std::shared_ptr<Parameter> parameter);

    /**
     * @brief Gets a parameter by index
     */
    std::shared_ptr<Parameter> getParameterByIndex(int index) const;

    /**
     * @brief Gets a parameter by name
     */
    std::shared_ptr<Parameter> getParameterByName(const std::string& name) const;

    /**
     * @brief Called when the play head state changes
     */
    virtual void setCurrentPlaybackState(bool isPlaying, double currentTimeSeconds, double tempo);

    /**
     * @brief Returns the current sample rate
     */
    double getSampleRate() const { return m_sampleRate; }

    /**
     * @brief Returns the current block size
     */
    int getBlockSize() const { return m_blockSize; }

protected:
    double m_sampleRate = 44100.0;
    int m_blockSize = 512;
    std::vector<std::shared_ptr<Parameter>> m_parameters;
    std::map<std::string, std::shared_ptr<Parameter>> m_namedParameters;
};

} // namespace Beam

#endif // AUDIO_PROCESSOR_HPP
