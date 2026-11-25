#pragma once
#include "PluginProcessor.h"
#include <array>

class RusticStudioLookAndFeel : public juce::LookAndFeel_V4
{
public:
    RusticStudioLookAndFeel();

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider& slider) override;

    void drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox& box) override;

    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                               const juce::Colour& backgroundColour,
                               bool isHighlighted, bool isDown) override;

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                           bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
};

class AnalogVuMeter : public juce::Component, private juce::Timer
{
public:
    explicit AnalogVuMeter (BenBocazStudioProProcessor& proc);
    void paint (juce::Graphics& g) override;
    void setShowGainReduction (bool showGr) { showGR = showGr; }
    bool isShowingGainReduction() const { return showGR; }

private:
    void timerCallback() override;
    BenBocazStudioProProcessor& processor;
    bool showGR = false;
    float smoothedNeedle = 0.0f;
};

class Studio4MicStagePad : public juce::Component, private juce::Timer
{
public:
    explicit Studio4MicStagePad (juce::AudioProcessorValueTreeState& apvtsRef);

    void setShangriLaImage (const juce::Image& img) { shangriLaImg = img; repaint(); }
    void setCanyonImage    (const juce::Image& img) { canyonImg = img; repaint(); }
    void setSourceImage    (int idx, const juce::Image& img);
    void setMicImage       (int idx, const juce::Image& img);

    void paint (juce::Graphics& g) override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent& e) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp   (const juce::MouseEvent& e) override;

private:
    void timerCallback() override { repaint(); }

    juce::Point<float> getStandBasePos (int idx, juce::Rectangle<float> area) const;
    float getStandScale (int idx) const;
    float getStandVisualHeight (int idx, float scale) const;
    int findClosestStand (juce::Point<float> pt, juce::Rectangle<float> area, float& outMinDist) const;
    void updateStandFromBasePoint (int idx, juce::Point<float> targetBase, juce::Rectangle<float> area);

    juce::AudioProcessorValueTreeState& apvts;
    juce::Image shangriLaImg;
    juce::Image canyonImg;
    std::array<juce::Image, 4> sourceImgs;
    std::array<juce::Image, 4> micImgs;

    int selectedStand = 0;
    int hoveredStand = -1;
    bool isDragging = false;
    juce::Point<float> dragOffset { 0.0f, 0.0f };
};

class BenBocazStudioProEditor : public juce::AudioProcessorEditor
{
public:
    explicit BenBocazStudioProEditor (BenBocazStudioProProcessor&);
    ~BenBocazStudioProEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void loadEmbeddedImages();
    void setupKnob (juce::Slider& slider, juce::Label& label, const juce::String& text);
    void setupCombo (juce::ComboBox& box, const juce::StringArray& items, juce::Label& label, const juce::String& text);
    void openIRFileChooser();

    BenBocazStudioProProcessor& audioProcessor;
    RusticStudioLookAndFeel rusticLAF;
    AnalogVuMeter vuMeter;
    Studio4MicStagePad roomPad;
    juce::Image chassisImage;

    juce::ToggleButton phaseAlignBtn { "PHASE SAFE" };
    juce::ToggleButton phaseInvBtn   { "POLARITY INV" };
    juce::ToggleButton vocalModBtn   { "VOCAL MIC MOD" };
    juce::ToggleButton mainStereoBtn { "MAIN STEREO" };
    juce::ToggleButton roomOnBtn     { "ROOM ON" };
    juce::ToggleButton chamberOnBtn  { "ON" };
    juce::ToggleButton eqOnBtn       { "EQ ON" };
    juce::ToggleButton compOnBtn     { "COMP ON" };

    juce::TextButton vuModeBtn { "VU: OUT" };
    juce::TextButton loadIRBtn { "LOAD IR (.WAV)" };
    juce::Label irNameLbl;
    std::unique_ptr<juce::FileChooser> fileChooser;

    juce::ComboBox modeBox, chainBox, vocalSrcBox, vocalTgtBox;
    juce::ComboBox sourceBox, monitorBox, roomBox, wallsBox;
    juce::ComboBox chamberBox, eqTypeBox, compBox;

    juce::Label modeLbl, chainLbl, vocalSrcLbl, vocalTgtLbl;
    juce::Label sourceLbl, monitorLbl, roomLbl, wallsLbl;
    juce::Label chamberLbl, eqTypeLbl, compLbl;

    juce::Slider vocalInSlider, vocalOutSlider, driveSlider;
    juce::Slider mic1GainSlider, mic2GainSlider, mic3GainSlider, mic4GainSlider;
    juce::Slider pairWidthSlider, deccaWidthSlider;
    juce::Slider chMixSlider, chDecaySlider, chPreSlider, chWidthSlider;
    juce::Slider eqHpfSlider, eqLowSlider, eqMidSlider, eqHighSlider;
    juce::Slider compThreshSlider, outGainSlider;

    juce::Label vocalInLbl, vocalOutLbl, driveLbl;
    juce::Label mic1GainLbl, mic2GainLbl, mic3GainLbl, mic4GainLbl;
    juce::Label pairWidthLbl, deccaWidthLbl;
    juce::Label chMixLbl, chDecayLbl, chPreLbl, chWidthLbl;
    juce::Label eqHpfLbl, eqLowLbl, eqMidLbl, eqHighLbl;
    juce::Label compThreshLbl, outGainLbl;

    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<ButtonAtt> phaseAlignAtt, phaseInvAtt, vocalModAtt, mainStereoAtt, roomOnAtt, chamberOnAtt, eqOnAtt, compOnAtt;
    std::unique_ptr<ComboAtt>  modeAtt, chainAtt, vocalSrcAtt, vocalTgtAtt;
    std::unique_ptr<ComboAtt>  sourceAtt, monitorAtt, roomAtt, wallsAtt;
    std::unique_ptr<ComboAtt>  chamberAtt, eqTypeAtt, compAtt;

    std::unique_ptr<SliderAtt> vocalInAtt, vocalOutAtt, driveAtt;
    std::unique_ptr<SliderAtt> mic1GainAtt, mic2GainAtt, mic3GainAtt, mic4GainAtt;
    std::unique_ptr<SliderAtt> pairWidthAtt, deccaWidthAtt;
    std::unique_ptr<SliderAtt> chMixAtt, chDecayAtt, chPreAtt, chWidthAtt;
    std::unique_ptr<SliderAtt> eqHpfAtt, eqLowAtt, eqMidAtt, eqHighAtt;
    std::unique_ptr<SliderAtt> compThreshAtt, outGainAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BenBocazStudioProEditor)
};
