#ifndef FLUX_SCRIPT_NODE_HPP
#define FLUX_SCRIPT_NODE_HPP

#include "flux_node.hpp"
#include "flux_script.hpp"
#include <fstream>

namespace Beam {

class FluxScriptNode : public FluxNode {
public:
    FluxScriptNode(const std::string& scriptPath, int bufferSize, float sr) 
        : m_sr(sr) 
    {
        std::ifstream file(scriptPath);
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            if (m_engine.compile(buffer.str())) {
                for(auto const& p : m_engine.getParams()) {
                    addParameter(std::make_shared<Parameter>(p.name, p.min, p.max, p.val));
                }
            }
        }
        setupBuffers(1, 1, bufferSize, 2);
    }

    void process(int frames) override {
        float* in = getInputBuffer(0);
        float* out = getOutputBuffer(0);
        
        std::vector<float> pVals;
        for(auto const& p : getParameters()) pVals.push_back(p.second->getValue());

        for (int i = 0; i < frames * 2; ++i) {
            m_engine.process(in[i], out[i], pVals, m_sr);
        }
    }

    std::string getName() const override { return "Script FX"; }

    std::vector<FluxNode::Port> getInputPorts() const override { return { {"In", 2} }; }
    std::vector<FluxNode::Port> getOutputPorts() const override { return { {"Out", 2} }; }

private:
    FluxScriptEngine m_engine;
    float m_sr;
};

} // namespace Beam

#endif
