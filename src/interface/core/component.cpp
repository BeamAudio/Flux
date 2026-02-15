#include "interface/core/component.hpp"
#include "interface/core/look_and_feel.hpp"
#include <iostream>

namespace Beam {

static ModernLookAndFeel globalDefaultLookAndFeel;

Component::Component() {}
Component::~Component() {
    if (m_fbo) { glDeleteFramebuffers(1, &m_fbo); }
    if (m_fboTexture) { glDeleteTextures(1, &m_fboTexture); }
}

void Component::localToScreen(float& x, float& y) {
    // 1. Apply Local Position
    x += m_bounds.x;
    y += m_bounds.y;
    // 2. Delegate to Parent
    if (m_parent) {
        m_parent->localToScreen(x, y);
    }
}

void Component::screenToLocal(float& x, float& y) {
    // 1. Delegate to Parent (Reverse Order)
    if (m_parent) {
        m_parent->screenToLocal(x, y);
    }
    // 2. Apply Local Position (Reverse)
    x -= m_bounds.x;
    y -= m_bounds.y;
}

void Component::addChildComponent(std::shared_ptr<Component> child) {
    if (child) {
        child->m_parent = this;
        m_children.push_back(child);
        m_needsRepaint = true; 
    }
}

void Component::removeChildComponent(Component* child) {
    m_children.erase(std::remove_if(m_children.begin(), m_children.end(),
        [child, this](const std::shared_ptr<Component>& c) { 
            if (c.get() == child) { m_needsRepaint = true; return true; }
            return false; 
        }), m_children.end());
}

void Component::setBounds(float x, float y, float w, float h) {
    if (std::abs(m_bounds.x - x) < 0.01f && std::abs(m_bounds.y - y) < 0.01f &&
        std::abs(m_bounds.w - w) < 0.01f && std::abs(m_bounds.h - h) < 0.01f) {
        return;
    }
    
    bool sizeChanged = (std::abs(m_bounds.w - w) > 0.01f || std::abs(m_bounds.h - h) > 0.01f);
    m_bounds = {x, y, w, h};
    if (sizeChanged) {
        resized();
        m_cachedWidth = 0; // Invalidate FBO
        m_needsRepaint = true;
    }
    if (m_parent) m_parent->childBoundsChanged(this);
}

#include "glad.h"

void Component::setBufferedToImage(bool buffered) {
    if (m_bufferedToImage != buffered) {
        m_bufferedToImage = buffered;
        if (buffered) {
            m_needsRepaint = true;
        } else {
            // Cleanup if disabling
            if (m_fbo) { glDeleteFramebuffers(1, &m_fbo); m_fbo = 0; }
            if (m_fboTexture) { glDeleteTextures(1, &m_fboTexture); m_fboTexture = 0; }
        }
    }
}

void Component::render(QuadBatcher& batcher, float dt, float screenW, float screenH) {
    if (!m_isVisible) return;

    if (m_bufferedToImage && m_bounds.w > 0 && m_bounds.h > 0) {
        // FBO Path
        int iw = (int)m_bounds.w;
        int ih = (int)m_bounds.h;
        
        // 0. Init / Resize FBO if needed
        if (m_fbo == 0 || iw != m_cachedWidth || ih != m_cachedHeight) {
            if (m_fbo == 0) glGenFramebuffers(1, &m_fbo);
            if (m_fboTexture == 0) glGenTextures(1, &m_fboTexture);
            
            glBindTexture(GL_TEXTURE_2D, m_fboTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, iw, ih, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            
            glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_fboTexture, 0);
            
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                std::cerr << "FBO Error for component " << m_name << std::endl;
                m_bufferedToImage = false; // Fallback
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            } else {
                m_cachedWidth = iw;
                m_cachedHeight = ih;
                m_needsRepaint = true;
            }
        }
        
        // 1. Update FBO Content if dirty
        if (m_needsRepaint && m_bufferedToImage) {
            batcher.flush(); // Finish screen work
            
            // Save View State
            GLint viewport[4]; glGetIntegerv(GL_VIEWPORT, viewport);
            
            glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
            glViewport(0, 0, iw, ih);
            glClearColor(0,0,0,0); // Transparent
            glClear(GL_COLOR_BUFFER_BIT);
            
            // Setup Batcher for FBO
            batcher.pushViewTransform(); // Save previous state
            batcher.resetViewTransform((float)iw, (float)ih);
            
            // We are drawing at 0,0 inside the FBO. 
            // "render" pushes offset (x,y). 
            // If we are in FBO, we should push (0,0) offset for THIS component.
            
            // setup
            float oldOffsetX = batcher.getOffsetX();
            float oldOffsetY = batcher.getOffsetY();
            batcher.setOffset(0.0f, 0.0f);
            
            // Paint Self
            paint(batcher);
            
            // Paint Children
            for (auto& child : m_children) {
                child->render(batcher, dt, (float)iw, (float)ih); 
            }
            
            batcher.flush();
            
            // Restore State
            batcher.setOffset(oldOffsetX, oldOffsetY);
            batcher.popViewTransform(); // Restore previous state
            
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
            
            m_needsRepaint = false;
        }

        // 2. Draw FBO Texture to Screen
        if (m_bufferedToImage) {
            // Need to restore view transform? 
            // Wait, I messed up the stack above.
            // I should push/pop inside the repaint block.
            
            // Draw Texture
            // Inverted V? FBO textures are often upside down in OpenGL relative to screen coords 
            // depending on projection. 
            // Our projection (BeamHost) sends 0,0 as Top Left.
            // Texture coords 0,0 is Bottom Left usually?
            // Let's rely on standard QuadBatcher drawTexture.
            
            batcher.drawTexture(m_fboTexture, m_bounds.x, m_bounds.y, m_bounds.w, m_bounds.h, 
                                0.0f, 1.0f, 1.0f, 0.0f, // Flip V?
                                1.0f, 1.0f, 1.0f, 1.0f);
        }
        
    } else {
        // Standard Path
        // 1. Push Offset for local drawing
        batcher.pushOffset(m_bounds.x, m_bounds.y);

        // 2. Clip to this component's bounds (using local coordinates)
        if (m_clipsChildren) {
            batcher.pushClip(0, 0, m_bounds.w, m_bounds.h, screenH);
        }

        // 3. Paint in local space (0,0 is top-left)
        paint(batcher);

        // 4. Render children
        for (auto& child : m_children) {
            child->render(batcher, dt, screenW, screenH);
        }

        if (m_clipsChildren) {
            batcher.popClip(screenH);
        }

        // 5. Pop Offset
        batcher.popOffset();
    }
}

