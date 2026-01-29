#ifndef APPLICATION_BASE_HPP
#define APPLICATION_BASE_HPP

#include <string>
#include <memory>

namespace Beam {

/**
 * @class ApplicationBase
 * @brief Base class for Flux applications, similar to JUCE's JUCEApplication
 */
class ApplicationBase {
public:
    ApplicationBase();
    virtual ~ApplicationBase();

    /**
     * @brief Called when the application starts up
     */
    virtual void initialise(const std::string& commandLine) = 0;

    /**
     * @brief Called when the application is shutting down
     */
    virtual void shutdown() = 0;

    /**
     * @brief Called periodically when there are no messages in the queue
     */
    virtual void suspended();

    /**
     * @brief Called when the application is brought back from suspended state
     */
    virtual void resumed();

    /**
     * @brief Called when the OS wants to shut down the application
     */
    virtual void systemRequestedQuit();

    /**
     * @brief Returns the application name
     */
    virtual std::string getApplicationName() const = 0;

    /**
     * @brief Returns the application version
     */
    virtual std::string getApplicationVersion() const = 0;

    /**
     * @brief Checks if the application should quit
     */
    bool shouldQuit() const { return m_shouldQuit; }

    /**
     * @brief Quits the application
     */
    void quit();

    /**
     * @brief Gets the singleton instance of the application
     */
    static ApplicationBase* getInstance();

protected:
    bool m_shouldQuit = false;
    static ApplicationBase* s_instance;
};

} // namespace Beam

#endif // APPLICATION_BASE_HPP


