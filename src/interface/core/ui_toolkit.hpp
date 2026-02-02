#ifndef UI_TOOLKIT_HPP
#define UI_TOOLKIT_HPP

// Core
#include "interface/core/component.hpp"
#include "interface/core/layout.hpp"
#include "interface/core/look_and_feel.hpp"
#include "interface/core/text_element.hpp"

// Widgets
#include "interface/widgets/button.hpp"
#include "interface/widgets/knob.hpp"
#include "interface/widgets/slider.hpp"
#include "interface/widgets/meter.hpp"
#include "interface/widgets/vu_meter.hpp"
#include "interface/widgets/label.hpp"
#include "interface/widgets/combo_box.hpp"

// Layout Helpers
namespace Beam {
namespace UI {

    /**
     * @brief Creates a standard parameter row with a label and a knob/slider.
     * @param parent The parent component to add to.
     * @param name The label text.
     * @param control The control component (Slider, Knob).
     * @return A LayoutItem for the container.
     */
    inline LayoutItem createParameterRow(const std::string& name, std::shared_ptr<Component> control) {
        // This is a conceptual helper - in practice, users would compose this using FlexBox directly.
        // But we can verify includes here.
        return LayoutItem(control.get()); 
    }

}
}

#endif // UI_TOOLKIT_HPP
