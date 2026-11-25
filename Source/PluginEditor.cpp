#include "PluginEditor.h"

#if HAS_CUSTOM_ASSETS
 #include "BinaryData.h"
#endif

//==============================================================================
// RusticStudioLookAndFeel (Soporta acentos de color por perilla en EQ y Comp)
//==============================================================================
RusticStudioLookAndFeel::RusticStudioLookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff1a1511));
    setColour (juce::ComboBox::textColourId, juce::Colour (0xfff2e6d0));
    setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff6b563c));
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff1b1612));
    setColour (juce::PopupMenu::textColourId, juce::Colour (0xfff2e6d0));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xffd69d46));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colour (0xff14100d));
    setColour (juce::TextButton::textColourOffId, juce::Colour (0xfff2e6d0));
    setColour (juce::TextButton::textColourOnId, juce::Colour (0xff14100d));
}

void RusticStudioLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                                float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                                juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                          static_cast<float> (width), static_cast<float> (height)).reduced (7.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const float cx = bounds.getCentreX();
    const float cy = bounds.getCentreY();
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    const auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);

    juce::Path bgArc;
    bgArc.addCentredArc (cx, cy, radius + 3.5f, radius + 3.5f, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (juce::Colour (0xff342b22));
    g.strokePath (bgArc, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path valArc;
    valArc.addCentredArc (cx, cy, radius + 3.5f, radius + 3.5f, 0.0f, rotaryStartAngle, angle, true);
    g.setColour (accent);
    g.strokePath (valArc, juce::PathStrokeType (2.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour (juce::Colours::black.withAlpha (0.65f));
    g.fillEllipse (cx - radius + 1.5f, cy - radius + 3.5f, radius * 2.0f, radius * 2.0f);

    juce::ColourGradient body (juce::Colour (0xff443a31), cx - radius * 0.5f, cy - radius * 0.5f,
                               juce::Colour (0xff15110e), cx + radius * 0.6f, cy + radius * 0.6f, true);
    g.setGradientFill (body);
    g.fillEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

    juce::ColourGradient cap (accent.darker (0.45f), cx - radius * 0.3f, cy - radius * 0.4f,
                              juce::Colour (0xff221c17), cx + radius * 0.4f, cy + radius * 0.4f, true);
    g.setGradientFill (cap);
    g.fillEllipse (cx - radius * 0.72f, cy - radius * 0.72f, radius * 1.44f, radius * 1.44f);

    g.setColour (accent.withAlpha (0.65f));
    g.drawEllipse (cx - radius * 0.72f, cy - radius * 0.72f, radius * 1.44f, radius * 1.44f, 1.1f);

    juce::Path pointer;
    pointer.addRoundedRectangle (-1.6f, -radius * 0.88f, 3.2f, radius * 0.52f, 1.6f);
    pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (cx, cy));
    g.setColour (juce::Colour (0xfffaf1df));
    g.fillPath (pointer);
}

void RusticStudioLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                            int, int, int, int, juce::ComboBox&)
{
    auto area = juce::Rectangle<float> (0.0f, 0.0f, static_cast<float> (width), static_cast<float> (height)).reduced (0.5f);
    g.setColour (juce::Colour (0xff18130f));
    g.fillRoundedRectangle (area, 4.0f);
    g.setColour (juce::Colour (0xff6b573e));
    g.drawRoundedRectangle (area, 4.0f, 1.0f);

    juce::Path arrow;
    const float ax = static_cast<float> (width) - 13.0f;
    const float ay = static_cast<float> (height) * 0.5f - 2.0f;
    arrow.addTriangle (ax, ay, ax + 7.0f, ay, ax + 3.5f, ay + 4.5f);
    g.setColour (juce::Colour (0xffe5a94c));
    g.fillPath (arrow);
}

void RusticStudioLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                                    const juce::Colour&, bool isHighlighted, bool isDown)
{
    auto area = button.getLocalBounds().toFloat().reduced (0.5f);
    const bool toggled = button.getToggleState();
    g.setColour (isDown || toggled ? juce::Colour (0xffd69d46)
                                   : (isHighlighted ? juce::Colour (0xff3c3024) : juce::Colour (0xff231c16)));
    g.fillRoundedRectangle (area, 4.0f);
    g.setColour (juce::Colour (0xffc99546));
    g.drawRoundedRectangle (area, 4.0f, 1.1f);
}

void RusticStudioLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                                bool isHighlighted, bool)
{
    auto area = button.getLocalBounds().toFloat().reduced (0.5f);
    const bool isOn = button.getToggleState();

    g.setColour (isOn ? juce::Colour (0xff3b2b18) : (isHighlighted ? juce::Colour (0xff29221c) : juce::Colour (0xff171310)));
    g.fillRoundedRectangle (area, 4.0f);
    g.setColour (isOn ? juce::Colour (0xffe8ab4c) : juce::Colour (0xff544535));
    g.drawRoundedRectangle (area, 4.0f, isOn ? 1.4f : 1.0f);

    const float ledR = 3.8f;
    const float ledX = area.getX() + 9.0f;
    const float ledY = area.getCentreY();

    if (isOn)
    {
        g.setColour (juce::Colour (0xffffb84d).withAlpha (0.35f));
        g.fillEllipse (ledX - ledR * 1.8f, ledY - ledR * 1.8f, ledR * 3.6f, ledR * 3.6f);
        g.setColour (juce::Colour (0xffffc866));
    }
    else
    {
        g.setColour (juce::Colour (0xff3d332a));
    }
    g.fillEllipse (ledX - ledR, ledY - ledR, ledR * 2.0f, ledR * 2.0f);

    g.setColour (isOn ? juce::Colour (0xfffaf0dc) : juce::Colour (0xff9c8e7d));
    g.setFont (juce::Font (juce::FontOptions (9.1f).withStyle ("Bold")));
    g.drawText (button.getButtonText(), area.withTrimmedLeft (16.0f).reduced (2.0f, 0.0f), juce::Justification::centredLeft);
}

//==============================================================================
// AnalogVuMeter
//==============================================================================
AnalogVuMeter::AnalogVuMeter (BenBocazStudioProProcessor& proc) : processor (proc) { startTimerHz (30); }

void AnalogVuMeter::timerCallback()
{
    float target = 0.0f;
    if (showGR)
        target = 1.0f - (std::clamp (processor.getCompGrDb(), 0.0f, 20.0f) / 20.0f);
    else
        target = juce::jmap (std::clamp (processor.getOutputVuDb(), -36.0f, 3.0f), -36.0f, 3.0f, 0.0f, 1.0f);

    smoothedNeedle = smoothedNeedle * 0.72f + target * 0.28f;
    repaint();
}

void AnalogVuMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (juce::Colour (0xff120e0b));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (juce::Colour (0xff786043));
    g.drawRoundedRectangle (r, 5.0f, 1.5f);

    auto glass = r.reduced (3.5f);
    juce::ColourGradient lamp (juce::Colour (0xfff8db96), glass.getCentreX(), glass.getBottom(),
                               juce::Colour (0xffb88130), glass.getCentreX(), glass.getY(), true);
    g.setGradientFill (lamp);
    g.fillRoundedRectangle (glass, 3.0f);

    const float pivotX = glass.getCentreX();
    const float pivotY = glass.getBottom() + 14.0f;
    const float rad = glass.getHeight() + 5.0f;

    juce::Path scaleArc;
    scaleArc.addCentredArc (pivotX, pivotY, rad * 0.86f, rad * 0.86f, 0.0f, -0.75f, 0.75f, true);
    g.setColour (juce::Colour (0xff2b1e0d));
    g.strokePath (scaleArc, juce::PathStrokeType (1.4f));

    juce::Path redArc;
    redArc.addCentredArc (pivotX, pivotY, rad * 0.86f, rad * 0.86f, 0.0f, 0.35f, 0.75f, true);
    g.setColour (juce::Colour (0xffb52318));
    g.strokePath (redArc, juce::PathStrokeType (2.4f));

    g.setColour (juce::Colour (0xff2b1e0d));
    g.setFont (juce::Font (juce::FontOptions (9.2f).withStyle ("Bold")));
    g.drawText (showGR ? "GR  dB" : "VU  OUT", glass.reduced (6.0f, 2.0f), juce::Justification::bottomLeft);
    g.drawText ("-20    -10    -5    0   +3", glass.reduced (8.0f, 3.0f), juce::Justification::centredTop);

    const float needleAngle = juce::jmap (smoothedNeedle, 0.0f, 1.0f, -0.75f, 0.75f);
    juce::Path needle;
    needle.startNewSubPath (pivotX, pivotY);
    needle.lineTo (pivotX + std::sin (needleAngle) * (rad * 0.92f),
                   pivotY - std::cos (needleAngle) * (rad * 0.92f));
    g.setColour (juce::Colour (0xff1a1209));
    g.strokePath (needle, juce::PathStrokeType (1.8f));
}

//==============================================================================
// Studio4MicStagePad (Composición 2.5D con Escalas Calibradas, Overlays y Tips)
//==============================================================================
Studio4MicStagePad::Studio4MicStagePad (BenBocazStudioProProcessor& proc)
    : processor (proc), apvts (proc.apvts)
{
    startTimerHz (24);
}

void Studio4MicStagePad::setSourceImage (int idx, const juce::Image& img)
{
    if (idx >= 0 && idx < 7) { sourceImgs[static_cast<size_t> (idx)] = img; repaint(); }
}

void Studio4MicStagePad::setMicImage (int idx, const juce::Image& img)
{
    if (idx >= 0 && idx < 4) { micImgs[static_cast<size_t> (idx)] = img; repaint(); }
}

