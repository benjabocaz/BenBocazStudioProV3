#include "PluginEditor.h"

namespace
{
    const juce::Colour kBgDark      { 0xff14161a };
    const juce::Colour kPanelBg     { 0xff1d2026 };
    const juce::Colour kPanelBorder { 0xff2f343f };
    const juce::Colour kAccentGold  { 0xffe0a84b };
    const juce::Colour kAccentCyan  { 0xff4ecdc4 };
    const juce::Colour kTextMain    { 0xffeceff4 };
    const juce::Colour kTextDim     { 0xff9aa3b2 };
}

//==============================================================================
// StudioStageComponent
//==============================================================================
StudioStageComponent::StudioStageComponent (BenBocazStudioProProcessor& p)
    : processor (p)
{
    startTimerHz (24);
}

juce::Rectangle<float> StudioStageComponent::getStageArea() const
{
    return getLocalBounds().toFloat().reduced (14.0f, 16.0f);
}

juce::Point<float> StudioStageComponent::normToScreen (float x, float y) const
{
    const auto r = getStageArea();
    const float sx = r.getCentreX() + x * (r.getWidth() * 0.46f);
    const float sy = r.getY() + 26.0f + y * (r.getHeight() - 42.0f);
    return { sx, sy };
}

void StudioStageComponent::screenToNorm (juce::Point<float> pt, float& outX, float& outY) const
{
    const auto r = getStageArea();
    outX = std::clamp ((pt.x - r.getCentreX()) / (r.getWidth() * 0.46f), -1.0f, 1.0f);
    outY = std::clamp ((pt.y - (r.getY() + 26.0f)) / (r.getHeight() - 42.0f), 0.0f, 1.0f);
}

void StudioStageComponent::setParamNorm (const juce::String& id, float value)
{
    if (auto* param = processor.apvts.getParameter (id))
        param->setValueNotifyingHost (param->convertTo0to1 (value));
}

void StudioStageComponent::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff101216));
    g.fillRoundedRectangle (bounds, 8.0f);
    g.setColour (kPanelBorder);
    g.drawRoundedRectangle (bounds.reduced (0.5f), 8.0f, 1.0f);

    const auto r = getStageArea();
    const int roomIdx   = static_cast<int> (processor.apvts.getRawParameterValue ("room")->load());
    const int wallsLive = static_cast<int> (processor.apvts.getRawParameterValue ("walls")->load());

    // Color de fondo según sala y paredes
    const juce::Colour roomTint = (roomIdx == 0) ? juce::Colour (0xff231e19)
                                : (roomIdx == 1) ? juce::Colour (0xff1b2126)
                                                 : juce::Colour (0xff241a1d);
    g.setColour (wallsLive == 1 ? roomTint.brighter (0.08f) : roomTint.darker (0.25f));
    g.fillRoundedRectangle (r, 6.0f);

    // Rejilla acústica
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    for (int i = 1; i < 4; ++i)
    {
        const float y = r.getY() + r.getHeight() * (static_cast<float> (i) / 4.0f);
        g.drawHorizontalLine (static_cast<int> (y), r.getX(), r.getRight());
    }
    g.drawVerticalLine (static_cast<int> (r.getCentreX()), r.getY(), r.getBottom());

    // Fuente sonora en la parte superior central
    const juce::Point<float> srcPt { r.getCentreX(), r.getY() + 14.0f };
    g.setColour (kAccentGold.withAlpha (0.22f));
    g.fillEllipse (srcPt.x - 16.0f, srcPt.y - 10.0f, 32.0f, 20.0f);
    g.setColour (kAccentGold);
    g.fillEllipse (srcPt.x - 5.0f, srcPt.y - 5.0f, 10.0f, 10.0f);
    g.setFont (11.0f);
    g.drawText ("SOURCE", static_cast<int> (srcPt.x - 36), static_cast<int> (srcPt.y - 14), 72, 12, juce::Justification::centred);

    // Dibujar los 4 atriles
    static constexpr std::array<const char*, 4> labels { "1:MAIN", "2:ROOM", "3:FAR L", "4:FAR R" };
    static const std::array<juce::Colour, 4> colors {
        kAccentGold, kAccentCyan, juce::Colour (0xffa78bfa), juce::Colour (0xfff472b6)
    };
    static constexpr std::array<const char*, 4> xIds { "mic1X", "mic2X", "mic3X", "mic4X" };
    static constexpr std::array<const char*, 4> yIds { "mic1Y", "mic2Y", "mic3Y", "mic4Y" };
    static constexpr std::array<const char*, 4> gIds { "mic1Gain", "mic2Gain", "mic3Gain", "mic4Gain" };

    const int soloIdx = processor.getSoloStand();

    for (int i = 0; i < 4; ++i)
    {
        const float mx = processor.apvts.getRawParameterValue (xIds[static_cast<size_t> (i)])->load();
        const float my = processor.apvts.getRawParameterValue (yIds[static_cast<size_t> (i)])->load();
        const float mg = processor.apvts.getRawParameterValue (gIds[static_cast<size_t> (i)])->load();

        const auto pt = normToScreen (mx, my);
        const bool dimmed = (mg <= 0.01f) || (soloIdx >= 0 && soloIdx != i);
        const auto col = colors[static_cast<size_t> (i)].withAlpha (dimmed ? 0.30f : 0.95f);

        // Línea de distancia hacia la fuente
        g.setColour (col.withAlpha (dimmed ? 0.08f : 0.22f));
        g.drawLine (srcPt.x, srcPt.y, pt.x, pt.y, 1.0f);

        // Halo y punto del atril
        const float radius = (draggingStand == i) ? 10.0f : 8.0f;
        g.setColour (col.withAlpha (dimmed ? 0.12f : 0.25f));
        g.fillEllipse (pt.x - radius * 1.6f, pt.y - radius * 1.6f, radius * 3.2f, radius * 3.2f);
        g.setColour (col);
        g.fillEllipse (pt.x - radius, pt.y - radius, radius * 2.0f, radius * 2.0f);

        g.setColour (kTextMain.withAlpha (dimmed ? 0.45f : 1.0f));
        g.setFont (10.5f);
        g.drawText (labels[static_cast<size_t> (i)],
                    static_cast<int> (pt.x - 28), static_cast<int> (pt.y + 9), 56, 14, juce::Justification::centred);
    }
}

