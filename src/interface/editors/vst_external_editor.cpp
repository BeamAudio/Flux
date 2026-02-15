#include "pluginterfaces/base/ftypes.h"
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/gui/iplugview.h"

#include "interface/editors/vst_external_editor.hpp"
#include "engine/hosting/vst3_utils.hpp"
#include "interface/core/theme.hpp"

#include <SDL3/SDL.h>

#include <iostream>
#include <fstream>
#include <dwmapi.h>
#include <objbase.h>
#include <algorithm>
#pragma comment(lib, "dwmapi.lib")

namespace Beam {
// ... register class ...

static const wchar_t* VST_WINDOW_CLASS = L"BeamVSTContainer";
static bool s_windowClassRegistered = false;

// Static Message Pump Members
std::vector<HWND> VSTExternalEditor::s_openWindows;
std::mutex VSTExternalEditor::s_windowsMutex;

void VSTExternalEditor::processMessageLoop() {
    std::vector<HWND> windows;
    {
        std::lock_guard<std::mutex> lock(s_windowsMutex);
        windows = s_openWindows;
    }
    
    for (HWND hwnd : windows) {
        try {
            if (!IsWindow(hwnd)) continue;
            MSG msg;
            // Pump messages for the VST Container and its children (the VST View)
            // Limit number of messages to prevent starvation of the Host Loop (e.g. if VST floods WM_PAINT)
            int messageCount = 0;
            const int MAX_MESSAGES_PER_FRAME = 100; // Increased throughput
            uint64_t startTime = SDL_GetTicks();
            const uint64_t MAX_TIME_MS = 5; // Increased time slice

            while (PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                
                messageCount++;
                if (messageCount >= MAX_MESSAGES_PER_FRAME) break;
                if (SDL_GetTicks() - startTime >= MAX_TIME_MS) break;
            }
        } catch (...) {
            std::cerr << "[VST] Warning: Exception in message pump for HWND " << hwnd << std::endl;
        }
    }
}

static void RegisterVSTWindowClass() {
    if (s_windowClassRegistered) return;
    
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = VSTExternalEditor::VSTContainerWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = VST_WINDOW_CLASS;
    
    if (RegisterClassExW(&wc)) {
        s_windowClassRegistered = true;
    }
}

LRESULT CALLBACK VSTExternalEditor::VSTContainerWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    VSTExternalEditor* editor = (VSTExternalEditor*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    
    switch (msg) {
        case WM_NCCREATE: {
            LPCREATESTRUCTW lpcs = (LPCREATESTRUCTW)lParam;
            editor = (VSTExternalEditor*)lpcs->lpCreateParams;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)editor);
            return 1;
        }
        case WM_CLOSE:
            if (editor) editor->closeWindow();
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

VSTExternalEditor::VSTExternalEditor(::Steinberg::Vst::IEditController* controller, void* parentWindowHandle, void* moduleHandle) 
    : m_controller(controller), m_parentHwnd((HWND)parentWindowHandle), m_moduleHandle(moduleHandle) {
    setName("VSTExternalEditor");
    std::cout << "[VSTExternalEditor] Constructor called. ParentHWND: " << m_parentHwnd << std::endl;
    RegisterVSTWindowClass();
}

VSTExternalEditor::~VSTExternalEditor() {
    std::cout << "[VSTExternalEditor] Destructor called." << std::endl;
    closeWindow();
}

void VSTExternalEditor::closeWindow() {
    std::cout << "[VSTExternalEditor] closeWindow()" << std::endl;
    if (!m_windowOpen) return;
    m_windowOpen = false;

    if (m_view) {
        m_view->removed();
    }
    
    // Unregister and Destroy Child Window (VST Container)
    if (m_vstHwnd) {
        {
            std::lock_guard<std::mutex> lock(s_windowsMutex);
            auto it = std::find(s_openWindows.begin(), s_openWindows.end(), m_vstHwnd);
            if (it != s_openWindows.end()) s_openWindows.erase(it);
        }
        std::cout << "[VSTExternalEditor] Destroying Child Window." << std::endl;
        DestroyWindow(m_vstHwnd);
        m_vstHwnd = NULL;
    }

    // Destroy SDL Window (Host Frame)
    if (m_sdlWindow) {
        std::cout << "[VSTExternalEditor] Destroying SDL Window." << std::endl;
        SDL_DestroyWindow((SDL_Window*)m_sdlWindow);
        m_sdlWindow = nullptr;
    }

    if (m_view) {
        m_view->release();
        m_view = nullptr;
    }
}

void VSTExternalEditor::hideWindow() {
    // SAFE for file dialog scenarios - just hide, don't destroy
    if (m_windowOpen && m_sdlWindow) {
        std::cout << "[VSTExternalEditor] hideWindow() - hiding SDL window" << std::endl;
        SDL_HideWindow((SDL_Window*)m_sdlWindow);
    }
}

// ... openWindow ...
void VSTExternalEditor::openWindow() {
    std::cout << "[VSTExternalEditor] openWindow() CALLED" << std::endl;
    if (m_windowOpen) {
        if (m_sdlWindow) {
            SDL_ShowWindow((SDL_Window*)m_sdlWindow);
            SDL_RaiseWindow((SDL_Window*)m_sdlWindow);
        }
        return;
    }
    
    // 1. Create the View Object (Main Thread)
    if (m_controller) {
        m_view = m_controller->createView(::Steinberg::Vst::ViewType::kEditor);
        if (m_view) {
            std::cout << "[VSTExternalEditor] View created." << std::endl;
            m_view->setFrame(this);
            // Get preferred size
            ::Steinberg::ViewRect rect;
            if (m_view->getSize(&rect) == ::Steinberg::kResultTrue) {
                int w = rect.right - rect.left;
                int h = rect.bottom - rect.top;
                if (w > 0) m_pluginWidth = w;
                if (h > 0) m_pluginHeight = h;
            }
        } else {
             std::cerr << "[VSTExternalEditor] Failed to create view!" << std::endl;
             return;
        }
    }

    if (!m_view) return;

    // 2. Create the SDL Window (Host Frame)
    // Use Always On Top to simulate floating tool window behavior
    std::cout << "[VSTExternalEditor] Creating SDL Window " << m_pluginWidth << "x" << m_pluginHeight << std::endl;
    
    // Create hidden initially. Removed ALWAYS_ON_TOP to avoid focus-related deadlocks in SDL3 multi-window.
    m_sdlWindow = SDL_CreateWindow("VST Plugin", m_pluginWidth, m_pluginHeight, SDL_WINDOW_HIDDEN);
    
    if (!m_sdlWindow) {
        std::cerr << "[VSTExternalEditor] SDL_CreateWindow Failed: " << SDL_GetError() << std::endl;
        m_view->release();
        m_view = nullptr;
        return;
    }

    // 3. Get Native HWND from SDL
    HWND sdlHwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties((SDL_Window*)m_sdlWindow), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
    
    if (!sdlHwnd) {
         std::cerr << "[VSTExternalEditor] Failed to get HWND from SDL Window!" << std::endl;
         SDL_DestroyWindow((SDL_Window*)m_sdlWindow);
         return;
    }

    // 4. Create Child Window for VST (To separate Event Loop)
    // This allows us to pump messages for this window specifically without stealing from SDL
    std::cout << "[VSTExternalEditor] Creating Child Window..." << std::endl;
    m_vstHwnd = CreateWindowExW(
        0, 
        VST_WINDOW_CLASS, L"VSTChild", 
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
        0, 0, m_pluginWidth, m_pluginHeight, 
        sdlHwnd, NULL, GetModuleHandle(NULL), this
    );

    if (!m_vstHwnd) {
        std::cerr << "[VSTExternalEditor] Failed to create child window! Error: " << GetLastError() << std::endl;
        SDL_DestroyWindow((SDL_Window*)m_sdlWindow);
        return;
    }

    // Register for manual pumping
    {
        std::lock_guard<std::mutex> lock(s_windowsMutex);
        s_openWindows.push_back(m_vstHwnd);
    }

    // Disable DWM Transitions on child?
    // Not needed for child usually, but can do.

    // 5. Attach View to Child
    std::cout << "[VSTExternalEditor] Attaching view to Child HWND..." << std::endl;
    if (m_view->attached((void*)m_vstHwnd, ::Steinberg::kPlatformTypeHWND) != ::Steinberg::kResultTrue) {
        std::cerr << "[VSTExternalEditor] Failed to attach view." << std::endl;
        // Unregister
        {
            std::lock_guard<std::mutex> lock(s_windowsMutex);
            auto it = std::find(s_openWindows.begin(), s_openWindows.end(), m_vstHwnd);
            if (it != s_openWindows.end()) s_openWindows.erase(it);
        }
        DestroyWindow(m_vstHwnd);
        SDL_DestroyWindow((SDL_Window*)m_sdlWindow);
        m_sdlWindow = nullptr;
        m_vstHwnd = NULL;
        m_view->release();
        m_view = nullptr;
        return;
    }

    // 6. Show Window
    ::Steinberg::ViewRect rect(0, 0, m_pluginWidth, m_pluginHeight);
    m_view->onSize(&rect);
    
    std::cout << "[VSTExternalEditor] Showing Window." << std::endl;
    SDL_ShowWindow((SDL_Window*)m_sdlWindow);
    m_windowOpen = true;
}

void VSTExternalEditor::getPreferredSize(float& w, float& h) const { w = 200.0f; h = 150.0f; }
void VSTExternalEditor::setBounds(float x, float y, float w, float h) { Component::setBounds(x, y, w, h); }
void VSTExternalEditor::setVisible(bool visible) {
    Component::setVisible(visible);

    if (!visible && m_windowOpen) {
        if (m_sdlWindow) SDL_HideWindow((SDL_Window*)m_sdlWindow);
    } else if (visible && m_windowOpen) {
        if (m_sdlWindow) SDL_ShowWindow((SDL_Window*)m_sdlWindow);
    }
}

void VSTExternalEditor::render(QuadBatcher& batcher, float dt, float screenW, float screenH) { 
    if (m_isVisible) paint(batcher); 
}

void VSTExternalEditor::update(float dt) {
    Component::update(dt);
    
    // Sync Window Position if open
    if (m_windowOpen && m_sdlWindow && m_isVisible) {
         // Optionally check for focus or other attributes
    }
}

void VSTExternalEditor::updateWindowPosition(int mainX, int mainY) {
    if (m_sdlWindow && m_windowOpen) {
        SDL_SetWindowPosition((SDL_Window*)m_sdlWindow, mainX + m_relativeX, mainY + m_relativeY);
    }
}

void VSTExternalEditor::paint(QuadBatcher& batcher) {
    // Main Body (Darker Rack Mount Look)
    batcher.drawChassisPanel(0, 0, m_bounds.w, m_bounds.h, 4.0f, Theme::Console.darker(0.2f).r, Theme::Console.darker(0.2f).g, Theme::Console.darker(0.2f).b, 1.0f);
    
    // Status Text
    const char* statusText = m_windowOpen ? "External Editor Open" : "Editor Closed";
    Color statusCol = m_windowOpen ? Theme::Emerald : Theme::GreyLight;
    batcher.drawVectorText(statusText, 10, 10, 11, statusCol.r, statusCol.g, statusCol.b, 1.0f);
    
    // Centered Button
    float btnW = 120.0f;
    float btnH = 28.0f;
    float btnX = (m_bounds.w - btnW) / 2.0f;
    float btnY = (m_bounds.h - btnH) / 2.0f;
    
    Color btnCol = m_windowOpen ? Theme::Red.darker(0.1f) : Theme::Secondary;
    
    batcher.drawRoundedGradientRect(btnX, btnY, btnW, btnH, 4.0f, 0.5f, 
                                   btnCol.r, btnCol.g, btnCol.b, 1.0f,
                                   btnCol.darker(0.3f).r, btnCol.darker(0.3f).g, btnCol.darker(0.3f).b, 1.0f);

    batcher.drawRect(btnX, btnY, btnW, btnH, 4.0f, 1.0f, 1.0f, 1.0f, 0.3f); // Highlight stroke
    
    const char* btnText = m_windowOpen ? "Close Editor" : "Open Editor";
    float textW = strlen(btnText) * 7.0f; 
    batcher.drawVectorText(btnText, btnX + (btnW - textW) / 2.0f, btnY + 7, 12, 1.0f, 1.0f, 1.0f, 1.0f);
}

bool VSTExternalEditor::onMouseDown(float x, float y, int button, bool shift) {
    // Component::onMouseDown checks bounds, so we know we are inside.
    // Convert parent coordinates (x, y) to LOCAL coordinates.
    float localX = x - m_bounds.x;
    float localY = y - m_bounds.y;

    float btnW = 120.0f;
    float btnH = 28.0f;
    float btnX = (m_bounds.w - btnW) / 2.0f;
    float btnY = (m_bounds.h - btnH) / 2.0f;
    
    // Debug Click
    std::cout << "[VSTExternalEditor] onMouseDown: Input(" << x << "," << y << ") "
              << "Local(" << localX << "," << localY << ") "
              << "Bounds(" << m_bounds.x << "," << m_bounds.y << " " << m_bounds.w << "x" << m_bounds.h << ") "
              << "Btn(" << btnX << "," << btnY << " " << btnW << "x" << btnH << ")" << std::endl;

    // Check if clicked the button area (roughly center)
    // Or just make the whole component clickable for ease of use
    bool clicked = (localX >= 0 && localX <= m_bounds.w && localY >= 0 && localY <= m_bounds.h);

    if (clicked) {
        std::cout << "[VST] Button/Area Clicked! Toggling window." << std::endl;
        if (m_windowOpen) closeWindow();
        else openWindow();
        return true;
    }
    return Component::onMouseDown(x, y, button, shift);
}

Steinberg::tresult PLUGIN_API VSTExternalEditor::resizeView(Steinberg::IPlugView* view, Steinberg::ViewRect* newSize) {
    if (!newSize || !view || view != m_view) return Steinberg::kResultFalse;
    
    int w = newSize->right - newSize->left;
    int h = newSize->bottom - newSize->top;
    
    m_pluginWidth = w;
    m_pluginHeight = h;
    
    std::cout << "[VSTExternalEditor] resizeView called: " << w << "x" << h << std::endl;

    if (m_sdlWindow) {
        std::cout << "[VST] Resizing SDL Window and Child to " << w << "x" << h << std::endl;
        SDL_SetWindowSize((SDL_Window*)m_sdlWindow, w, h);
        
        if (m_vstHwnd) {
             SetWindowPos(m_vstHwnd, NULL, 0, 0, w, h, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
        }
    } 
    return Steinberg::kResultTrue; 
}

Steinberg::tresult PLUGIN_API VSTExternalEditor::queryInterface(const Steinberg::TUID _iid, void** obj) {
    if (Steinberg::FUnknownPrivate::iidEqual(_iid, Steinberg::IPlugFrame::iid) ||
        Steinberg::FUnknownPrivate::iidEqual(_iid, Steinberg::FUnknown::iid)) {
        *obj = static_cast<Steinberg::IPlugFrame*>(this);
        addRef();
        return Steinberg::kResultTrue;
    }
    *obj = nullptr;
    return Steinberg::kNoInterface;
}

} // namespace Beam
