#ifndef FLUX_GRAMMAR_HPP
#define FLUX_GRAMMAR_HPP

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <cmath>
#include <algorithm>
#include <iostream>

namespace Beam {
namespace FluxScript {

struct Context {
    std::map<std::string, float> vars;
    std::map<std::string, float> params;
    
    float getVar(const std::string& name) {
        if (vars.count(name)) return vars[name];
        if (params.count(name)) return params[name];
        return 0.0f;
    }
    
    void setVar(const std::string& name, float val) {
        vars[name] = val;
    }
};

enum class Op { Add, Sub, Mul, Div, Lt, Gt, Le, Ge, Eq, Neq };

struct Expr {
    virtual ~Expr() = default;
    virtual std::string transpile() const = 0;
    virtual float evaluate(Context& ctx) const = 0;
};

struct NumberExpr : Expr {
    float value;
    NumberExpr(float v) : value(v) {}
    std::string transpile() const override { return std::to_string(value) + "f"; }
    float evaluate(Context& ctx) const override { return value; }
};

struct VariableExpr : Expr {
    std::string name;
    VariableExpr(const std::string& n) : name(n) {}
    std::string transpile() const override { return name; }
    float evaluate(Context& ctx) const override { return ctx.getVar(name); }
};

struct BinaryExpr : Expr {
    std::shared_ptr<Expr> left, right;
    Op op;
    BinaryExpr(std::shared_ptr<Expr> l, Op o, std::shared_ptr<Expr> r) : left(l), op(o), right(r) {}
    std::string transpile() const override {
        std::string opStr;
        switch(op) {
            case Op::Add: opStr="+"; break; case Op::Sub: opStr="-"; break;
            case Op::Mul: opStr="*"; break; case Op::Div: opStr="/"; break;
            case Op::Lt: opStr="<"; break; case Op::Gt: opStr=">"; break;
            case Op::Le: opStr="<="; break; case Op::Ge: opStr=">="; break;
            case Op::Eq: opStr="=="; break; case Op::Neq: opStr="!="; break;
        }
        return "(" + left->transpile() + " " + opStr + " " + right->transpile() + ")";
    }
    float evaluate(Context& ctx) const override {
        float l = left->evaluate(ctx);
        float r = right->evaluate(ctx);
        switch(op) {
            case Op::Add: return l + r;
            case Op::Sub: return l - r;
            case Op::Mul: return l * r;
            case Op::Div: return (r != 0.0f) ? l / r : 0.0f;
            case Op::Lt: return (l < r) ? 1.0f : 0.0f;
            case Op::Gt: return (l > r) ? 1.0f : 0.0f;
            case Op::Le: return (l <= r) ? 1.0f : 0.0f;
            case Op::Ge: return (l >= r) ? 1.0f : 0.0f;
            case Op::Eq: return (l == r) ? 1.0f : 0.0f;
            case Op::Neq: return (l != r) ? 1.0f : 0.0f;
        }
        return 0.0f;
    }
};

struct CallExpr : Expr {
    std::string callee;
    std::vector<std::shared_ptr<Expr>> args;
    CallExpr(const std::string& name, std::vector<std::shared_ptr<Expr>> a) : callee(name), args(a) {}
    std::string transpile() const override {
        std::string s = "std::" + callee + "(";
        for(size_t i=0; i<args.size(); ++i) {
            s += args[i]->transpile();
            if(i < args.size()-1) s += ", ";
        }
        s += ")";
        return s;
    }
    float evaluate(Context& ctx) const override {
        if (args.empty()) return 0.0f;
        float v0 = args[0]->evaluate(ctx);
        if (callee == "sin") return std::sin(v0);
        if (callee == "cos") return std::cos(v0);
        if (callee == "tan") return std::tan(v0);
        if (callee == "tanh") return std::tanh(v0);
        if (callee == "abs") return std::abs(v0);
        if (callee == "sqrt") return (v0 > 0) ? std::sqrt(v0) : 0.0f;
        if (callee == "pow" && args.size() > 1) return std::pow(v0, args[1]->evaluate(ctx));
        if (callee == "min" && args.size() > 1) return (std::min)(v0, args[1]->evaluate(ctx));
        if (callee == "max" && args.size() > 1) return (std::max)(v0, args[1]->evaluate(ctx));
        return 0.0f;
    }
};

struct Stmt {
    virtual ~Stmt() = default;
    virtual std::string transpile() const = 0;
    virtual void execute(Context& ctx) const = 0;
};

struct VarDeclStmt : Stmt {
    std::string name;
    std::shared_ptr<Expr> initializer;
    VarDeclStmt(const std::string& n, std::shared_ptr<Expr> init) : name(n), initializer(init) {}
    std::string transpile() const override {
        return "float " + name + " = " + (initializer ? initializer->transpile() : "0.0f") + ";";
    }
    void execute(Context& ctx) const override {
        ctx.vars[name] = initializer ? initializer->evaluate(ctx) : 0.0f;
    }
};

struct AssignStmt : Stmt {
    std::string name;
    std::shared_ptr<Expr> value;
    AssignStmt(const std::string& n, std::shared_ptr<Expr> v) : name(n), value(v) {}
    std::string transpile() const override {
        return name + " = " + value->transpile() + ";";
    }
    void execute(Context& ctx) const override {
        ctx.setVar(name, value->evaluate(ctx));
    }
};

struct CppBlockStmt : Stmt {
    std::string code;
    CppBlockStmt(const std::string& c) : code(c) {}
    std::string transpile() const override { return " {\n" + code + "\n} "; }
    void execute(Context& ctx) const override { 
        // Cannot execute raw C++ in interpreter.
        std::cerr << "Warning: Skipping cpp {} block in interpreter." << std::endl;
    }
};

struct Program {
    struct Param { std::string name; float min, max, def; };
    struct StateVar { std::string name; float val; };
    
