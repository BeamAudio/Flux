#ifndef FLUX_SCRIPT_NODE_HPP
#define FLUX_SCRIPT_NODE_HPP

#include "engine/core/flux_node.hpp"
#include "engine/scripting/flux_script.hpp"
#include "engine/core/flux_compiler.hpp"
#include <fstream>

namespace Beam {

class FluxScriptProcessor : public FluxProcessor {
public:
    FluxScriptProcessor(const FluxScriptEngine& engine, std::shared_ptr<MeterSource> meterSource) 
        : m_engine(engine), m_meterSource(meterSource) {}
    
    void process(const float** inputs, float** outputs, int frames) override {
        const float* in = inputs[0]; 
        float* out = outputs[0];

        float peak = 0.0f;
        for (int i = 0; i < frames * 2; ++i) {
            float s = 0.0f;
            m_engine.process(in[i], s, m_paramVals, 44100.0f); 
            out[i] = s;
            float absS = std::abs(s);
            if (absS > peak) peak = absS;
        }

        if (m_meterSource) {
            m_meterSource->updateMeter(0, peak);
        }
    }

    void updateParameters(const float* params) override {
        if (params) {
            m_paramVals.assign(params, params + m_engine.getParams().size());
        }
    }

private:
    FluxScriptEngine m_engine; 
    std::shared_ptr<MeterSource> m_meterSource;
    std::vector<float> m_paramVals;
};

class FluxScriptNode : public FluxNode {
public:
    FluxScriptNode(const std::string& scriptPath, int bufferSize, float sr) 
        : m_sr(sr), m_scriptPath(scriptPath), m_bufferSize(bufferSize)
    {
        m_meterSource->addMeter("Level");
        reload();
    }

    void reload() {
        std::ifstream file(m_scriptPath);
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            if (m_engine.compile(buffer.str())) {
                for(auto const& p : m_engine.getParams()) {
                    if (!getParameter(p.name)) {
                        addParameter(std::make_shared<Parameter>(p.name, p.min, p.max, p.val));
                    }
                }
            }
        }
    }

    bool compileToNative(const std::string& pluginName) {
        if (FluxCompiler::compileAndLoad(m_scriptPath, pluginName)) {
            // Load it!
            std::string dllPath = "build/plugins/" + pluginName + ".dll"; // Windows assumption
            auto native = FluxCompiler::loadPlugin(dllPath, m_bufferSize, m_sr);
            if (native) {
                m_nativeNode = native;
                std::cout << "Switched to Native Node: " << pluginName << std::endl;
                return true;
            }
        }
        return false;
    }

    // Factory: Returns either Native Processor (if compiled) or Interpreter
    std::unique_ptr<FluxProcessor> createProcessor() override {
        if (m_nativeNode) {
            return m_nativeNode->createProcessor(); // Proxy to native
        }
        return std::make_unique<FluxScriptProcessor>(m_engine, m_meterSource); 
    }

    std::string getName() const override { return "Script FX"; }
    std::vector<Port> getInputPorts() const override { return { {"In", 2} }; }
    std::vector<Port> getOutputPorts() const override { return { {"Out", 2} }; }

    std::shared_ptr<Component> createEditor(const NodeEditorContext& context) override; 

private:
    FluxScriptEngine m_engine;
    std::shared_ptr<FluxNode> m_nativeNode = nullptr;
    float m_sr;
    int m_bufferSize;
    std::string m_scriptPath;
};

}



#endif
