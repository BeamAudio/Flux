// VST3 SDK Headers - MUST BE AT THE VERY TOP
#include "pluginterfaces/base/ftypes.h"
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/gui/iplugview.h"

#include "interface/editors/vst_external_editor.hpp"
#include "interface/core/theme.hpp"

namespace Beam {

VSTExternalEditor::VSTExternalEditor(::Steinberg::Vst::IEditController* controller) 
    : m_controller(controller) {
    setName("VSTExternalEditor");
}

VSTExternalEditor::~VSTExternalEditor() {
    if (m_view) {
        m_view->removed();
        m_view->release();
    }
}

void VSTExternalEditor::getPreferredSize(float& w, float& h) const {
    if (m_view) {
        ::Steinberg::ViewRect rect;
        if (m_view->getSize(&rect) == ::Steinberg::kResultTrue) {
            w = (float)(rect.right - rect.left);
            h = (float)(rect.bottom - rect.top);
            return;
        }
    }
    w = 400; h = 300; 
}

void VSTExternalEditor::setBounds(float x, float y, float w, float h) {
    Component::setBounds(x, y, w, h);
    updateNativeWindow();
}

void VSTExternalEditor::setVisible(bool visible) {
    Component::setVisible(visible);
    if (m_hwnd) {
        ShowWindow(m_hwnd, visible ? SW_SHOW : SW_HIDE);
    }
}

void VSTExternalEditor::render(QuadBatcher& batcher, float dt, float screenW, float screenH) {
    if (!m_isVisible) return;
    
    if (!m_view && m_controller) {
        m_view = m_controller->createView(::Steinberg::Vst::ViewType::kEditor);
        if (m_view && m_parentHwnd) {
            attachToNative(m_parentHwnd);
        }
    }

    paint(batcher);
}

void VSTExternalEditor::paint(QuadBatcher& batcher) {
    batcher.drawRect(0, 0, m_bounds.w, m_bounds.h, 1.0f, 0.1f, 0.1f, 0.1f, 1.0f);
    if (!m_view) {
         batcher.drawText("LOADING VST GUI...", 10, 10, 14, 1, 1, 1, 1);
    } else if (!m_isAttached) {
         batcher.drawText("VST VIEW CREATED, NOT ATTACHED", 10, 10, 14, 1, 1, 0, 1);
    }
}

void VSTExternalEditor::updateNativeWindow() {
    if (!m_isAttached || !m_view) return;

    Rect screenBounds = getScreenBounds();
    
    if (!m_hwnd) {
         // EnumChildWindows is safer but let's try a more direct approach first
         m_hwnd = FindWindowExA(m_parentHwnd, NULL, NULL, NULL); 
    }

    if (m_hwnd) {
        // Show/Hide based on component visibility
        ShowWindow(m_hwnd, m_isVisible ? SW_SHOW : SW_HIDE);
        
        // Match screen position exactly
        SetWindowPos(m_hwnd, HWND_TOP, (int)screenBounds.x, (int)screenBounds.y, (int)screenBounds.w, (int)screenBounds.h, SWP_NOACTIVATE);
        
        ::Steinberg::ViewRect rect;
        rect.left = 0;
        rect.top = 0;
        rect.right = (::Steinberg::int32)m_bounds.w;
        rect.bottom = (::Steinberg::int32)m_bounds.h;
        m_view->onSize(&rect);
    }
}

void VSTExternalEditor::attachToNative(HWND parentHWnd) {
    m_parentHwnd = parentHWnd;
    if (m_view && !m_isAttached && m_parentHwnd) {
        if (m_view->attached((void*)m_parentHwnd, ::Steinberg::kPlatformTypeHWND) == ::Steinberg::kResultTrue) {
            m_isAttached = true;
            // Force a resize immediately to trigger window creation/discovery
            updateNativeWindow();
        }
    }
}

} // namespace Beam