    std::string category = "User";
    std::string uiStyle = "ClassicBakelite";
    std::string subtitle = "FLUX SCRIPT";
    std::vector<Param> params;
    std::vector<StateVar> stateVars;
    std::vector<std::shared_ptr<Stmt>> processStmts;
    
    std::string transpile(const std::string& className) const {
        std::stringstream ss;
        ss << "#include \"sdk/beam_sdk.hpp\"\n#include <cmath>\n#include <algorithm>\n\nusing namespace Beam;\n\n";
        
        // Unified Node & Processor Class
        ss << "class " << className << " : public SDK::BeamPlugin {\npublic:\n";
        ss << "    " << className << "(int b, float s) : BeamPlugin(\"" << className << "\", \"User\") {\n";
        ss << "        SDK::PanelStyle style = { {0.1f, 0.12f, 0.15f, 1.0f}, {0.8f, 0.8f, 0.9f, 1.0f}, Theme::MaterialType::Standard, true, \"" << className << "\", \"" << subtitle << "\" };\n";
        ss << "        style.knobStyle = Theme::KnobStyle::" << uiStyle << ";\n";
        ss << "        setPanelStyle(style);\n\n";
        
        for(auto& p : params) ss << "        p_refs[" << (&p - &params[0]) << "] = &addFloatParam(\"" << p.name << "\", " << p.min << "f, " << p.max << "f, " << p.def << "f);\n";
        ss << "        addMeter(\"Level\");\n";
        ss << "    }\n\n";

        for(auto& v : stateVars) ss << "    float v_" << v.name << " = " << v.val << "f;\n";
        ss << "    Parameter* p_refs[" << params.size() << "];\n\n";

        ss << "    void process(float** io, int samples) override {\n";
        for(auto& p : params) ss << "        float p_" << p.name << " = p_refs[" << (&p - &params[0]) << "]->getValue();\n";
        ss << "        float sr = m_sampleRate;\n\n";
        ss << "        for(int i=0; i<samples; ++i) {\n";
        ss << "            for(int ch=0; ch<2; ++ch) {\n";
        ss << "                if(!io[ch]) continue;\n";
        ss << "                float in_sample = io[ch][i];\n";
        ss << "                float out_sample = 0.0f;\n";
        for(auto& stmt : processStmts) ss << "                " << stmt->transpile() << "\n";
        ss << "                io[ch][i] = out_sample;\n";
        ss << "            }\n";
        ss << "        }\n";
        ss << "        float peak = 0.0f; for(int ch=0; ch<2; ++ch) if(io[ch]) for(int i=0; i<samples; ++i) peak = std::max(peak, std::abs(io[ch][i]));\n";
        ss << "        updateMeter(0, peak);\n";
        ss << "    }\n";
        
        ss << "    void releaseNode() override { delete this; }\n";
        ss << "};\n\n";

        ss << "extern \"C\" __declspec(dllexport) FluxNode* create_plugin(int b, float s) { return new " << className << "(b, s); }\n";
        
        std::string code = ss.str();
        auto replace = [&](std::string key, std::string val) {
            size_t pos = 0;
            while((pos = code.find(key, pos)) != std::string::npos) {
                // Check if inside cpp block?
                // For simplicity, we assume user knows what they are doing in cpp block or uses v_/p_ explicitly?
                // The replacement is global.
                // If user wrote "float in = ...", it becomes "float in_sample = ...".
                // This is acceptable for "Embedded C++" which usually expects access to the context variables.
                char prev = (pos>0) ? code[pos-1] : ' ';
                char next = (pos+key.size() < code.size()) ? code[pos+key.size()] : ' ';
                if(!isalnum(prev) && prev!='_' && !isalnum(next) && next!='_') {
                    code.replace(pos, key.size(), val);
                    pos += val.size();
                } else pos += key.size();
            }
        };
        replace("in", "in_sample");
        replace("out", "out_sample");
        for(auto& v : stateVars) replace(v.name, "v_" + v.name);
        for(auto& p : params) replace(p.name, "p_" + p.name);
        return code;
    }
    
