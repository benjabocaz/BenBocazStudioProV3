#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

class AndesLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AndesLookAndFeel();

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, const float rotaryStartAngle, const float rotaryEndAngle,
                          juce::Slider& slider) override;

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool isHover, bool isDown) override;
};

class AnalogVUMeter : public juce::Component
{
public:
    void setLevelDb(float db, bool isGainReduction);
    void paint(juce::Graphics& g) override;

private:
    float smoothedNeedle = 0.0f;
    bool grMode = false;
};

class Studio4MicStagePad : public juce::Component
{
public:
    explicit Studio4MicStagePad(juce::AudioProcessorValueTreeState& state);

    void setSelectedMic(int idx);
    void centerSelectedMic();

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

private:
    juce::Image removeGreenScreen(const juce::Image& src);
    void loadEmbeddedImages();
    juce::Point<float> normToScreen(float nx, float ny, juce::Rectangle<float> area) const;
    juce::Point<float> screenToNorm(juce::Point<float> pt, juce::Rectangle<float> area) const;
    void beginMicGesture(int idx);
    void endMicGesture(int idx);
    void setMicPos(int idx, float nx, float ny);

    juce::AudioProcessorValueTreeState& apvts;

    int selectedMic = 0;
    int draggingMic = -1;

    juce::Image imgVolcanoOpen, imgVolcanoClosed, imgShangrila, imgCanyon;
    juce::Image imgMicMono, imgMicXY, imgMicPair, imgMicFarL, imgMicFarR;
    juce::Image imgSrcVocals, imgSrcDrums, imgSrcBass, imgSrcGuitarMono, imgSrcGuitarStereo, imgSrcEnsemble, imgSrcMonitors;
};

class BenBocazStudioProAudioProcessorEditor : public juce::AudioProcessorEditor,
                                              private juce::Timer
{
public:
    explicit BenBocazStudioProAudioProcessorEditor(BenBocazStudioProAudioProcessor& p);
    ~BenBocazStudioProAudioProcessorEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;
    void setupKnob(juce::Slider& s, const juce::String& suffix = {});
    void setupLabel(juce::Label& l, const juce::String& text,
                    float fontSize = 10.0f, juce::Justification just = juce::Justification::centred);
    void updateHqButtonText();

    BenBocazStudioProAudioProcessor& processor;
    AndesLookAndFeel lnf;
    Studio4MicStagePad stagePad;
    AnalogVUMeter vuMeter;

    juce::Image imgChassisWood;

    bool vuGrMode = false;
    bool expandedStage = false;
    bool syncingFarGains = false;

    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;

    std::vector<std::unique_ptr<ComboAtt>>  comboAtts;
    std::vector<std::unique_ptr<ButtonAtt>> buttonAtts;
    std::vector<std::unique_ptr<SliderAtt>> sliderAtts;

    juce::ComboBox cbMode, cbRouting, cbPresets;
    juce::ToggleButton btHqMode, btPhaseSafe, btPolarity, btSnapCenter, btLink34;
    juce::TextButton btCenterMic { "CENTER MIC" }, btVuMode { "VU: OUT" }, btExpandStage { "EXPAND STAGE" };

    juce::ToggleButton btMainStereo { "MAIN STEREO" }, btRoomOn { "ROOM ON" };

    juce::ToggleButton btVocalMod { "VOCAL MIC MOD" };
    juce::ComboBox cbVocalIn, cbVocalTarget, cbSource, cbMonitor, cbRoom, cbWalls, cbCircuit;
    juce::Slider slVocalIn, slVocalOut, slDrive;
    juce::Label lbVocalIn, lbVocalOut, lbDrive;

    juce::TextButton btMicSelect[4];
    juce::Slider slMicGain[4], slPairWide, slFarWide;
    juce::ToggleButton btMicSolo[4];
    juce::Label lbPairWide, lbFarWide;

    juce::ToggleButton btChamberOn { "ON" };
    juce::ComboBox cbChamberModel;
    juce::Slider slChamberSend, slChamberDecay, slChamberPre, slChamberWidth;
    juce::Label lbChSend, lbChDecay, lbChPre, lbChWidth;

    juce::ToggleButton btEqOn { "EQ ON" };
    juce::ComboBox cbEqModel, cbLowFreq, cbMidFreq, cbHighFreq;
    juce::Slider slHpf, slLowGain, slMidGain, slHighGain;
    juce::Label lbHpf, lbLow, lbMid, lbHigh;

    juce::ToggleButton btCompOn { "COMP ON" }, btScLink { "SC LINK" }, btScFilter { "SC FILTER" }, btParCrush { "PAR CRUSH" };
    juce::ComboBox cbCompModel, cbParRoute;
    juce::Slider slCompThresh, slCompMix, slParBlend;
    juce::Label lbCompThresh, lbCompMix, lbParBlend;

    juce::Slider slOutput;
    juce::Label lbOutput;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BenBocazStudioProAudioProcessorEditor)
};