void StudioStageComponent::mouseDown (const juce::MouseEvent& e)
{
    static constexpr std::array<const char*, 4> xIds { "mic1X", "mic2X", "mic3X", "mic4X" };
    static constexpr std::array<const char*, 4> yIds { "mic1Y", "mic2Y", "mic3Y", "mic4Y" };

    draggingStand = -1;
    float bestDist = 22.0f;
    for (int i = 0; i < 4; ++i)
    {
        const float mx = processor.apvts.getRawParameterValue (xIds[static_cast<size_t> (i)])->load();
        const float my = processor.apvts.getRawParameterValue (yIds[static_cast<size_t> (i)])->load();
        const float d = normToScreen (mx, my).getDistanceFrom (e.position);
        if (d < bestDist)
        {
            bestDist = d;
            draggingStand = i;
        }
    }
}

void StudioStageComponent::mouseDrag (const juce::MouseEvent& e)
{
    if (draggingStand < 0) return;

    float nx = 0.0f, ny = 0.0f;
    screenToNorm (e.position, nx, ny);

    static constexpr std::array<const char*, 4> xIds { "mic1X", "mic2X", "mic3X", "mic4X" };
    static constexpr std::array<const char*, 4> yIds { "mic1Y", "mic2Y", "mic3Y", "mic4Y" };

    setParamNorm (xIds[static_cast<size_t> (draggingStand)], nx);
    setParamNorm (yIds[static_cast<size_t> (draggingStand)], ny);

    if (linkFar && (draggingStand == 2 || draggingStand == 3))
    {
        const int other = (draggingStand == 2) ? 3 : 2;
        setParamNorm (xIds[static_cast<size_t> (other)], -nx);
        setParamNorm (yIds[static_cast<size_t> (other)],  ny);
    }

    repaint();
}

void StudioStageComponent::mouseUp (const juce::MouseEvent&)
{
    draggingStand = -1;
    repaint();
}

