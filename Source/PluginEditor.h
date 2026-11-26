#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include <array>

//==============================================================================
// Look & Feel estilo madera / bronce / ambar (Los Andes Capital)
class AndesLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AndesLookAndFeel();

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override;

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
};

//==============================================================================
// Medidor VU analogico (OUT / GR)
class AnalogVUMeter : public juce::Component
{
public:
    void setLevelDb(float db, bool isGainReduction);
    void paint(juce::Graphics& g) override;

private:
    float smoothedNeedle = 0.0f;
    bool grMode = false;
};

//==============================================================================
// Escenario 3D interactivo de 4 atriles (Volcano / Shangri-La / Canyon)
class Studio4MicStagePad : public juce::Component
{
public:
    explicit Studio4MicStagePad(juce::AudioProcessorValueTreeState& state);

    void loadEmbeddedImages();
    void setSelectedMic(int idx);
    int getSelectedMic() const noexcept { return selectedMic; }
    void centerSelectedMic();

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

private:
    static juce::Image removeGreenScreen(const juce::Image& src);
    juce::Point<float> normToScreen(float nx, float ny, juce::Rectangle<float> area) const;
    juce::Point<float> screenToNorm(juce::Point<float> pt, juce::Rectangle<float> area) const;
    void setMicPos(int idx, float nx, float ny);

    juce::AudioProcessorValueTreeState& apvts;
    int selectedMic = 0;
    int draggingMic = -1;

    juce::Image imgVolcanoOpen, imgVolcanoClosed, imgShangrila, imgCanyon;
    juce::Image imgMicMono, imgMicXY, imgMicPair, imgMicFarL, imgMicFarR;
    juce::Image imgSrcDrums, imgSrcBass, imgSrcEnsemble;
};

//==============================================================================
// Editor Principal
class BenBocazStudioProAudioProcessorEditor : public juce::AudioProcessorEditor,
                                              private juce::Timer
{
public:
    explicit BenBocazStudioProAudioProcessorEditor(BenBocazStudioProAudioProcessor&);
    ~BenBocazStudioProAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setupKnob(juce::Slider& s, const juce::String& suffix = {});
    void setupLabel(juce::Label& l, const juce::String& text, float fontSize = 11.0f,
                    juce::Justification just = juce::Justification::centred);
    void updateHqButtonText();

    BenBocazStudioProAudioProcessor& processor;
    AndesLookAndFeel lnf;
    juce::Image imgChassisWood;

    bool expandedStage = false;
    bool vuGrMode = false;

    // Top bar
    juce::ComboBox cbMode, cbRouting, cbPresets;
    juce::ToggleButton btHqMode { "HQ 4x" }, btPhaseSafe { "PHASE SAFE" }, btPolarity { "POLARITY INV" };
    juce::ToggleButton btSnapCenter { "SNAP CENTER" }, btLink34 { "LINK 3+4" };
    juce::TextButton btCenterMic { "CENTER MIC" }, btVuMode { "VU: OUT" }, btExpandStage { "EXPAND STAGE" };
    AnalogVUMeter vuMeter;

    // Stage header & pad
    juce::ToggleButton btMainStereo { "MAIN STEREO" }, btRoomOn { "ROOM ON" };
    Studio4MicStagePad stagePad;

    // Left panel: Vocal Mic Mod & Source
    juce::ToggleButton btVocalMod { "VOCAL MIC MOD" };
    juce::ComboBox cbVocalIn, cbVocalTarget, cbSource, cbMonitor, cbRoom, cbWalls, cbCircuit;
    juce::Slider slVocalIn, slVocalOut, slDrive;
    juce::Label lbVocalIn, lbVocalOut, lbDrive;

    // Stage bottom strip: 4 Mics + Widths
    std::array<juce::TextButton, 4> btMicSelect;
    std::array<juce::Slider, 4> slMicGain;
    std::array<juce::ToggleButton, 4> btMicSolo;
    juce::Slider slPairWide, slFarWide;
    juce::Label lbPairWide, lbFarWide;

    // Right panel: Echo Chambers
    juce::ToggleButton btChamberOn { "ON" };
    juce::ComboBox cbChamberModel;
    juce::Slider slChamberSend, slChamberDecay, slChamberPre, slChamberWidth;
    juce::Label lbChSend, lbChDecay, lbChPre, lbChWidth;

    // Bottom left: Analog Console EQ
    juce::ToggleButton btEqOn { "EQ ON" };
    juce::ComboBox cbEqModel, cbLowFreq, cbMidFreq, cbHighFreq;
    juce::Slider slHpf, slLowGain, slMidGain, slHighGain;
    juce::Label lbHpf, lbLow, lbMid, lbHigh;

    // Bottom middle: Outboard & Parallel Comp
    juce::ToggleButton btCompOn { "COMP ON" }, btScLink { "SC LINK" }, btScFilter { "SC FILTER" }, btParCrush { "PAR CRUSH" };
    juce::ComboBox cbCompModel, cbParRoute;
    juce::Slider slCompThresh, slCompMix, slParBlend;
    juce::Label lbCompThresh, lbCompMix, lbParBlend;

    // Bottom right: Master
    juce::Slider slOutput;
    juce::Label lbOutput;

    // Attachments
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::vector<std::unique_ptr<SliderAtt>> sliderAtts;
    std::vector<std::unique_ptr<ComboAtt>> comboAtts;
    std::vector<std::unique_ptr<ButtonAtt>> buttonAtts;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BenBocazStudioProAudioProcessorEditor)
};
