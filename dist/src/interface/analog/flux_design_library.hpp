#ifndef FLUX_DESIGN_LIBRARY_HPP
#define FLUX_DESIGN_LIBRARY_HPP

#include "interface/analog/analog_ui_templates.hpp"
#include <map>
#include <string>

namespace Beam {

/**
 * @class FluxDesignLibrary
 * @brief Central registry for hardware UI presets and modular design tokens.
 */
class FluxDesignLibrary {
public:
    static RackStyle getStyle(const std::string& name) {
        if (name == "Pultec") return RackStyle::Pultec();
        if (name == "SSL") return RackStyle::SSL();
        if (name == "API") return RackStyle::API();
        if (name == "FET") return RackStyle::FET();
        if (name == "Aluminum") return RackStyle::Aluminum();
        return RackStyle::Utility(name);
    }

    static Theme::MaterialType getMaterial(const std::string& name) {
        if (name == "BrushedAluminum") return Theme::MaterialType::BrushedAluminum;
        if (name == "Bakelite") return Theme::MaterialType::Bakelite;
        if (name == "WrinklePaint") return Theme::MaterialType::WrinklePaint;
        return Theme::MaterialType::Standard;
    }

    static Theme::KnobStyle getKnobStyle(const std::string& name) {
        if (name == "Bakelite") return Theme::KnobStyle::ClassicBakelite;
        if (name == "ColoredCap") return Theme::KnobStyle::ModernColored;
        if (name == "Aluminum") return Theme::KnobStyle::BrushedAluminum;
        if (name == "Industrial") return Theme::KnobStyle::FlutedIndustrial;
        return Theme::KnobStyle::ClassicBakelite;
    }
};

} // namespace Beam

#endif // FLUX_DESIGN_LIBRARY_HPP