void Studio4MicStagePad::centerStand (int idx)
{
    const juce::String pfx = "mic" + juce::String (std::clamp (idx, 0, 3) + 1);
    if (auto* pX = apvts.getParameter (pfx + "X"))
        pX->setValueNotifyingHost (0.5f);
}

juce::Point<float> Studio4MicStagePad::getStandBasePos (int idx, juce::Rectangle<float> area) const
{
    const juce::String pfx = "mic" + juce::String (idx + 1);
    const float mx = apvts.getRawParameterValue (pfx + "X")->load();
    const float my = apvts.getRawParameterValue (pfx + "Y")->load();

    const float yTop  = area.getY() + area.getHeight() * 0.66f;
    const float yBot  = area.getBottom() - 20.0f;
    const float sy    = juce::jmap (my, 0.0f, 1.0f, yTop, yBot);
    const float halfW = juce::jmap (my, 0.0f, 1.0f, area.getWidth() * 0.30f, area.getWidth() * 0.43f);
    const float sx    = area.getCentreX() + mx * halfW;
    return { sx, sy };
}

float Studio4MicStagePad::getStandScale (int idx) const
{
    const float my = apvts.getRawParameterValue ("mic" + juce::String (idx + 1) + "Y")->load();
    return juce::jmap (my, 0.0f, 1.0f, 0.72f, 1.14f);
}

float Studio4MicStagePad::getStandVisualHeight (int idx, float scale) const
{
    const float baseH = (idx == 2) ? 126.0f : (idx == 3 ? 112.0f : 100.0f);
    return baseH * scale;
}

int Studio4MicStagePad::findClosestStand (juce::Point<float> pt, juce::Rectangle<float> area, float& outMinDist) const
{
    outMinDist = 1e9f;
    int best = selectedStand;

    for (int i = 0; i < 4; ++i)
    {
        const auto base = getStandBasePos (i, area);
        const float sc  = getStandScale (i);
        const float vh  = getStandVisualHeight (i, sc);
        const juce::Point<float> topPt (base.x, base.y - vh * 0.82f);
        const juce::Point<float> midPt (base.x, base.y - vh * 0.42f);

        const float d = std::min ({ pt.getDistanceFrom (base), pt.getDistanceFrom (topPt), pt.getDistanceFrom (midPt) });
        if (d < outMinDist)
        {
            outMinDist = d;
            best = i;
        }
    }
    return best;
}

void Studio4MicStagePad::drawAmpFallback (juce::Graphics& g, float cx, float floorY, int sourceIdx, float scale, float alpha) const
{
    auto drawCabinet = [&] (float x, float y, float w, float h, juce::Colour tolex, juce::Colour grille, int speakers)
    {
        g.setColour (tolex.withAlpha (alpha));
        g.fillRoundedRectangle (x - w * 0.5f, y - h, w, h, 3.0f * scale);
        g.setColour (juce::Colour (0xffd6b372).withAlpha (0.85f * alpha));
        g.drawRoundedRectangle (x - w * 0.5f, y - h, w, h, 3.0f * scale, 1.2f);

        g.setColour (grille.withAlpha (0.92f * alpha));
        g.fillRect (x - w * 0.43f, y - h * 0.88f, w * 0.86f, h * 0.78f);

        g.setColour (juce::Colour (0xff14110f).withAlpha (0.55f * alpha));
        if (speakers == 1)
        {
            const float r = std::min (w, h) * 0.28f;
            g.fillEllipse (x - r, y - h * 0.48f - r, r * 2.0f, r * 2.0f);
        }
        else
        {
            const float r = std::min (w * 0.22f, h * 0.22f);
            g.fillEllipse (x - w * 0.22f - r, y - h * 0.48f - r, r * 2.0f, r * 2.0f);
            g.fillEllipse (x + w * 0.22f - r, y - h * 0.48f - r, r * 2.0f, r * 2.0f);
        }
    };

    if (sourceIdx == 4) // Guitar Cab Mono (Combo 1x12)
    {
        drawCabinet (cx, floorY, 50.0f * scale, 42.0f * scale, juce::Colour (0xff241c16), juce::Colour (0xff8a7152), 1);
    }
    else if (sourceIdx == 5) // Guitar Cab Stereo (Par de combos L/R lado a lado)
    {
        drawCabinet (cx - 32.0f * scale, floorY, 46.0f * scale, 38.0f * scale, juce::Colour (0xff241c16), juce::Colour (0xff8a7152), 1);
        drawCabinet (cx + 32.0f * scale, floorY, 46.0f * scale, 38.0f * scale, juce::Colour (0xff241c16), juce::Colour (0xff8a7152), 1);
    }
    else // Bass Amp (Stack 4x10 + Cabezal)
    {
        drawCabinet (cx, floorY, 56.0f * scale, 52.0f * scale, juce::Colour (0xff191716), juce::Colour (0xff3a3836), 2);
        g.setColour (juce::Colour (0xff2b2927).withAlpha (alpha));
        g.fillRoundedRectangle (cx - 26.0f * scale, floorY - 66.0f * scale, 52.0f * scale, 13.0f * scale, 2.0f);
        g.setColour (juce::Colour (0xffe5a94c).withAlpha (0.85f * alpha));
        g.drawRect (cx - 22.0f * scale, floorY - 62.0f * scale, 44.0f * scale, 5.0f * scale, 1.0f);
    }
}

void Studio4MicStagePad::drawTipsOverlay (juce::Graphics& g, juce::Rectangle<float> area, int sourceIdx) const
{
    const float w = std::min (265.0f, area.getWidth() * 0.44f);
    const float h = 136.0f;
    juce::Rectangle<float> card (area.getRight() - w - 10.0f, area.getY() + 30.0f, w, h);

    g.setColour (juce::Colour (0xff1a130d).withAlpha (0.93f));
    g.fillRoundedRectangle (card, 6.0f);
    g.setColour (juce::Colour (0xffe5a94c));
    g.drawRoundedRectangle (card, 6.0f, 1.3f);

    static const std::array<juce::String, 7> titles {
        "TIPS: VOCALS (3 TECHNIQUES)",
        "TIPS: DRUMS (3 TECHNIQUES)",
        "TIPS: ENSEMBLE (3 TECHNIQUES)",
        "TIPS: MONITORS (3 TECHNIQUES)",
        "TIPS: GUITAR MONO (3 TECHNIQUES)",
        "TIPS: GUITAR STEREO (3 TECHNIQUES)",
        "TIPS: BASS AMP (3 TECHNIQUES)"
    };

    static const std::array<juce::String, 7> bodies {
        "1. Single Mic Anchor: 1:MAIN Mono centered close, MID/AIR/DECCA at 0.\n"
        "2. Air & Intimacy: 1:MAIN close + 3:AIR soft at rear for natural halo.\n"
        "3. Open Room Depth: Move 1:MAIN back with PHASE SAFE + short Chamber.",

        "1. Coincident Overhead: 1:MAIN X-Y centered for punchy kit focus.\n"
        "2. Studio Room Punch: 2:MID close/mid + 3:AIR wide for cymbals.\n"
        "3. Bonham Huge Room: Add 4:DECCA far back + DE-MUD + Parallel Crush.",

        "1. Single Acoustic Photo: 1:MAIN X-Y centered at medium distance.\n"
        "2. Wood Width: 1:MAIN anchor + 2:MID / 3:AIR spaced pair.\n"
        "3. Orchestral Depth: 1:MAIN + 4:DECCA wide at rear for 3D tail.",

        "1. Reference Focus: 1:MAIN centered to capture direct speaker punch.\n"
        "2. Controlled Room: 1:MAIN X-Y + 2:MID low for natural wall bounce.\n"
        "3. Swedien Wide Stage: Push 2:MID + 3:AIR / 4:DECCA for lush spread.",

        "1. Cone Edge Bite: 1:MAIN Mono close & centered for tight riffs.\n"
        "2. Amp + Room Air: 1:MAIN close + 3:AIR far back for organic body.\n"
        "3. Symmetrical Depth: Enable LINK 3+4 with AIR & DECCA mirrored.",

        "1. Stereo Cab Capture: 1:MAIN X-Y centered to preserve chorus/delay.\n"
        "2. Wide A/B Stage: 2:MID + 3:AIR with PAIR WIDE open on both cabs.\n"
        "3. Huge Live Wall: Add 4:DECCA at rear + Chamber 3 for 3D width.",

        "1. Sub Core Anchor: 1:MAIN Mono close/center, ambient mics low.\n"
        "2. Growl + Wood Body: 1:MAIN + 2:MID with DE-MUD to avoid low buildup.\n"
        "3. Warm Room Weight: Subtle 4:DECCA far back with PHASE SAFE locked."
    };

    const size_t idx = static_cast<size_t> (std::clamp (sourceIdx, 0, 6));
    auto inner = card.reduced (9.0f, 7.0f);

    g.setColour (juce::Colour (0xfff4c370));
    g.setFont (juce::Font (juce::FontOptions (9.4f).withStyle ("Bold")));
    g.drawText (titles[idx], inner.removeFromTop (15.0f), juce::Justification::centredLeft);

    g.setColour (juce::Colour (0xffe8dcc6));
    g.setFont (juce::Font (juce::FontOptions (8.7f)));
    g.drawFittedText (bodies[idx], inner.toNearestInt(), juce::Justification::topLeft, 9);
}

