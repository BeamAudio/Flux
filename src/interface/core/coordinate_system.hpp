#ifndef COORDINATE_SYSTEM_HPP
#define COORDINATE_SYSTEM_HPP

#include <cmath>

namespace Beam {

/**
 * @class CoordinateSystem
 * @brief Singleton that manages the global coordinate space transformations
 * between Screen Space (Static UI) and World Space (Workspace/Nodes).
 */
class CoordinateSystem {
public:
    static CoordinateSystem& get() {
        static CoordinateSystem instance;
        return instance;
    }

    // --- State setters (called by Workspace) ---
    void setZoom(float zoom) { m_zoom = zoom; }
    void setPan(float x, float y) { m_panX = x; m_panY = y; }
    void setWorkspaceOrigin(float x, float y) { m_originX = x; m_originY = y; }
    void setScreenDimensions(float w, float h) { m_screenW = w; m_screenH = h; }

    // --- Getters ---
    float getZoom() const { return m_zoom; }
    float getPanX() const { return m_panX; }
    float getPanY() const { return m_panY; }
    
    // --- Transformations ---

    /**
     * @brief Converts Screen Coordinates (Mouse) to World Coordinates (Virtual Space).
     * Used for input handling in Workspace and Nodes.
     */
    void screenToWorld(float screenX, float screenY, float& worldX, float& worldY) const {
        // 1. Remove Workspace Origin (convert to Workspace-Local)
        float localX = screenX - m_originX;
        float localY = screenY - m_originY;

        // 2. Apply Inverse Pan & Zoom
        // Formula: screen = (world * zoom) + pan
        // world = (screen - pan) / zoom
        worldX = (localX - m_panX) / m_zoom;
        worldY = (localY - m_panY) / m_zoom;
    }

    /**
     * @brief Converts World Coordinates (Virtual Space) to Screen Coordinates.
     * Used for determining where to draw overlays or popups.
     */
    void worldToScreen(float worldX, float worldY, float& screenX, float& screenY) const {
        // 1. Apply Pan & Zoom
        float localX = (worldX * m_zoom) + m_panX;
        float localY = (worldY * m_zoom) + m_panY;

        // 2. Add Workspace Origin
        screenX = localX + m_originX;
        screenY = localY + m_originY;
    }

private:
    CoordinateSystem() = default;
    
    float m_zoom = 1.0f;
    float m_panX = 0.0f;
    float m_panY = 0.0f;
    float m_originX = 0.0f;
    float m_originY = 0.0f;
    float m_screenW = 1920.0f;
    float m_screenH = 1080.0f;
};

} // namespace Beam

#endif // COORDINATE_SYSTEM_HPP
