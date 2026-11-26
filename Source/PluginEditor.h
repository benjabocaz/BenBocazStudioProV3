#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
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
                           bool isHighlighted, bool isDown) override;
};

class AnalogVuMeter : public juce::Component, private juce::Timer
{
public:
    explicit AnalogVuMeter (BenBocazStudioProProcessor& proc);
    void setShowGainReduction (bool gr) { showGR = gr; repaint(); }
    bool isShowingGainReduction() const { return showGR; }

    void paint (juce::Graphics& g) override;

private:
    void timerCallback() override;
    BenBocazStudioProProcessor& processor;
    float smoothedNeedle = 0.0f;
    bool showGR = false;
};

class Studio4MicStagePad : public juce::Component, private juce::Timer
{
public:
    explicit Studio4MicStagePad (BenBocazStudioProProcessor& proc);

    void setShangriLaImage (const juce::Image& img) { shangriLaImg = img; repaint(); }
    void setCanyonImage (const juce::Image& img)    { canyonImg = img; repaint(); }
    void setVolcanoOpenImage (const juce::Image& img)   { volcanoOpenImg = img; repaint(); }
    void setVolcanoClosedImage (const juce::Image& img) { volcanoClosedImg = img; repaint(); }
    void setSourceImage (int idx, const juce::Image& img);
    void setMicImage (int idx, const juce::Image& img);
    void setFarMicImage (int idx, const juce::Image& img);

    void setSnapToCenter (bool snap) { snapToCenter = snap; }
    void setLinkAirDecca (bool link) { linkAirDecca = link; repaint(); }
    bool isAirDeccaLinked() const { return linkAirDecca; }
    void setShowTipsOverlay (bool show) { showTipsOverlay = show; repaint(); }
    void setSelectedStand (int idx);
    int getSelectedStand() const { return selectedStand; }
    void centerStand (int idx);

    std::function<void (int)> onSelectedStandChanged;

