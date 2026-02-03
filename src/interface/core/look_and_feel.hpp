#ifndef LOOK_AND_FEEL_HPP
#define LOOK_AND_FEEL_HPP

#include "interface/core/component.hpp"
#include <string>

namespace Beam {

class Button;
class Slider;
class Label;
class Knob;
class LuminousMeter;
class VUMeter;
class SpectrumModule;
class AudioModule;

/**
 * @class LookAndFeel
 * @brief Base class for look-and-feel strategies, similar to JUCE's LookAndFeel
 */
class LookAndFeel {
public:
    virtual ~LookAndFeel() = default;

    // Button drawing
    virtual void drawButtonBackground(QuadBatcher& g, Button& button, 
                                    bool isMouseOver, bool isButtonDown);
    virtual void drawButtonText(QuadBatcher& g, Button& button, 
                               bool isMouseOver, bool isButtonDown);

    // Slider drawing
    virtual void drawSliderBackground(QuadBatcher& g, Slider& slider, 
                                    float sliderPos, float rotaryStartAngle, float rotaryEndAngle);
    virtual void drawSliderPointer(QuadBatcher& g, Slider& slider, 
                                  float sliderPos, float rotaryStartAngle, float rotaryEndAngle);

    // Knob drawing
    virtual void drawKnob(QuadBatcher& g, Knob& knob, float sliderPos);

    // Meter drawing
    virtual void drawLuminousMeter(QuadBatcher& g, LuminousMeter& meter, float level, float peak);
    virtual void drawVUMeter(QuadBatcher& g, VUMeter& meter, float level);

    // Spectrogram / Spectrum Analyzer
    virtual void drawSpectrumAnalyzer(QuadBatcher& g, class SpectrumModule& spectrum);

    // Audio Module base frame
    virtual void drawAudioModule(QuadBatcher& g, class AudioModule& module);

};

/**
 * @class DefaultLookAndFeel
 * @brief Default implementation of LookAndFeel
 */
class DefaultLookAndFeel : public LookAndFeel {
public:
    void drawButtonBackground(QuadBatcher& g, Button& button, 
                            bool isMouseOver, bool isButtonDown) override;
    void drawButtonText(QuadBatcher& g, Button& button, 
                       bool isMouseOver, bool isButtonDown) override;

    void drawSliderBackground(QuadBatcher& g, Slider& slider, 
                            float sliderPos, float rotaryStartAngle, float rotaryEndAngle) override;
    void drawSliderPointer(QuadBatcher& g, Slider& slider, 
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle) override;

    void drawKnob(QuadBatcher& g, Knob& knob, float sliderPos) override;
    
    void drawLuminousMeter(QuadBatcher& g, LuminousMeter& meter, float level, float peak) override;
    void drawVUMeter(QuadBatcher& g, VUMeter& meter, float level) override;
    void drawSpectrumAnalyzer(QuadBatcher& g, class SpectrumModule& spectrum) override;
    void drawAudioModule(QuadBatcher& g, class AudioModule& module) override;
};

/**
 * @class ModernLookAndFeel
 * @brief A sleek, dark look and feel
 */
class ModernLookAndFeel : public DefaultLookAndFeel {
public:
        void drawButtonBackground(QuadBatcher& g, Button& button, 
                                  bool isMouseOver, bool isButtonDown) override;
        void drawButtonText(QuadBatcher& g, Button& button, 
                            bool isMouseOver, bool isButtonDown) override;
        void drawSliderBackground(QuadBatcher& g, Slider& slider, 
                                  float sliderPos, float rotaryStartAngle, float rotaryEndAngle) override;    void drawSliderPointer(QuadBatcher& g, Slider& slider, 
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle) override;
    
    void drawKnob(QuadBatcher& g, Knob& knob, float sliderPos) override;
    void drawLuminousMeter(QuadBatcher& g, LuminousMeter& meter, float level, float peak) override;
    void drawVUMeter(QuadBatcher& g, VUMeter& meter, float level) override;
    void drawSpectrumAnalyzer(QuadBatcher& g, class SpectrumModule& spectrum) override;
    void drawAudioModule(QuadBatcher& g, class AudioModule& module) override;
};

} // namespace Beam

#endif // LOOK_AND_FEEL_HPP
