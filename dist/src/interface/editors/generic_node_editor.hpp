#ifndef GENERIC_NODE_EDITOR_HPP
#define GENERIC_NODE_EDITOR_HPP

#include "interface/core/auto_flex_container.hpp"
#include "interface/core/text_element.hpp"
#include "interface/core/ui_toolkit.hpp"
#include "interface/widgets/meter.hpp"
#include "engine/core/flux_node.hpp"

namespace Beam {

/**
 * @class ParameterRow
 * @brief Helper component for GenericNodeEditor. Left: Label, Right: Control.
 */
class ParameterRow : public Component {
public:
    ParameterRow(const std::string& name, std::shared_ptr<Parameter> param) {
        setName("ParamRow_" + name);
        
        AutoFlexContainer::Config cfg;
        cfg.direction = AutoFlexContainer::Direction::Row;
        cfg.crossAlign = AutoFlexContainer::Alignment::Center;
        cfg.padding = 4.0f; // More internal padding
        cfg.gap = 10.0f;    // More space between label and slider
        cfg.wrap = false;
        
        m_flexContainer = std::make_shared<AutoFlexContainer>(cfg);
        addChildComponent(m_flexContainer);
        
        m_textElement = std::make_shared<TextElement>(name);
        m_textElement->setColor(0.85f, 0.85f, 0.85f, 1.0f); 
        m_textElement->setFontSize(9.5f);
        m_textElement->setJustification(TextElement::Justification::Left);
        
        m_slider = std::make_shared<Slider>();
        m_slider->setParameter(param);
        
        m_flexContainer->addFlexChild(m_textElement);
        m_flexContainer->addFlexChild(m_slider, 1.0f); // Make slider grow
    }

    void resized() override {
        m_flexContainer->setBounds(0, 0, m_bounds.w, m_bounds.h);
        m_flexContainer->resized();
    }

    void getPreferredSize(float& w, float& h) const override {
        float lw = 0, lh = 0;
        // Hint the text element about the typical width it will have
        m_textElement->setWrapWidth(100.0f); 
        m_textElement->getPreferredSize(lw, lh);
        
        w = (std::max)(lw + 120.0f, 180.0f); 
        h = (std::max)(28.0f, lh + 4.0f); // Dynamic height to fit labels
    }

    std::shared_ptr<TextElement> getTextElement() { return m_textElement; }
    std::shared_ptr<Slider> getSlider() { return m_slider; }

private:
    std::shared_ptr<AutoFlexContainer> m_flexContainer;
    std::shared_ptr<TextElement> m_textElement;
    std::shared_ptr<Slider> m_slider;
};

class GenericNodeEditor : public Component {
public:
    GenericNodeEditor(FluxNode* node) : m_node(node) {
        m_nodeName = node->getName();
        setName("GenericEditor_" + m_nodeName);
        setClipsChildren(true);
        
        AutoFlexContainer::Config cfg;
        cfg.direction = AutoFlexContainer::Direction::Column;
        cfg.padding = 10.0f; 
        cfg.gap = 8.0f;     
        cfg.wrap = false; 
        
        m_flexContainer = std::make_shared<AutoFlexContainer>(cfg);
        addChildComponent(m_flexContainer);
        
        // Add Meter if available
        if (node->getMeterSource() && node->getMeterSource()->getNumMeters() > 0) {
            m_meter = std::make_shared<LuminousMeter>();
            m_meter->setOrientation(LuminousMeter::Orientation::Horizontal);
            // We'll wrap it in a container or just let flex handle it.
            // Since LuminousMeter defaults to vertical, we set Horizontal.
            // But LuminousMeter might need fixed height.
            
            // Hack: Create a wrapper component for the meter to give it size in Flex
            auto meterContainer = std::make_shared<Component>();
            meterContainer->setName("MeterContainer");
            meterContainer->addChildComponent(m_meter);
            
            // We need a layout item that performs layout on the meter
            // But for now, let's just add the meter directly and override its getPreferredSize if possible,
            // or rely on FlexBox item config.
            
            // Actually, LuminousMeter doesn't implement getPreferredSize probably.
            // Let's manually add it to flex with a fixed height item.
            
            m_flexContainer->addFlexChild(m_meter, 0.0f); // No grow
            // We need to ensure the meter gets bounds in resized().
            // AutoFlexContainer handles that if we add it.
        }

        buildUI(node->getParameterOrder());
    }

    void buildUI(const std::vector<std::shared_ptr<Parameter>>& parameters) {
        for (const auto& param : parameters) {
            auto row = std::make_shared<ParameterRow>(param->getName(), param);
            m_flexContainer->addFlexChild(row);
            m_rows.push_back(row);
        }
    }