//==============================================================================
// BenBocazStudioProEditor
//==============================================================================
BenBocazStudioProEditor::BenBocazStudioProEditor (BenBocazStudioProProcessor& p)
    : AudioProcessorEditor (&p), processor (p), stageView (p)
{
    setSize (1140, 740);

    // 1. Barra superior y 14 Presets
    setupCombo (presetBox, {
        "01. Studio A - Lead Vocal Pro",
        "02. Studio A - Drum Room Punch",
        "03. Studio A - Live Ensemble",
        "04. Studio B - Intimate Dry Vocal",
        "05. Studio B - Tight Drum Booth",
        "06. Studio B - NS-10 Swedien Re-Mic",
        "07. Studio A - Altec 604 Re-Mic",
        "08. Volcano - Epic Arena Drums",
        "09. Volcano - Cinematic Choir / Strings",
        "10. Guitar Cab Mono - Vintage Crunch",
        "11. Guitar Cab Stereo - Wide Shoegaze",
        "12. Bass Amp - Warm Studio Low End",
        "13. Vocal Tight Doubler (Haas L/R)",
        "14. Chorus Wide Loose (Organic Room)"
    });
    presetBox.onChange = [this]
    {
        const int idx = presetBox.getSelectedItemIndex();
        if (idx >= 0) applyPreset (idx);
    };

    setupCombo (modeBox, { "Reverb", "Re-Mic" });
    setupCombo (chainOrderBox, { "EQ -> Comp -> Chamber", "Comp -> EQ -> Chamber", "Chamber -> Comp -> EQ" });
    setupToggle (hqModeBtn, "4x HQ (OFF=0ms)");
    setupToggle (phaseAlignBtn, "PHASE SAFE");
    setupToggle (phaseInvertBtn, "POLARITY Ø");

    modeAtt        = std::make_unique<ComboAtt> (processor.apvts, "mode", modeBox);
    chainOrderAtt  = std::make_unique<ComboAtt> (processor.apvts, "chainOrder", chainOrderBox);
    hqModeAtt      = std::make_unique<ButtonAtt> (processor.apvts, "hqMode", hqModeBtn);
    phaseAlignAtt  = std::make_unique<ButtonAtt> (processor.apvts, "phaseAlign", phaseAlignBtn);
    phaseInvertAtt = std::make_unique<ButtonAtt> (processor.apvts, "phaseInvert", phaseInvertBtn);

    // 2. Vocal Mic Modeler
    setupToggle (vocalModOnBtn, "VOCAL MIC MOD");
    setupCombo (vocalSrcBox, { "Shure SM27", "Shure SM57", "Shure SM7B", "Generic LDC" });
    setupCombo (vocalTgtBox, { "Sony C-800G", "Neumann TLM 103", "Aston Origin" });
    setupKnob (vocalInSlider,  vocalInLbl,  "IN GAIN",  " dB");
    setupKnob (vocalOutSlider, vocalOutLbl, "OUT GAIN", " dB");

    vocalModOnAtt = std::make_unique<ButtonAtt> (processor.apvts, "vocalModOn", vocalModOnBtn);
    vocalSrcAtt   = std::make_unique<ComboAtt>  (processor.apvts, "vocalSrcMic", vocalSrcBox);
    vocalTgtAtt   = std::make_unique<ComboAtt>  (processor.apvts, "vocalTgtMic", vocalTgtBox);
    vocalInAtt    = std::make_unique<SliderAtt> (processor.apvts, "vocalInGain", vocalInSlider);
    vocalOutAtt   = std::make_unique<SliderAtt> (processor.apvts, "vocalOutGain", vocalOutSlider);

    // 3. Sala 3D y Escenario
    addAndMakeVisible (stageView);
    setupToggle (roomOnBtn, "ROOM ON");
    setupToggle (mainStereoBtn, "MAIN STEREO");
    setupToggle (linkFarBtn, "LINK 3+4");
    linkFarBtn.setToggleState (true, juce::dontSendNotification);
    linkFarBtn.onClick = [this] { stageView.setLinkFarStands (linkFarBtn.getToggleState()); };

    setupCombo (sourceBox, { "Vocals", "Drums", "Ensemble", "Monitors (Swedien)", "Guitar Cab (Mono)", "Guitar Cab (Stereo)", "Bass Amp (Low End)" });
    setupCombo (monitorBox, { "Altec Main", "NS-10 Nearfield", "JBL PA Wedge" });
    setupCombo (roomBox, { "Studio A (Shangri-La)", "Studio B (Canyon)", "Volcano" });
    setupCombo (wallsBox, { "Tight / Damped", "Live / Cedar Wood" });

    setupKnob (pairSpreadSlider,  pairSpreadLbl,  "PAIR WIDTH");
    setupKnob (roomAirTailSlider, roomAirTailLbl, "AIR / TAIL");

    roomOnAtt      = std::make_unique<ButtonAtt> (processor.apvts, "roomOn", roomOnBtn);
    mainStereoAtt  = std::make_unique<ButtonAtt> (processor.apvts, "mainStereo", mainStereoBtn);
    sourceAtt      = std::make_unique<ComboAtt>  (processor.apvts, "source", sourceBox);
    monitorAtt     = std::make_unique<ComboAtt>  (processor.apvts, "monitorType", monitorBox);
    roomAtt        = std::make_unique<ComboAtt>  (processor.apvts, "room", roomBox);
    wallsAtt       = std::make_unique<ComboAtt>  (processor.apvts, "walls", wallsBox);
    pairSpreadAtt  = std::make_unique<SliderAtt> (processor.apvts, "pairSpread", pairSpreadSlider);
    roomAirTailAtt = std::make_unique<SliderAtt> (processor.apvts, "roomAirTail", roomAirTailSlider);

    static constexpr std::array<const char*, 4> micNames { "1: MAIN", "2: ROOM", "3: FAR L", "4: FAR R" };
    static constexpr std::array<const char*, 4> micIds   { "mic1Gain", "mic2Gain", "mic3Gain", "mic4Gain" };
    for (int i = 0; i < 4; ++i)
    {
        setupKnob (micGainSliders[static_cast<size_t> (i)], micGainLabels[static_cast<size_t> (i)], micNames[static_cast<size_t> (i)]);
        micGainAtts[static_cast<size_t> (i)] = std::make_unique<SliderAtt> (processor.apvts, micIds[static_cast<size_t> (i)], micGainSliders[static_cast<size_t> (i)]);

        auto& sb = micSoloBtns[static_cast<size_t> (i)];
        sb.setButtonText ("S");
        sb.setClickingTogglesState (false);
        sb.onClick = [this, i]
        {
            const int cur = processor.getSoloStand();
            processor.setSoloStand (cur == i ? -1 : i);
            updateSoloButtons();
        };
        addAndMakeVisible (sb);
    }
    updateSoloButtons();

    // 4. Echo Chambers + IR
    setupToggle (chamberOnBtn, "CHAMBER ON");
    setupCombo (chamberTypeBox, { "Chamber 1 (Bright)", "Chamber 3 (Classic)", "Chamber 5 (Deep)", "Chamber 8 (Dense)", "Custom WAV IR" });
    setupKnob (chamberMixSlider,   chamberMixLbl,   "MIX");
    setupKnob (chamberDecaySlider, chamberDecayLbl, "DECAY");
    setupKnob (chamberPreSlider,   chamberPreLbl,   "PRE-DELAY", " ms");
    setupKnob (chamberWidthSlider, chamberWidthLbl, "WIDTH");

    addAndMakeVisible (loadIrBtn);
    loadIrBtn.onClick = [this]
    {
        irChooser = std::make_unique<juce::FileChooser> ("Select Impulse Response (.wav)", juce::File {}, "*.wav;*.aiff");
        irChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                                [this] (const juce::FileChooser& fc)
                                {
                                    const auto f = fc.getResult();
                                    if (f.existsAsFile())
                                    {
                                        processor.loadCustomIR (f);
                                        setParam ("chamberType", 4.0f);
                                        irNameLbl.setText ("IR: " + processor.getLoadedIRName(), juce::dontSendNotification);
                                    }
                                });
    };
    irNameLbl.setText ("IR: " + processor.getLoadedIRName(), juce::dontSendNotification);
    irNameLbl.setColour (juce::Label::textColourId, kTextDim);
    irNameLbl.setFont (11.0f);
    addAndMakeVisible (irNameLbl);

    chamberOnAtt    = std::make_unique<ButtonAtt> (processor.apvts, "chamberOn", chamberOnBtn);
    chamberTypeAtt  = std::make_unique<ComboAtt>  (processor.apvts, "chamberType", chamberTypeBox);
    chamberMixAtt   = std::make_unique<SliderAtt> (processor.apvts, "chamberMix", chamberMixSlider);
    chamberDecayAtt = std::make_unique<SliderAtt> (processor.apvts, "chamberDecay", chamberDecaySlider);
    chamberPreAtt   = std::make_unique<SliderAtt> (processor.apvts, "chamberPreDelay", chamberPreSlider);
    chamberWidthAtt = std::make_unique<SliderAtt> (processor.apvts, "chamberWidth", chamberWidthSlider);

    // 5. Consola: Preamp + EQ
    setupCombo (tubeModelBox, { "Neve Class-A", "Pultec 12AX7", "API 2520", "SSL Clean" });
    setupKnob (driveSlider, driveLbl, "PRE DRIVE");

    setupToggle (eqOnBtn, "EQ ON");
    setupCombo (eqTypeBox, { "Neve 1073", "Pultec EQP-1A", "API 550A", "SSL 4000E" });
    setupCombo (eqLowFreqBox,  { "60 Hz", "110 Hz", "220 Hz" });
    setupCombo (eqMidFreqBox,  { "700 Hz", "1.6 kHz", "3.2 kHz", "4.8 kHz" });
    setupCombo (eqHighFreqBox, { "10 kHz", "12 kHz", "16 kHz" });

    setupKnob (eqHpfSlider,  eqHpfLbl,  "HPF",  " Hz");
    setupKnob (eqLowSlider,  eqLowLbl,  "LOW",  " dB");
    setupKnob (eqMidSlider,  eqMidLbl,  "MID",  " dB");
    setupKnob (eqHighSlider, eqHighLbl, "HIGH", " dB");

    tubeModelAtt  = std::make_unique<ComboAtt>  (processor.apvts, "eqTubeModel", tubeModelBox);
    driveAtt      = std::make_unique<SliderAtt> (processor.apvts, "consoleDrive", driveSlider);
    eqOnAtt       = std::make_unique<ButtonAtt> (processor.apvts, "eqOn", eqOnBtn);
    eqTypeAtt     = std::make_unique<ComboAtt>  (processor.apvts, "eqType", eqTypeBox);
    eqLowFreqAtt  = std::make_unique<ComboAtt>  (processor.apvts, "eqLowFreq", eqLowFreqBox);
    eqMidFreqAtt  = std::make_unique<ComboAtt>  (processor.apvts, "eqMidFreq", eqMidFreqBox);
    eqHighFreqAtt = std::make_unique<ComboAtt>  (processor.apvts, "eqHighFreq", eqHighFreqBox);
    eqHpfAtt      = std::make_unique<SliderAtt> (processor.apvts, "eqHpf", eqHpfSlider);
    eqLowAtt      = std::make_unique<SliderAtt> (processor.apvts, "eqLow", eqLowSlider);
    eqMidAtt      = std::make_unique<SliderAtt> (processor.apvts, "eqMid", eqMidSlider);
    eqHighAtt     = std::make_unique<SliderAtt> (processor.apvts, "eqHigh", eqHighSlider);

    // 6. Compresor + Parallel Crush + Output
    setupToggle (compOnBtn, "COMP ON");
    setupCombo (compTypeBox, { "1176LN (FET)", "LA-2A (Opto)", "Mastering (Tube)" });
    setupKnob (compThreshSlider, compThreshLbl, "THRESH", " dB");
    setupKnob (compMixSlider,    compMixLbl,    "COMP MIX");
    setupToggle (compScLinkBtn, "SC LINK");
    setupToggle (compScFilterBtn, "SC HPF");

    setupToggle (parCompOnBtn, "PAR CRUSH");
    setupCombo (parCompRouteBox, { "All Bus", "Room Mics", "Chamber" });
    setupKnob (parCompAmtSlider, parCompAmtLbl, "BLEND");
    setupKnob (outputGainSlider, outputGainLbl, "OUTPUT", " dB");

    compOnAtt       = std::make_unique<ButtonAtt> (processor.apvts, "compOn", compOnBtn);
    compTypeAtt     = std::make_unique<ComboAtt>  (processor.apvts, "compType", compTypeBox);
    compThreshAtt   = std::make_unique<SliderAtt> (processor.apvts, "compThresh", compThreshSlider);
    compMixAtt      = std::make_unique<SliderAtt> (processor.apvts, "compMix", compMixSlider);
    compScLinkAtt   = std::make_unique<ButtonAtt> (processor.apvts, "compScLink", compScLinkBtn);
    compScFilterAtt = std::make_unique<ButtonAtt> (processor.apvts, "compScFilter", compScFilterBtn);
    parCompOnAtt    = std::make_unique<ButtonAtt> (processor.apvts, "parCompOn", parCompOnBtn);
    parCompRouteAtt = std::make_unique<ComboAtt>  (processor.apvts, "parCompRoute", parCompRouteBox);
    parCompAmtAtt   = std::make_unique<SliderAtt> (processor.apvts, "parCompAmt", parCompAmtSlider);
    outputGainAtt   = std::make_unique<SliderAtt> (processor.apvts, "outputGain", outputGainSlider);

    startTimerHz (24);
}