void Studio4MicStagePad::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat().reduced (2.0f);
    const bool roomIsOn   = apvts.getRawParameterValue ("roomOn")->load() >= 0.5f;
    const int roomIdx     = static_cast<int> (apvts.getRawParameterValue ("room")->load());
    const int wallsLive   = static_cast<int> (apvts.getRawParameterValue ("walls")->load());
    const int sourceIdx   = static_cast<int> (apvts.getRawParameterValue ("source")->load());
    const int monitorIdx  = static_cast<int> (apvts.getRawParameterValue ("monitorType")->load());
    const bool phaseOk    = apvts.getRawParameterValue ("phaseAlign")->load() >= 0.5f;
    const bool mainStereo = apvts.getRawParameterValue ("mainStereo")->load() >= 0.5f;
    const float pairW     = apvts.getRawParameterValue ("pairSpread")->load();
    const float deccaW    = apvts.getRawParameterValue ("deccaWidth")->load();

    juce::Path clip;
    clip.addRoundedRectangle (area, 6.0f);
    g.saveState();
    g.reduceClipRegion (clip);

    // 1. Fondo fotográfico real (Shangri-La o Canyon)
    const juce::Image& activeRoom = (roomIdx == 1 && canyonImg.isValid()) ? canyonImg
                                                                          : (shangriLaImg.isValid() ? shangriLaImg : canyonImg);
    if (activeRoom.isValid())
    {
        g.drawImage (activeRoom, area, juce::RectanglePlacement::fillDestination);
        const float dimAlpha = ! roomIsOn ? 0.72f : (wallsLive ? 0.08f : 0.30f);
        g.setColour (juce::Colour (0xff140e09).withAlpha (dimAlpha));
        g.fillRect (area);
    }
    else
    {
        juce::ColourGradient wood (roomIdx == 0 ? juce::Colour (0xff4a311e) : juce::Colour (0xff2e1c11), area.getTopLeft(),
                                   juce::Colour (0xff170f0a), area.getBottomRight(), false);
        g.setGradientFill (wood);
        g.fillRect (area);
    }

    const float stageAlpha = roomIsOn ? 1.0f : 0.35f;

    // 2. Fuente Sonora Proporcionada y Separada de la Pared Trasera (sobre la alfombra)
    const float srcFloorX = area.getCentreX();
    const float srcFloorY = area.getY() + area.getHeight() * 0.71f;
    const float roomScale = (roomIdx == 1) ? 0.78f : 0.94f;

    g.setColour (juce::Colours::black.withAlpha (0.45f * stageAlpha));
    g.fillEllipse (srcFloorX - 50.0f * roomScale, srcFloorY - 6.0f, 100.0f * roomScale, 14.0f);

    const size_t sIdx = static_cast<size_t> (std::clamp (sourceIdx, 0, 6));
    const auto& srcImg = sourceImgs[sIdx];
    if (srcImg.isValid())
    {
        static constexpr std::array<float, 7> srcHeightRatios { 0.31f, 0.27f, 0.28f, 0.27f, 0.23f, 0.23f, 0.26f };
        const float drawH  = area.getHeight() * srcHeightRatios[sIdx] * roomScale;
        const float aspect = static_cast<float> (srcImg.getWidth()) / std::max (1.0f, static_cast<float> (srcImg.getHeight()));
        const float drawW  = drawH * aspect;

        juce::Rectangle<float> srcDest (srcFloorX - drawW * 0.5f, srcFloorY - drawH + 3.0f, drawW, drawH);
        g.setOpacity (stageAlpha);
        g.drawImage (srcImg, srcDest, juce::RectanglePlacement::stretchToFit);
        g.setOpacity (1.0f);
    }
    else if (sourceIdx >= 4)
    {
        drawAmpFallback (g, srcFloorX, srcFloorY, sourceIdx, roomScale, stageAlpha);
    }

    // Etiqueta limpia de la fuente
    static const std::array<juce::String, 7> srcLabels {
        "SOURCE: VOCALS", "SOURCE: DRUMS", "SOURCE: ENSEMBLE", "MONITORS",
        "GUITAR CAB (MONO)", "GUITAR CAB (STEREO)", "BASS AMP (LOW END)"
    };
    juce::String srcLbl = srcLabels[sIdx];
    if (sourceIdx == 3)
        srcLbl = (monitorIdx == 0 ? "MONITORS: ALTEC" : (monitorIdx == 1 ? "MONITORS: NS-10" : "MONITORS: JBL PA"));

    g.setColour (juce::Colour (0xff120d09).withAlpha (0.82f * stageAlpha));
    g.fillRoundedRectangle (srcFloorX - 66.0f, area.getY() + 28.0f, 132.0f, 14.5f, 3.5f);
    g.setColour (juce::Colour (0xfff2e6d0).withAlpha (0.88f * stageAlpha));
    g.drawRoundedRectangle (srcFloorX - 66.0f, area.getY() + 28.0f, 132.0f, 14.5f, 3.5f, 0.9f);
    g.setFont (juce::Font (juce::FontOptions (8.2f).withStyle ("Bold")));
    g.drawText (srcLbl, static_cast<int> (srcFloorX - 66), static_cast<int> (area.getY() + 28), 132, 15, juce::Justification::centred);

    // Guía visual de enlace simétrico entre AIR (3) y DECCA (4)
    if (linkAirDecca && roomIsOn)
    {
        const auto p3 = getStandBasePos (2, area);
        const auto p4 = getStandBasePos (3, area);
        g.setColour (juce::Colour (0xff8fc7e8).withAlpha (0.45f));
        float dash[2] = { 4.0f, 4.0f };
        g.drawDashedLine (juce::Line<float> (p3, p4), dash, 2, 1.3f);
    }

    // 3. Ordenar los 4 atriles de atrás hacia adelante (Z-sorting real por distancia)
    std::array<int, 4> order { 0, 1, 2, 3 };
    std::sort (order.begin(), order.end(), [this] (int a, int b)
    {
        const float ya = apvts.getRawParameterValue ("mic" + juce::String (a + 1) + "Y")->load();
        const float yb = apvts.getRawParameterValue ("mic" + juce::String (b + 1) + "Y")->load();
        return ya < yb;
    });

    static const std::array<juce::String, 4> baseNames { "MAIN", "MID", "AIR", "DECCA" };
    static const std::array<juce::Colour, 4> cols {
        juce::Colour (0xfff2b24c), // 1: MAIN
        juce::Colour (0xffe07a52), // 2: MID
        juce::Colour (0xff8fc7e8), // 3: AIR
        juce::Colour (0xfff5ebd6)  // 4: DECCA
    };

    const float roomD = (roomIdx == 0) ? 12.0f : 8.5f;
    const int soloIdx = processor.getSoloStand();

    for (int idx : order)
    {
        const auto base   = getStandBasePos (idx, area);
        const float sc    = getStandScale (idx);
        const float vh    = getStandVisualHeight (idx, sc);
        const float headX = base.x;
        const float headY = base.y - vh * 0.84f;

        const float gain  = apvts.getRawParameterValue ("mic" + juce::String (idx + 1) + "Gain")->load();
        const float my    = apvts.getRawParameterValue ("mic" + juce::String (idx + 1) + "Y")->load();
        const float distM = juce::jmap (my, 0.0f, 1.0f, 0.8f, roomD - 1.5f);
        const bool mutedBySolo = (soloIdx >= 0 && soloIdx != idx);
        const float alpha = ((gain > 0.01f && ! mutedBySolo) ? 1.0f : 0.26f) * stageAlpha;
        const auto col    = cols[static_cast<size_t> (idx)];
        const bool active = (idx == selectedStand || idx == hoveredStand) && roomIsOn;

        // Sombra proyectada y halo interactivo de piso
        const float haloRx = (active ? 24.0f : 18.0f) * sc;
        const float haloRy = (active ? 8.0f  : 5.8f)  * sc;

        g.setColour (juce::Colours::black.withAlpha (0.55f * alpha));
        g.fillEllipse (base.x - 15.0f * sc, base.y - 4.0f * sc, 30.0f * sc, 9.0f * sc);

        g.setColour (col.withAlpha ((active ? 0.30f : 0.14f) * alpha));
        g.fillEllipse (base.x - haloRx, base.y - haloRy, haloRx * 2.0f, haloRy * 2.0f);
        g.setColour (col.withAlpha ((active ? 0.95f : 0.58f) * alpha));
        g.drawEllipse (base.x - haloRx, base.y - haloRy, haloRx * 2.0f, haloRy * 2.0f, active ? 1.8f : 1.1f);

        int micSpriteIdx = 0;
        float widthStretch = 1.0f;
        if (idx == 0)
            micSpriteIdx = mainStereo ? 1 : 0;
        else if (idx == 1 || idx == 2)
        {
            micSpriteIdx = (pairW < 0.08f) ? 0 : 2;
            widthStretch = (pairW < 0.08f) ? 0.92f : juce::jmap (pairW, 0.08f, 1.5f, 0.68f, 1.32f);
        }
        else
        {
            micSpriteIdx = 3;
            widthStretch = juce::jmap (deccaW, 0.2f, 1.5f, 0.70f, 1.35f);
        }

        const auto& mImg = micImgs[static_cast<size_t> (micSpriteIdx)];

        const float ringR = (idx == 3 ? 21.0f : 16.0f) * sc;
        g.setColour (col.withAlpha ((active ? 0.26f : 0.14f) * alpha));
        g.fillEllipse (headX - ringR, headY - ringR, ringR * 2.0f, ringR * 2.0f);
        g.setColour (col.withAlpha ((active ? 0.92f : 0.65f) * alpha));
        g.drawEllipse (headX - ringR, headY - ringR, ringR * 2.0f, ringR * 2.0f, active ? 1.7f : 1.1f);

        if (mImg.isValid())
        {
            const float aspect = static_cast<float> (mImg.getWidth()) / std::max (1.0f, static_cast<float> (mImg.getHeight()));
            const float drawW  = vh * aspect * widthStretch;
            juce::Rectangle<float> dest (base.x - drawW * 0.5f, base.y - vh + 4.0f * sc, drawW, vh);

            g.setOpacity (alpha);
            g.drawImage (mImg, dest, juce::RectanglePlacement::stretchToFit);
            g.setOpacity (1.0f);
        }

        // Etiqueta flotante del atril
        juce::String tag = juce::String (idx + 1) + ":" + baseNames[static_cast<size_t> (idx)];
        if (idx == 0) tag += (mainStereo ? " X-Y" : " MONO");
        tag += " " + juce::String (distM, 1) + "m";

        const float tagW = (idx == 0) ? 96.0f : 74.0f;
        const float tagY = std::max (area.getY() + 26.0f, base.y - vh - 13.0f);
        g.setColour (juce::Colour (0xff120d0a).withAlpha (0.86f * stageAlpha));
        g.fillRoundedRectangle (headX - tagW * 0.5f, tagY, tagW, 13.5f, 3.0f);
        g.setColour (col.withAlpha (active ? 1.0f : 0.85f * alpha));
        g.drawRoundedRectangle (headX - tagW * 0.5f, tagY, tagW, 13.5f, 3.0f, active ? 1.2f : 0.7f);
        g.setFont (juce::Font (juce::FontOptions (8.1f).withStyle ("Bold")));
        g.drawText (tag, static_cast<int> (headX - tagW * 0.5f), static_cast<int> (tagY),
                    static_cast<int> (tagW), 14, juce::Justification::centred);
    }

    // 4. Overlay de Coordenadas Numéricas del Atril Seleccionado
    if (roomIsOn)
    {
        const juce::String selPfx = "mic" + juce::String (selectedStand + 1);
        const float selX = apvts.getRawParameterValue (selPfx + "X")->load();
        const float selY = apvts.getRawParameterValue (selPfx + "Y")->load();
        const float selDist = juce::jmap (selY, 0.0f, 1.0f, 0.8f, roomD - 1.5f);

        juce::String panStr = "CENTER (0.00)";
        if (selX < -0.02f) panStr = juce::String (std::abs (selX) * 100.0f, 0) + "% L";
        else if (selX > 0.02f) panStr = juce::String (selX * 100.0f, 0) + "% R";

        juce::Rectangle<float> hud (area.getRight() - 186.0f, area.getBottom() - 44.0f, 178.0f, 36.0f);
        g.setColour (juce::Colour (0xff120d09).withAlpha (0.84f));
        g.fillRoundedRectangle (hud, 4.0f);
        g.setColour (cols[static_cast<size_t> (selectedStand)].withAlpha (0.8f));
        g.drawRoundedRectangle (hud, 4.0f, 1.0f);

        g.setFont (juce::Font (juce::FontOptions (8.4f).withStyle ("Bold")));
        g.drawText ("SELECTED: " + juce::String (selectedStand + 1) + ":" + baseNames[static_cast<size_t> (selectedStand)],
                    hud.reduced (7.0f, 3.0f).removeFromTop (14.0f), juce::Justification::centredLeft);
        g.setColour (juce::Colour (0xffe8dcc6));
        g.drawText ("DIST: " + juce::String (selDist, 1) + "m  |  PAN: " + panStr,
                    hud.reduced (7.0f, 3.0f).removeFromBottom (14.0f), juce::Justification::centredLeft);
    }

    // 5. Overlay de Guía de Técnicas ("i TIPS")
    if (showTipsOverlay)
        drawTipsOverlay (g, area, sourceIdx);

    g.restoreState();

    // Borde exterior de la ventana del estudio
    g.setColour ((roomIsOn && wallsLive) ? juce::Colour (0xffd9a04a) : juce::Colour (0xff5c4f41));
    g.drawRoundedRectangle (area, 6.0f, (roomIsOn && wallsLive) ? 2.0f : 1.4f);

    // Barra superior de estado (ASCII limpio)
    auto topBar = area.reduced (8.0f).removeFromTop (16.0f);
    g.setColour (juce::Colour (0xff140f0b).withAlpha (0.82f));
    g.fillRoundedRectangle (topBar.expanded (4.0f, 2.0f), 3.0f);

    g.setColour (juce::Colour (0xfff4d08c).withAlpha (roomIsOn ? 1.0f : 0.55f));
    g.setFont (juce::Font (juce::FontOptions (9.6f).withStyle ("Bold")));
    juce::String roomTitle = (roomIdx == 0 ? "STUDIO A (SHANGRI-LA)" : "STUDIO B (CANYON WOOD)");
    roomTitle += (wallsLive ? "  |  LIVE" : "  |  DAMPED");
    g.drawText (roomTitle, topBar, juce::Justification::centredLeft);

    if (! roomIsOn)
    {
        g.setColour (juce::Colour (0xffc99e6b));
        g.drawText ("ROOM BYPASSED (DIRECT CHANNEL STRIP)", topBar, juce::Justification::centredRight);
    }
    else
    {
        g.setColour (phaseOk ? juce::Colour (0xff8ce08e) : juce::Colour (0xffe8a94c));
        g.drawText (phaseOk ? "[OK] PHASE LOCKED" : "[!] FREE DELAY", topBar, juce::Justification::centredRight);
    }
}

