#ifndef ANALYTIC_GRAPH_HPP
#define ANALYTIC_GRAPH_HPP

#include "interface/core/component.hpp"
#include "interface/core/theme.hpp"
#include <vector>
#include <functional>

namespace Beam {

/**
 * @class AnalyticGraph
 * @brief Base component for rendering curves and data visualizations.
 */
class AnalyticGraph : public Component {
public:
    struct GridConfig {
        int vLines = 4;
        int hLines = 3;
        Color color = Theme::White.withAlpha(0.1f);
    };

    AnalyticGraph() {
        setName("AnalyticGraph");
    }

    void setGridConfig(const GridConfig& config) { m_gridConfig = config; }
    void setLineColor(Color color) { m_lineColor = color; }
    void setThickness(float thickness) { m_thickness = thickness; }

    void getPreferredSize(float& w, float& h) const override {
        w = 150.0f; h = 85.0f;
    }

    /**
     * @brief Set the data points to be rendered.
     * Points should be normalized (0.0 to 1.0) for both X and Y.
     */
    void setDataPoints(const std::vector<std::pair<float, float>>& points) {
        m_points = points;
    }

    void paint(QuadBatcher& batcher) override {
        float w = m_bounds.w;
        float h = m_bounds.h;

        // 1. Background
        batcher.drawRoundedRect(0, 0, w, h, 3.0f, 1.0f, 0.02f, 0.02f, 0.03f, 1.0f);

        // 2. Grid
        if (m_gridConfig.vLines > 0) {
            float step = w / (m_gridConfig.vLines + 1);
            for (int i = 1; i <= m_gridConfig.vLines; ++i) {
                batcher.drawQuad(i * step, 0, 1, h, m_gridConfig.color.r, m_gridConfig.color.g, m_gridConfig.color.b, m_gridConfig.color.a);
            }
        }
        if (m_gridConfig.hLines > 0) {
            float step = h / (m_gridConfig.hLines + 1);
            for (int i = 1; i <= m_gridConfig.hLines; ++i) {
                batcher.drawQuad(0, i * step, w, 1, m_gridConfig.color.r, m_gridConfig.color.g, m_gridConfig.color.b, m_gridConfig.color.a);
            }
        }

        // 3. Curve
        if (m_points.size() >= 2) {
            std::vector<std::pair<float, float>> screenPoints;
            screenPoints.reserve(m_points.size());
            for (const auto& p : m_points) {
                screenPoints.push_back({ p.first * w, (1.0f - p.second) * h });
            }
            batcher.drawCurve(screenPoints, m_thickness, m_lineColor.r, m_lineColor.g, m_lineColor.b, m_lineColor.a);
        }
    }

protected:
    std::vector<std::pair<float, float>> m_points;
    GridConfig m_gridConfig;
    Color m_lineColor = Theme::Emerald;
    float m_thickness = 2.0f;
};

} // namespace Beam

#endif // ANALYTIC_GRAPH_HPP