BenBocazStudioProEditor::~BenBocazStudioProEditor()
{
    stopTimer();
}

void BenBocazStudioProEditor::setupKnob (juce::Slider& s, juce::Label& l, const juce::String& text, const juce::String& suffix)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 62, 16);
    s.setTextValueSuffix (suffix);
    s.setColour (juce::Slider::rotarySliderFillColourId, kAccentGold);
    s.setColour (juce::Slider::thumbColourId, kTextMain);
    s.setColour (juce::Slider::textBoxTextColourId, kTextMain);
    s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (s);

    l.setText (text, juce::dontSendNotification);
    l.setJustificationType (juce::Justification::centred);
    l.setColour (juce::Label::textColourId, kTextDim);
    l.setFont (10.5f);
    addAndMakeVisible (l);
}

void BenBocazStudioProEditor::setupToggle (juce::ToggleButton& b, const juce::String& text)
{
    b.setButtonText (text);
    b.setColour (juce::ToggleButton::textColourId, kTextMain);
    b.setColour (juce::ToggleButton::tickColourId, kAccentGold);
    addAndMakeVisible (b);
}

void BenBocazStudioProEditor::setupCombo (juce::ComboBox& c, const juce::StringArray& items)
{
    c.addItemList (items, 1);
    c.setSelectedId (1, juce::dontSendNotification);
    addAndMakeVisible (c);
}

void BenBocazStudioProEditor::updateSoloButtons()
{
    const int soloIdx = processor.getSoloStand();
    for (int i = 0; i < 4; ++i)
    {
        const bool isSolo = (soloIdx == i);
        micSoloBtns[static_cast<size_t> (i)].setColour (juce::TextButton::buttonColourId,
                                                        isSolo ? kAccentGold : juce::Colour (0xff2b303a));
        micSoloBtns[static_cast<size_t> (i)].setColour (juce::TextButton::textColourOffId,
                                                        isSolo ? juce::Colours::black : kTextMain);
    }
}

