#ifndef LAYOUT_HPP
#define LAYOUT_HPP

#include "interface/core/component.hpp"
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
    float maxWidth = 100000.0f; // Larger default max
    float maxHeight = 100000.0f;
    float flexBasis = 0.0f; // Preferred size
    float flexGrow = 0.0f;  // Growth factor
    float flexShrink = 1.0f;// Shrink factor
    float marginLeft = 0.0f;
    float marginRight = 0.0f;
    float marginTop = 0.0f;
    float marginBottom = 0.0f;

    LayoutItem(Component* c) : component(c) {
        if (c) {
            float pw = 0, ph = 0;
            c->getPreferredSize(pw, ph);
            flexBasis = (pw > 0) ? pw : 0.0f; // Default basis if component has preferred width
            minWidth = pw;
            minHeight = ph;
        }
    }
    
    LayoutItem() {} // Spacer

    LayoutItem& withFlex(float grow, float shrink = 1.0f, float basis = 0.0f) {
        flexGrow = grow;
        flexShrink = shrink;
        flexBasis = basis;
        return *this;
    }

    LayoutItem& withFixedSize(float w, float h) {
        if (w >= 0) { minWidth = maxWidth = w; flexBasis = w; }
        if (h >= 0) { minHeight = maxHeight = h; if (flexBasis == 0) flexBasis = h; } 
        // If h < 0, we leave minHeight/maxHeight as default (0 / 100000), allowing stretch
        return *this;
    }

    LayoutItem& withWidth(float w) {
        minWidth = maxWidth = w;
        flexBasis = w;
        return *this;
    }

    LayoutItem& withHeight(float h) {
        minHeight = maxHeight = h;
        flexBasis = h;
        return *this;
    }

    LayoutItem& withMinSize(float w, float h) {
        minWidth = w;
        minHeight = h;
        return *this;
    }
    
    LayoutItem& withMaxSize(float w, float h) {
        maxWidth = w;
        maxHeight = h;
        return *this;
    }

    LayoutItem& withMargin(float m) {
        marginLeft = marginRight = marginTop = marginBottom = m;
        return *this;
    }
};

/**
 * @class FlexBox
 * @brief A FlexBox-like layout engine for Beam Audio Flux.
 */
class FlexBox {
public:
    enum class Direction { Row, Column };
    enum class JustifyContent { FlexStart, Center, FlexEnd, SpaceBetween, SpaceAround, SpaceEvenly };
    enum class AlignItems { FlexStart, Center, FlexEnd, Stretch };

    FlexBox() {}

    FlexBox& flexDirection(Direction d) { m_direction = d; return *this; }
    FlexBox& justifyContent(JustifyContent j) { m_justifyContent = j; return *this; }
    FlexBox& alignItems(AlignItems a) { m_alignItems = a; return *this; }
    FlexBox& flexWrap(bool wrap) { m_flexWrap = wrap; return *this; }
    
    FlexBox& padding(float p) { m_paddingLeft = m_paddingRight = m_paddingTop = m_paddingBottom = p; return *this; }
    FlexBox& padding(float l, float r, float t, float b) {
        m_paddingLeft = l; m_paddingRight = r; m_paddingTop = t; m_paddingBottom = b;
        return *this;
    }

    FlexBox& addItem(LayoutItem item) {
        m_items.push_back(item);
        return *this;
    }

    FlexBox& addItem(Component& component) {
        m_items.push_back(LayoutItem(&component));
        return *this;
    }

    float calculateHeight(float width) const {
        float h = 0;
        if (m_direction == Direction::Row && !m_flexWrap) {
            for (const auto& item : m_items) {
                float pw = 0, ph = 0;
                if (item.component) item.component->getPreferredSize(pw, ph);
                float itemH = (ph > 0 ? ph : item.minHeight) + item.marginTop + item.marginBottom;
                if (itemH > h) h = itemH;
            }
        } else {
            for (const auto& item : m_items) {
                float pw = 0, ph = 0;
                if (item.component) item.component->getPreferredSize(pw, ph);
                h += (ph > 0 ? ph : item.minHeight) + item.marginTop + item.marginBottom;
            }
        }
        return h + m_paddingTop + m_paddingBottom;
    }

