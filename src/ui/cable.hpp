#ifndef CABLE_HPP
#define CABLE_HPP

#include "port.hpp"

namespace Beam {

struct Cable {
    Port* output;
    Port* input;

    void render(QuadBatcher& batcher) {
        Rect outPos = output->getBounds();
        Rect inPos = input->getBounds();

        float x1 = outPos.x + outPos.w / 2;
        float y1 = outPos.y + outPos.h / 2;
        float x2 = inPos.x + inPos.w / 2;
        float y2 = inPos.y + inPos.h / 2;

        // Draw segments to simulate a curve (Proper Bezier shader coming next)
        int segments = 10;
        for (int i = 0; i < segments; ++i) {
            float t = (float)i / segments;
            float nt = (float)(i + 1) / segments;
            
            float px = x1 + (x2 - x1) * t;
            float py = y1 + (y2 - y1) * t;
            
            batcher.drawQuad(px, py, 4, 4, 0.25f, 0.5f, 1.0f, 0.8f);
        }
    }
};

} // namespace Beam

#endif // CABLE_HPP
