#ifndef CRT_LOUDNESS_METER_HPP
#define CRT_LOUDNESS_METER_HPP

#include "interface/core/component.hpp"
#include "interface/core/theme.hpp"
#include <vector>
#include <string>
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace Beam {

class CRTLoudnessMeter : public Component {
public:
    CRTLoudnessMeter() {
        setName("CRTLoudnessMeter");
    }

    void setUpdate(float momentary, float shortTerm, float truePeak) {
        m_momentary = momentary;
        m_shortTerm = shortTerm;
        m_truePeak = truePeak;
    }
    
    void paint(QuadBatcher& batcher) override {
        float w = m_bounds.w;
        float h = m_bounds.h;

        batcher.drawRoundedRect(0, 0, w, h, 6.0f, 0.5f, 0.05f, 0.06f, 0.06f, 1.0f);
        
        drawGraticule(batcher, w, h);
        
        float pad = 20.0f;
        float barW = (w - pad * 4) / 3.0f;
        float x = pad;
        
        drawBar(batcher, "M", m_momentary, x, barW, h);
        x += barW + pad;
        
        drawBar(batcher, "S", m_shortTerm, x, barW, h);
        x += barW + pad;
        
        drawBar(batcher, "TP", m_truePeak, x, barW, h, true);

        for (float y = 0; y < h; y += 3.0f) {
            batcher.drawQuad(0, y, w, 1, 0.0f, 0.0f, 0.0f, 0.2f);
        }
        
        batcher.drawRoundedGradientRect(0, 0, w, h, 6.0f, 0.5f,
                                       1.0f, 1.0f, 1.0f, 0.05f,
                                       1.0f, 1.0f, 1.0f, 0.0f);
    }

private:
    float m_momentary = -60.0f;
    float m_shortTerm = -60.0f;
    float m_truePeak = -60.0f;
    
    void drawGraticule(QuadBatcher& batcher, float w, float h) {
        float dbs[] = {0.0f, -6.0f, -12.0f, -18.0f, -24.0f, -48.0f};
        for (float db : dbs) {
            float minDb = -60.0f;
            float maxDb = 6.0f; 
            
            float norm = (db - minDb) / (maxDb - minDb);
            float y = h - (norm * h);
            
            if (y >= 0 && y <= h) {
                batcher.drawQuad(0, y, w, 1, 0.4f, 0.6f, 0.5f, 0.3f);
                std::stringstream ss;
                ss << (int)db;
                batcher.drawText(ss.str(), 2, y - 8, 9, 0.4f, 0.6f, 0.5f, 0.8f);
            }
        }
        
        float targetY = h - ((-14.0f + 60.0f) / 66.0f * h);
        batcher.drawQuad(0, targetY, w, 2, 0.2f, 0.9f, 0.3f, 0.8f);
    }
    
    void drawBar(QuadBatcher& batcher, const std::string& label, float val, float x, float w, float h, bool isTP = false) {
        float minDb = -60.0f;
        float maxDb = 6.0f;
        
        float norm = (val - minDb) / (maxDb - minDb);
        norm = std::clamp(norm, 0.0f, 1.0f);
        
        float barH = norm * h;
        float y = h - barH;
        
        batcher.drawRoundedRect(x, 0, w, h, 2.0f, 0.5f, 0.1f, 0.12f, 0.12f, 1.0f);
        
        float r=0.0f, g=0.0f, b=0.0f;
        if (val > 0.0f || (isTP && val > -1.0f)) { 
            r=1.0f; g=0.2f; b=0.2f; 
        } else if (val > -14.0f) {
            r=1.0f; g=0.9f; b=0.0f; 
        } else {
            r=0.2f; g=0.9f; b=0.4f; 
        }
        
        batcher.drawRoundedRect(x, y, w, barH, 2.0f, 0.5f, r, g, b, 0.8f);
        batcher.drawRoundedRect(x + 2, y, w - 4, barH, 1.0f, 0.5f, r*1.2f, g*1.2f, b*1.2f, 1.0f);
        
        batcher.drawText(label, x + w/2 - 6, h - 20, 12, 1.0f, 1.0f, 1.0f, 0.8f);
        
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << val;
        batcher.drawText(ss.str(), x + 2, h - barH - 14, 10, 1.0f, 1.0f, 1.0f, 1.0f);
    }
};

} // namespace Beam

#endif // CRT_LOUDNESS_METER_HPP
