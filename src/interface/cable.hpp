#ifndef CABLE_HPP
#define CABLE_HPP

#include "port.hpp"
#include <vector>
#include <cmath>

namespace Beam {

struct Cable {
    Port* output;
    Port* input;

    void render(QuadBatcher& batcher, float dt) {
        Rect outPos = output->getBounds();
        Rect inPos = input->getBounds();

        float x1 = outPos.x + outPos.w / 2;
        float y1 = outPos.y + outPos.h / 2;
        float x2 = inPos.x + inPos.w / 2;
        float y2 = inPos.y + inPos.h / 2;

        // Use Quadratic Bezier for more "sagging" physical cable look
        // Control point is the midpoint shifted down
        float cx = (x1 + x2) * 0.5f;
        float cy = (y1 + y2) * 0.5f + std::abs(x2 - x1) * 0.2f + 20.0f;

        std::vector<std::pair<float, float>> curvePoints;
        const int segments = 24;
        for (int i = 0; i <= segments; ++i) {
            float t = (float)i / segments;
            float invT = 1.0f - t;
            
            // Bezier formula: P = (1-t)^2*P1 + 2(1-t)t*C + t^2*P2
            float px = invT * invT * x1 + 2.0f * invT * t * cx + t * t * x2;
            float py = invT * invT * y1 + 2.0f * invT * t * cy + t * t * y2;
            curvePoints.push_back({px, py});
        }

        // Draw shadow/depth
        batcher.drawCurve(curvePoints, 6.0f, 0.05f, 0.05f, 0.05f, 0.4f);
        // Draw cable core
        batcher.drawCurve(curvePoints, 3.0f, 0.25f, 0.55f, 1.0f, 1.0f);
        // Draw highlights
        batcher.drawCurve(curvePoints, 1.0f, 0.5f, 0.8f, 1.0f, 0.7f);
    }
};

} // namespace Beam

#endif // CABLE_HPP



