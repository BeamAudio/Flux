#ifndef VST_EXTERNAL_EDITOR_HPP
#define VST_EXTERNAL_EDITOR_HPP

#include "interface/core/component.hpp"
#include "pluginterfaces/gui/iplugview.h"
#include <windows.h>
#include <vector>
#include <mutex>

namespace Steinberg {
    namespace Vst {
        class IEditController;
    }
}

namespace Beam {

/**
 * @class VSTExternalEditor
 * @brief Hosts a VST3 plugin GUI.
 * Uses the Main Thread for window creation and message handling (via SDL/Host loop),
 * which ensures compatibility with complex plugins (e.g., Ozone) and Drag&Drop.
 */
class VSTExternalEditor : public Component, public Steinberg::IPlugFrame {
public:
    VSTExternalEditor(::Steinberg::Vst::IEditController* controller, void* parentWindowHandle, void* moduleHandle = nullptr);
    virtual ~VSTExternalEditor();

    // Component overrides
    void getPreferredSize(float& w, float& h) const override;
    void setBounds(float x, float y, float w, float h) override;
    void setVisible(bool visible) override;
    void render(QuadBatcher& batcher, float dt, float screenW, float screenH) override;
    void update(float dt) override;
    void paint(QuadBatcher& batcher) override;
    bool onMouseDown(float x, float y, int button, bool shift) override;

    // IPlugFrame overrides
    Steinberg::tresult PLUGIN_API resizeView(Steinberg::IPlugView* view, Steinberg::ViewRect* newSize) override;
    Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID _iid, void** obj) override;
    Steinberg::uint32 PLUGIN_API addRef() override { return 1; }
    Steinberg::uint32 PLUGIN_API release() override { return 1; }

    static LRESULT CALLBACK VSTContainerWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    static void processMessageLoop();

    void openWindow();
    void closeWindow();
    void hideWindow();  // Hide without destroying - safe for file dialog scenarios

    bool isWindowOpen() const { return m_windowOpen; }
    void updateWindowPosition(int mainX, int mainY);

private:
    static std::vector<HWND> s_openWindows;
    static std::mutex s_windowsMutex;
    Steinberg::Vst::IEditController* m_controller = nullptr;
    Steinberg::IPlugView* m_view = nullptr;
    HWND m_parentHwnd = NULL;
    HWND m_vstHwnd = NULL;
    struct SDL_Window* m_sdlWindow = nullptr; // SDL Window Handle
    void* m_moduleHandle = nullptr;
    
    bool m_windowOpen = false;
    int m_pluginWidth = 800;
    int m_pluginHeight = 600;

    int m_relativeX = 100; // Relative to main window
    int m_relativeY = 100;
};

} // namespace Beam

#endif