    void execute(Context& ctx) {
        for(auto& stmt : processStmts) stmt->execute(ctx);
    }
};

class Parser {
    std::string src;
    size_t pos = 0;
    
    char peek() { while(pos < src.size() && isspace(src[pos])) pos++; if(pos==src.size()) return 0; return src[pos]; }
    char advance() { char c = peek(); if(pos<src.size()) pos++; return c; }
    
    std::string parseToken() {
        char c = peek();
        if(isalpha(c)) {
            size_t start = pos;
            while(pos < src.size() && (isalnum(src[pos]) || src[pos]=='_')) pos++;
            return src.substr(start, pos-start);
        }
        if(isdigit(c) || c == '.') {
            size_t start = pos;
            while(pos < src.size() && (isdigit(src[pos]) || src[pos]=='.')) pos++;
            return src.substr(start, pos-start);
        }
        return std::string(1, advance());
    }
    
    std::string rawBlock() {
        std::string block;
        int depth = 0;
        // Search for '{'
        while(pos < src.size() && src[pos] != '{') pos++;
        if(pos == src.size()) return "";
        pos++; depth++; // Skip '{'
        
        while(pos < src.size()) {
            char c = src[pos++];
            if(c == '{') depth++;
            if(c == '}') depth--;
            if(depth == 0) return block;
            block += c;
        }
        return block;
    }
    
    std::string peekToken() { size_t old = pos; std::string t = parseToken(); pos = old; return t; }
    
    std::shared_ptr<Expr> parsePrimary() {
        std::string t = parseToken();
        if(isdigit(t[0]) || t[0]=='.') return std::make_shared<NumberExpr>(std::stof(t));
        if(t == "(") { auto e = parseExpression(); parseToken(); return e; }
        if(peek() == '(') {
            advance();
            std::vector<std::shared_ptr<Expr>> args;
            if(peek() != ')') {
                while(true) {
                    args.push_back(parseExpression());
                    if(peek() == ')') break;
                    if(peek() == ',') advance();
                }
            }
            advance();
            return std::make_shared<CallExpr>(t, args);
        }
        return std::make_shared<VariableExpr>(t);
    }
    
    std::shared_ptr<Expr> parseTerm() { 
        auto left = parsePrimary();
        while(peek() == '*' || peek() == '/') {
            char opC = advance();
            auto right = parsePrimary();
            left = std::make_shared<BinaryExpr>(left, opC=='*' ? Op::Mul : Op::Div, right);
        }
        return left;
    }
    
    std::shared_ptr<Expr> parseExpression() { 
        auto left = parseTerm();
        while(peek() == '+' || peek() == '-') {
            char opC = advance();
            auto right = parseTerm();
            left = std::make_shared<BinaryExpr>(left, opC=='+' ? Op::Add : Op::Sub, right);
        }
        return left;
    }
    
public:
    Parser(const std::string& s) : src(s) {}
    
    float parseFloat() {
        std::string t = parseToken();
        if (t.empty() || (!std::isdigit(t[0]) && t[0] != '-' && t[0] != '.')) return 0.0f;
        try {
            return std::stof(t);
        } catch (...) {
            return 0.0f;
        }
    }

    Program parse() {
        Program prog;
        std::string token;
        while((token = parseToken()) != "") {
            if (token == "category") {
                prog.category = parseToken();
            }
            else if (token == "ui") {
                parseToken(); // '{'
                while (peek() != '}' && peek() != 0) {
                    std::string t = parseToken();
                    if (t == "style") prog.uiStyle = parseToken();
                    else if (t == "subtitle") prog.subtitle = parseToken();
                }
                parseToken(); // '}'
            }
            else if (token == "param") {
                std::string name = parseToken();
                float min = parseFloat();
                float max = parseFloat();
                float def = parseFloat();
                prog.params.push_back({name, min, max, def});
            }
            else if (token == "var") {
                std::string name = parseToken();
                float val = parseFloat();
                prog.stateVars.push_back({name, val});
            }
            else if (token == "process") {
                parseToken(); 
                while(peek() != 0) {
                    std::string t = peekToken();
                    if(t == "float") {
                        parseToken(); std::string name = parseToken(); parseToken(); 
                        auto expr = parseExpression(); parseToken();
                        prog.processStmts.push_back(std::make_shared<VarDeclStmt>(name, expr));
                    }
                    else if(t == "cpp") {
                        parseToken(); // cpp
                        std::string code = rawBlock();
                        prog.processStmts.push_back(std::make_shared<CppBlockStmt>(code));
                    }
                    else if(!t.empty()) {
                        std::string name = parseToken();
                        if(peekToken() == "=") {
                            parseToken(); auto expr = parseExpression(); parseToken();
                            prog.processStmts.push_back(std::make_shared<AssignStmt>(name, expr));
                        }
                    } else break;
                }
            }
        }
        return prog;
    }
};

} // namespace FluxScript
} // namespace Beam

#endif