    /**
     * @brief Calculates the total preferred size of all items in the current direction.
     */
    void getPreferredSize(float& w, float& h) const {
        w = 0; h = 0;
        bool isRow = (m_direction == Direction::Row);
        
        for (const auto& item : m_items) {
            float itemW = item.minWidth;
            float itemH = item.minHeight;
            
            // If it wraps a component, ensure we have its latest preference
            if (item.component) {
                item.component->getPreferredSize(itemW, itemH);
            }
            
            float totalItemW = itemW + item.marginLeft + item.marginRight;
            float totalItemH = itemH + item.marginTop + item.marginBottom;
            
            if (isRow) {
                w += totalItemW;
                h = (std::max)(h, totalItemH);
            } else {
                h += totalItemH;
                w = (std::max)(w, totalItemW);
            }
        }
        
        w += m_paddingLeft + m_paddingRight;
        h += m_paddingTop + m_paddingBottom;
    }

    void performLayout(Rect bounds) {
        // if (m_items.empty()) return; // Don't return early, might need to clear bounds? No, just render nothing.

        // Apply Padding
        Rect innerBounds = {
            bounds.x + m_paddingLeft,
            bounds.y + m_paddingTop,
            bounds.w - (m_paddingLeft + m_paddingRight),
            bounds.h - (m_paddingTop + m_paddingBottom)
        };

        if (innerBounds.w <= 0 || innerBounds.h <= 0) return;

        bool isRow = (m_direction == Direction::Row);
        float mainSize = isRow ? innerBounds.w : innerBounds.h;
        float crossSizeTotal = isRow ? innerBounds.h : innerBounds.w;
        
        // 1. Group items into lines (if wrap is enabled)
        struct Line {
            std::vector<size_t> itemIndices;
            float totalBasis = 0.0f;
            float totalGrow = 0.0f;
            float totalShrink = 0.0f;
            float crossSize = 0.0f;
        };

        std::vector<Line> lines;
        lines.push_back(Line());

        for (size_t i = 0; i < m_items.size(); ++i) {
            auto& item = m_items[i];
            
            // IMPORTANT: Query fresh preferred size from component (not stale LayoutItem values)
            float itemMainBasis = item.flexBasis;
            float itemCrossMin = isRow ? item.minHeight : item.minWidth;
            if (item.component) {
                float pw = 0, ph = 0;
                item.component->getPreferredSize(pw, ph);
                // Use the larger of specified basis or preferred size
                if (isRow) {
                    if (pw > itemMainBasis) itemMainBasis = pw;
                    if (ph > itemCrossMin) itemCrossMin = ph;
                } else {
                    if (ph > itemMainBasis) itemMainBasis = ph;
                    if (pw > itemCrossMin) itemCrossMin = pw;
                }
            }
            
            float itemMainMargin = isRow ? (item.marginLeft + item.marginRight) : (item.marginTop + item.marginBottom);
            float itemBasis = itemMainBasis + itemMainMargin;
            
            // Wrap logic
            if (m_flexWrap && lines.back().totalBasis + itemBasis > mainSize && !lines.back().itemIndices.empty()) {
                lines.push_back(Line());
            }
            
            Line& currentLine = lines.back();
            currentLine.itemIndices.push_back(i);
            currentLine.totalBasis += itemBasis;
            if (item.flexGrow > 0) currentLine.totalGrow += item.flexGrow;
            if (item.flexShrink > 0) currentLine.totalShrink += item.flexShrink;
            
            float itemCrossMargin = isRow ? (item.marginTop + item.marginBottom) : (item.marginLeft + item.marginRight);
            float itemCrossSize = itemCrossMin + itemCrossMargin;
            
            if (itemCrossSize > currentLine.crossSize) currentLine.crossSize = itemCrossSize;
        }

        // 1.5 Fix Line Cross Size if only one line and Stretch is enabled, 
        // force it to fill container if the calculated size is 0 or small.
        // This simulates 'align-content: stretch' which is default for single line in some implementations
        // or just 'height: 100%' behavior. 
        if (lines.size() == 1) {
             lines[0].crossSize = (std::max)(lines[0].crossSize, crossSizeTotal);
        }

        // 2. Layout each line
        float currentCrossPos = isRow ? innerBounds.y : innerBounds.x;

        for (auto& line : lines) {
            float lineFreeSpace = mainSize - line.totalBasis;
            float justifyOffset = 0.0f;
            float spacing = 0.0f;

            // Justification (Main Axis)
            if (line.totalGrow == 0.0f && lineFreeSpace > 0.0f) {
                switch (m_justifyContent) {
                    case JustifyContent::Center: justifyOffset = lineFreeSpace / 2.0f; break;
                    case JustifyContent::FlexEnd: justifyOffset = lineFreeSpace; break;
                    case JustifyContent::SpaceBetween: 
                        if (line.itemIndices.size() > 1) spacing = lineFreeSpace / (line.itemIndices.size() - 1); 
                        break;
                    case JustifyContent::SpaceAround: 
                        spacing = lineFreeSpace / line.itemIndices.size();
                        justifyOffset = spacing / 2.0f;
                        break;
                    case JustifyContent::SpaceEvenly:
                        spacing = lineFreeSpace / (line.itemIndices.size() + 1);
                        justifyOffset = spacing;
                        break;
                    default: break;
                }
            }

            float currentMainPos = (isRow ? innerBounds.x : innerBounds.y) + justifyOffset;

            for (size_t idx : line.itemIndices) {
                auto& item = m_items[idx];
                float itemMainSize = item.flexBasis;
                
                // Apply flex grow/shrink
                if (lineFreeSpace > 0 && line.totalGrow > 0) {
                    itemMainSize += lineFreeSpace * (item.flexGrow / line.totalGrow);
                } else if (lineFreeSpace < 0 && line.totalShrink > 0) {
                    itemMainSize -= (-lineFreeSpace) * (item.flexShrink / line.totalShrink);
                }
                itemMainSize = (std::max)(0.0f, itemMainSize);
                
                // Query fresh preferred size for cross-axis
                float itemCrossSize = isRow ? item.minHeight : item.minWidth;
                if (item.component) {
                    float pw = 0, ph = 0;
                    item.component->getPreferredSize(pw, ph);
                    if (isRow && ph > itemCrossSize) itemCrossSize = ph;
                    if (!isRow && pw > itemCrossSize) itemCrossSize = pw;
                }
                
                float crossOffset = 0.0f;
                float availableCross = line.crossSize - (isRow ? (item.marginTop + item.marginBottom) : (item.marginLeft + item.marginRight));

                if (m_alignItems == AlignItems::Stretch) {
                    itemCrossSize = (std::max)(itemCrossSize, availableCross);
                    if (isRow) itemCrossSize = (std::min)(itemCrossSize, item.maxHeight);
                    else       itemCrossSize = (std::min)(itemCrossSize, item.maxWidth);
                } else {
                    if (itemCrossSize <= 0) itemCrossSize = availableCross;
                    
                    switch (m_alignItems) {
                        case AlignItems::Center: crossOffset = (availableCross - itemCrossSize) / 2.0f; break;
                        case AlignItems::FlexEnd: crossOffset = availableCross - itemCrossSize; break;
                        default: break;
                    }
                }

                if (isRow) {
                    float x = std::round(currentMainPos + item.marginLeft);
                    float y = std::round(currentCrossPos + item.marginTop + crossOffset);
                    float w = std::round(itemMainSize);
                    float h = std::round(itemCrossSize);
                    if (item.component) item.component->setBounds(x, y, w, h);
                    currentMainPos += itemMainSize + item.marginLeft + item.marginRight + spacing;
                } else {
                    float x = std::round(currentCrossPos + item.marginLeft + crossOffset);
                    float y = std::round(currentMainPos + item.marginTop);
                    float w = std::round(itemCrossSize);
                    float h = std::round(itemMainSize);
                    if (item.component) item.component->setBounds(x, y, w, h);
                    currentMainPos += itemMainSize + item.marginTop + item.marginBottom + spacing;
                }
            }
            currentCrossPos += line.crossSize;
        }
    }

private:
    Direction m_direction = Direction::Row;
    JustifyContent m_justifyContent = JustifyContent::FlexStart;
    AlignItems m_alignItems = AlignItems::Stretch;
    bool m_flexWrap = false;
    std::vector<LayoutItem> m_items;
    float m_paddingLeft = 0, m_paddingRight = 0, m_paddingTop = 0, m_paddingBottom = 0;
};

} // namespace Beam

#endif // LAYOUT_HPP