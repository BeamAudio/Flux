#ifndef AUTOTUNE_EDITOR_HPP
#define AUTOTUNE_EDITOR_HPP

#include "interface/core/ui_toolkit.hpp"
#include "interface/core/auto_flex_container.hpp"
#include "interface/widgets/combo_box.hpp"
#include "engine/core/flux_node.hpp"
#include <SDL3/SDL.h>

namespace Beam {

/**
 * @class PitchDisplay
 * @brief Circular chromatic display for AutoTune.
 */
class PitchDisplay : public Component {
public:
    PitchDisplay(FluxNode* node) : m_node(node) {}
    void paint(QuadBatcher& g) override;
private:
    FluxNode* m_node;
};

/**
 * @class AutoTuneEditor
 * @brief Professional Hardware-style UI for the AutoTuneNode.
 */
class AutoTuneEditor : public Component {
public:
    AutoTuneEditor(FluxNode* node);
    void getPreferredSize(float& w, float& h) const override { w = 360.0f; h = 320.0f; }
    void resized() override;
    void paint(QuadBatcher& g) override;

private:
    FluxNode* m_node;
    std::shared_ptr<PitchDisplay> m_pitchDisplay;
    std::shared_ptr<AutoFlexContainer> m_mainControls;
    std::shared_ptr<ComboBox> m_keyCombo;
    std::shared_ptr<ComboBox> m_scaleCombo;
    std::shared_ptr<Knob> m_retuneKnob;
    std::shared_ptr<Knob> m_humanizeKnob;
    std::shared_ptr<Slider> m_transposeSlider;
};

} // namespace Beam

#endif
