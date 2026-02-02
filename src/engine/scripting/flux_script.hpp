#ifndef FLUX_SCRIPT_HPP
#define FLUX_SCRIPT_HPP

#include <vector>
#include <string>
#include <map>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <stack>

namespace Beam {

enum class FluxOp {
    PUSH_VAL, PUSH_VAR, PUSH_PARAM, POP_VAR,
    ADD, SUB, MUL, DIV, 
    ABS, SIN, COS, TANH, POW,
    BUF_READ, BUF_WRITE, BUF_LEN,
    INPUT, OUTPUT, SR
};

struct FluxInst {
    FluxOp op;
    float val = 0.0f;
    int index = 0;
};

class FluxScriptEngine {
public:
    struct Param { std::string name; float min, max, val; };
    
    bool compile(const std::string& source) {
        m_instructions.clear();
        m_params.clear();
        m_vars.clear();
        m_buffers.clear();
        m_varMap.clear();
        m_paramMap.clear();
        m_bufferMap.clear();

        // Built-in vars
        m_varMap["in"] = 0; m_vars.push_back(0.0f);
        m_varMap["out"] = 1; m_vars.push_back(0.0f);
        m_varMap["sr"] = 2; m_vars.push_back(44100.0f);

        std::istringstream stream(source);
        std::string line;
        bool inProcess = false;

        while (std::getline(stream, line)) {
            // Trim and skip comments
            size_t hash = line.find('#');
            if (hash != std::string::npos) line = line.substr(0, hash);
            if (line.find_first_not_of(" \t\r\n") == std::string::npos) continue;

            std::stringstream ss(line);
            std::string word;
            ss >> word;

            if (word == "param") {
                std::string name; float mn, mx, def;
                ss >> name >> mn >> mx >> def;
                m_paramMap[name] = (int)m_params.size();
                m_params.push_back({name, mn, mx, def});
            }
            else if (word == "var") {
                std::string name; float val;
                ss >> name >> val;
                m_varMap[name] = (int)m_vars.size();
                m_vars.push_back(val);
            }
            else if (word == "buffer") {
                std::string name; int sz;
                ss >> name >> sz;
                m_bufferMap[name] = (int)m_buffers.size();
                m_buffers.push_back(std::vector<float>(sz, 0.0f));
            }
            else if (word == "process:") {
                inProcess = true;
            }
            else if (inProcess) {
                // Tokenize the rest of the line
                std::string token;
                ss.seekg(0); ss >> token; // Re-read first word if it was part of instruction
                
                // We'll process word by word for RPN
                std::stringstream lineSS(line);
                while(lineSS >> token) {
                    if (token == "+") m_instructions.push_back({FluxOp::ADD});
                    else if (token == "-") m_instructions.push_back({FluxOp::SUB});
                    else if (token == "*") m_instructions.push_back({FluxOp::MUL});
                    else if (token == "/") m_instructions.push_back({FluxOp::DIV});
                    else if (token == "abs") m_instructions.push_back({FluxOp::ABS});
                    else if (token == "sin") m_instructions.push_back({FluxOp::SIN});
                    else if (token == "tanh") m_instructions.push_back({FluxOp::TANH});
                    else if (token == "=") {
                        // Pop from stack into NEXT token's var
                        std::string varName; lineSS >> varName;
                        if (m_varMap.count(varName)) m_instructions.push_back({FluxOp::POP_VAR, 0, m_varMap[varName]});
                    }
                    else if (token == "read") {
                        std::string bufName; lineSS >> bufName;
                        if (m_bufferMap.count(bufName)) m_instructions.push_back({FluxOp::BUF_READ, 0, m_bufferMap[bufName]});
                    }
                    else if (token == "write") {
                        std::string bufName; lineSS >> bufName;
                        if (m_bufferMap.count(bufName)) m_instructions.push_back({FluxOp::BUF_WRITE, 0, m_bufferMap[bufName]});
                    }
                    else if (m_paramMap.count(token)) {
                        m_instructions.push_back({FluxOp::PUSH_PARAM, 0, m_paramMap[token]});
                    }
                    else if (m_varMap.count(token)) {
                        m_instructions.push_back({FluxOp::PUSH_VAR, 0, m_varMap[token]});
                    }
                    else {
                        try {
                            float v = std::stof(token);
                            m_instructions.push_back({FluxOp::PUSH_VAL, v});
                        } catch(...) {}
                    }
                }
            }
        }
        return true;
    }

    void process(float in, float& out, const std::vector<float>& pVals, float sr) {
        m_vars[0] = in;
        m_vars[2] = sr;
        
        float stack[32];
        int sp = 0;

        for (auto const& inst : m_instructions) {
            switch(inst.op) {
                case FluxOp::PUSH_VAL: stack[sp++] = inst.val; break;
                case FluxOp::PUSH_VAR: stack[sp++] = m_vars[inst.index]; break;
                case FluxOp::PUSH_PARAM: stack[sp++] = pVals[inst.index]; break;
                case FluxOp::POP_VAR: m_vars[inst.index] = stack[--sp]; break;
                case FluxOp::ADD: { float b = stack[--sp]; float a = stack[--sp]; stack[sp++] = a + b; break; }
                case FluxOp::SUB: { float b = stack[--sp]; float a = stack[--sp]; stack[sp++] = a - b; break; }
                case FluxOp::MUL: { float b = stack[--sp]; float a = stack[--sp]; stack[sp++] = a * b; break; }
                case FluxOp::DIV: { float b = stack[--sp]; float a = stack[--sp]; stack[sp++] = (b != 0) ? a / b : 0; break; }
                case FluxOp::ABS: { stack[sp-1] = std::abs(stack[sp-1]); break; }
                case FluxOp::TANH: { stack[sp-1] = std::tanh(stack[sp-1]); break; }
                case FluxOp::BUF_READ: {
                    int idx = (int)stack[--sp];
                    auto& b = m_buffers[inst.index];
                    if (idx < 0) idx = 0; if (idx >= b.size()) idx = (int)b.size()-1;
                    stack[sp++] = b[idx];
                    break;
                }
                case FluxOp::BUF_WRITE: {
                    float val = stack[--sp];
                    int idx = (int)stack[--sp];
                    auto& b = m_buffers[inst.index];
                    if (idx >= 0 && idx < b.size()) b[idx] = val;
                    break;
                }
            }
        }
        out = m_vars[1];
    }

    const std::vector<Param>& getParams() const { return m_params; }

private:
    std::vector<FluxInst> m_instructions;
    std::vector<Param> m_params;
    std::vector<float> m_vars;
    std::vector<std::vector<float>> m_buffers;
    std::map<std::string, int> m_varMap, m_paramMap, m_bufferMap;
};

} // namespace Beam

#endif
