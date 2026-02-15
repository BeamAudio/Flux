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
        float shrink = 1.0f;
        float basis = -1.0f; // -1 means use preferred size
    };
    
    AutoFlexContainer(Config config = {}) : m_config(config) {
        setName("AutoFlexContainer");
    }
    
    Config& getConfig() { return m_config; }
    void setConfig(Config cfg) { m_config = cfg; invalidateLayout(); }

    void addFlexChild(std::shared_ptr<Component> child, float grow = 0.0f, float shrink = 1.0f, float basis = -1.0f) {
        m_flexChildren.push_back({child, {grow, shrink, basis}});
        addChildComponent(child);
        invalidateLayout();
    }
    
    void clearFlexChildren() {
        for (auto& c : m_flexChildren) {
            removeChildComponent(c.comp.get());
        }
        m_flexChildren.clear();
        invalidateLayout();
    }

    void invalidateLayout() { m_layoutValid = false; }

    struct Line {
        std::vector<int> itemIndices;
        float mainSize = 0;
        float crossSize = 0;
        float totalGrow = 0;
        float totalShrink = 0;
    };

    std::vector<Line> computeLines(float targetW, float targetH) const {
        if (m_layoutValid && std::abs(m_lastTargetW - targetW) < 0.1f && std::abs(m_lastTargetH - targetH) < 0.1f) {
            return m_cachedLines;
        }

        std::vector<Line> lines;
        if (m_flexChildren.empty()) return lines;

        bool isRow = m_config.direction == Direction::Row;
        float targetMain = isRow ? targetW : targetH;
        float maxMain = m_config.wrap ? (std::max)(1.0f, targetMain - m_config.padding * 2) : 1e10f;
        
        lines.push_back(Line());
        float currentMain = 0;

        for (int i = 0; i < (int)m_flexChildren.size(); ++i) {
            float cw = 0, ch = 0;
            m_flexChildren[i].comp->getPreferredSize(cw, ch);
            
            float itemMain = isRow ? cw : ch;
            if (m_flexChildren[i].params.basis >= 0) itemMain = m_flexChildren[i].params.basis;
            
            // Respect component min/max constraints
            float minM = isRow ? m_flexChildren[i].comp->getMinWidth() : m_flexChildren[i].comp->getMinHeight();
            if (minM >= 0) itemMain = (std::max)(itemMain, minM);

            float itemCross = isRow ? ch : cw;
            float minC = isRow ? m_flexChildren[i].comp->getMinHeight() : m_flexChildren[i].comp->getMinWidth();
            if (minC >= 0) itemCross = (std::max)(itemCross, minC);

            if (m_config.wrap && !lines.back().itemIndices.empty() && currentMain + itemMain + m_config.gap > maxMain) {
                lines.push_back(Line());
                currentMain = 0;
            }

            lines.back().itemIndices.push_back(i);
            lines.back().mainSize += itemMain;
            if (itemCross > lines.back().crossSize) lines.back().crossSize = itemCross;
            lines.back().totalGrow += m_flexChildren[i].params.grow;
            lines.back().totalShrink += m_flexChildren[i].params.shrink;
            
            currentMain += itemMain + m_config.gap;
        }
        
        // Add gaps to mainSize (except at end)
        for (auto& line : lines) {
            if (line.itemIndices.size() > 1) {
                line.mainSize += m_config.gap * (line.itemIndices.size() - 1);
            }
        }

        m_cachedLines = lines;
        m_lastTargetW = targetW;
        m_lastTargetH = targetH;
        m_layoutValid = true;
        return lines;
    }
    
    void getPreferredSize(float& w, float& h) const override {
        if (m_flexChildren.empty()) { w = 0; h = 0; return; }
        
        // Use current width if available to determine wrapping/height
        // Fix: Use a very large default if bounds are 0 to simulate 'max-content' or 'unbounded' measurement
        // This prevents feedback loops where bounds=0 -> small pref size -> small bounds
        float targetW = (m_bounds.w > 1.0f) ? m_bounds.w : 10000.0f;
        float targetH = (m_bounds.h > 1.0f) ? m_bounds.h : 10000.0f;

        auto lines = computeLines(targetW, targetH);
        bool isRow = m_config.direction == Direction::Row;
        
        float totalCrossAcrossLines = 0;
        float maxLineMain = 0;
        
        for (const auto& line : lines) {
            if (line.mainSize > maxLineMain) maxLineMain = line.mainSize;
            totalCrossAcrossLines += line.crossSize + m_config.gap;
        }
        if (!lines.empty()) totalCrossAcrossLines -= m_config.gap;
        
        if (isRow) {
            w = maxLineMain + m_config.padding * 2;
            h = totalCrossAcrossLines + m_config.padding * 2;
        } else {
            h = maxLineMain + m_config.padding * 2;
            w = totalCrossAcrossLines + m_config.padding * 2;
        }
    }
    
    void resized() override {
        if (m_flexChildren.empty()) return;
        
        bool isRow = m_config.direction == Direction::Row;
        float availMain = (std::max)(0.0f, (isRow ? m_bounds.w : m_bounds.h) - m_config.padding * 2);
        float availCross = (std::max)(0.0f, (isRow ? m_bounds.h : m_bounds.w) - m_config.padding * 2);
        
        auto lines = computeLines(m_bounds.w, m_bounds.h);
        float currentCrossPos = m_config.padding;
        
        for (const auto& line : lines) {
            float currentMainPos = m_config.padding;
            float diff = availMain - line.mainSize;
            
            // If stretching and only one line, ensure the line takes full cross space
            float lineCrossSize = line.crossSize;
            if (m_config.crossAlign == Alignment::Stretch && lines.size() == 1) {
                lineCrossSize = (std::max)(lineCrossSize, availCross);
            }

            float xOffset = 0;
            float extraGap = 0;
            if (diff > 0 && line.totalGrow <= 0) {
                if (m_config.justify == Justify::Center) xOffset = diff / 2;
                else if (m_config.justify == Justify::End) xOffset = diff;
                else if (m_config.justify == Justify::SpaceBetween && line.itemIndices.size() > 1) {
                    extraGap = diff / (line.itemIndices.size() - 1);
                }
            }
            
            currentMainPos += (std::max)(0.0f, xOffset);
            
            for (int idx : line.itemIndices) {
                float cw = 0, ch = 0;
                m_flexChildren[idx].comp->getPreferredSize(cw, ch);
                
                float itemMain = isRow ? cw : ch;
                if (m_flexChildren[idx].params.basis >= 0) itemMain = m_flexChildren[idx].params.basis;
                
                float minM = isRow ? m_flexChildren[idx].comp->getMinWidth() : m_flexChildren[idx].comp->getMinHeight();
                if (minM >= 0) itemMain = (std::max)(itemMain, minM);

                float itemCross = isRow ? ch : cw;
                float minC = isRow ? m_flexChildren[idx].comp->getMinHeight() : m_flexChildren[idx].comp->getMinWidth();
                if (minC >= 0) itemCross = (std::max)(itemCross, minC);

                if (diff > 0 && line.totalGrow > 0 && m_flexChildren[idx].params.grow > 0) {
                    float extra = diff * (m_flexChildren[idx].params.grow / line.totalGrow);
                    itemMain += extra;
                } else if (diff < 0 && line.totalShrink > 0 && m_flexChildren[idx].params.shrink > 0) {
                    float shrink = diff * (m_flexChildren[idx].params.shrink / line.totalShrink);
                    itemMain = (std::max)(minM >= 0 ? minM : 20.0f, itemMain + shrink); // Don't shrink below minM or 20.0f
                }
                
                float crossOffset = 0;
                float finalCrossSize = itemCross;
                if (m_config.crossAlign == Alignment::Center) crossOffset = (lineCrossSize - itemCross) / 2;
                else if (m_config.crossAlign == Alignment::End) crossOffset = lineCrossSize - itemCross;
                else if (m_config.crossAlign == Alignment::Stretch) finalCrossSize = lineCrossSize;
                
                if (isRow) {
                    m_flexChildren[idx].comp->setBounds(std::floor(currentMainPos + 0.5f), 
                                                        std::floor(currentCrossPos + crossOffset + 0.5f), 
                                                        std::floor(itemMain + 0.5f), 
                                                        std::floor(finalCrossSize + 0.5f));
                } else {
                    m_flexChildren[idx].comp->setBounds(std::floor(currentCrossPos + crossOffset + 0.5f), 
                                                        std::floor(currentMainPos + 0.5f), 
                                                        std::floor(finalCrossSize + 0.5f), 
                                                        std::floor(itemMain + 0.5f));
                }
                
                currentMainPos += itemMain + m_config.gap + extraGap;
            }
            currentCrossPos += lineCrossSize + m_config.gap;
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
    
    mutable std::vector<Line> m_cachedLines;
    mutable float m_lastTargetW = -1;
    mutable float m_lastTargetH = -1;
    mutable bool m_layoutValid = false;
};

} // namespace Beam

#endif // AUTO_FLEX_CONTAINER_HPP
