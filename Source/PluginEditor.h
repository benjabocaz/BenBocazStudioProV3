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
    explicit Studio4MicStagePad (BenBocazStudioProProcessor& proc);

    void setShangriLaImage (const juce::Image& img) { shangriLaImg = img; repaint(); }
    void setCanyonImage    (const juce::Image& img) { canyonImg = img; repaint(); }
    void setSourceImage    (int idx, const juce::Image& img);
    void setMicImage       (int idx, const juce::Image& img);

    void paint (juce::Graphics& g) override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent& e) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDoubleClick (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp   (const juce::MouseEvent& e) override;

    void setSnapToCenter (bool b) { snapToCenter = b; repaint(); }
    bool isSnapToCenter() const { return snapToCenter; }

    void setLinkAirDecca (bool b) { linkAirDecca = b; repaint(); }
    bool isLinkAirDecca() const { return linkAirDecca; }

    void setShowTipsOverlay (bool b) { showTipsOverlay = b; repaint(); }
    bool isShowingTipsOverlay() const { return showTipsOverlay; }

    int getSelectedStand() const { return selectedStand; }
    void centerStand (int idx);

private:
    void timerCallback() override { repaint(); }

    juce::Point<float> getStandBasePos (int idx, juce::Rectangle<float> area) const;
    float getStandScale (int idx) const;
    float getStandVisualHeight (int idx, float scale) const;
    int findClosestStand (juce::Point<float> pt, juce::Rectangle<float> area, float& outMinDist) const;
    void updateStandFromBasePoint (int idx, juce::Point<float> targetBase, juce::Rectangle<float> area);
    void drawAmpFallback (juce::Graphics& g, float cx, float floorY, int sourceIdx, float scale, float alpha) const;
    void drawTipsOverlay (juce::Graphics& g, juce::Rectangle<float> area, int sourceIdx) const;

    BenBocazStudioProProcessor& processor;
    juce::AudioProcessorValueTreeState& apvts;
    juce::Image shangriLaImg;
    juce::Image canyonImg;
    std::array<juce::Image, 7> sourceImgs; // 0..6
    std::array<juce::Image, 4> micImgs;    // 0: Main Mono, 1: Main X-Y, 2: Stereo Pair, 3: Decca

    int selectedStand = 0;
    int hoveredStand = -1;
    bool isDragging = false;
    bool snapToCenter = true;
    bool linkAirDecca = false;
    bool showTipsOverlay = false;
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
    void setupKnob (juce::Slider& slider, juce::Label& label, const juce::String& text, juce::Colour accent = juce::Colour (0xffe5a94c));
    void setupCombo (juce::ComboBox& box, const juce::StringArray& items, juce::Label& label, const juce::String& text);
    void openIRFileChooser();
    void applyFactoryPreset (int presetIdx);
    void updateSoloButtonsUI();

    BenBocazStudioProProcessor& audioProcessor;
    RusticStudioLookAndFeel rusticLAF;
    AnalogVuMeter vuMeter;
    Studio4MicStagePad roomPad;
    juce::Image chassisImage;
    bool stageExpanded = false;

    // Presets y Life Hacks en barra superior
    juce::Label presetLbl;
    juce::ComboBox presetBox;
    juce::TextButton roomGlueBtn { "ROOM GLUE" };
    juce::ToggleButton deMudBtn  { "DE-MUD" };

    juce::ToggleButton phaseAlignBtn { "PHASE SAFE" };
    juce::ToggleButton phaseInvBtn   { "POLARITY INV" };
    juce::ToggleButton vocalModBtn   { "VOCAL MIC MOD" };
    juce::ToggleButton mainStereoBtn { "MAIN STEREO" };
    juce::ToggleButton roomOnBtn     { "ROOM ON" };
    juce::ToggleButton chamberOnBtn  { "ON" };
    juce::ToggleButton eqOnBtn       { "EQ ON" };
    juce::ToggleButton eqTubeSyncBtn { "TUBE SYNC" };
    juce::ToggleButton compOnBtn     { "COMP ON" };
    juce::ToggleButton compScLinkBtn { "SC LINK" };
    juce::ToggleButton compScFiltBtn { "SC FILTER" };
    juce::ToggleButton parCompOnBtn  { "PAR CRUSH" };

    // Botones de Precisión, Tips y Vista Expandida sobre el escenario
    juce::ToggleButton snapCenterBtn { "SNAP CENTER" };
    juce::ToggleButton link34Btn     { "LINK 3+4" };
    juce::TextButton centerSelBtn    { "CENTER MIC" };
    juce::TextButton tipsBtn         { "i  TIPS" };
    juce::TextButton expandStageBtn  { "EXPAND STAGE" };

    // Botones Solo por Atril (S)
    juce::TextButton solo1Btn { "S" }, solo2Btn { "S" }, solo3Btn { "S" }, solo4Btn { "S" };

    juce::TextButton vuModeBtn { "VU: OUT" };
    juce::TextButton loadIRBtn { "LOAD IR (.WAV)" };
    juce::Label irNameLbl;
    std::unique_ptr<juce::FileChooser> fileChooser;

    juce::ComboBox modeBox, chainBox, vocalSrcBox, vocalTgtBox;
    juce::ComboBox sourceBox, monitorBox, roomBox, wallsBox;
    juce::ComboBox chamberBox, eqTypeBox, eqTubeBox, eqLowFreqBox, eqMidFreqBox, eqHighFreqBox;
    juce::ComboBox compBox, parRouteBox;

    juce::Label modeLbl, chainLbl, vocalSrcLbl, vocalTgtLbl;
    juce::Label sourceLbl, monitorLbl, roomLbl, wallsLbl;
    juce::Label chamberLbl, eqTypeLbl, eqTubeLbl, compLbl, parRouteLbl;

    juce::Slider vocalInSlider, vocalOutSlider, driveSlider;
    juce::Slider mic1GainSlider, mic2GainSlider, mic3GainSlider, mic4GainSlider;
    juce::Slider pairWidthSlider, deccaWidthSlider;
    juce::Slider chMixSlider, chDecaySlider, chPreSlider, chWidthSlider;
    juce::Slider eqHpfSlider, eqLowSlider, eqMidSlider, eqHighSlider;
    juce::Slider compThreshSlider, compMixSlider, parAmtSlider, outGainSlider;

    juce::Label vocalInLbl, vocalOutLbl, driveLbl;
    juce::Label mic1GainLbl, mic2GainLbl, mic3GainLbl, mic4GainLbl;
    juce::Label pairWidthLbl, deccaWidthLbl;
    juce::Label chMixLbl, chDecayLbl, chPreLbl, chWidthLbl;
    juce::Label eqHpfLbl, eqLowLbl, eqMidLbl, eqHighLbl;
    juce::Label compThreshLbl, compMixLbl, parAmtLbl, outGainLbl;

    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<ButtonAtt> deMudAtt, phaseAlignAtt, phaseInvAtt, vocalModAtt, mainStereoAtt, roomOnAtt;
    std::unique_ptr<ButtonAtt> chamberOnAtt, eqOnAtt, eqTubeSyncAtt, compOnAtt, compScLinkAtt, compScFiltAtt, parCompOnAtt;

    std::unique_ptr<ComboAtt>  modeAtt, chainAtt, vocalSrcAtt, vocalTgtAtt;
    std::unique_ptr<ComboAtt>  sourceAtt, monitorAtt, roomAtt, wallsAtt;
    std::unique_ptr<ComboAtt>  chamberAtt, eqTypeAtt, eqTubeAtt, eqLowFreqAtt, eqMidFreqAtt, eqHighFreqAtt;
    std::unique_ptr<ComboAtt>  compAtt, parRouteAtt;

    std::unique_ptr<SliderAtt> vocalInAtt, vocalOutAtt, driveAtt;
    std::unique_ptr<SliderAtt> mic1GainAtt, mic2GainAtt, mic3GainAtt, mic4GainAtt;
    std::unique_ptr<SliderAtt> pairWidthAtt, deccaWidthAtt;
    std::unique_ptr<SliderAtt> chMixAtt, chDecayAtt, chPreAtt, chWidthAtt;
    std::unique_ptr<SliderAtt> eqHpfAtt, eqLowAtt, eqMidAtt, eqHighAtt;
    std::unique_ptr<SliderAtt> compThreshAtt, compMixAtt, parAmtAtt, outGainAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BenBocazStudioProEditor)
};
