#ifndef SCROLLABLE_CONTAINER_HPP
#define SCROLLABLE_CONTAINER_HPP

#include "interface/core/component.hpp"

namespace Beam {

/**
 * @class ScrollableContainer
 * @brief A container that provides vertical scrolling for its content.
 */
class ScrollableContainer : public Component {
public:
    ScrollableContainer() {
        setName("ScrollableContainer");
        setClipsChildren(true);
    }

    void setContent(std::shared_ptr<Component> content) {
        if (m_content) removeChildComponent(m_content.get());
        m_content = content;
        if (m_content) {
            addChildComponent(m_content);
            m_content->setParent(this);
            updateContentBounds();
        }
    }

    std::shared_ptr<Component> getContent() { return m_content; }

    void scroll(float delta) {
        m_scrollY -= delta * 30.0f; // Scroll speed
        clampScroll();
        updateContentBounds();
    }

    void scrollToTop() {
        m_scrollY = 0.0f;
        updateContentBounds();
    }

    void resized() override {
        updateContentBounds();
    }

    void updateContentBounds() {
        if (!m_content) return;
        
        float pw = 0, ph = 0;
        m_content->getPreferredSize(pw, ph);
        
        // Match container width, use preferred height
        float contentW = m_bounds.w;
        float contentH = (std::max)(ph, m_bounds.h);
        
        m_contentHeight = contentH;
        clampScroll();
        
        m_content->setBounds(0, -m_scrollY, contentW, contentH);
    }

    bool onMouseWheel(float x, float y, float delta, bool shift) override {
        if (!m_isVisible || !m_bounds.contains(x, y)) return false;
        scroll(delta);
        return true;
    }

    // Capture mouse events and offset them for content
    bool onMouseDown(float x, float y, int button, bool shift) override {
        if (!m_isVisible || !m_bounds.contains(x, y)) return false;
        
        if (m_content) {
            // x, y are relative to our parent. 
            // Content is at (0, -scrollY) relative to OUR (0,0).
            // Our (0,0) is at (m_bounds.x, m_bounds.y) relative to parent.
            float lx = x - m_bounds.x;
            float ly = y - m_bounds.y;
            
            float cx = lx - m_content->getX();
            float cy = ly - m_content->getY();
            return m_content->onMouseDown(cx, cy, button, shift);
        }
        return true; 
    }

    bool onMouseUp(float x, float y, int button, bool shift) override {
        if (m_content) {
            float lx = x - m_bounds.x;
            float ly = y - m_bounds.y;
            float cx = lx - m_content->getX();
            float cy = ly - m_content->getY();
            return m_content->onMouseUp(cx, cy, button, shift);
        }
        return false;
    }

    bool onMouseMove(float x, float y, bool shift) override {
        if (m_content) {
            float lx = x - m_bounds.x;
            float ly = y - m_bounds.y;
            float cx = lx - m_content->getX();
            float cy = ly - m_content->getY();
            return m_content->onMouseMove(cx, cy, shift);
        }
        return false;
    }

    void render(QuadBatcher& batcher, float dt, float screenW, float screenH) override {
        if (!m_isVisible) return;

        batcher.pushOffset(m_bounds.x, m_bounds.y);
        batcher.pushClip(0, 0, m_bounds.w, m_bounds.h, screenH);

        if (m_content) {
            m_content->render(batcher, dt, screenW, screenH);
        }

        // Draw Scrollbar handle if needed
        if (m_contentHeight > m_bounds.h) {
            float handleH = (m_bounds.h / m_contentHeight) * m_bounds.h;
            float handleY = (m_scrollY / m_contentHeight) * m_bounds.h;
            batcher.drawRoundedRect(m_bounds.w - 6, handleY, 4, handleH, 2.0f, 0.5f, 1.0f, 1.0f, 1.0f, 0.3f);
        }

        batcher.popClip(screenH);
        batcher.popOffset();
    }

private:
    void clampScroll() {
        float maxScroll = (std::max)(0.0f, m_contentHeight - m_bounds.h);
        m_scrollY = std::clamp(m_scrollY, 0.0f, maxScroll);
    }

    std::shared_ptr<Component> m_content;
    float m_scrollY = 0.0f;
    float m_contentHeight = 0.0f;
};

} // namespace Beam

#endif