void Studio4MicStagePad::mouseMove (const juce::MouseEvent& e)
{
    auto area = getLocalBounds().toFloat().reduced (2.0f);
    float dist = 0.0f;
    const int nearest = findClosestStand (e.position, area, dist);
    const int newHover = (dist < 40.0f) ? nearest : -1;

    if (newHover != hoveredStand)
    {
        hoveredStand = newHover;
        setMouseCursor (hoveredStand >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void Studio4MicStagePad::mouseExit (const juce::MouseEvent&)
{
    hoveredStand = -1;
    if (! isDragging) setMouseCursor (juce::MouseCursor::NormalCursor);
    repaint();
}

void Studio4MicStagePad::mouseDown (const juce::MouseEvent& e)
{
    auto area = getLocalBounds().toFloat().reduced (2.0f);
    float dist = 0.0f;
    selectedStand = findClosestStand (e.position, area, dist);
    isDragging = true;
    setMouseCursor (juce::MouseCursor::DraggingHandCursor);

    const auto currentBase = getStandBasePos (selectedStand, area);
    if (dist < 48.0f)
        dragOffset = currentBase - e.position;
    else
    {
        dragOffset = { 0.0f, 0.0f };
        updateStandFromBasePoint (selectedStand, e.position, area);
    }
    repaint();
}

void Studio4MicStagePad::mouseDoubleClick (const juce::MouseEvent& e)
{
    auto area = getLocalBounds().toFloat().reduced (2.0f);
    float dist = 0.0f;
    const int nearest = findClosestStand (e.position, area, dist);
    if (dist < 48.0f)
    {
        selectedStand = nearest;
        centerStand (nearest);
    }
}

void Studio4MicStagePad::mouseDrag (const juce::MouseEvent& e)
{
    if (! isDragging) return;
    auto area = getLocalBounds().toFloat().reduced (2.0f);
    updateStandFromBasePoint (selectedStand, e.position + dragOffset, area);
}

void Studio4MicStagePad::mouseUp (const juce::MouseEvent& e)
{
    isDragging = false;
    mouseMove (e);
}

void Studio4MicStagePad::updateStandFromBasePoint (int idx, juce::Point<float> targetBase, juce::Rectangle<float> area)
{
    const float yTop = area.getY() + area.getHeight() * 0.66f;
    const float yBot = area.getBottom() - 20.0f;

    const float normY = juce::jlimit (0.0f, 1.0f, (targetBase.y - yTop) / std::max (1.0f, yBot - yTop));
    const float halfW = juce::jmap (normY, 0.0f, 1.0f, area.getWidth() * 0.30f, area.getWidth() * 0.43f);
    float normX = juce::jlimit (0.0f, 1.0f, 0.5f + 0.5f * ((targetBase.x - area.getCentreX()) / std::max (1.0f, halfW)));

    // Imantado preciso al eje central (X = 0)
    if (snapToCenter && std::abs (normX - 0.5f) < 0.032f)
        normX = 0.5f;

    const juce::String pfx = "mic" + juce::String (idx + 1);
    if (auto* pX = apvts.getParameter (pfx + "X")) pX->setValueNotifyingHost (normX);
    if (auto* pY = apvts.getParameter (pfx + "Y")) pY->setValueNotifyingHost (normY);

    // Enlace simétrico de distancia e imagen espejo entre 3:AIR y 4:DECCA
    if (linkAirDecca && (idx == 2 || idx == 3))
    {
        const int partner = (idx == 2) ? 3 : 2;
        const juce::String partnerPfx = "mic" + juce::String (partner + 1);
        if (auto* ppX = apvts.getParameter (partnerPfx + "X")) ppX->setValueNotifyingHost (1.0f - normX);
        if (auto* ppY = apvts.getParameter (partnerPfx + "Y")) ppY->setValueNotifyingHost (normY);
    }
}

//==============================================================================
// BenBocazStudioProEditor
//==============================================================================
BenBocazStudioProEditor::BenBocazStudioProEditor (BenBocazStudioProProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), vuMeter (p), roomPad (p)
{
    setLookAndFeel (&rusticLAF);
    setSize (1260, 790);

    loadEmbeddedImages();

    addAndMakeVisible (vuMeter);
    addAndMakeVisible (roomPad);

    // Barra de Presets y Life Hacks
    presetLbl.setText ("PRESETS", juce::dontSendNotification);
    presetLbl.setColour (juce::Label::textColourId, juce::Colour (0xffe5a94c));
    presetLbl.setFont (juce::Font (juce::FontOptions (9.6f).withStyle ("Bold")));
    addAndMakeVisible (presetLbl);

    presetBox.addItemList ({
        "01. Clean Main Centered (Simple)",
        "02. Vocal C-800G Warm & Air",
        "03. Shangri-La Live Drums",
        "04. Canyon Guitar Stereo",
        "05. Bass Amp Low Weight",
        "06. Swedien NS-10 Re-Mic",
        "07. Room Instant Glue (1-Click)"
    }, 1);
    presetBox.setSelectedItemIndex (1, juce::dontSendNotification);
    presetBox.onChange = [this] { applyFactoryPreset (presetBox.getSelectedItemIndex()); };
    addAndMakeVisible (presetBox);

    roomGlueBtn.onClick = [this] { applyFactoryPreset (6); presetBox.setSelectedItemIndex (6, juce::dontSendNotification); };
    addAndMakeVisible (roomGlueBtn);
    addAndMakeVisible (deMudBtn);

    // Controles de precisión, Tips y Vista Expandida
    snapCenterBtn.setToggleState (true, juce::dontSendNotification);
    snapCenterBtn.onClick = [this] { roomPad.setSnapToCenter (snapCenterBtn.getToggleState()); };
    addAndMakeVisible (snapCenterBtn);

    link34Btn.onClick = [this] { roomPad.setLinkAirDecca (link34Btn.getToggleState()); };
    addAndMakeVisible (link34Btn);

    centerSelBtn.onClick = [this] { roomPad.centerStand (roomPad.getSelectedStand()); };
    addAndMakeVisible (centerSelBtn);

    tipsBtn.setClickingTogglesState (true);
    tipsBtn.onClick = [this] { roomPad.setShowTipsOverlay (tipsBtn.getToggleState()); };
    addAndMakeVisible (tipsBtn);

    expandStageBtn.setClickingTogglesState (true);
    expandStageBtn.onClick = [this]
    {
        stageExpanded = expandStageBtn.getToggleState();
        expandStageBtn.setButtonText (stageExpanded ? "SHOW RACKS" : "EXPAND STAGE");
        resized();
        repaint();
    };
    addAndMakeVisible (expandStageBtn);

    // Botones Solo por atril
    auto setupSolo = [this] (juce::TextButton& b, int idx)
    {
        b.setClickingTogglesState (true);
        b.onClick = [this, idx]
        {
            const int current = audioProcessor.getSoloStand();
            audioProcessor.setSoloStand (current == idx ? -1 : idx);
            updateSoloButtonsUI();
            roomPad.repaint();
        };
        addAndMakeVisible (b);
    };
    setupSolo (solo1Btn, 0);
    setupSolo (solo2Btn, 1);
    setupSolo (solo3Btn, 2);
    setupSolo (solo4Btn, 3);

    addAndMakeVisible (phaseAlignBtn);
    addAndMakeVisible (phaseInvBtn);
    addAndMakeVisible (vocalModBtn);
    addAndMakeVisible (mainStereoBtn);
    addAndMakeVisible (roomOnBtn);
    addAndMakeVisible (chamberOnBtn);
    addAndMakeVisible (eqOnBtn);
    addAndMakeVisible (eqTubeSyncBtn);
    addAndMakeVisible (compOnBtn);
    addAndMakeVisible (compScLinkBtn);
    addAndMakeVisible (compScFiltBtn);
    addAndMakeVisible (parCompOnBtn);

    vuModeBtn.onClick = [this]
    {
        const bool next = ! vuMeter.isShowingGainReduction();
        vuMeter.setShowGainReduction (next);
        vuModeBtn.setButtonText (next ? "VU: GR" : "VU: OUT");
    };
    addAndMakeVisible (vuModeBtn);

    loadIRBtn.onClick = [this] { openIRFileChooser(); };
    addAndMakeVisible (loadIRBtn);

    irNameLbl.setText (audioProcessor.getLoadedIRName(), juce::dontSendNotification);
    irNameLbl.setJustificationType (juce::Justification::centredLeft);
    irNameLbl.setColour (juce::Label::textColourId, juce::Colour (0xffc9b99e));
    irNameLbl.setFont (juce::Font (juce::FontOptions (9.5f)));
    addAndMakeVisible (irNameLbl);

    setupCombo (modeBox,      { "Reverb", "Re-Mic" }, modeLbl, "MODE");
    setupCombo (chainBox,     { "EQ -> Comp -> Chamber", "Comp -> EQ -> Chamber", "Chamber -> Comp -> EQ" }, chainLbl, "ROUTING");
    setupCombo (vocalSrcBox,  { "Shure SM27", "Shure SM57", "Shure SM7B", "Generic LDC" }, vocalSrcLbl, "YOUR INPUT MIC");
    setupCombo (vocalTgtBox,  { "Sony C-800G", "Neumann TLM 103", "Aston Origin" }, vocalTgtLbl, "TARGET VOCAL MIC");
    setupCombo (sourceBox,    { "Vocals", "Drums", "Ensemble", "Monitors (Swedien)", "Guitar Cab (Mono)", "Guitar Cab (Stereo)", "Bass Amp (Low End)" }, sourceLbl, "ROOM SOURCE");
    setupCombo (monitorBox,   { "Altec Main", "NS-10 Nearfield", "JBL PA Wedge" }, monitorLbl, "MONITOR SPEAKER");
    setupCombo (roomBox,      { "Studio A (Shangri-La)", "Studio B (Canyon)" }, roomLbl, "STUDIO ROOM");
    setupCombo (wallsBox,     { "Tight / Damped", "Live / Cedar Wood" }, wallsLbl, "ROOM WALLS");
    setupCombo (chamberBox,   { "Chamber 1 (Bright)", "Chamber 3 (Classic)", "Chamber 5 (Deep)", "Chamber 8 (Dense)", "Custom WAV IR" }, chamberLbl, "ECHO CHAMBER");
    setupCombo (eqTypeBox,    { "Neve 1073 (British)", "Pultec EQP-1A (Tube)", "API 550A (Punch)", "SSL 4000E (Clean)" }, eqTypeLbl, "EQ MODEL");
    setupCombo (eqTubeBox,    { "Neve Class-A", "Pultec 12AX7", "API 2520", "SSL Clean" }, eqTubeLbl, "TUBE CIRCUIT");
    setupCombo (compBox,      { "1176LN (FET)", "LA-2A (Opto)", "Mastering (Tube)" }, compLbl, "COMP MODEL");
    setupCombo (parRouteBox,  { "All Bus", "Room Mics", "Chamber" }, parRouteLbl, "PAR ROUTE");

    eqLowFreqBox.addItemList  ({ "60 Hz", "110 Hz", "220 Hz" }, 1);
    eqMidFreqBox.addItemList  ({ "700 Hz", "1.6 kHz", "3.2 kHz", "4.8 kHz" }, 1);
    eqHighFreqBox.addItemList ({ "10 kHz", "12 kHz", "16 kHz" }, 1);
    addAndMakeVisible (eqLowFreqBox);
    addAndMakeVisible (eqMidFreqBox);
    addAndMakeVisible (eqHighFreqBox);

    setupKnob (vocalInSlider,    vocalInLbl,    "MIC IN");
    setupKnob (vocalOutSlider,   vocalOutLbl,   "MIC OUT");
    setupKnob (driveSlider,      driveLbl,      "TUBE / TAPE DRIVE");
    setupKnob (mic1GainSlider,   mic1GainLbl,   "1: MAIN", juce::Colour (0xfff2b24c));
    setupKnob (mic2GainSlider,   mic2GainLbl,   "2: MID",  juce::Colour (0xffe07a52));
    setupKnob (mic3GainSlider,   mic3GainLbl,   "3: AIR",  juce::Colour (0xff8fc7e8));
    setupKnob (mic4GainSlider,   mic4GainLbl,   "4: DECCA",juce::Colour (0xfff5ebd6));
    setupKnob (pairWidthSlider,  pairWidthLbl,  "PAIR WIDE");
    setupKnob (deccaWidthSlider, deccaWidthLbl, "DECCA WIDE");
    setupKnob (chMixSlider,      chMixLbl,      "SEND");
    setupKnob (chDecaySlider,    chDecayLbl,    "DECAY");
    setupKnob (chPreSlider,      chPreLbl,      "PRE-DELAY");
    setupKnob (chWidthSlider,    chWidthLbl,    "WIDTH");

    // Perillas de EQ codificadas por color
    setupKnob (eqHpfSlider,  eqHpfLbl,  "HPF",  juce::Colour (0xffd6b372));
    setupKnob (eqLowSlider,  eqLowLbl,  "LOW",  juce::Colour (0xffe07a52));
    setupKnob (eqMidSlider,  eqMidLbl,  "MID",  juce::Colour (0xff78c284));
    setupKnob (eqHighSlider, eqHighLbl, "HIGH", juce::Colour (0xff7cc4e8));

    // Perillas de Compresor Principal + Paralelo
    setupKnob (compThreshSlider, compThreshLbl, "THRESHOLD");
    setupKnob (compMixSlider,    compMixLbl,    "MIX",      juce::Colour (0xff8fc7e8));
    setupKnob (parAmtSlider,     parAmtLbl,     "PAR BLEND",juce::Colour (0xffe07a52));
    setupKnob (outGainSlider,    outGainLbl,    "OUTPUT");

    auto& v = audioProcessor.apvts;
    deMudAtt      = std::make_unique<ButtonAtt> (v, "deMudOn", deMudBtn);
    phaseAlignAtt = std::make_unique<ButtonAtt> (v, "phaseAlign", phaseAlignBtn);
    phaseInvAtt   = std::make_unique<ButtonAtt> (v, "phaseInvert", phaseInvBtn);
    vocalModAtt   = std::make_unique<ButtonAtt> (v, "vocalModOn", vocalModBtn);
    mainStereoAtt = std::make_unique<ButtonAtt> (v, "mainStereo", mainStereoBtn);
    roomOnAtt     = std::make_unique<ButtonAtt> (v, "roomOn", roomOnBtn);
    chamberOnAtt  = std::make_unique<ButtonAtt> (v, "chamberOn", chamberOnBtn);
    eqOnAtt       = std::make_unique<ButtonAtt> (v, "eqOn", eqOnBtn);
    eqTubeSyncAtt = std::make_unique<ButtonAtt> (v, "eqTubeSync", eqTubeSyncBtn);
    compOnAtt     = std::make_unique<ButtonAtt> (v, "compOn", compOnBtn);
    compScLinkAtt = std::make_unique<ButtonAtt> (v, "compScLink", compScLinkBtn);
    compScFiltAtt = std::make_unique<ButtonAtt> (v, "compScFilter", compScFiltBtn);
    parCompOnAtt  = std::make_unique<ButtonAtt> (v, "parCompOn", parCompOnBtn);

    modeAtt       = std::make_unique<ComboAtt> (v, "mode", modeBox);
    chainAtt      = std::make_unique<ComboAtt> (v, "chainOrder", chainBox);
    vocalSrcAtt   = std::make_unique<ComboAtt> (v, "vocalSrcMic", vocalSrcBox);
    vocalTgtAtt   = std::make_unique<ComboAtt> (v, "vocalTgtMic", vocalTgtBox);
    sourceAtt     = std::make_unique<ComboAtt> (v, "source", sourceBox);
    monitorAtt    = std::make_unique<ComboAtt> (v, "monitorType", monitorBox);
    roomAtt       = std::make_unique<ComboAtt> (v, "room", roomBox);
    wallsAtt      = std::make_unique<ComboAtt> (v, "walls", wallsBox);
    chamberAtt    = std::make_unique<ComboAtt> (v, "chamberType", chamberBox);
    eqTypeAtt     = std::make_unique<ComboAtt> (v, "eqType", eqTypeBox);
    eqTubeAtt     = std::make_unique<ComboAtt> (v, "eqTubeModel", eqTubeBox);
    eqLowFreqAtt  = std::make_unique<ComboAtt> (v, "eqLowFreq", eqLowFreqBox);
    eqMidFreqAtt  = std::make_unique<ComboAtt> (v, "eqMidFreq", eqMidFreqBox);
    eqHighFreqAtt = std::make_unique<ComboAtt> (v, "eqHighFreq", eqHighFreqBox);
    compAtt       = std::make_unique<ComboAtt> (v, "compType", compBox);
    parRouteAtt   = std::make_unique<ComboAtt> (v, "parCompRoute", parRouteBox);

    // Sincronización automática de válvula al cambiar el EQ + Mono/Estéreo al cambiar fuente
    eqTypeBox.onChange = [this]
    {
        if (eqTubeSyncBtn.getToggleState())
            eqTubeBox.setSelectedItemIndex (eqTypeBox.getSelectedItemIndex(), juce::sendNotification);
    };

    sourceBox.onChange = [this]
    {
        const int srcIdx = sourceBox.getSelectedItemIndex();
        const bool isMonoSource = (srcIdx == 0 || srcIdx == 4 || srcIdx == 6);
        if (auto* pMainSt = audioProcessor.apvts.getParameter ("mainStereo"))
            pMainSt->setValueNotifyingHost (isMonoSource ? 0.0f : 1.0f);
        roomPad.repaint();
    };

    vocalInAtt    = std::make_unique<SliderAtt> (v, "vocalInGain", vocalInSlider);
    vocalOutAtt   = std::make_unique<SliderAtt> (v, "vocalOutGain", vocalOutSlider);
    driveAtt      = std::make_unique<SliderAtt> (v, "consoleDrive", driveSlider);
    mic1GainAtt   = std::make_unique<SliderAtt> (v, "mic1Gain", mic1GainSlider);
    mic2GainAtt   = std::make_unique<SliderAtt> (v, "mic2Gain", mic2GainSlider);
    mic3GainAtt   = std::make_unique<SliderAtt> (v, "mic3Gain", mic3GainSlider);
    mic4GainAtt   = std::make_unique<SliderAtt> (v, "mic4Gain", mic4GainSlider);
    pairWidthAtt  = std::make_unique<SliderAtt> (v, "pairSpread", pairWidthSlider);
    deccaWidthAtt = std::make_unique<SliderAtt> (v, "deccaWidth", deccaWidthSlider);
    chMixAtt      = std::make_unique<SliderAtt> (v, "chamberMix", chMixSlider);
    chDecayAtt    = std::make_unique<SliderAtt> (v, "chamberDecay", chDecaySlider);
    chPreAtt      = std::make_unique<SliderAtt> (v, "chamberPreDelay", chPreSlider);
    chWidthAtt    = std::make_unique<SliderAtt> (v, "chamberWidth", chWidthSlider);
    eqHpfAtt      = std::make_unique<SliderAtt> (v, "eqHpf", eqHpfSlider);
    eqLowAtt      = std::make_unique<SliderAtt> (v, "eqLow", eqLowSlider);
    eqMidAtt      = std::make_unique<SliderAtt> (v, "eqMid", eqMidSlider);
    eqHighAtt     = std::make_unique<SliderAtt> (v, "eqHigh", eqHighSlider);
    compThreshAtt = std::make_unique<SliderAtt> (v, "compThresh", compThreshSlider);
    compMixAtt    = std::make_unique<SliderAtt> (v, "compMix", compMixSlider);
    parAmtAtt     = std::make_unique<SliderAtt> (v, "parCompAmt", parAmtSlider);
    outGainAtt    = std::make_unique<SliderAtt> (v, "outputGain", outGainSlider);
}

BenBocazStudioProEditor::~BenBocazStudioProEditor() { setLookAndFeel (nullptr); }

void BenBocazStudioProEditor::updateSoloButtonsUI()
{
    const int s = audioProcessor.getSoloStand();
    solo1Btn.setToggleState (s == 0, juce::dontSendNotification);
    solo2Btn.setToggleState (s == 1, juce::dontSendNotification);
    solo3Btn.setToggleState (s == 2, juce::dontSendNotification);
    solo4Btn.setToggleState (s == 3, juce::dontSendNotification);
}

void BenBocazStudioProEditor::applyFactoryPreset (int idx)
{
    auto setParam = [this] (const juce::String& id, float plainVal)
    {
        if (auto* param = audioProcessor.apvts.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (plainVal));
    };

    if (idx == 0) // 01. Clean Main Centered (Simple)
    {
        setParam ("mode", 1); setParam ("roomOn", 1); setParam ("mainStereo", 0);
        setParam ("mic1X", 0.0f); setParam ("mic1Y", 0.16f); setParam ("mic1Gain", 1.0f);
        setParam ("mic2Gain", 0.0f); setParam ("mic3Gain", 0.0f); setParam ("mic4Gain", 0.0f);
        setParam ("chamberMix", 0.12f); setParam ("eqLow", 0.0f); setParam ("eqMid", 0.0f); setParam ("eqHigh", 0.0f);
        setParam ("parCompOn", 0); setParam ("deMudOn", 0);
    }
    else if (idx == 1) // 02. Vocal C-800G Warm & Air
    {
        setParam ("mode", 1); setParam ("source", 0); setParam ("room", 0); setParam ("walls", 1);
        setParam ("vocalModOn", 1); setParam ("vocalSrcMic", 2); setParam ("vocalTgtMic", 0);
        setParam ("mainStereo", 0); setParam ("mic1X", 0.0f); setParam ("mic1Y", 0.16f);
        setParam ("mic1Gain", 1.05f); setParam ("mic2Gain", 0.25f); setParam ("mic3Gain", 0.35f); setParam ("mic4Gain", 0.15f);
        setParam ("eqType", 1); setParam ("eqHighFreq", 2); setParam ("eqHigh", 2.0f);
        setParam ("compType", 1); setParam ("compMix", 0.85f); setParam ("chamberMix", 0.22f);
    }
    else if (idx == 2) // 03. Shangri-La Live Drums
    {
        setParam ("mode", 1); setParam ("source", 1); setParam ("room", 0); setParam ("walls", 1);
        setParam ("mainStereo", 1); setParam ("mic1X", 0.0f); setParam ("mic1Y", 0.24f);
        setParam ("mic1Gain", 0.95f); setParam ("mic2Gain", 0.65f); setParam ("mic3Gain", 0.55f); setParam ("mic4Gain", 0.50f);
        setParam ("pairSpread", 1.15f); setParam ("deccaWidth", 1.20f); setParam ("deMudOn", 1);
        setParam ("eqType", 2); setParam ("parCompOn", 1); setParam ("parCompRoute", 1); setParam ("parCompAmt", 0.55f);
    }
    else if (idx == 3) // 04. Canyon Guitar Stereo
    {
        setParam ("mode", 1); setParam ("source", 5); setParam ("room", 1); setParam ("walls", 1);
        setParam ("mainStereo", 1); setParam ("mic1X", 0.0f); setParam ("mic1Y", 0.20f);
        setParam ("mic1Gain", 1.0f); setParam ("mic2Gain", 0.50f); setParam ("mic3Gain", 0.45f); setParam ("mic4Gain", 0.30f);
        setParam ("eqType", 0); setParam ("eqMidFreq", 2); setParam ("eqMid", 1.8f);
    }
    else if (idx == 4) // 05. Bass Amp Low Weight
    {
        setParam ("mode", 1); setParam ("source", 6); setParam ("room", 1); setParam ("walls", 0);
        setParam ("mainStereo", 0); setParam ("mic1X", 0.0f); setParam ("mic1Y", 0.14f);
        setParam ("mic1Gain", 1.10f); setParam ("mic2Gain", 0.32f); setParam ("mic3Gain", 0.12f); setParam ("mic4Gain", 0.22f);
        setParam ("deMudOn", 1); setParam ("eqType", 0); setParam ("eqLowFreq", 1); setParam ("eqLow", 2.5f);
        setParam ("compType", 0); setParam ("compScFilter", 1);
    }
    else if (idx == 5) // 06. Swedien NS-10 Re-Mic
    {
        setParam ("mode", 1); setParam ("source", 3); setParam ("monitorType", 1); setParam ("room", 0);
        setParam ("mainStereo", 1); setParam ("mic1Gain", 0.85f); setParam ("mic2Gain", 0.55f);
        setParam ("mic3Gain", 0.45f); setParam ("mic4Gain", 0.40f); setParam ("consoleDrive", 0.32f);
    }
    else // 07. Room Instant Glue (1-Click Life Hack)
    {
        setParam ("mode", 0); setParam ("roomOn", 1); setParam ("phaseAlign", 1); setParam ("deMudOn", 1);
        setParam ("mic1Gain", 0.55f); setParam ("mic2Gain", 0.38f); setParam ("mic3Gain", 0.35f); setParam ("mic4Gain", 0.30f);
        setParam ("pairSpread", 0.90f); setParam ("compOn", 1); setParam ("compType", 2); setParam ("compMix", 0.75f);
        setParam ("chamberMix", 0.16f);
    }
}

void BenBocazStudioProEditor::loadEmbeddedImages()
{
#if HAS_CUSTOM_ASSETS
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        juce::String resName (BinaryData::namedResourceList[i]);
        int dataSize = 0;
        if (const char* data = BinaryData::getNamedResource (BinaryData::namedResourceList[i], dataSize))
        {
            auto img = juce::ImageFileFormat::loadFrom (data, static_cast<size_t> (dataSize));
            if (! img.isValid()) continue;

            if (resName.containsIgnoreCase ("canyon"))
                roomPad.setCanyonImage (img);
            else if (resName.containsIgnoreCase ("shangri"))
                roomPad.setShangriLaImage (img);
            else if (resName.containsIgnoreCase ("chassis") || resName.containsIgnoreCase ("wood"))
                chassisImage = img;
            else if (resName.containsIgnoreCase ("vocals"))
                roomPad.setSourceImage (0, img);
            else if (resName.containsIgnoreCase ("drums"))
                roomPad.setSourceImage (1, img);
            else if (resName.containsIgnoreCase ("ensemble"))
                roomPad.setSourceImage (2, img);
            else if (resName.containsIgnoreCase ("monitors"))
                roomPad.setSourceImage (3, img);
            else if (resName.containsIgnoreCase ("guitar_mono"))
                roomPad.setSourceImage (4, img);
            else if (resName.containsIgnoreCase ("guitar_stereo"))
                roomPad.setSourceImage (5, img);
            else if (resName.containsIgnoreCase ("bass"))
                roomPad.setSourceImage (6, img);
            else if (resName.containsIgnoreCase ("main_mono"))
                roomPad.setMicImage (0, img);
            else if (resName.containsIgnoreCase ("main_xy"))
                roomPad.setMicImage (1, img);
            else if (resName.containsIgnoreCase ("stereo_pair"))
                roomPad.setMicImage (2, img);
            else if (resName.containsIgnoreCase ("decca"))
                roomPad.setMicImage (3, img);
            else if (resName.containsIgnoreCase ("room"))
                roomPad.setShangriLaImage (img);
        }
    }
#endif
}

void BenBocazStudioProEditor::openIRFileChooser()
{
    fileChooser = std::make_unique<juce::FileChooser> ("Select Studio or Chamber IR (.wav)", juce::File{}, "*.wav;*.aiff;*.aif");
    auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync (flags, [this] (const juce::FileChooser& fc)
    {
        auto file = fc.getResult();
        if (file.existsAsFile())
        {
            audioProcessor.loadCustomIR (file);
            irNameLbl.setText (audioProcessor.getLoadedIRName(), juce::dontSendNotification);
            chamberBox.setSelectedItemIndex (4, juce::sendNotification);
        }
    });
}

void BenBocazStudioProEditor::setupKnob (juce::Slider& slider, juce::Label& label, const juce::String& text, juce::Colour accent)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 54, 14);
    slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
    slider.setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffe0d1b8));
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (slider);

    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, accent);
    label.setFont (juce::Font (juce::FontOptions (9.1f).withStyle ("Bold")));
    addAndMakeVisible (label);
}