void BenBocazStudioProEditor::setParam (const juce::String& id, float rawValue)
{
    if (auto* param = processor.apvts.getParameter (id))
        param->setValueNotifyingHost (param->convertTo0to1 (rawValue));
}

void BenBocazStudioProEditor::applyPreset (int idx)
{
    // Valores base limpios antes de aplicar cada preset
    processor.setSoloStand (-1);
    updateSoloButtons();
    setParam ("vocalModOn", 0.0f);
    setParam ("roomOn", 1.0f);
    setParam ("mode", 1.0f);
    setParam ("phaseAlign", 1.0f);
    setParam ("phaseInvert", 0.0f);
    setParam ("mainStereo", 0.0f);
    setParam ("pairSpread", 1.0f);
    setParam ("roomAirTail", 0.35f);
    setParam ("parCompOn", 0.0f);
    setParam ("eqOn", 1.0f);
    setParam ("compOn", 1.0f);
    setParam ("eqLow", 0.0f); setParam ("eqMid", 0.0f); setParam ("eqHigh", 0.0f); setParam ("eqHpf", 20.0f);
    setParam ("outputGain", 0.0f);

    auto setMics = [this] (float x1, float y1, float g1, float x2, float y2, float g2,
                           float x3, float y3, float g3, float x4, float y4, float g4)
    {
        setParam ("mic1X", x1); setParam ("mic1Y", y1); setParam ("mic1Gain", g1);
        setParam ("mic2X", x2); setParam ("mic2Y", y2); setParam ("mic2Gain", g2);
        setParam ("mic3X", x3); setParam ("mic3Y", y3); setParam ("mic3Gain", g3);
        setParam ("mic4X", x4); setParam ("mic4Y", y4); setParam ("mic4Gain", g4);
    };

    switch (idx)
    {
        case 0: // 01. Studio A - Lead Vocal Pro
            setParam ("source", 0.0f); setParam ("room", 0.0f); setParam ("walls", 1.0f);
            setParam ("roomAirTail", 0.42f);
            setMics (0.0f, 0.16f, 1.0f, -0.30f, 0.48f, 0.28f, -0.58f, 0.76f, 0.14f, 0.58f, 0.76f, 0.14f);
            setParam ("chamberOn", 1.0f); setParam ("chamberType", 1.0f); setParam ("chamberMix", 0.22f); setParam ("chamberDecay", 0.72f); setParam ("chamberPreDelay", 26.0f);
            setParam ("eqType", 1.0f); setParam ("eqTubeModel", 1.0f); setParam ("consoleDrive", 0.22f); setParam ("eqHpf", 85.0f); setParam ("eqHigh", 2.2f);
            setParam ("compType", 1.0f); setParam ("compThresh", -18.0f); setParam ("compMix", 1.0f);
            break;

        case 1: // 02. Studio A - Drum Room Punch
            setParam ("source", 1.0f); setParam ("room", 0.0f); setParam ("walls", 1.0f);
            setParam ("mainStereo", 1.0f); setParam ("roomAirTail", 0.30f);
            setMics (0.0f, 0.22f, 0.95f, 0.0f, 0.52f, 0.55f, -0.72f, 0.80f, 0.42f, 0.72f, 0.80f, 0.42f);
            setParam ("chamberOn", 0.0f);
            setParam ("eqType", 2.0f); setParam ("eqTubeModel", 2.0f); setParam ("consoleDrive", 0.30f); setParam ("eqLow", 2.5f); setParam ("eqMid", 1.5f);
            setParam ("compType", 0.0f); setParam ("compThresh", -20.0f); setParam ("parCompOn", 1.0f); setParam ("parCompRoute", 1.0f); setParam ("parCompAmt", 0.42f);
            break;

        case 2: // 03. Studio A - Live Ensemble
            setParam ("source", 2.0f); setParam ("room", 0.0f); setParam ("walls", 1.0f);
            setParam ("mainStereo", 1.0f); setParam ("roomAirTail", 0.50f);
            setMics (0.0f, 0.28f, 0.92f, 0.0f, 0.56f, 0.48f, -0.65f, 0.82f, 0.35f, 0.65f, 0.82f, 0.35f);
            setParam ("chamberOn", 1.0f); setParam ("chamberType", 3.0f); setParam ("chamberMix", 0.18f); setParam ("chamberDecay", 0.76f);
            setParam ("eqType", 0.0f); setParam ("eqTubeModel", 0.0f); setParam ("consoleDrive", 0.18f);
            setParam ("compType", 2.0f); setParam ("compThresh", -14.0f);
            break;

        case 3: // 04. Studio B - Intimate Dry Vocal
            setParam ("source", 0.0f); setParam ("room", 1.0f); setParam ("walls", 0.0f);
            setParam ("roomAirTail", 0.25f);
            setMics (0.0f, 0.12f, 1.05f, -0.22f, 0.42f, 0.18f, -0.45f, 0.70f, 0.0f, 0.45f, 0.70f, 0.0f);
            setParam ("chamberOn", 1.0f); setParam ("chamberType", 0.0f); setParam ("chamberMix", 0.14f); setParam ("chamberDecay", 0.52f); setParam ("chamberPreDelay", 18.0f);
            setParam ("eqType", 0.0f); setParam ("eqTubeModel", 0.0f); setParam ("consoleDrive", 0.16f); setParam ("eqHpf", 95.0f); setParam ("eqHigh", 1.8f);
            setParam ("compType", 1.0f); setParam ("compThresh", -16.0f);
            break;

        case 4: // 05. Studio B - Tight Drum Booth
            setParam ("source", 1.0f); setParam ("room", 1.0f); setParam ("walls", 0.0f);
            setParam ("mainStereo", 1.0f); setParam ("roomAirTail", 0.22f);
            setMics (0.0f, 0.18f, 1.0f, 0.0f, 0.44f, 0.45f, -0.52f, 0.72f, 0.25f, 0.52f, 0.72f, 0.25f);
            setParam ("chamberOn", 0.0f);
            setParam ("eqType", 2.0f); setParam ("eqTubeModel", 2.0f); setParam ("consoleDrive", 0.32f); setParam ("eqLow", 2.0f);
            setParam ("compType", 0.0f); setParam ("compThresh", -22.0f); setParam ("parCompOn", 1.0f); setParam ("parCompRoute", 0.0f); setParam ("parCompAmt", 0.35f);
            break;

        case 5: // 06. Studio B - NS-10 Swedien Re-Mic
            setParam ("source", 3.0f); setParam ("monitorType", 1.0f); setParam ("room", 1.0f); setParam ("walls", 1.0f);
            setParam ("roomAirTail", 0.35f);
            setMics (0.0f, 0.20f, 0.95f, 0.0f, 0.50f, 0.40f, -0.60f, 0.78f, 0.28f, 0.60f, 0.78f, 0.28f);
            setParam ("chamberOn", 1.0f); setParam ("chamberType", 1.0f); setParam ("chamberMix", 0.16f);
            setParam ("eqType", 3.0f); setParam ("eqTubeModel", 3.0f); setParam ("consoleDrive", 0.15f);
            setParam ("compType", 0.0f); setParam ("compThresh", -16.0f);
            break;

        case 6: // 07. Studio A - Altec 604 Re-Mic
            setParam ("source", 3.0f); setParam ("monitorType", 0.0f); setParam ("room", 0.0f); setParam ("walls", 1.0f);
            setParam ("mainStereo", 1.0f); setParam ("roomAirTail", 0.45f);
            setMics (0.0f, 0.24f, 0.95f, 0.0f, 0.55f, 0.45f, -0.70f, 0.82f, 0.34f, 0.70f, 0.82f, 0.34f);
            setParam ("chamberOn", 1.0f); setParam ("chamberType", 1.0f); setParam ("chamberMix", 0.20f);
            setParam ("eqType", 0.0f); setParam ("eqTubeModel", 0.0f); setParam ("consoleDrive", 0.24f);
            setParam ("compType", 2.0f); setParam ("compThresh", -15.0f);
            break;

        case 7: // 08. Volcano - Epic Arena Drums
            setParam ("source", 1.0f); setParam ("room", 2.0f); setParam ("walls", 1.0f);
            setParam ("mainStereo", 1.0f); setParam ("roomAirTail", 0.58f);
            setMics (0.0f, 0.20f, 0.90f, 0.0f, 0.56f, 0.58f, -0.82f, 0.88f, 0.62f, 0.82f, 0.88f, 0.62f);
            setParam ("chamberOn", 1.0f); setParam ("chamberType", 2.0f); setParam ("chamberMix", 0.16f); setParam ("chamberDecay", 0.84f);
            setParam ("eqType", 2.0f); setParam ("eqTubeModel", 2.0f); setParam ("consoleDrive", 0.28f); setParam ("eqLow", 2.2f);
            setParam ("compType", 0.0f); setParam ("compThresh", -22.0f); setParam ("parCompOn", 1.0f); setParam ("parCompRoute", 1.0f); setParam ("parCompAmt", 0.50f);
            break;

        case 8: // 09. Volcano - Cinematic Choir / Strings
            setParam ("source", 2.0f); setParam ("room", 2.0f); setParam ("walls", 1.0f);
            setParam ("mainStereo", 1.0f); setParam ("roomAirTail", 0.72f);
            setMics (0.0f, 0.32f, 0.85f, 0.0f, 0.60f, 0.55f, -0.85f, 0.90f, 0.55f, 0.85f, 0.90f, 0.55f);
            setParam ("chamberOn", 1.0f); setParam ("chamberType", 2.0f); setParam ("chamberMix", 0.32f); setParam ("chamberDecay", 0.90f); setParam ("chamberPreDelay", 34.0f);
            setParam ("eqType", 1.0f); setParam ("eqTubeModel", 1.0f); setParam ("consoleDrive", 0.16f); setParam ("eqHigh", 2.0f);
            setParam ("compType", 2.0f); setParam ("compThresh", -12.0f);
            break;

        case 9: // 10. Guitar Cab Mono - Vintage Crunch
            setParam ("source", 4.0f); setParam ("room", 1.0f); setParam ("walls", 1.0f);
            setParam ("roomAirTail", 0.30f);
            setMics (0.08f, 0.14f, 1.05f, -0.35f, 0.48f, 0.32f, -0.58f, 0.74f, 0.18f, 0.58f, 0.74f, 0.18f);
            setParam ("chamberOn", 1.0f); setParam ("chamberType", 1.0f); setParam ("chamberMix", 0.12f); setParam ("chamberDecay", 0.58f);
            setParam ("eqType", 0.0f); setParam ("eqTubeModel", 0.0f); setParam ("consoleDrive", 0.38f); setParam ("eqMid", 1.8f);
            setParam ("compType", 0.0f); setParam ("compThresh", -18.0f);
            break;

        case 10: // 11. Guitar Cab Stereo - Wide Shoegaze
            setParam ("source", 5.0f); setParam ("room", 0.0f); setParam ("walls", 1.0f);
            setParam ("mainStereo", 1.0f); setParam ("roomAirTail", 0.62f);
            setMics (0.0f, 0.22f, 0.95f, 0.0f, 0.54f, 0.45f, -0.78f, 0.84f, 0.42f, 0.78f, 0.84f, 0.42f);
            setParam ("chamberOn", 1.0f); setParam ("chamberType", 3.0f); setParam ("chamberMix", 0.34f); setParam ("chamberDecay", 0.86f);
            setParam ("eqType", 1.0f); setParam ("eqTubeModel", 1.0f); setParam ("consoleDrive", 0.28f);
            setParam ("compType", 1.0f); setParam ("compThresh", -16.0f);
            break;

        case 11: // 12. Bass Amp - Warm Studio Low End
            setParam ("source", 6.0f); setParam ("room", 1.0f); setParam ("walls", 0.0f);
            setParam ("roomAirTail", 0.18f);
            setMics (0.0f, 0.15f, 1.05f, -0.25f, 0.45f, 0.22f, -0.48f, 0.70f, 0.0f, 0.48f, 0.70f, 0.0f);
            setParam ("chamberOn", 0.0f);
            setParam ("eqType", 1.0f); setParam ("eqTubeModel", 1.0f); setParam ("consoleDrive", 0.34f); setParam ("eqLow", 2.8f);
            setParam ("compType", 1.0f); setParam ("compThresh", -18.0f); setParam ("compScFilter", 1.0f);
            break;

        case 12: // 13. Vocal Tight Doubler (Haas L/R)
            setParam ("source", 0.0f); setParam ("room", 1.0f); setParam ("walls", 0.0f);
            setParam ("phaseAlign", 0.0f); // Activa retardo físico real + micro-modulación orgánica
            setParam ("roomAirTail", 0.48f);
            setMics (0.0f, 0.12f, 1.0f, 0.0f, 0.45f, 0.0f, -0.78f, 0.44f, 0.58f, 0.78f, 0.62f, 0.58f);
            setParam ("chamberOn", 1.0f); setParam ("chamberType", 0.0f); setParam ("chamberMix", 0.12f); setParam ("chamberDecay", 0.45f); setParam ("chamberPreDelay", 16.0f);
            setParam ("eqType", 3.0f); setParam ("eqTubeModel", 1.0f); setParam ("consoleDrive", 0.16f); setParam ("eqHpf", 95.0f);
            setParam ("compType", 1.0f); setParam ("compThresh", -18.0f);
            break;

        case 13: // 14. Chorus Wide Loose (Organic Room)
            setParam ("source", 0.0f); setParam ("room", 0.0f); setParam ("walls", 1.0f);
            setParam ("phaseAlign", 0.0f); // Micro-modulación orgánica + retardos asimétricos
            setParam ("pairSpread", 1.35f); setParam ("roomAirTail", 0.68f);
            setMics (0.0f, 0.18f, 0.72f, 0.0f, 0.52f, 0.42f, -0.92f, 0.68f, 0.68f, 0.92f, 0.86f, 0.68f);
            setParam ("chamberOn", 1.0f); setParam ("chamberType", 3.0f); setParam ("chamberMix", 0.24f); setParam ("chamberDecay", 0.72f); setParam ("chamberWidth", 1.35f);
            setParam ("eqType", 1.0f); setParam ("eqTubeModel", 1.0f); setParam ("consoleDrive", 0.18f); setParam ("eqHpf", 110.0f); setParam ("eqHigh", 2.0f);
            setParam ("compType", 1.0f); setParam ("compThresh", -16.0f);
            break;

        default: break;
    }
}

