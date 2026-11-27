#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include <array>
#include <memory>

//==============================================================================
// Escenario Interactivo de la Sala (4 Atriles Arrastrables en X e Y)
//==============================================================================
class StudioStageComponent : public juce::Component, private juce::Timer
{
public:
    explicit StudioStageComponent (BenBocazStudioProProcessor& p);
    ~StudioStageComponent() override = default;

    void setLinkFarStands (bool shouldLink) { linkFar = shouldLink; }

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    void timerCallback() override { repaint(); }

    juce::Rectangle<float> getStageArea() const;
    juce::Point<float> normToScreen (float x, float y) const;
    void screenToNorm (juce::Point<float> pt, float& outX, float& outY) const;
    void setParamNorm (const juce::String& id, float value);

    BenBocazStudioProProcessor& processor;
    int draggingStand = -1;
    bool linkFar = true;
};

//==============================================================================
// Editor Principal de Ben Bocaz Studio Pro
//==============================================================================
class BenBocazStudioProEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit BenBocazStudioProEditor (BenBocazStudioProProcessor&);
    ~BenBocazStudioProEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setupKnob (juce::Slider& s, juce::Label& l, const juce::String& text, const juce::String& suffix = {});
    void setupToggle (juce::ToggleButton& b, const juce::String& text);
    void setupCombo (juce::ComboBox& c, const juce::StringArray& items);
    void updateSoloButtons();
    void applyPreset (int presetIndex);
    void setParam (const juce::String& id, float rawValue);

    BenBocazStudioProProcessor& processor;

    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

    // Barra superior
    juce::ComboBox presetBox, modeBox, chainOrderBox;
    juce::ToggleButton hqModeBtn, phaseAlignBtn, phaseInvertBtn;
    std::unique_ptr<ComboAtt> modeAtt, chainOrderAtt;
    std::unique_ptr<ButtonAtt> hqModeAtt, phaseAlignAtt, phaseInvertAtt;

    // Vocal Mic Modeler
    juce::ToggleButton vocalModOnBtn;
    juce::ComboBox vocalSrcBox, vocalTgtBox;
    juce::Slider vocalInSlider, vocalOutSlider;
    juce::Label vocalInLbl, vocalOutLbl;
    std::unique_ptr<ButtonAtt> vocalModOnAtt;
    std::unique_ptr<ComboAtt> vocalSrcAtt, vocalTgtAtt;
    std::unique_ptr<SliderAtt> vocalInAtt, vocalOutAtt;

    // Escenario y Sala 3D
    StudioStageComponent stageView;
    juce::ToggleButton roomOnBtn, mainStereoBtn, linkFarBtn;
    juce::ComboBox sourceBox, monitorBox, roomBox, wallsBox;
    juce::Slider pairSpreadSlider, roomAirTailSlider;
    juce::Label pairSpreadLbl, roomAirTailLbl;
    std::unique_ptr<ButtonAtt> roomOnAtt, mainStereoAtt;
    std::unique_ptr<ComboAtt> sourceAtt, monitorAtt, roomAtt, wallsAtt;
    std::unique_ptr<SliderAtt> pairSpreadAtt, roomAirTailAtt;

    std::array<juce::Slider, 4> micGainSliders;
    std::array<juce::Label, 4> micGainLabels;
    std::array<juce::TextButton, 4> micSoloBtns;
    std::array<std::unique_ptr<SliderAtt>, 4> micGainAtts;

    // Echo Chambers + Custom IR
    juce::ToggleButton chamberOnBtn;
    juce::ComboBox chamberTypeBox;
    juce::TextButton loadIrBtn { "Load WAV IR" };
    juce::Label irNameLbl;
    juce::Slider chamberMixSlider, chamberDecaySlider, chamberPreSlider, chamberWidthSlider;
    juce::Label chamberMixLbl, chamberDecayLbl, chamberPreLbl, chamberWidthLbl;
    std::unique_ptr<ButtonAtt> chamberOnAtt;
    std::unique_ptr<ComboAtt> chamberTypeAtt;
    std::unique_ptr<SliderAtt> chamberMixAtt, chamberDecayAtt, chamberPreAtt, chamberWidthAtt;
    std::unique_ptr<juce::FileChooser> irChooser;

    // Consola: Preamp + EQ + Comp + Parallel + Output
    juce::ComboBox tubeModelBox, eqTypeBox, eqLowFreqBox, eqMidFreqBox, eqHighFreqBox;
    juce::ToggleButton eqOnBtn;
    juce::Slider driveSlider, eqHpfSlider, eqLowSlider, eqMidSlider, eqHighSlider;
    juce::Label driveLbl, eqHpfLbl, eqLowLbl, eqMidLbl, eqHighLbl;
    std::unique_ptr<ComboAtt> tubeModelAtt, eqTypeAtt, eqLowFreqAtt, eqMidFreqAtt, eqHighFreqAtt;
    std::unique_ptr<ButtonAtt> eqOnAtt;
    std::unique_ptr<SliderAtt> driveAtt, eqHpfAtt, eqLowAtt, eqMidAtt, eqHighAtt;

    juce::ToggleButton compOnBtn, compScLinkBtn, compScFilterBtn, parCompOnBtn;
    juce::ComboBox compTypeBox, parCompRouteBox;
    juce::Slider compThreshSlider, compMixSlider, parCompAmtSlider, outputGainSlider;
    juce::Label compThreshLbl, compMixLbl, parCompAmtLbl, outputGainLbl;
    std::unique_ptr<ButtonAtt> compOnAtt, compScLinkAtt, compScFilterAtt, parCompOnAtt;
    std::unique_ptr<ComboAtt> compTypeAtt, parCompRouteAtt;
    std::unique_ptr<SliderAtt> compThreshAtt, compMixAtt, parCompAmtAtt, outputGainAtt;

    // Medidores
    float displayVuDb = -60.0f;
    float displayGrDb = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BenBocazStudioProEditor)
};