void BenBocazStudioProEditor::setupCombo (juce::ComboBox& box, const juce::StringArray& items, juce::Label& label, const juce::String& text)
{
    box.addItemList (items, 1);
    addAndMakeVisible (box);

    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centredLeft);
    label.setColour (juce::Label::textColourId, juce::Colour (0xffc9b89e));
    label.setFont (juce::Font (juce::FontOptions (8.9f).withStyle ("Bold")));
    addAndMakeVisible (label);
}

void BenBocazStudioProEditor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    if (chassisImage.isValid())
    {
        g.drawImage (chassisImage, bounds.toFloat(), juce::RectanglePlacement::fillDestination);
        g.setColour (juce::Colour (0xff140f0b).withAlpha (0.24f));
        g.fillRect (bounds);
    }
    else
    {
        juce::ColourGradient bg (juce::Colour (0xff2b221a), 0.0f, 0.0f,
                                 juce::Colour (0xff140f0c), 0.0f, static_cast<float> (getHeight()), false);
        g.setGradientFill (bg);
        g.fillRect (bounds);
    }

    auto drawWoodRail = [&] (juce::Rectangle<int> r)
    {
        juce::ColourGradient wood (juce::Colour (0xff5c361c), static_cast<float> (r.getX()), 0.0f,
                                   juce::Colour (0xff261308), static_cast<float> (r.getRight()), 0.0f, false);
        g.setGradientFill (wood);
        g.fillRect (r);
        g.setColour (juce::Colours::black.withAlpha (0.3f));
        for (int x = r.getX() + 3; x < r.getRight(); x += 5)
            g.drawVerticalLine (x, 0.0f, static_cast<float> (getHeight()));
    };
    drawWoodRail (bounds.removeFromLeft (18));
    drawWoodRail (bounds.removeFromRight (18));

    // Header + Barra de Presets
    auto header = bounds.removeFromTop (68);
    g.setColour (juce::Colour (0xff120e0b).withAlpha (0.88f));
    g.fillRect (header);
    g.setColour (juce::Colour (0xffe5a94c).withAlpha (0.5f));
    g.drawHorizontalLine (header.getBottom() - 1, static_cast<float> (header.getX()), static_cast<float> (header.getRight()));

    g.setColour (juce::Colour (0xfff6ead4));
    g.setFont (juce::Font (juce::FontOptions (21.0f).withStyle ("Bold")));
    g.drawText ("BEN BOCAZ STUDIO PRO", header.reduced (14, 8).removeFromTop (26), juce::Justification::centredLeft);

    g.setColour (juce::Colour (0xffe5a94c));
    g.setFont (juce::Font (juce::FontOptions (9.6f).withStyle ("Bold")));
    g.drawText ("RUSTIC ANALOG ROOMS  |  3D 4-STAND RE-MIC  |  VOCAL MIC MOD",
                header.reduced (14, 0).withTrimmedTop (38), juce::Justification::centredLeft);

    auto presetBar = bounds.removeFromTop (32);
    g.setColour (juce::Colour (0xff18120e).withAlpha (0.88f));
    g.fillRect (presetBar);
    g.setColour (juce::Colour (0xff5c4935).withAlpha (0.7f));
    g.drawHorizontalLine (presetBar.getBottom() - 1, static_cast<float> (presetBar.getX()), static_cast<float> (presetBar.getRight()));

    auto drawPanel = [&] (juce::Rectangle<int> r, const juce::String& title)
    {
        g.setColour (juce::Colour (0xff1a1511).withAlpha (0.78f));
        g.fillRoundedRectangle (r.toFloat(), 6.0f);
        g.setColour (juce::Colour (0xff785f44).withAlpha (0.85f));
        g.drawRoundedRectangle (r.toFloat(), 6.0f, 1.15f);

        g.setColour (juce::Colour (0xffa6885f));
        g.fillEllipse (static_cast<float> (r.getX() + 6), static_cast<float> (r.getY() + 6), 5.5f, 5.5f);
        g.fillEllipse (static_cast<float> (r.getRight() - 12), static_cast<float> (r.getY() + 6), 5.5f, 5.5f);

        g.setColour (juce::Colour (0xffe6ab50));
        g.setFont (juce::Font (juce::FontOptions (10.2f).withStyle ("Bold")));
        g.drawText (title, r.reduced (18, 5).removeFromTop (16), juce::Justification::centredLeft);
    };

    if (stageExpanded)
    {
        drawPanel ({ 24, 108, 1212, 668 }, "LIVE ROOM 3D STAGE  (EXPANDED VIEW)");
    }
    else
    {
        drawPanel ({ 24,   108, 256, 404 }, "VOCAL MIC MOD & SOURCE");
        drawPanel ({ 292,  108, 656, 404 }, "LIVE ROOM 3D STAGE");
        drawPanel ({ 960,  108, 276, 404 }, "ECHO CHAMBERS & IR");
        drawPanel ({ 24,   524, 576, 252 }, "ANALOG CONSOLE EQ");
        drawPanel ({ 612,  524, 476, 252 }, "OUTBOARD & PARALLEL COMP");
        drawPanel ({ 1100, 524, 136, 252 }, "MASTER");

        g.setColour (juce::Colour (0xff614e39).withAlpha (0.55f));
        g.drawHorizontalLine (294, 36.0f, 268.0f);
        g.drawVerticalLine (916, 558.0f, 762.0f);
    }
}