void BenBocazStudioProEditor::timerCallback()
{
    const float targetVu = processor.getOutputVuDb();
    const float targetGr = processor.getCompGrDb();
    displayVuDb = displayVuDb * 0.75f + targetVu * 0.25f;
    displayGrDb = displayGrDb * 0.75f + targetGr * 0.25f;
    repaint (getWidth() - 220, 8, 212, 42);
}

void BenBocazStudioProEditor::paint (juce::Graphics& g)
{
    g.fillAll (kBgDark);

    // Barra superior
    g.setColour (kPanelBg);
    g.fillRect (0, 0, getWidth(), 54);
    g.setColour (kPanelBorder);
    g.drawHorizontalLine (54, 0.0f, static_cast<float> (getWidth()));

    g.setColour (kAccentGold);
    g.setFont (juce::Font (18.0f, juce::Font::bold));
    g.drawText ("BEN BOCAZ STUDIO PRO", 14, 12, 230, 30, juce::Justification::centredLeft);

    // Paneles principales
    auto drawPanel = [&] (juce::Rectangle<int> r, const juce::String& title)
    {
        g.setColour (kPanelBg);
        g.fillRoundedRectangle (r.toFloat(), 8.0f);
        g.setColour (kPanelBorder);
        g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 8.0f, 1.0f);
        g.setColour (kAccentGold);
        g.setFont (juce::Font (11.5f, juce::Font::bold));
        g.drawText (title, r.getX() + 10, r.getY() + 6, r.getWidth() - 20, 16, juce::Justification::centredLeft);
    };

    drawPanel ({ 12,  64, 240, 390 }, "VOCAL MIC MODELER");
    drawPanel ({ 262, 64, 576, 390 }, "3D ACOUSTIC STAGE & ROOM MICS");
    drawPanel ({ 848, 64, 280, 390 }, "ECHO CHAMBERS & CUSTOM IR");
    drawPanel ({ 12,  464, 1116, 264 }, "ANALOG CONSOLE STRIP (PREAMP / EQ / COMP / PARALLEL / MASTER)");

    // Medidores VU y GR en barra superior derecha
    const auto meterArea = juce::Rectangle<int> (getWidth() - 210, 12, 198, 30);
    g.setColour (juce::Colour (0xff0d0f12));
    g.fillRoundedRectangle (meterArea.toFloat(), 4.0f);

    const float vuNorm = juce::jlimit (0.0f, 1.0f, (displayVuDb + 48.0f) / 48.0f);
    const float grNorm = juce::jlimit (0.0f, 1.0f, displayGrDb / 20.0f);

    g.setColour (kAccentCyan);
    g.fillRect (meterArea.getX() + 34, meterArea.getY() + 5, static_cast<int> (154.0f * vuNorm), 8);
    g.setColour (kAccentGold);
    g.fillRect (meterArea.getX() + 34, meterArea.getY() + 17, static_cast<int> (154.0f * grNorm), 8);

    g.setColour (kTextDim);
    g.setFont (10.0f);
    g.drawText ("OUT", meterArea.getX() + 4, meterArea.getY() + 3, 28, 12, juce::Justification::centredLeft);
    g.drawText ("GR",  meterArea.getX() + 4, meterArea.getY() + 15, 28, 12, juce::Justification::centredLeft);
}