    void update(float dt) override {
        if (m_node && m_meter && m_node->getMeterSource()) {
            // Simple stereo/mono sum or just first channel
            float v = m_node->getMeterSource()->getValue(0);
            if (m_node->getMeterSource()->getNumMeters() > 1) {
                v = (std::max)(v, m_node->getMeterSource()->getValue(1));
            }
            m_meter->setLevel(v);
        }
        Component::update(dt);
    }

    void resized() override {
        // Adjust container to sit inside rack ears
        float earWidth = 12.0f;
        m_flexContainer->setBounds(earWidth, 0, m_bounds.w - 2 * earWidth, m_bounds.h);
        
        // Force meter height if present
        if (m_meter) {
            // Find the item for the meter and set fixed height?
            // AutoFlexContainer doesn't expose items easily.
            // But we know it's the first child.
            // Actually, AutoFlexContainer::resized() calls getPreferredSize on children.
            // We should wrap Meter in a class that provides size.
        }
        
        m_flexContainer->resized();
        
        // If we didn't wrap meter, we might need to manually set its height if FlexBox collapsed it.
        // But LuminousMeter might not have a size.
        // Let's assume AutoFlexContainer stretches width.
        if (m_meter) {
             // Hack: LuminousMeter might need explicit bounds update if FlexContainer didn't do it right
             // But FlexContainer calls setBounds.
             // We just need to ensure it has a height.
             // Let's enforce it in paint or update? No.
             // Let's modify GenericNodeEditor to use a layout wrapper for meter.
        }
    }

    void getPreferredSize(float& w, float& h) const override {
        m_flexContainer->getPreferredSize(w, h);
        if (m_meter) h += 15.0f; // Add room for meter if Flex didn't count it well
        w = (std::max)(w + 24.0f, 250.0f); // Add room for ears
    }

    void paint(QuadBatcher& batcher) override {
        // Generate consistent color from name
        size_t seed = 0;
        for(char c : m_nodeName) seed = seed * 101 + c;
        
        // Palettes: 0=BlueGrey, 1=DarkRed, 2=Olive, 3=DeepBlue, 4=Slate
        float r=0.1f, g=0.1f, b=0.1f;
        switch(seed % 5) {
            case 0: r=0.15f; g=0.17f; b=0.20f; break; // Slate
            case 1: r=0.20f; g=0.10f; b=0.10f; break; // Red
            case 2: r=0.15f; g=0.18f; b=0.12f; break; // Olive
            case 3: r=0.10f; g=0.12f; b=0.22f; break; // Blue
            case 4: r=0.12f; g=0.12f; b=0.12f; break; // Charcoal
        }

        // Main Chassis
        batcher.drawBeveledRect(0, 0, m_bounds.w, m_bounds.h, 2.0f, 0.5f, r, g, b, 1.0f);
        
        // Texture/Noise overlay
        batcher.drawChassisPanel(0, 0, m_bounds.w, m_bounds.h, 0, 0, 0, 0, 0.1f);

        // Rack Ears
        float earW = 12.0f;
        Color earCol = Theme::Bakelite.darker(0.2f);
        batcher.drawQuad(0, 0, earW, m_bounds.h, earCol.r, earCol.g, earCol.b, 1.0f);
        batcher.drawQuad(m_bounds.w - earW, 0, earW, m_bounds.h, earCol.r, earCol.g, earCol.b, 1.0f);
        
        // Mounting Holes
        auto drawHole = [&](float x, float y) {
             batcher.drawRoundedRect(x, y, 6, 8, 3.0f, 0.5f, 0.05f, 0.05f, 0.05f, 1.0f);
             batcher.drawRoundedRect(x+1, y+1, 4, 6, 2.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.8f);
        };
        drawHole(3, 10);
        drawHole(m_bounds.w - 9, 10);
        drawHole(3, m_bounds.h - 18);
        drawHole(m_bounds.w - 9, m_bounds.h - 18);

        // Top/Bottom Vent Grills
        if (m_bounds.h > 100) {
            float ventY = 5.0f;
            float ventW = m_bounds.w - 2 * earW - 20.0f;
            float ventX = earW + 10.0f;
            for(int i=0; i<3; ++i) {
                batcher.drawQuad(ventX, ventY + i*3, ventW, 1, 0.0f, 0.0f, 0.0f, 0.3f);
            }
        }
    }

private:
    FluxNode* m_node;
    std::string m_nodeName;
    std::shared_ptr<AutoFlexContainer> m_flexContainer;
    std::vector<std::shared_ptr<ParameterRow>> m_rows;
    std::shared_ptr<LuminousMeter> m_meter;
};

} // namespace Beam

#endif // GENERIC_NODE_EDITOR_HPP