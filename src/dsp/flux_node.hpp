#ifndef FLUX_NODE_HPP
#define FLUX_NODE_HPP

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <map>
#include "../core/parameter.hpp"

namespace Beam {

class FluxNode {
public:
    virtual ~FluxNode() = default;

    struct Port {
        std::string name;
        int channels;
    };

    virtual void process(int frames) = 0;
    
    virtual std::string getName() const = 0;
    virtual std::vector<Port> getInputPorts() const = 0;
    virtual std::vector<Port> getOutputPorts() const = 0;

    float* getInputBuffer(int portIdx) { return m_inputs[portIdx].data(); }
    float* getOutputBuffer(int portIdx) { return m_outputs[portIdx].data(); }

    void setBypass(bool bypass) { m_bypassed = bypass; }
    bool isBypassed() const { return m_bypassed; }

    void addParameter(std::shared_ptr<Parameter> param) {
        m_parameters[param->getName()] = param;
    }

    std::shared_ptr<Parameter> getParameter(const std::string& name) {
        auto it = m_parameters.find(name);
        return (it != m_parameters.end()) ? it->second : nullptr;
    }

    const std::map<std::string, std::shared_ptr<Parameter>>& getParameters() const {
        return m_parameters;
    }

protected:
    void setupBuffers(int numInputs, int numOutputs, int bufferSize, int channels) {
        m_inputs.assign(numInputs, std::vector<float>(bufferSize * channels, 0.0f));
        m_outputs.assign(numOutputs, std::vector<float>(bufferSize * channels, 0.0f));
    }

    std::vector<std::vector<float>> m_inputs;
    std::vector<std::vector<float>> m_outputs;
    std::map<std::string, std::shared_ptr<Parameter>> m_parameters;
    std::atomic<bool> m_bypassed{false};
};

} // namespace Beam

#endif // FLUX_NODE_HPP
