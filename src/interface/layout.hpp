#ifndef LAYOUT_HPP
#define LAYOUT_HPP

#include "component.hpp"
#include <vector>
#include <algorithm>
#include <cmath>

namespace Beam {

/**
 * @struct LayoutItem
 * @brief Represents an item in a FlexBox layout. Can be a component or a spacer.
 */
struct LayoutItem {
    Component* component = nullptr;
    float minWidth = 0.0f;
    float minHeight = 0.0f;
    float maxWidth = 10000.0f;
    float maxHeight = 10000.0f;
    float flexBasis = 0.0f; // Preferred size
    float flexGrow = 0.0f;  // Growth factor
    float flexShrink = 1.0f;// Shrink factor
    float margin = 0.0f;    // Uniform margin
    float marginLeft = 0.0f;
    float marginRight = 0.0f;
    float marginTop = 0.0f;
    float marginBottom = 0.0f;

    LayoutItem(Component* c) : component(c) {
        if (c) {
            // Default to component's current size as basis if not set
            flexBasis = 0.0f; 
        }
    }
    
    LayoutItem() {} // Spacer

    LayoutItem& withFlex(float grow, float shrink = 1.0f, float basis = 0.0f) {
        flexGrow = grow;
        flexShrink = shrink;
        flexBasis = basis;
        return *this;
    }

    LayoutItem& withsize(float w, float h) {
        minWidth = w; maxWidth = w;
        minHeight = h; maxHeight = h;
        flexBasis = (w > 0) ? w : h; // Rough heuristic
        return *this;
    }

    LayoutItem& withMargin(float m) {
        margin = m;
        marginLeft = marginRight = marginTop = marginBottom = m;
        return *this;
    }
};

/**
 * @class FlexBox
 * @brief A simple FlexBox-like layout engine for Beam Audio Flux.
 * Allows arranging components in rows or columns with alignment and justification.
 */
class FlexBox {
public:
    enum class Direction { Row, Column };
    enum class JustifyContent { FlexStart, Center, FlexEnd, SpaceBetween, SpaceAround };
    enum class AlignItems { FlexStart, Center, FlexEnd, Stretch };

    FlexBox() {}

    FlexBox& flexDirection(Direction d) { m_direction = d; return *this; }
    FlexBox& justifyContent(JustifyContent j) { m_justifyContent = j; return *this; }
    FlexBox& alignItems(AlignItems a) { m_alignItems = a; return *this; }
    
    FlexBox& addItem(LayoutItem item) {
        m_items.push_back(item);
        return *this;
    }

    FlexBox& addItem(Component& component) {
        m_items.push_back(LayoutItem(&component));
        return *this;
    }

