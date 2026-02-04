#ifndef FLUX_COMPILER_HPP
#define FLUX_COMPILER_HPP

#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <map>
#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <memory>
#include "engine/core/flux_node.hpp" 
#include "engine/plugins/plugin_registry.hpp"

#ifdef _WIN32
    #include <windows.h>
    #define LOAD_LIB(path) LoadLibraryA(path)
    #define GET_PROC(handle, name) GetProcAddress((HMODULE)handle, name)
    #define CLOSE_LIB(handle) FreeLibrary((HMODULE)handle)
#else
    #include <dlfcn.h>
    #define LOAD_LIB(path) dlopen(path, RTLD_NOW)
    #define GET_PROC(handle, name) dlsym(handle, name)
    #define CLOSE_LIB(handle) dlclose(handle)
#endif

#include "engine/scripting/flux_grammar.hpp"
#include "engine/hosting/vst3_host.hpp"
#include "engine/plugins/plugin_library.hpp"

namespace Beam {

class FluxCompiler {
public:
    static bool transpile(const std::string& scriptSource, const std::string& className, std::string& outCpp, std::string* outCategory = nullptr) {
        try {
            FluxScript::Parser parser(scriptSource);
            auto prog = parser.parse();
            outCpp = prog.transpile(className);
            if (outCategory) *outCategory = prog.category;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Transpilation error: " << e.what() << std::endl;
            return false;
        } catch (...) {
            std::cerr << "Unknown transpilation error." << std::endl;
            return false;
        }
    }
    
    enum class CompilerType { None, MSVC, GCC, Clang };

    static CompilerType detectCompiler(std::string& outCmd) {
        // 1. Check for Bundled Compiler
        std::string bundledPath = "tools/compiler/bin/";
        #ifdef _WIN32
        if (std::filesystem::exists(bundledPath + "g++.exe")) {
            outCmd = bundledPath + "g++.exe";
            return CompilerType::GCC;
        }
        if (std::filesystem::exists(bundledPath + "cl.exe")) {
            outCmd = bundledPath + "cl.exe";
            return CompilerType::MSVC;
        }
        #endif

        // 2. Try System MSVC
        if (system("cl >nul 2>nul") == 0 || system("cl /? >nul 2>nul") == 0) {
             outCmd = "cl";
             return CompilerType::MSVC;
        }
        
        // Try GCC
        if (system("g++ --version >nul 2>nul") == 0) {
            outCmd = "g++";
            return CompilerType::GCC;
        }

        // Try Clang
        if (system("clang++ --version >nul 2>nul") == 0) {
            outCmd = "clang++";
            return CompilerType::Clang;
        }

        return CompilerType::None;
    }

    static bool compileAndLoad(const std::string& scriptPath, const std::string& pluginName, const std::string& category = "User") {
        // 1. Read script
        std::ifstream f(scriptPath);
        if(!f.is_open()) return false;
        std::stringstream buf; buf << f.rdbuf();
        
        std::string cppCode;
        if(!transpile(buf.str(), pluginName, cppCode)) return false;
        
        // Ensure build directory exists
        std::filesystem::create_directories("plugins");

        // 2. Write C++
        std::string cppPath = "plugins/" + pluginName + ".cpp";
        std::ofstream out(cppPath);
        out << cppCode;
        out.close();
        
        // 3. Detect Compiler
        std::string compilerCmd;
        CompilerType type = detectCompiler(compilerCmd);
        
        if (type == CompilerType::None) {
            std::cerr << "No compatible C++ compiler found (cl, g++, or clang++)." << std::endl;
            return false;
        }

        // 4. Build DLL
        std::string dllPath = "plugins/" + pluginName + ".dll";
        std::string cmd;

        if (type == CompilerType::MSVC) {
            // MSVC Command
            cmd = compilerCmd + " /LD /EHsc /std:c++20 /I\"src\" /I\"third_party\" \"" + cppPath + "\" /Fe\"" + dllPath + "\"";
        } else {
            // GCC/Clang Command
            cmd = compilerCmd + " -shared -fPIC -std=c++20 -I\"src\" -I\"third_party\" \"" + cppPath + "\" -o \"" + dllPath + "\"";
        }

        std::cout << "Compiling: " << cmd << std::endl;
        int res = system(cmd.c_str());
        
        if (res != 0) {
            std::cerr << "Compilation failed with code: " << res << std::endl;
            return false;
        }

        // 5. Load DLL
        std::cout << "Plugin compiled successfully to: " << dllPath << std::endl;
        registerInLibrary(pluginName, dllPath, category, "FluxScript");
        return true; 
    }

    using PluginFactory = FluxNode* (*)(int, float);

    static void scanUserPlugins() {
        std::string pluginsDir = "plugins";
        if (!std::filesystem::exists(pluginsDir)) return;
        
        for (const auto& entry : std::filesystem::directory_iterator(pluginsDir)) {
            if (entry.path().extension() == ".dll") {
                std::string pluginName = entry.path().stem().string();
                std::string dllPath = entry.path().string();
                
                PluginLibrary::get().addEntry({pluginName, dllPath, "User", "FluxScript"});
                
                PluginRegistry::get().registerPlugin(pluginName, "User", [dllPath](int b, float s) {
                    return FluxCompiler::loadPlugin(dllPath, b, s);
                });
            }
        }
    }

    static void scanVST3(const std::string& directory) {
        if (!std::filesystem::exists(directory)) return;

        for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
            if (entry.path().extension() == ".vst3") {
                std::string pluginName = entry.path().stem().string();
                std::string path = entry.path().string();

                if (std::filesystem::is_directory(entry.path())) {
                    std::string dllPath = path + "/Contents/x86_64-win/" + pluginName + ".vst3";
                    if (std::filesystem::exists(dllPath)) path = dllPath;
                    else continue; 
                }

                PluginLibrary::get().addEntry({pluginName, path, "VST3", "VST3"});

                PluginRegistry::get().registerPlugin(pluginName, "VST3", [path](int b, float s) {
                    auto node = std::make_shared<VST3HostNode>(path);
                    if (node->load()) return node;
                    return std::shared_ptr<VST3HostNode>(nullptr);
                });
            }
        }
    }

    static void registerInLibrary(const std::string& name, const std::string& path, const std::string& cat, const std::string& type) {
        PluginLibrary::get().addEntry({name, path, cat, type});
        if (type == "FluxScript") {
            PluginRegistry::get().registerPlugin(name, cat, [path](int b, float s) {
                return FluxCompiler::loadPlugin(path, b, s);
            });
        }
    }

    static std::shared_ptr<FluxNode> loadPlugin(const std::string& dllPath, int bufferSize, float sampleRate) {
        void* handle = LOAD_LIB(dllPath.c_str());
        if (!handle) {
            std::cerr << "Failed to load library: " << dllPath << std::endl;
            return nullptr;
        }

        auto factory = (PluginFactory)GET_PROC(handle, "create_plugin");
        if (!factory) {
            std::cerr << "Could not find 'create_plugin' symbol in " << dllPath << std::endl;
            CLOSE_LIB(handle);
            return nullptr;
        }

        // Create the node (Raw pointer from DLL)
        FluxNode* rawNode = factory(bufferSize, sampleRate);
        
        // Use custom deleter that calls releaseNode()
        return std::shared_ptr<FluxNode>(rawNode, [](FluxNode* n) { if(n) n->releaseNode(); });
    }
};

} // namespace Beam

#endif