void BenBocazStudioProEditor::resized()
{
    // 1. Barra superior
    presetBox.setBounds      (236, 14, 195, 26);
    modeBox.setBounds        (437, 14, 78,  26);
    chainOrderBox.setBounds  (521, 14, 140, 26);
    hqModeBtn.setBounds      (667, 14, 108, 26);
    phaseAlignBtn.setBounds  (777, 14, 92,  26);
    phaseInvertBtn.setBounds (870, 14, 58,  26);

    // 2. Vocal Mic Modeler (x: 12..252, y: 64..454)
    vocalModOnBtn.setBounds  (24, 90, 216, 24);
    vocalSrcBox.setBounds    (24, 124, 216, 26);
    vocalTgtBox.setBounds    (24, 158, 216, 26);
    vocalInLbl.setBounds     (28, 204, 96, 16);
    vocalInSlider.setBounds  (28, 222, 96, 96);
    vocalOutLbl.setBounds    (136, 204, 96, 16);
    vocalOutSlider.setBounds (136, 222, 96, 96);

    // 3. Sala 3D y Escenario (x: 262..838, y: 64..454)
    roomOnBtn.setBounds     (274, 88, 86, 24);
    mainStereoBtn.setBounds (362, 88, 102, 24);
    linkFarBtn.setBounds    (468, 88, 84, 24);
    sourceBox.setBounds     (274, 118, 134, 24);
    monitorBox.setBounds    (414, 118, 134, 24);
    roomBox.setBounds       (554, 118, 138, 24);
    wallsBox.setBounds      (698, 118, 128, 24);

    stageView.setBounds     (274, 150, 356, 292);

    // Controles laterales de sala (PAIR WIDTH + AIR/TAIL + 4 Mic Gains)
    pairSpreadLbl.setBounds     (640, 152, 88, 14);
    pairSpreadSlider.setBounds  (640, 166, 88, 84);
    roomAirTailLbl.setBounds    (734, 152, 88, 14);
    roomAirTailSlider.setBounds (734, 166, 88, 84);

    for (int i = 0; i < 4; ++i)
    {
        const int col = i % 2;
        const int row = i / 2;
        const int x = 640 + col * 94;
        const int y = 258 + row * 94;
        micGainLabels[static_cast<size_t> (i)].setBounds  (x, y, 64, 14);
        micSoloBtns[static_cast<size_t> (i)].setBounds    (x + 64, y - 2, 22, 18);
        micGainSliders[static_cast<size_t> (i)].setBounds (x, y + 16, 88, 76);
    }

    // 4. Echo Chambers (x: 848..1128, y: 64..454)
    chamberOnBtn.setBounds   (860, 88, 110, 24);
    loadIrBtn.setBounds      (980, 88, 134, 24);
    chamberTypeBox.setBounds (860, 118, 254, 24);
    irNameLbl.setBounds      (860, 146, 254, 16);

    chamberMixLbl.setBounds      (868, 174, 110, 14);
    chamberMixSlider.setBounds   (868, 190, 110, 96);
    chamberDecayLbl.setBounds    (992, 174, 110, 14);
    chamberDecaySlider.setBounds (992, 190, 110, 96);

    chamberPreLbl.setBounds      (868, 304, 110, 14);
    chamberPreSlider.setBounds   (868, 320, 110, 96);
    chamberWidthLbl.setBounds    (992, 304, 110, 14);
    chamberWidthSlider.setBounds (992, 320, 110, 96);

    // 5. Consola inferior (x: 12..1128, y: 464..728)
    // Preamp
    tubeModelBox.setBounds (24, 492, 122, 24);
    driveLbl.setBounds     (34, 534, 100, 14);
    driveSlider.setBounds  (34, 552, 100, 106);

    // EQ
    eqOnBtn.setBounds       (162, 492, 72, 24);
    eqTypeBox.setBounds     (238, 492, 140, 24);
    eqHpfLbl.setBounds      (162, 534, 82, 14);
    eqHpfSlider.setBounds   (162, 552, 82, 96);

    eqLowFreqBox.setBounds  (252, 656, 82, 22);
    eqLowLbl.setBounds      (252, 534, 82, 14);
    eqLowSlider.setBounds   (252, 552, 82, 96);

    eqMidFreqBox.setBounds  (342, 656, 82, 22);
    eqMidLbl.setBounds      (342, 534, 82, 14);
    eqMidSlider.setBounds   (342, 552, 82, 96);

    eqHighFreqBox.setBounds (432, 656, 82, 22);
    eqHighLbl.setBounds     (432, 534, 82, 14);
    eqHighSlider.setBounds  (432, 552, 82, 96);

    // Compresor
    compOnBtn.setBounds       (536, 492, 86, 24);
    compTypeBox.setBounds     (626, 492, 146, 24);
    compThreshLbl.setBounds   (540, 534, 96, 14);
    compThreshSlider.setBounds(540, 552, 96, 96);
    compMixLbl.setBounds      (646, 534, 96, 14);
    compMixSlider.setBounds   (646, 552, 96, 96);
    compScLinkBtn.setBounds   (540, 656, 90, 22);
    compScFilterBtn.setBounds (646, 656, 90, 22);

    // Parallel Crush
    parCompOnBtn.setBounds    (792, 492, 100, 24);
    parCompRouteBox.setBounds (896, 492, 100, 24);
    parCompAmtLbl.setBounds   (842, 534, 104, 14);
    parCompAmtSlider.setBounds(842, 552, 104, 106);

    // Output Master
    outputGainLbl.setBounds   (1010, 534, 104, 14);
    outputGainSlider.setBounds(1010, 552, 104, 106);
}