void BenBocazStudioProEditor::resized()
{
    // Header
    modeLbl.setBounds       (390, 10, 96, 13);  modeBox.setBounds       (390, 25, 100, 25);
    chainLbl.setBounds      (500, 10, 165, 13); chainBox.setBounds      (500, 25, 176, 25);
    phaseAlignBtn.setBounds (688, 9,  112, 23);
    phaseInvBtn.setBounds   (688, 36, 112, 23);
    vuMeter.setBounds       (812, 6,  312, 56);
    vuModeBtn.setBounds     (1134, 20, 94, 26);

    // Barra de Presets + Life Hacks
    presetLbl.setBounds     (32,  73, 58,  22);
    presetBox.setBounds     (92,  73, 240, 22);
    roomGlueBtn.setBounds   (346, 73, 94,  22);
    deMudBtn.setBounds      (448, 73, 82,  22);
    snapCenterBtn.setBounds (546, 73, 106, 22);
    link34Btn.setBounds     (660, 73, 86,  22);
    centerSelBtn.setBounds  (754, 73, 92,  22);
    tipsBtn.setBounds       (1038, 73, 74,  22);
    expandStageBtn.setBounds(1120, 73, 110, 22);

    // Visibilidad según modo expandido o normal
    const bool showRacks = ! stageExpanded;
    for (auto* c : std::initializer_list<juce::Component*> {
        &vocalModBtn, &vocalSrcLbl, &vocalSrcBox, &vocalTgtLbl, &vocalTgtBox,
        &vocalInLbl, &vocalInSlider, &vocalOutLbl, &vocalOutSlider,
        &sourceLbl, &sourceBox, &monitorLbl, &monitorBox, &roomLbl, &roomBox, &wallsLbl, &wallsBox,
        &driveLbl, &driveSlider,
        &chamberOnBtn, &chamberLbl, &chamberBox, &loadIRBtn, &irNameLbl,
        &chMixLbl, &chMixSlider, &chDecayLbl, &chDecaySlider, &chPreLbl, &chPreSlider, &chWidthLbl, &chWidthSlider,
        &eqOnBtn, &eqTubeSyncBtn, &eqTypeLbl, &eqTypeBox, &eqTubeLbl, &eqTubeBox,
        &eqHpfLbl, &eqHpfSlider, &eqLowLbl, &eqLowSlider, &eqLowFreqBox,
        &eqMidLbl, &eqMidSlider, &eqMidFreqBox, &eqHighLbl, &eqHighSlider, &eqHighFreqBox,
        &compOnBtn, &compLbl, &compBox, &compScLinkBtn, &compScFiltBtn,
        &compThreshLbl, &compThreshSlider, &compMixLbl, &compMixSlider,
        &parCompOnBtn, &parAmtLbl, &parAmtSlider, &parRouteLbl, &parRouteBox,
        &outGainLbl, &outGainSlider
    })
    {
        c->setVisible (showRacks);
    }

    if (stageExpanded)
    {
        mainStereoBtn.setBounds (1016, 113, 114, 20);
        roomOnBtn.setBounds     (1138, 113, 86,  20);
        roomPad.setBounds       (36,   138, 1188, 534);

        const int yLbl = 680, yKnb = 694;
        mic1GainLbl.setBounds (120, yLbl, 96, 13); mic1GainSlider.setBounds (134, yKnb, 68, 70); solo1Btn.setBounds (204, yKnb + 18, 22, 22);
        mic2GainLbl.setBounds (300, yLbl, 96, 13); mic2GainSlider.setBounds (314, yKnb, 68, 70); solo2Btn.setBounds (384, yKnb + 18, 22, 22);
        mic3GainLbl.setBounds (480, yLbl, 96, 13); mic3GainSlider.setBounds (494, yKnb, 68, 70); solo3Btn.setBounds (564, yKnb + 18, 22, 22);
        mic4GainLbl.setBounds (660, yLbl, 96, 13); mic4GainSlider.setBounds (674, yKnb, 68, 70); solo4Btn.setBounds (744, yKnb + 18, 22, 22);
        pairWidthLbl.setBounds  (840, yLbl, 96, 13); pairWidthSlider.setBounds  (854, yKnb, 68, 70);
        deccaWidthLbl.setBounds (1010,yLbl, 96, 13); deccaWidthSlider.setBounds (1024,yKnb, 68, 70);
        return;
    }

    // Panel Izquierdo: Vocal Mic Mod + Room Source + Tube Drive
    vocalModBtn.setBounds    (36,  134, 232, 22);
    vocalSrcLbl.setBounds    (36,  160, 112, 12); vocalSrcBox.setBounds    (36,  173, 112, 22);
    vocalTgtLbl.setBounds    (156, 160, 112, 12); vocalTgtBox.setBounds    (156, 173, 112, 22);
    vocalInLbl.setBounds     (46,  202, 92,  12); vocalInSlider.setBounds  (56,  215, 72,  70);
    vocalOutLbl.setBounds    (166, 202, 92,  12); vocalOutSlider.setBounds (176, 215, 72,  70);

    sourceLbl.setBounds   (36,  302, 112, 12); sourceBox.setBounds   (36,  315, 112, 22);
    monitorLbl.setBounds  (156, 302, 112, 12); monitorBox.setBounds  (156, 315, 112, 22);
    roomLbl.setBounds     (36,  345, 112, 12); roomBox.setBounds     (36,  358, 112, 22);
    wallsLbl.setBounds    (156, 345, 112, 12); wallsBox.setBounds    (156, 358, 112, 22);
    driveLbl.setBounds    (85,  390, 134, 12); driveSlider.setBounds (110, 404, 84,  92);

    // Panel Central: Escenario 3D + Solo por atril
    mainStereoBtn.setBounds (728, 113, 114, 20);
    roomOnBtn.setBounds     (850, 113, 86,  20);
    roomPad.setBounds       (304, 138, 632, 276);

    mic1GainLbl.setBounds   (302, 421, 96, 13); mic1GainSlider.setBounds   (310, 435, 68, 68); solo1Btn.setBounds (379, 452, 20, 20);
    mic2GainLbl.setBounds   (408, 421, 96, 13); mic2GainSlider.setBounds   (416, 435, 68, 68); solo2Btn.setBounds (485, 452, 20, 20);
    mic3GainLbl.setBounds   (514, 421, 96, 13); mic3GainSlider.setBounds   (522, 435, 68, 68); solo3Btn.setBounds (591, 452, 20, 20);
    mic4GainLbl.setBounds   (620, 421, 96, 13); mic4GainSlider.setBounds   (628, 435, 68, 68); solo4Btn.setBounds (697, 452, 20, 20);
    pairWidthLbl.setBounds  (728, 421, 98, 13); pairWidthSlider.setBounds  (742, 435, 68, 68);
    deccaWidthLbl.setBounds (834, 421, 98, 13); deccaWidthSlider.setBounds (848, 435, 68, 68);

    // Panel Derecho: Cámaras + IR
    chamberOnBtn.setBounds (1154, 113, 70, 20);
    chamberLbl.setBounds   (972, 140, 160, 13); chamberBox.setBounds (972, 154, 252, 24);
    loadIRBtn.setBounds    (972, 186, 114, 24); irNameLbl.setBounds  (1092, 186, 132, 24);

    chMixLbl.setBounds   (990,  230, 96, 13); chMixSlider.setBounds   (995,  245, 86, 94);
    chDecayLbl.setBounds (1110, 230, 96, 13); chDecaySlider.setBounds (1115, 245, 86, 94);
    chPreLbl.setBounds   (990,  366, 96, 13); chPreSlider.setBounds   (995,  381, 86, 94);
    chWidthLbl.setBounds (1110, 366, 96, 13); chWidthSlider.setBounds (1115, 381, 86, 94);

    // Panel Inferior Izquierdo: Analog Console EQ (Frecuencias reales + Tube Sync + Colores)
    eqOnBtn.setBounds       (516, 529, 72, 21);
    eqTubeSyncBtn.setBounds (420, 529, 90, 21);
    eqTypeLbl.setBounds     (38,  556, 70, 12); eqTypeBox.setBounds (38,  569, 188, 22);
    eqTubeLbl.setBounds     (236, 556, 90, 12); eqTubeBox.setBounds (236, 569, 156, 22);

    eqHpfLbl.setBounds  (46,  604, 96, 13); eqHpfSlider.setBounds  (49,  618, 90, 102);
    eqLowLbl.setBounds  (182, 604, 96, 13); eqLowSlider.setBounds  (185, 618, 90, 102); eqLowFreqBox.setBounds  (188, 726, 84, 21);
    eqMidLbl.setBounds  (318, 604, 96, 13); eqMidSlider.setBounds  (321, 618, 90, 102); eqMidFreqBox.setBounds  (324, 726, 84, 21);
    eqHighLbl.setBounds (454, 604, 96, 13); eqHighSlider.setBounds (457, 618, 90, 102); eqHighFreqBox.setBounds (460, 726, 84, 21);

    // Panel Inferior Centro: Outboard Compressor + Compresor Paralelo Dedicado
    compOnBtn.setBounds     (998, 529, 78, 21);
    compLbl.setBounds       (626, 556, 140, 12); compBox.setBounds       (626, 569, 152, 22);
    compScLinkBtn.setBounds (786, 556, 114, 21);
    compScFiltBtn.setBounds (786, 581, 114, 21);

    compThreshLbl.setBounds (628, 612, 96, 13); compThreshSlider.setBounds (631, 626, 90, 106);
    compMixLbl.setBounds    (758, 612, 96, 13); compMixSlider.setBounds    (761, 626, 90, 106);

    // Columna derecha del panel de dinámica: Compresor Paralelo
    parCompOnBtn.setBounds  (930, 562, 144, 22);
    parAmtLbl.setBounds     (954, 596, 96,  13); parAmtSlider.setBounds (957, 610, 90, 92);
    parRouteLbl.setBounds   (934, 708, 136, 12); parRouteBox.setBounds  (934, 722, 136, 22);

    // Panel Inferior Derecho: Master Output
    outGainLbl.setBounds    (1120, 604, 96, 13); outGainSlider.setBounds (1123, 618, 90, 114);
}
