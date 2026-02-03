#ifndef FLUX_SCRIPT_HPP
#define FLUX_SCRIPT_HPP

#include "engine/scripting/flux_grammar.hpp"
#include <vector>
#include <map>
#include <string>

namespace Beam {

class FluxScriptEngine {
public:
    struct Param { std::string name; float min, max, val; };
    
    bool compile(const std::string& source) {
        try {
            FluxScript::Parser parser(source);
            m_program = parser.parse();
            
            // Initialize Context
            m_ctx = FluxScript::Context();
            
            // Initialize State Vars
            for(auto& v : m_program.stateVars) {
                m_ctx.vars[v.name] = v.val;
            }
            
            // Expose Params
            m_params.clear();
            for(auto& p : m_program.params) {
                m_params.push_back({p.name, p.min, p.max, p.def});
                m_ctx.params[p.name] = p.def;
            }
            
            return true;
        } catch(...) {
            return false;
        }
    }

    void process(float in, float& out, const std::vector<float>& pVals, float sr) {
        // Update Context Inputs
        m_ctx.vars["in"] = in;
        m_ctx.vars["sr"] = sr;
        
        // Update Context Params
        // Optimization: Don't map strings every sample if possible?
        // But for Interpreter fallback, correctness first.
        for(size_t i=0; i<m_params.size() && i<pVals.size(); ++i) {
            m_ctx.params[m_params[i].name] = pVals[i];
        }
        
        // Execute Program
        m_program.execute(m_ctx);
        
        // Retrieve Output
        if (m_ctx.vars.count("out")) {
            out = m_ctx.vars["out"];
        } else {
            out = 0.0f;
        }
    }

    const std::vector<Param>& getParams() const { return m_params; }

private:
    FluxScript::Program m_program;
    FluxScript::Context m_ctx;
    std::vector<Param> m_params;
};

} // namespace Beam

#endif
