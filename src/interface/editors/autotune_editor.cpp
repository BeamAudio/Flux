#include "interface/editors/autotune_editor.hpp"
#include "engine/nodes/pitch_fx_nodes.hpp"
#include "interface/core/theme.hpp"
#include <cmath>

namespace Beam {

AutoTuneEditor::AutoTuneEditor(FluxNode* node) : m_node(node) {
    setName("AutoTuneEditor");
    
    m_pitchDisplay = std::make_shared<PitchDisplay>(node);
    addChildComponent(m_pitchDisplay);

    // 1. Controls Group
    m_mainControls = std::make_shared<AutoFlexContainer>();
    auto& cfg = m_mainControls->getConfig();
    cfg.direction = AutoFlexContainer::Direction::Column;
    cfg.padding = 15.0f;
    cfg.gap = 12.0f;
    cfg.crossAlign = AutoFlexContainer::Alignment::Center;
    addChildComponent(m_mainControls);

    // --- ROW 1: Scale Selection ---
    auto scaleRow = std::make_shared<AutoFlexContainer>();
    scaleRow->getConfig().direction = AutoFlexContainer::Direction::Row;
    scaleRow->getConfig().gap = 10.0f;
    scaleRow->getConfig().crossAlign = AutoFlexContainer::Alignment::Center;
    
    m_keyCombo = std::make_shared<ComboBox>();
    const char* keys[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    for(auto k : keys) m_keyCombo->addItem(k);
    
    auto keyParam = node->getParameter("Key");
    if (keyParam) {
        m_keyCombo->setSelectedId((int)keyParam->getValue());
        m_keyCombo->setOnChange([keyParam](int id) { keyParam->setValue((float)id); });
    }

    m_scaleCombo = std::make_shared<ComboBox>();
    m_scaleCombo->addItem("Chromatic");
    m_scaleCombo->addItem("Major");
    m_scaleCombo->addItem("Minor");
    
    auto scaleParam = node->getParameter("Scale");
    if (scaleParam) {
        m_scaleCombo->setSelectedId((int)scaleParam->getValue());
        m_scaleCombo->setOnChange([scaleParam](int id) { scaleParam->setValue((float)id); });
    }

    scaleRow->addFlexChild(std::make_shared<Label>("KEY:"));
    scaleRow->addFlexChild(m_keyCombo);
    scaleRow->addFlexChild(std::make_shared<Label>("SCALE:"));
    scaleRow->addFlexChild(m_scaleCombo);
    m_mainControls->addFlexChild(scaleRow);

    // --- ROW 2: Main Dials ---
    auto dialRow = std::make_shared<AutoFlexContainer>();
    dialRow->getConfig().direction = AutoFlexContainer::Direction::Row;
    dialRow->getConfig().gap = 25.0f;
    
    m_retuneKnob = std::make_shared<Knob>();
    auto retuneParam = node->getParameter("Retune Speed");
    if (retuneParam) m_retuneKnob->setParameter(retuneParam);
    m_retuneKnob->setLabel("RETUNE");
    m_retuneKnob->setStyle(Theme::KnobStyle::ModernColored);
    
    m_humanizeKnob = std::make_shared<Knob>();
    auto humanParam = node->getParameter("Humanize");
    if (humanParam) m_humanizeKnob->setParameter(humanParam);
    m_humanizeKnob->setLabel("HUMANIZE");
    m_humanizeKnob->setStyle(Theme::KnobStyle::BrushedAluminum);

    dialRow->addFlexChild(m_retuneKnob);
    dialRow->addFlexChild(m_humanizeKnob);
    m_mainControls->addFlexChild(dialRow);

    // --- ROW 3: Transpose ---
    auto transRow = std::make_shared<AutoFlexContainer>();
    transRow->getConfig().direction = AutoFlexContainer::Direction::Row;
    transRow->getConfig().gap = 10.0f;
    transRow->getConfig().crossAlign = AutoFlexContainer::Alignment::Center;
    
    m_transposeSlider = std::make_shared<Slider>();
    auto transParam = node->getParameter("Transpose");
    if (transParam) m_transposeSlider->setParameter(transParam);
    m_transposeSlider->setSliderStyle(SliderStyle::LinearHorizontal);
    
    transRow->addFlexChild(std::make_shared<Label>("TRANSPOSE:"));
    transRow->addFlexChild(m_transposeSlider, 1.0f);
    m_mainControls->addFlexChild(transRow);
}

void AutoTuneEditor::resized() {
    float displaySize = 140.0f;
    m_pitchDisplay->setBounds((m_bounds.w - displaySize) * 0.5f, 50, displaySize, displaySize);
    
    m_mainControls->setBounds(0, 160, m_bounds.w, m_bounds.h - 160);
    m_mainControls->resized();
}

void AutoTuneEditor::paint(QuadBatcher& g) {
    // Futuristic Carbon/Emerald Chassis
    g.drawBeveledRect(0, 0, m_bounds.w, m_bounds.h, 4.0f, 0.5f, 0.08f, 0.09f, 0.10f, 1.0f);
    
    // Circular Display Area
    float cx = m_bounds.w * 0.5f;
    float cy = 120.0f;
    float r = 65.0f;
    g.drawRoundedRect(cx - r, cy - r, r*2, r*2, r, 0.5f, 0.02f, 0.02f, 0.03f, 1.0f);
    g.drawRoundedRect(cx - r + 2, cy - r + 2, (r-2)*2, (r-2)*2, r-2, 1.0f, Theme::Emerald.r, Theme::Emerald.g, Theme::Emerald.b, 0.05f); 
}

void PitchDisplay::paint(QuadBatcher& g) {
    float cx = m_bounds.w * 0.5f;
    float cy = m_bounds.h * 0.5f;
    float r = (std::min)(cx, cy) - 15.0f;

    auto* atNode = dynamic_cast<AutoTuneNode*>(m_node);
    float detNote = atNode ? atNode->getLastDetectedNote() : -1.0f;
    float targetNote = atNode ? atNode->getLastTargetNote() : -1.0f;

    // Draw Chromatic Ring
    const char* notes[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    for (int i = 0; i < 12; ++i) {
        float angle = (float)i * (360.0f / 12.0f) - 90.0f;
        float rad = angle * 3.14159f / 180.0f;
        float nx = cx + std::cos(rad) * r;
        float ny = cy + std::sin(rad) * r;

        float alpha = 0.3f;
        float size = 9.0f;
        Color col = {0.5f, 0.5f, 0.5f, 1.0f};

        if (detNote >= 0 && std::round(detNote) == i) {
            alpha = 1.0f;
            size = 12.0f;
            col = Theme::White;
        }
        if (targetNote >= 0 && std::round(targetNote) == i) {
            col = Theme::Emerald;
            alpha = 1.0f;
        }

        g.drawText(notes[i], nx - 6, ny - 6, size, col.r, col.g, col.b, alpha);
    }

    // Central Activity Glow
    if (detNote >= 0) {
        float pulse = 0.5f + 0.5f * std::sin((float)SDL_GetTicks() * 0.01f);
        g.drawRoundedRect(cx - 15, cy - 15, 30, 30, 15, 0.5f, Theme::Emerald.r, Theme::Emerald.g, Theme::Emerald.b, 0.1f + pulse * 0.1f);
    }
}

} // namespace Beam