    /**
     * @brief Performs the layout on the given bounds.
     * Updates the bounds of all associated components.
     */
    void performLayout(Rect bounds) {
        if (m_items.empty()) return;

        bool isRow = (m_direction == Direction::Row);
        float mainSize = isRow ? bounds.w : bounds.h;
        float crossSize = isRow ? bounds.h : bounds.w;
        float startX = bounds.x;
        float startY = bounds.y;

        // 1. Calculate total basis and flexible space
        float totalBasis = 0.0f;
        float totalGrow = 0.0f;
        float totalShrink = 0.0f;

        for (auto& item : m_items) {
            float marginMain = isRow ? (item.marginLeft + item.marginRight) : (item.marginTop + item.marginBottom);
            float basis = item.flexBasis + marginMain;
            totalBasis += basis;
            totalGrow += item.flexGrow;
            totalShrink += item.flexShrink;
        }

        float freeSpace = mainSize - totalBasis;
        
        // 2. Distribute free space
        std::vector<float> itemSizes;
        itemSizes.reserve(m_items.size());

        for (auto& item : m_items) {
            float marginMain = isRow ? (item.marginLeft + item.marginRight) : (item.marginTop + item.marginBottom);
            float baseSize = item.flexBasis;
            float targetSize = baseSize;

            if (freeSpace > 0 && totalGrow > 0) {
                targetSize += freeSpace * (item.flexGrow / totalGrow);
            } else if (freeSpace < 0 && totalShrink > 0) {
                // Shrink
                float shrinkAmount = (-freeSpace) * (item.flexShrink / totalShrink); // Simplified
                targetSize -= shrinkAmount;
            }
            
            // Clamping would go here (min/max size)
            itemSizes.push_back(std::max(0.0f, targetSize));
        }

        // 3. Position items (Main Axis)
        float currentPos = 0.0f;
        
        // Handle JustifyContent if no growth happened (or logic overrides)
        // If totalGrow > 0, we filled the space, so Justify doesn't matter much unless we clamped.
        // For simplicity, let's assume if totalGrow == 0, we use Justify.
        
        float spacing = 0.0f;
        float justifyOffset = 0.0f;

        if (totalGrow == 0.0f && freeSpace > 0.0f) {
            switch (m_justifyContent) {
                case JustifyContent::FlexStart: justifyOffset = 0.0f; break;
                case JustifyContent::Center: justifyOffset = freeSpace / 2.0f; break;
                case JustifyContent::FlexEnd: justifyOffset = freeSpace; break;
                case JustifyContent::SpaceBetween: 
                    if (m_items.size() > 1) spacing = freeSpace / (m_items.size() - 1); 
                    break;
                case JustifyContent::SpaceAround: 
                    if (!m_items.empty()) {
                        spacing = freeSpace / m_items.size();
                        justifyOffset = spacing / 2.0f;
                    }
                    break;
            }
        }

        currentPos += justifyOffset;

        for (size_t i = 0; i < m_items.size(); ++i) {
            auto& item = m_items[i];
            float sizeMain = itemSizes[i];
            
            // Cross Axis Alignment
            float sizeCross = crossSize;
            float crossPos = 0.0f;
            
            float marginCrossTotal = isRow ? (item.marginTop + item.marginBottom) : (item.marginLeft + item.marginRight);
            float availableCross = crossSize - marginCrossTotal;

            // Apply specific cross size if items have explicit constraints (simplified here)
            // If stretch, use available. Else use basis? Or min? 
            // Let's assume Stretch fills availableCross, others use minHeight/Width or flexBasis if cross-oriented?
            // For now, let's just stick to Stretch filling and Center centering a "default" size.
            
            // Assume items want to be 'availableCross' size if Stretch.
            // If Center/Start/End, we need a 'size' for the item. 
            // We'll use item.minHeight/Width as the 'native' size for cross axis if not stretching.
            float itemCrossSize = isRow ? item.minHeight : item.minWidth;
            if (itemCrossSize <= 0.0f) itemCrossSize = availableCross; // Default if no size set

            switch (m_alignItems) {
                case AlignItems::Stretch: 
                    sizeCross = availableCross; 
                    crossPos = 0.0f;
                    break;
                case AlignItems::FlexStart:
                    sizeCross = itemCrossSize;
                    crossPos = 0.0f;
                    break;
                case AlignItems::Center:
                    sizeCross = itemCrossSize;
                    crossPos = (availableCross - sizeCross) / 2.0f;
                    break;
                case AlignItems::FlexEnd:
                    sizeCross = itemCrossSize;
                    crossPos = availableCross - sizeCross;
                    break;
            }

            // Apply Margins
            float x, y, w, h;
            if (isRow) {
                x = startX + currentPos + item.marginLeft;
                y = startY + crossPos + item.marginTop;
                w = sizeMain;
                h = sizeCross;
                currentPos += sizeMain + item.marginLeft + item.marginRight + spacing;
            } else {
                x = startX + crossPos + item.marginLeft;
                y = startY + currentPos + item.marginTop;
                w = sizeCross;
                h = sizeMain;
                currentPos += sizeMain + item.marginTop + item.marginBottom + spacing;
            }

            if (item.component) {
                item.component->setBounds(x, y, w, h);
            }
        }
    }

private:
    Direction m_direction = Direction::Row;
    JustifyContent m_justifyContent = JustifyContent::FlexStart;
    AlignItems m_alignItems = AlignItems::Stretch;
    std::vector<LayoutItem> m_items;
};

} // namespace Beam

#endif // LAYOUT_HPP
