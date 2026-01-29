#ifndef PROJECT_MANAGER_HPP
#define PROJECT_MANAGER_HPP

#include "json.hpp"
#include <string>
#include <fstream>
#include <iostream>

namespace Beam {

class ProjectManager {
public:
    static void saveProject(const std::string& filename, const nlohmann::json& data) {
        std::ofstream file(filename);
        if (file.is_open()) {
            file << data.dump(4);
        }
    }

    static nlohmann::json loadProject(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) return nlohmann::json();
        nlohmann::json data;
        try {
            file >> data;
        } catch(...) { return nlohmann::json(); }
        return data;
    }
};

} // namespace Beam

#endif // PROJECT_MANAGER_HPP






