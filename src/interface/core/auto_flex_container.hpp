#ifndef AUTO_FLEX_CONTAINER_HPP
#define AUTO_FLEX_CONTAINER_HPP

#include "interface/core/component.hpp"
#include <vector>
#include <algorithm>
#include <cmath>
#include <memory>

namespace Beam {

/**
 * @class AutoFlexContainer
 * @brief A robust CSS-like flex container with proper measure/layout phases.
 */
class AutoFlexContainer : public Component {
public:
    enum class Direction { Row, Column };
    enum class Alignment { Start, Center, End, Stretch };
    enum class Justify { Start, Center, End, SpaceBetween, SpaceAround };
    
    struct Config {
        Direction direction = Direction::Row;
        Alignment crossAlign = Alignment::Start;
        Justify justify = Justify::Start;
        float gap = 8.0f;
        float padding = 10.0f;
        float preferredWidth = 300.0f;
        float preferredHeight = 300.0f;
        bool wrap = true;
    };
    
    struct FlexParams {
        float grow = 0.0f;
    };
    
    AutoFlexContainer(Config config = {}) : m_config(config) {
        setName("AutoFlexContainer");
    }
    
    void addFlexChild(std::shared_ptr<Component> child, float grow = 0.0f) {
        m_flexChildren.push_back({child, {grow}});
        addChildComponent(child);
    }
    
    void clearFlexChildren() {
        for (auto& c : m_flexChildren) {
            removeChildComponent(c.comp.get());
        }
        m_flexChildren.clear();
    }

    struct Line {
        std::vector<int> itemIndices;
        float mainSize = 0;
        float crossSize = 0;
        float totalGrow = 0;
    };

    std::vector<Line> computeLines(float targetW, float targetH) const {
        std::vector<Line> lines;
        if (m_flexChildren.empty()) return lines;

        bool isRow = m_config.direction == Direction::Row;
        float maxMain = (isRow ? targetW : targetH) - m_config.padding * 2;
        
        lines.push_back(Line());
        float currentMain = 0;

        for (int i = 0; i < (int)m_flexChildren.size(); ++i) {
            float cw = 0, ch = 0;
            m_flexChildren[i].comp->getPreferredSize(cw, ch);
            
            float itemMain = isRow ? cw : ch;
            float itemCross = isRow ? ch : cw;

            if (m_config.wrap && !lines.back().itemIndices.empty() && currentMain + itemMain > maxMain) {
                lines.push_back(Line());
                currentMain = 0;
            }

            lines.back().itemIndices.push_back(i);
            lines.back().mainSize = currentMain + itemMain;
            if (itemCross > lines.back().crossSize) lines.back().crossSize = itemCross;
            lines.back().totalGrow += m_flexChildren[i].params.grow;
            
            currentMain += itemMain + m_config.gap;
        }
        return lines;
    }
    
    void getPreferredSize(float& w, float& h) const override {
        if (m_flexChildren.empty()) { w = 0; h = 0; return; }
        
        auto lines = computeLines(m_config.preferredWidth, m_config.preferredHeight);
        bool isRow = m_config.direction == Direction::Row;
        
        float totalMainAcrossLines = 0;
        float maxLineMain = 0;
        
        for (const auto& line : lines) {
            if (line.mainSize > maxLineMain) maxLineMain = line.mainSize;
            totalMainAcrossLines += line.crossSize + m_config.gap;
        }
        if (!lines.empty()) totalMainAcrossLines -= m_config.gap;
        
        if (isRow) {
            w = maxLineMain + m_config.padding * 2;
            h = totalMainAcrossLines + m_config.padding * 2;
        } else {
            h = maxLineMain + m_config.padding * 2;
            w = totalMainAcrossLines + m_config.padding * 2;
        }
    }
    
    void resized() override {
        if (m_flexChildren.empty()) return;
        
        bool isRow = m_config.direction == Direction::Row;
        float availMain = (isRow ? m_bounds.w : m_bounds.h) - m_config.padding * 2;
        
        auto lines = computeLines(m_bounds.w, m_bounds.h);
        float currentCrossPos = m_config.padding;
        
        for (const auto& line : lines) {
            float currentMainPos = m_config.padding;
            float freeSpace = availMain - line.mainSize;
            
            float xOffset = 0;
            float extraGap = 0;
            if (line.totalGrow <= 0) {
                if (m_config.justify == Justify::Center) xOffset = freeSpace / 2;
                else if (m_config.justify == Justify::End) xOffset = freeSpace;
                else if (m_config.justify == Justify::SpaceBetween && line.itemIndices.size() > 1) {
                    extraGap = freeSpace / (line.itemIndices.size() - 1);
                }
            }
            
            currentMainPos += xOffset;
            
            for (int idx : line.itemIndices) {
                float cw = 0, ch = 0;
                m_flexChildren[idx].comp->getPreferredSize(cw, ch);
                
                float itemMain = isRow ? cw : ch;
                float itemCross = isRow ? ch : cw;
                
                if (line.totalGrow > 0 && m_flexChildren[idx].params.grow > 0) {
                    float extra = freeSpace * (m_flexChildren[idx].params.grow / line.totalGrow);
                    // Prevent total collapse, especially for sliders
                    itemMain = (std::max)(20.0f, itemMain + extra);
                }
                
                float crossOffset = 0;
                float finalCrossSize = itemCross;
                if (m_config.crossAlign == Alignment::Center) crossOffset = (line.crossSize - itemCross) / 2;
                else if (m_config.crossAlign == Alignment::End) crossOffset = line.crossSize - itemCross;
                else if (m_config.crossAlign == Alignment::Stretch) finalCrossSize = line.crossSize;
                
                if (isRow) {
                    m_flexChildren[idx].comp->setBounds(currentMainPos, currentCrossPos + crossOffset, itemMain, finalCrossSize);
                } else {
                    m_flexChildren[idx].comp->setBounds(currentCrossPos + crossOffset, currentMainPos, finalCrossSize, itemMain);
                }
                
                currentMainPos += itemMain + m_config.gap + extraGap;
            }
            currentCrossPos += line.crossSize + m_config.gap;
        }
    }
    
    void paint(QuadBatcher& batcher) override {}

private:
    struct ChildEntry {
        std::shared_ptr<Component> comp;
        FlexParams params;
    };
    Config m_config;
    std::vector<ChildEntry> m_flexChildren;
};

} // namespace Beam

#endif // AUTO_FLEX_CONTAINER_HPP
