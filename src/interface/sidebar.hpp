#ifndef SIDEBAR_HPP
#define SIDEBAR_HPP

#include "component.hpp"
#include <functional>
#include <string>

namespace Beam {

class Sidebar : public Component {
public:
    enum class Side { Left, Right };

    Sidebar(Side side) : m_side(side), m_category("NONE") {}

    void setVisible(bool visible) { m_isVisible = visible; }

    void render(QuadBatcher& batcher, float dt, float screenW, float screenH) override {
        if (!m_isVisible) return;
        batcher.drawQuad(m_bounds.x, m_bounds.y, m_bounds.w, m_bounds.h, 0.1f, 0.11f, 0.12f, 1.0f);
        float borderX = (m_side == Side::Left) ? m_bounds.x + m_bounds.w - 1 : m_bounds.x;
        batcher.drawQuad(borderX, m_bounds.y, 1, m_bounds.h, 0.2f, 0.2f, 0.2f, 1.0f);

        if (m_side == Side::Left) {
            float yOff = m_bounds.y + 20;
            batcher.drawText(m_category == "NONE" ? "CATEGORIES" : "< " + m_category, m_bounds.x + 15, yOff, 12, 0.5f, 0.5f, 0.5f, 1.0f);
            yOff += 30;

            auto drawBtn = [&](const std::string& label, bool isSelected = false) {
                batcher.drawRoundedRect(m_bounds.x + 10, yOff, m_bounds.w - 20, 28, 4.0f, 0.5f, isSelected ? 0.2f : 0.15f, isSelected ? 0.4f : 0.16f, isSelected ? 0.8f : 0.17f, 1.0f);
                batcher.drawText(label, m_bounds.x + 20, yOff + 8, 12, 0.9f, 0.9f, 0.9f, 1.0f);
                yOff += 32;
            };

            if (m_category == "NONE") {
                drawBtn("EQUALIZERS");
                drawBtn("DYNAMICS");
                drawBtn("SPACE");
                drawBtn("TIME");
                drawBtn("UTILITY");
            } else if (m_category == "EQUALIZERS") {
                drawBtn("Tube-P EQ");
                drawBtn("Console-E");
                drawBtn("Vintage-G");
                drawBtn("Graphic-10");
                drawBtn("Air-Lift");
            } else if (m_category == "DYNAMICS") {
                drawBtn("Opto-2A");
                drawBtn("FET-76");
                drawBtn("VCA-Bus");
                drawBtn("Vari-Mu");
                drawBtn("Tube Limiter");
            } else if (m_category == "SPACE") {
                drawBtn("Steel Plate");
                drawBtn("Golden Hall");
                drawBtn("Copper Spring");
                drawBtn("Cathedral");
                drawBtn("Grain Verb");
            } else if (m_category == "TIME") {
                drawBtn("Echo-Plex");
                drawBtn("BBD-Bucket");
                drawBtn("Reverse");
                drawBtn("Ping-Pong");
                drawBtn("Space Shift");
            } else if (m_category == "UTILITY") {
                drawBtn("Gain");
                drawBtn("Filter");
                drawBtn("Empty Tape");
            }
        }
    }

    bool onMouseDown(float x, float y, int button) override {
        if (!m_isVisible || m_side != Side::Left) return false;

        float yOff = m_bounds.y + 20;
        // Back to categories
        if (y > yOff && y < yOff + 25) { m_category = "NONE"; return true; }
        yOff += 30;

        auto checkBtn = [&](const std::string& label) {
            bool hit = (y > yOff && y < yOff + 28);
            yOff += 32;
            return hit;
        };

        if (m_category == "NONE") {
            if (checkBtn("EQUALIZERS")) m_category = "EQUALIZERS";
            else if (checkBtn("DYNAMICS")) m_category = "DYNAMICS";
            else if (checkBtn("SPACE")) m_category = "SPACE";
            else if (checkBtn("TIME")) m_category = "TIME";
            else if (checkBtn("UTILITY")) m_category = "UTILITY";
        } else {
            // Category lists
            std::vector<std::string> items;
            if (m_category == "EQUALIZERS") items = {"Tube-P EQ", "Console-E", "Vintage-G", "Graphic-10", "Air-Lift"};
            else if (m_category == "DYNAMICS") items = {"Opto-2A", "FET-76", "VCA-Bus", "Vari-Mu", "Tube Limiter"};
            else if (m_category == "SPACE") items = {"Steel Plate", "Golden Hall", "Copper Spring", "Cathedral", "Grain Verb"};
            else if (m_category == "TIME") items = {"Echo-Plex", "BBD-Bucket", "Reverse", "Ping-Pong", "Space Shift"};
            else if (m_category == "UTILITY") items = {"Gain", "Filter", "Empty Tape"};

            for (auto const& item : items) {
                if (checkBtn(item)) {
                    if (onAddFX) onAddFX(item);
                    return true;
                }
            }
        }
        return false;
    }

    std::function<void(std::string)> onAddFX;

private:
    Side m_side;
    std::string m_category;
};

} // namespace Beam

#endif // SIDEBAR_HPP