bool Component::onMouseDown(float x, float y, int button, bool shift) {
    if (!m_isVisible) return false;
    if (!m_bounds.contains(x, y)) return false;

    float localX = x - m_bounds.x;
    float localY = y - m_bounds.y;

    // 1. Give children a chance first - iterate backwards by index to be safe against removals
    for (int i = (int)m_children.size() - 1; i >= 0; --i) {
        // Ensure index is still valid (in case previous child removed siblings)
        if (i >= (int)m_children.size()) continue; 
        
        auto child = m_children[i];
        if (child->onMouseDown(localX, localY, button, shift)) {
            m_capturedChild = child; // Capture for subsequent Move/Up
            return true;
        }
    }

    // 2. If no child consumed it, handle locally
    if (m_isDraggable) startDragging(x, y);
    mouseDown(MouseEvent(localX, localY, button, shift));

    return true; 
}

bool Component::onMouseUp(float x, float y, int button, bool shift) {
    if (!m_isVisible) return false;
    
    m_isDragging = false;
    
    // We notify even if not 'contained' to ensure button releases etc.
    float localX = x - m_bounds.x;
    float localY = y - m_bounds.y;

    mouseUp(MouseEvent(localX, localY, button, shift));

    if (m_capturedChild) {
        float childX = localX - m_capturedChild->m_bounds.x;
        float childY = localY - m_capturedChild->m_bounds.y;
        m_capturedChild->onMouseUp(childX, childY, button, shift);
        m_capturedChild = nullptr; // Release capture
    } else {
        for (int i = (int)m_children.size() - 1; i >= 0; --i) {
            if (i >= (int)m_children.size()) continue;
            auto child = m_children[i];
            float childX = localX - child->m_bounds.x;
            float childY = localY - child->m_bounds.y;
            child->onMouseUp(childX, childY, button, shift);
        }
    }
    return true;
}

bool Component::onMouseMove(float x, float y, bool shift) {
    if (!m_isVisible) return false;

    if (m_isDragging) {
        // Dragging happens in parent space
        setBounds(x - m_dragStartX, y - m_dragStartY, m_bounds.w, m_bounds.h);
        mouseDrag(MouseEvent(x - m_bounds.x, y - m_bounds.y, 0, shift));
        return true;
    }

    float localX = x - m_bounds.x;
    float localY = y - m_bounds.y;

    if (m_capturedChild) {
        float childX = localX - m_capturedChild->m_bounds.x;
        float childY = localY - m_capturedChild->m_bounds.y;
        
        // During capture, call mouseDrag on the captured child (what widgets like Knob expect)
        m_capturedChild->mouseDrag(MouseEvent(childX, childY, 0, shift));
        // Also propagate onMouseMove so nested captures work
        m_capturedChild->onMouseMove(childX, childY, shift);
        return true;
    }

    bool hit = m_bounds.contains(x, y);
    if (hit) {
        mouseMove(MouseEvent(localX, localY, 0, shift));
    }

    // Default propagation (if no capture)
    for (int i = (int)m_children.size() - 1; i >= 0; --i) {
        if (i >= (int)m_children.size()) continue;
        auto child = m_children[i];
        float childX = localX - child->m_bounds.x;
        float childY = localY - child->m_bounds.y;
        child->onMouseMove(childX, childY, shift);
    }

    m_lastMouseX = localX;
    m_lastMouseY = localY;
    return hit;
}

bool Component::onMouseWheel(float x, float y, float delta, bool shift) {
    if (!m_isVisible || !m_bounds.contains(x, y)) return false;

    float localX = x - m_bounds.x;
    float localY = y - m_bounds.y;

    for (int i = (int)m_children.size() - 1; i >= 0; --i) {
        if (i >= (int)m_children.size()) continue;
        if (m_children[i]->onMouseWheel(localX, localY, delta, shift)) return true;
    }
    return true;
}

LookAndFeel& Component::getLookAndFeel() const {
    if (m_lookAndFeel) return *m_lookAndFeel;
    return globalDefaultLookAndFeel;
}

} // namespace Beam