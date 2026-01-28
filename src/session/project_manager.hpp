#ifndef PROJECT_MANAGER_HPP
#define PROJECT_MANAGER_HPP

#include "json.hpp"
#include <string>
#include <fstream>
#include <iostream>

namespace Beam {

class ProjectManager {
public:
    static bool saveProject(const std::string& filename, const nlohmann::json& projectData) {
        std::ofstream file(filename);
        if (!file.is_open()) return false;
        file << projectData.dump(4);
        return true;
    }

    static nlohmann::json loadProject(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) return nlohmann::json();
        nlohmann::json data;
        file >> data;
        return data;
    }
};

} // namespace Beam

#endif // PROJECT_MANAGER_HPP