    void paint (juce::Graphics& g) override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent& e) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDoubleClick (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;

private:
    void timerCallback() override { repaint(); }
    juce::Point<float> getStandBasePos (int idx, juce::Rectangle<float> area) const;
    float getStandScale (int idx) const;
    float getStandVisualHeight (int idx, float scale) const;
    juce::Rectangle<float> getStandTagRect (int idx, juce::Rectangle<float> area) const;
    int findTargetStand (juce::Point<float> pt, juce::Rectangle<float> area, float& outMinDist, bool preferSelected) const;
    void updateStandFromBasePoint (int idx, juce::Point<float> targetBase, juce::Rectangle<float> area);
    void drawAmpFallback (juce::Graphics& g, float cx, float floorY, int sourceIdx, float scale, float alpha) const;
    void drawTipsOverlay (juce::Graphics& g, juce::Rectangle<float> area, int sourceIdx) const;

    BenBocazStudioProProcessor& processor;
    juce::AudioProcessorValueTreeState& apvts;

    juce::Image shangriLaImg, canyonImg, volcanoOpenImg, volcanoClosedImg;
    std::array<juce::Image, 7> sourceImgs;
    std::array<juce::Image, 4> micImgs;
    std::array<juce::Image, 2> farMicImgs;

    bool snapToCenter = true;
    bool linkAirDecca = true;
    bool showTipsOverlay = false;
    bool isDragging = false;
    int selectedStand = 0;
    int hoveredStand = -1;
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
    void openIRFileChooser();
    void applyFactoryPreset (int idx);
    void updateSoloButtonsUI();
    void updateSelectedStandButtonsUI (int selIdx);
    void updateHqModeButtonText();
    void setupKnob (juce::Slider& slider, juce::Label& label, const juce::String& text);
    void setupStandKnob (juce::Slider& slider, juce::TextButton& selectBtn, const juce::String& text, int standIdx);
    void setupCombo (juce::ComboBox& box, const juce::StringArray& items, juce::Label& label, const juce::String& text);

    BenBocazStudioProProcessor& audioProcessor;
    RusticStudioLookAndFeel rusticLAF;
    AnalogVuMeter vuMeter;
    Studio4MicStagePad roomPad;

    juce::Image chassisImage;
    std::unique_ptr<juce::FileChooser> fileChooser;
    bool stageExpanded = false;
    bool syncingFarGains = false;

    juce::Label presetLbl;
    juce::ComboBox presetBox;
    juce::ToggleButton snapCenterBtn { "SNAP CENTER" };
    juce::ToggleButton link34Btn     { "LINK 3+4" };
    juce::TextButton centerSelBtn   { "CENTER MIC" };
    juce::ToggleButton hqModeBtn     { "MIXING 4x HQ" };
    juce::TextButton tipsBtn        { "i TIPS" };
    juce::TextButton expandStageBtn { "EXPAND STAGE" };

    juce::TextButton solo1Btn { "S" }, solo2Btn { "S" }, solo3Btn { "S" }, solo4Btn { "S" };
    juce::TextButton mic1SelBtn, mic2SelBtn, mic3SelBtn, mic4SelBtn;

    juce::ToggleButton phaseAlignBtn { "PHASE SAFE" };
    juce::ToggleButton phaseInvBtn   { "POLARITY INV" };
    juce::ToggleButton vocalModBtn   { "VOCAL MIC MOD" };
    juce::ToggleButton mainStereoBtn { "MAIN STEREO" };
    juce::ToggleButton roomOnBtn     { "ROOM ON" };
    juce::ToggleButton chamberOnBtn  { "ON" };
    juce::ToggleButton eqOnBtn       { "EQ ON" };
    juce::ToggleButton compOnBtn     { "COMP ON" };
    juce::ToggleButton compScLinkBtn { "SC LINK" };
    juce::ToggleButton compScFiltBtn { "SC FILTER" };
    juce::ToggleButton parCompOnBtn  { "PAR CRUSH" };
    juce::TextButton vuModeBtn       { "VU: OUT" };
    juce::TextButton loadIRBtn       { "LOAD IR (.WAV)" };
    juce::Label irNameLbl;

    juce::ComboBox modeBox, chainBox, vocalSrcBox, vocalTgtBox, sourceBox, monitorBox, roomBox, wallsBox, eqTubeBox, chamberBox;
    juce::ComboBox eqTypeBox, eqLowFreqBox, eqMidFreqBox, eqHighFreqBox, compBox, parRouteBox;
    juce::Label modeLbl, chainLbl, vocalSrcLbl, vocalTgtLbl, sourceLbl, monitorLbl, roomLbl, wallsLbl, eqTubeLbl, chamberLbl;
    juce::Label eqTypeLbl, compLbl, parRouteLbl;

    juce::Slider vocalInSlider, vocalOutSlider, driveSlider;
    juce::Slider mic1GainSlider, mic2GainSlider, mic3GainSlider, mic4GainSlider;
    juce::Slider pairWidthSlider, deccaWidthSlider;
    juce::Slider chMixSlider, chDecaySlider, chPreSlider, chWidthSlider;
    juce::Slider eqHpfSlider, eqLowSlider, eqMidSlider, eqHighSlider;
    juce::Slider compThreshSlider, compMixSlider, parAmtSlider, outGainSlider;

    juce::Label vocalInLbl, vocalOutLbl, driveLbl;
    juce::Label pairWidthLbl, deccaWidthLbl;
    juce::Label chMixLbl, chDecayLbl, chPreLbl, chWidthLbl;
    juce::Label eqHpfLbl, eqLowLbl, eqMidLbl, eqHighLbl;
    juce::Label compThreshLbl, compMixLbl, parAmtLbl, outGainLbl;

    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<ButtonAtt> phaseAlignAtt, phaseInvAtt, hqModeAtt, vocalModAtt, mainStereoAtt, roomOnAtt, chamberOnAtt;
    std::unique_ptr<ButtonAtt> eqOnAtt, compOnAtt, compScLinkAtt, compScFiltAtt, parCompOnAtt;
    std::unique_ptr<ComboAtt>  modeAtt, chainAtt, vocalSrcAtt, vocalTgtAtt, sourceAtt, monitorAtt, roomAtt, wallsAtt, eqTubeAtt, chamberAtt;
    std::unique_ptr<ComboAtt>  eqTypeAtt, eqLowFreqAtt, eqMidFreqAtt, eqHighFreqAtt, compAtt, parRouteAtt;
    std::unique_ptr<SliderAtt> vocalInAtt, vocalOutAtt, driveAtt;
    std::unique_ptr<SliderAtt> mic1GainAtt, mic2GainAtt, mic3GainAtt, mic4GainAtt;
    std::unique_ptr<SliderAtt> pairWidthAtt, deccaWidthAtt;
    std::unique_ptr<SliderAtt> chMixAtt, chDecayAtt, chPreAtt, chWidthAtt;
    std::unique_ptr<SliderAtt> eqHpfAtt, eqLowAtt, eqMidAtt, eqHighAtt;
    std::unique_ptr<SliderAtt> compThreshAtt, compMixAtt, parAmtAtt, outGainAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BenBocazStudioProEditor)
};
