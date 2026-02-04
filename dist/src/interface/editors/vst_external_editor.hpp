#ifndef VST_EXTERNAL_EDITOR_HPP
#define VST_EXTERNAL_EDITOR_HPP

#include "interface/core/component.hpp"
#include <windows.h>

namespace Steinberg {
    class IPlugView;
    namespace Vst {
        class IEditController;
    }
}

namespace Beam {

/**
 * @class VSTExternalEditor
 * @brief A component that hosts an external VST3 GUI window.
 */
class VSTExternalEditor : public Component {
public:
    VSTExternalEditor(::Steinberg::Vst::IEditController* controller);
    ~VSTExternalEditor() override;

    void getPreferredSize(float& w, float& h) const override;
    void setBounds(float x, float y, float w, float h) override;
    void setVisible(bool visible) override;
    void render(QuadBatcher& batcher, float dt, float screenW, float screenH) override;
    void paint(QuadBatcher& batcher) override;

    void updateNativeWindow();
    void attachToNative(HWND parentHWnd);

private:
    ::Steinberg::Vst::IEditController* m_controller = nullptr;
    ::Steinberg::IPlugView* m_view = nullptr;
    bool m_isAttached = false;
    HWND m_parentHwnd = NULL;
    HWND m_hwnd = NULL;
};

} // namespace Beam

#endif