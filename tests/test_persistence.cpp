#include "../src/session/project_manager.hpp"
#include <iostream>
#include <cassert>

int main() {
    std::string testFile = "test_project.flux";
    nlohmann::json testData;
    testData["projectName"] = "Test Session";
    testData["panX"] = 150.5f;
    testData["modules"] = { {"id", 1, "type", "Oscillator"}, {"id", 2, "type", "Gain"} };

    // Test Save
    bool saved = Beam::ProjectManager::saveProject(testFile, testData);
    assert(saved);
    std::cout << "Persistence Test: Save Success." << std::endl;

    // Test Load
    nlohmann::json loadedData = Beam::ProjectManager::loadProject(testFile);
    assert(loadedData["projectName"] == "Test Session");
    assert(loadedData["panX"] == 150.5f);
    assert(loadedData["modules"].size() == 2);
    std::cout << "Persistence Test: Load Success." << std::endl;

    return 0;
}


