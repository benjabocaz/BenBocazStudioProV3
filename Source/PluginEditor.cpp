#include "PluginEditor.h"

#if HAS_CUSTOM_ASSETS
 #include "BinaryData.h"
#endif

//==============================================================================
// RusticStudioLookAndFeel
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
                                                juce::Slider&)
{
    auto bounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                          static_cast<float> (width), static_cast<float> (height)).reduced (7.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const float cx = bounds.getCentreX();
    const float cy = bounds.getCentreY();
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    juce::Path bgArc;
    bgArc.addCentredArc (cx, cy, radius + 3.5f, radius + 3.5f, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (juce::Colour (0xff342b22));
    g.strokePath (bgArc, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path valArc;
    valArc.addCentredArc (cx, cy, radius + 3.5f, radius + 3.5f, 0.0f, rotaryStartAngle, angle, true);
    g.setColour (juce::Colour (0xffe5a94c));
    g.strokePath (valArc, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour (juce::Colours::black.withAlpha (0.65f));
    g.fillEllipse (cx - radius + 1.5f, cy - radius + 3.5f, radius * 2.0f, radius * 2.0f);

    juce::ColourGradient body (juce::Colour (0xff443a31), cx - radius * 0.5f, cy - radius * 0.5f,
                               juce::Colour (0xff15110e), cx + radius * 0.6f, cy + radius * 0.6f, true);
    g.setGradientFill (body);
    g.fillEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

    juce::ColourGradient cap (juce::Colour (0xff5c4e3f), cx - radius * 0.3f, cy - radius * 0.4f,
                              juce::Colour (0xff26201a), cx + radius * 0.4f, cy + radius * 0.4f, true);
    g.setGradientFill (cap);
    g.fillEllipse (cx - radius * 0.72f, cy - radius * 0.72f, radius * 1.44f, radius * 1.44f);

    g.setColour (juce::Colour (0xffb08b55).withAlpha (0.55f));
    g.drawEllipse (cx - radius * 0.72f, cy - radius * 0.72f, radius * 1.44f, radius * 1.44f, 1.0f);

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
    const float ax = static_cast<float> (width) - 14.0f;
    const float ay = static_cast<float> (height) * 0.5f - 2.0f;
    arrow.addTriangle (ax, ay, ax + 8.0f, ay, ax + 4.0f, ay + 5.0f);
    g.setColour (juce::Colour (0xffe5a94c));
    g.fillPath (arrow);
}

void RusticStudioLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                                    const juce::Colour&, bool isHighlighted, bool isDown)
{
    auto area = button.getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (isDown ? juce::Colour (0xffd69d46) : (isHighlighted ? juce::Colour (0xff3c3024) : juce::Colour (0xff231c16)));
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

    const float ledR = 4.0f;
    const float ledX = area.getX() + 9.5f;
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
    g.setFont (juce::Font (juce::FontOptions (9.4f).withStyle ("Bold")));
    g.drawText (button.getButtonText(), area.withTrimmedLeft (17.0f).reduced (2.0f, 0.0f), juce::Justification::centredLeft);
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
    g.setFont (juce::Font (juce::FontOptions (9.5f).withStyle ("Bold")));
    g.drawText (showGR ? "GR  dB" : "VU  OUT", glass.reduced (6.0f, 3.0f), juce::Justification::bottomLeft);
    g.drawText ("-20    -10    -5    0   +3", glass.reduced (8.0f, 4.0f), juce::Justification::centredTop);

    const float needleAngle = juce::jmap (smoothedNeedle, 0.0f, 1.0f, -0.75f, 0.75f);
    juce::Path needle;
    needle.startNewSubPath (pivotX, pivotY);
    needle.lineTo (pivotX + std::sin (needleAngle) * (rad * 0.92f),
                   pivotY - std::cos (needleAngle) * (rad * 0.92f));
    g.setColour (juce::Colour (0xff1a1209));
    g.strokePath (needle, juce::PathStrokeType (1.8f));
}

//==============================================================================
// Studio4MicStagePad (Composición 2.5D Fotorrealista con Sprites Recortados)
//==============================================================================
Studio4MicStagePad::Studio4MicStagePad (juce::AudioProcessorValueTreeState& apvtsRef) : apvts (apvtsRef)
{
    startTimerHz (24);
}

void Studio4MicStagePad::setSourceImage (int idx, const juce::Image& img)
{
    if (idx >= 0 && idx < 4) { sourceImgs[static_cast<size_t> (idx)] = img; repaint(); }
}

void Studio4MicStagePad::setMicImage (int idx, const juce::Image& img)
{
    if (idx >= 0 && idx < 4) { micImgs[static_cast<size_t> (idx)] = img; repaint(); }
}

juce::Point<float> Studio4MicStagePad::getStandBasePos (int idx, juce::Rectangle<float> area) const
{
    const juce::String pfx = "mic" + juce::String (idx + 1);
    const float mx = apvts.getRawParameterValue (pfx + "X")->load();
    const float my = apvts.getRawParameterValue (pfx + "Y")->load();

    const float yTop  = area.getY() + area.getHeight() * 0.62f;
    const float yBot  = area.getBottom() - 18.0f;
    const float sy    = juce::jmap (my, 0.0f, 1.0f, yTop, yBot);
    const float halfW = juce::jmap (my, 0.0f, 1.0f, area.getWidth() * 0.30f, area.getWidth() * 0.43f);
    const float sx    = area.getCentreX() + mx * halfW;
    return { sx, sy };
}

float Studio4MicStagePad::getStandScale (int idx) const
{
    const float my = apvts.getRawParameterValue ("mic" + juce::String (idx + 1) + "Y")->load();
    return juce::jmap (my, 0.0f, 1.0f, 0.72f, 1.15f);
}

float Studio4MicStagePad::getStandVisualHeight (int idx, float scale) const
{
    const float baseH = (idx == 2) ? 132.0f : (idx == 3 ? 116.0f : 104.0f);
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

    // 2. Fuente Sonora Fotorrealista sobre la alfombra
    const float srcFloorX = area.getCentreX();
    const float srcFloorY = area.getY() + area.getHeight() * 0.65f;

    g.setColour (juce::Colours::black.withAlpha (0.48f * stageAlpha));
    g.fillEllipse (srcFloorX - 78.0f, srcFloorY - 9.0f, 156.0f, 22.0f);

    const auto& srcImg = sourceImgs[static_cast<size_t> (std::clamp (sourceIdx, 0, 3))];
    if (srcImg.isValid())
    {
        static constexpr std::array<float, 4> srcHeightRatios { 0.42f, 0.46f, 0.48f, 0.41f };
        const float drawH  = area.getHeight() * srcHeightRatios[static_cast<size_t> (std::clamp (sourceIdx, 0, 3))];
        const float aspect = static_cast<float> (srcImg.getWidth()) / std::max (1.0f, static_cast<float> (srcImg.getHeight()));
        const float drawW  = drawH * aspect;

        juce::Rectangle<float> srcDest (srcFloorX - drawW * 0.5f, srcFloorY - drawH + 5.0f, drawW, drawH);
        g.setOpacity (stageAlpha);
        g.drawImage (srcImg, srcDest, juce::RectanglePlacement::stretchToFit);
        g.setOpacity (1.0f);
    }

    juce::String srcLbl = (sourceIdx == 0 ? "SOURCE: VOCALS" : (sourceIdx == 1 ? "SOURCE: DRUMS" : (sourceIdx == 2 ? "SOURCE: ENSEMBLE" :
                          (monitorIdx == 0 ? "MONITORS: ALTEC" : (monitorIdx == 1 ? "MONITORS: NS-10" : "MONITORS: JBL PA")))));
    g.setColour (juce::Colour (0xff120d09).withAlpha (0.80f * stageAlpha));
    g.fillRoundedRectangle (srcFloorX - 54.0f, area.getY() + 30.0f, 108.0f, 15.0f, 3.5f);
    g.setColour (juce::Colour (0xfff2e6d0).withAlpha (0.88f * stageAlpha));
    g.drawRoundedRectangle (srcFloorX - 54.0f, area.getY() + 30.0f, 108.0f, 15.0f, 3.5f, 0.9f);
    g.setFont (juce::Font (juce::FontOptions (8.4f).withStyle ("Bold")));
    g.drawText (srcLbl, static_cast<int> (srcFloorX - 54), static_cast<int> (area.getY() + 30), 108, 15, juce::Justification::centred);

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
        juce::Colour (0xfff2b24c), // 1: MAIN (Ámbar)
        juce::Colour (0xffe07a52), // 2: MID (Cobre)
        juce::Colour (0xff8fc7e8), // 3: AIR (Celeste)
        juce::Colour (0xfff5ebd6)  // 4: DECCA (Marfil)
    };

    const float roomD = (roomIdx == 0) ? 12.0f : 8.5f;

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
        const float alpha = (gain > 0.01f ? 1.0f : 0.30f) * stageAlpha;
        const auto col    = cols[static_cast<size_t> (idx)];
        const bool active = (idx == selectedStand || idx == hoveredStand) && roomIsOn;

        // Sombra proyectada y halo interactivo de piso
        const float haloRx = (active ? 26.0f : 19.0f) * sc;
        const float haloRy = (active ? 8.5f  : 6.0f)  * sc;

        g.setColour (juce::Colours::black.withAlpha (0.55f * alpha));
        g.fillEllipse (base.x - 16.0f * sc, base.y - 4.0f * sc, 32.0f * sc, 10.0f * sc);

        g.setColour (col.withAlpha ((active ? 0.30f : 0.14f) * alpha));
        g.fillEllipse (base.x - haloRx, base.y - haloRy, haloRx * 2.0f, haloRy * 2.0f);
        g.setColour (col.withAlpha ((active ? 0.95f : 0.58f) * alpha));
        g.drawEllipse (base.x - haloRx, base.y - haloRy, haloRx * 2.0f, haloRy * 2.0f, active ? 1.8f : 1.1f);

        if (active)
        {
            g.setColour (col.withAlpha (0.95f));
            const float ax = haloRx + 4.0f;
            g.drawLine (base.x - ax - 6.0f, base.y, base.x - ax, base.y, 1.6f);
            g.drawLine (base.x + ax, base.y, base.x + ax + 6.0f, base.y, 1.6f);
        }

        // Sprite fotorrealista según tipo de atril y apertura estéreo
        int micSpriteIdx = 0;
        float widthStretch = 1.0f;
        if (idx == 0)
        {
            micSpriteIdx = mainStereo ? 1 : 0;
        }
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

        // Halo luminoso detrás de las cápsulas
        const float ringR = (idx == 3 ? 22.0f : 17.0f) * sc;
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
        else
        {
            g.setColour (juce::Colour (0xff1a1714).withAlpha (alpha));
            g.drawLine (base.x, base.y, headX, headY, 3.0f * sc);
            g.setColour (col.withAlpha (alpha));
            g.fillEllipse (headX - 6.0f * sc, headY - 6.0f * sc, 12.0f * sc, 12.0f * sc);
        }

        if ((idx == 1 || idx == 2) && pairW >= 0.08f)
        {
            const float spreadPx = juce::jmap (pairW, 0.08f, 1.5f, 14.0f, 44.0f) * sc;
            g.setColour (col.withAlpha (0.65f * alpha));
            g.drawLine (headX - spreadPx, headY, headX - ringR, headY, 1.2f);
            g.drawLine (headX + ringR, headY, headX + spreadPx, headY, 1.2f);
        }
        else if (idx == 3)
        {
            const float wingPx = juce::jmap (deccaW, 0.2f, 1.5f, 16.0f, 42.0f) * sc;
            juce::Path tri;
            tri.startNewSubPath (headX, headY - 12.0f * sc);
            tri.lineTo (headX - wingPx, headY + 12.0f * sc);
            tri.lineTo (headX + wingPx, headY + 12.0f * sc);
            tri.closeSubPath();
            g.setColour (col.withAlpha (0.58f * alpha));
            g.strokePath (tri, juce::PathStrokeType (1.2f));
        }

        // Etiqueta flotante del atril
        juce::String tag = juce::String (idx + 1) + ":" + baseNames[static_cast<size_t> (idx)];
        if (idx == 0) tag += (mainStereo ? " X-Y" : " MONO");
        tag += " " + juce::String (distM, 1) + "m";

        const float tagW = (idx == 0) ? 98.0f : 76.0f;
        const float tagY = std::max (area.getY() + 26.0f, base.y - vh - 14.0f);
        g.setColour (juce::Colour (0xff120d0a).withAlpha (0.86f * stageAlpha));
        g.fillRoundedRectangle (headX - tagW * 0.5f, tagY, tagW, 14.0f, 3.0f);
        g.setColour (col.withAlpha (active ? 1.0f : 0.85f * alpha));
        g.drawRoundedRectangle (headX - tagW * 0.5f, tagY, tagW, 14.0f, 3.0f, active ? 1.2f : 0.7f);
        g.setFont (juce::Font (juce::FontOptions (8.3f).withStyle ("Bold")));
        g.drawText (tag, static_cast<int> (headX - tagW * 0.5f), static_cast<int> (tagY),
                    static_cast<int> (tagW), 14, juce::Justification::centred);
    }

    g.restoreState();

    // Borde exterior de la ventana del estudio
    g.setColour ((roomIsOn && wallsLive) ? juce::Colour (0xffd9a04a) : juce::Colour (0xff5c4f41));
    g.drawRoundedRectangle (area, 6.0f, (roomIsOn && wallsLive) ? 2.0f : 1.4f);

    // Barra superior de estado
    auto topBar = area.reduced (8.0f).removeFromTop (16.0f);
    g.setColour (juce::Colour (0xff140f0b).withAlpha (0.82f));
    g.fillRoundedRectangle (topBar.expanded (4.0f, 2.0f), 3.0f);

    g.setColour (juce::Colour (0xfff4d08c).withAlpha (roomIsOn ? 1.0f : 0.55f));
    g.setFont (juce::Font (juce::FontOptions (9.8f).withStyle ("Bold")));
    juce::String roomTitle = (roomIdx == 0 ? "STUDIO A (SHANGRI-LA)" : "STUDIO B (CANYON WOOD)");
    roomTitle += (wallsLive ? "  •  LIVE" : "  •  DAMPED");
    g.drawText (roomTitle, topBar, juce::Justification::centredLeft);

    if (! roomIsOn)
    {
        g.setColour (juce::Colour (0xffc99e6b));
        g.drawText ("ROOM BYPASSED (DRY)", topBar, juce::Justification::centredRight);
    }
    else
    {
        g.setColour (phaseOk ? juce::Colour (0xff8ce08e) : juce::Colour (0xffe8a94c));
        g.drawText (phaseOk ? "● PHASE LOCKED" : "● FREE DELAY", topBar, juce::Justification::centredRight);
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
    {
        dragOffset = currentBase - e.position;
    }
    else
    {
        dragOffset = { 0.0f, 0.0f };
        updateStandFromBasePoint (selectedStand, e.position, area);
    }
    repaint();
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
    const float yTop = area.getY() + area.getHeight() * 0.62f;
    const float yBot = area.getBottom() - 18.0f;

    const float normY = juce::jlimit (0.0f, 1.0f, (targetBase.y - yTop) / std::max (1.0f, yBot - yTop));
    const float halfW = juce::jmap (normY, 0.0f, 1.0f, area.getWidth() * 0.30f, area.getWidth() * 0.43f);
    const float normX = juce::jlimit (0.0f, 1.0f, 0.5f + 0.5f * ((targetBase.x - area.getCentreX()) / std::max (1.0f, halfW)));

    const juce::String pfx = "mic" + juce::String (idx + 1);
    if (auto* pX = apvts.getParameter (pfx + "X")) pX->setValueNotifyingHost (normX);
    if (auto* pY = apvts.getParameter (pfx + "Y")) pY->setValueNotifyingHost (normY);
}

//==============================================================================
// BenBocazStudioProEditor
//==============================================================================
BenBocazStudioProEditor::BenBocazStudioProEditor (BenBocazStudioProProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), vuMeter (p), roomPad (p.apvts)
{
    setLookAndFeel (&rusticLAF);
    setSize (1240, 760);

    loadEmbeddedImages();

    addAndMakeVisible (vuMeter);
    addAndMakeVisible (roomPad);

    addAndMakeVisible (phaseAlignBtn);
    addAndMakeVisible (phaseInvBtn);
    addAndMakeVisible (vocalModBtn);
    addAndMakeVisible (mainStereoBtn);
    addAndMakeVisible (roomOnBtn);
    addAndMakeVisible (chamberOnBtn);
    addAndMakeVisible (eqOnBtn);
    addAndMakeVisible (compOnBtn);

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
    irNameLbl.setFont (juce::Font (juce::FontOptions (10.0f)));
    addAndMakeVisible (irNameLbl);

    setupCombo (modeBox,     { "Reverb", "Re-Mic" }, modeLbl, "MODE");
    setupCombo (chainBox,    { "EQ -> Comp -> Chamber", "Comp -> EQ -> Chamber", "Chamber -> Comp -> EQ" }, chainLbl, "SIGNAL ROUTING");
    setupCombo (vocalSrcBox, { "Shure SM27", "Shure SM57", "Shure SM7B", "Generic LDC" }, vocalSrcLbl, "YOUR INPUT MIC");
    setupCombo (vocalTgtBox, { "Sony C-800G", "Neumann TLM 103", "Aston Origin" }, vocalTgtLbl, "TARGET VOCAL MIC");
    setupCombo (sourceBox,   { "Vocals", "Drums", "Ensemble", "Monitors (Swedien)" }, sourceLbl, "ROOM SOURCE");
    setupCombo (monitorBox,  { "Altec Main", "NS-10 Nearfield", "JBL PA Wedge" }, monitorLbl, "MONITOR SPEAKER");
    setupCombo (roomBox,     { "Studio A (Shangri-La)", "Studio B (Canyon)" }, roomLbl, "STUDIO ROOM");
    setupCombo (wallsBox,    { "Tight / Damped", "Live / Cedar Wood" }, wallsLbl, "ROOM WALLS");
    setupCombo (chamberBox,  { "Chamber 1 (Bright)", "Chamber 3 (Classic)", "Chamber 5 (Deep)", "Chamber 8 (Dense)", "Custom WAV IR" }, chamberLbl, "ECHO CHAMBER");
    setupCombo (eqTypeBox,   { "Neve 1073 (British)", "Pultec EQP-1A (Tube)", "API 550A (Punch)", "SSL 4000E (Clean)" }, eqTypeLbl, "EQ MODEL");
    setupCombo (compBox,     { "1176LN (FET)", "LA-2A (Opto)", "Mastering (Tube)" }, compLbl, "COMP MODEL");

    setupKnob (vocalInSlider,    vocalInLbl,    "MIC IN");
    setupKnob (vocalOutSlider,   vocalOutLbl,   "MIC OUT");
    setupKnob (driveSlider,      driveLbl,      "TUBE / TAPE DRIVE");
    setupKnob (mic1GainSlider,   mic1GainLbl,   "1: MAIN");
    setupKnob (mic2GainSlider,   mic2GainLbl,   "2: MID");
    setupKnob (mic3GainSlider,   mic3GainLbl,   "3: AIR");
    setupKnob (mic4GainSlider,   mic4GainLbl,   "4: DECCA");
    setupKnob (pairWidthSlider,  pairWidthLbl,  "PAIR WIDE");
    setupKnob (deccaWidthSlider, deccaWidthLbl, "DECCA WIDE");
    setupKnob (chMixSlider,      chMixLbl,      "SEND");
    setupKnob (chDecaySlider,    chDecayLbl,    "DECAY");
    setupKnob (chPreSlider,      chPreLbl,      "PRE-DELAY");
    setupKnob (chWidthSlider,    chWidthLbl,    "WIDTH");
    setupKnob (eqHpfSlider,      eqHpfLbl,      "HPF");
    setupKnob (eqLowSlider,      eqLowLbl,      "LOW");
    setupKnob (eqMidSlider,      eqMidLbl,      "MID");
    setupKnob (eqHighSlider,     eqHighLbl,     "HIGH");
    setupKnob (compThreshSlider, compThreshLbl, "THRESHOLD");
    setupKnob (outGainSlider,    outGainLbl,    "OUTPUT");

    auto& v = audioProcessor.apvts;
    phaseAlignAtt = std::make_unique<ButtonAtt> (v, "phaseAlign", phaseAlignBtn);
    phaseInvAtt   = std::make_unique<ButtonAtt> (v, "phaseInvert", phaseInvBtn);
    vocalModAtt   = std::make_unique<ButtonAtt> (v, "vocalModOn", vocalModBtn);
    mainStereoAtt = std::make_unique<ButtonAtt> (v, "mainStereo", mainStereoBtn);
    roomOnAtt     = std::make_unique<ButtonAtt> (v, "roomOn", roomOnBtn);
    chamberOnAtt  = std::make_unique<ButtonAtt> (v, "chamberOn", chamberOnBtn);
    eqOnAtt       = std::make_unique<ButtonAtt> (v, "eqOn", eqOnBtn);
    compOnAtt     = std::make_unique<ButtonAtt> (v, "compOn", compOnBtn);

    modeAtt     = std::make_unique<ComboAtt> (v, "mode", modeBox);
    chainAtt    = std::make_unique<ComboAtt> (v, "chainOrder", chainBox);
    vocalSrcAtt = std::make_unique<ComboAtt> (v, "vocalSrcMic", vocalSrcBox);
    vocalTgtAtt = std::make_unique<ComboAtt> (v, "vocalTgtMic", vocalTgtBox);
    sourceAtt   = std::make_unique<ComboAtt> (v, "source", sourceBox);
    monitorAtt  = std::make_unique<ComboAtt> (v, "monitorType", monitorBox);
    roomAtt     = std::make_unique<ComboAtt> (v, "room", roomBox);
    wallsAtt    = std::make_unique<ComboAtt> (v, "walls", wallsBox);
    chamberAtt  = std::make_unique<ComboAtt> (v, "chamberType", chamberBox);
    eqTypeAtt   = std::make_unique<ComboAtt> (v, "eqType", eqTypeBox);
    compAtt     = std::make_unique<ComboAtt> (v, "compType", compBox);

    sourceBox.onChange = [this]
    {
        const int srcIdx = sourceBox.getSelectedItemIndex();
        if (auto* pMainSt = audioProcessor.apvts.getParameter ("mainStereo"))
            pMainSt->setValueNotifyingHost (srcIdx == 0 ? 0.0f : 1.0f);
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
    outGainAtt    = std::make_unique<SliderAtt> (v, "outputGain", outGainSlider);
}

BenBocazStudioProEditor::~BenBocazStudioProEditor() { setLookAndFeel (nullptr); }

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

void BenBocazStudioProEditor::setupKnob (juce::Slider& slider, juce::Label& label, const juce::String& text)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 58, 15);
    slider.setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffe0d1b8));
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (slider);

    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, juce::Colour (0xffe6ab50));
    label.setFont (juce::Font (juce::FontOptions (9.4f).withStyle ("Bold")));
    addAndMakeVisible (label);
}

void BenBocazStudioProEditor::setupCombo (juce::ComboBox& box, const juce::StringArray& items, juce::Label& label, const juce::String& text)
{
    box.addItemList (items, 1);
    addAndMakeVisible (box);

    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centredLeft);
    label.setColour (juce::Label::textColourId, juce::Colour (0xffc9b89e));
    label.setFont (juce::Font (juce::FontOptions (9.2f).withStyle ("Bold")));
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

    auto header = bounds.removeFromTop (74);
    g.setColour (juce::Colour (0xff120e0b).withAlpha (0.86f));
    g.fillRect (header);
    g.setColour (juce::Colour (0xffe5a94c).withAlpha (0.5f));
    g.drawHorizontalLine (header.getBottom() - 1, static_cast<float> (header.getX()), static_cast<float> (header.getRight()));

    g.setColour (juce::Colour (0xfff6ead4));
    g.setFont (juce::Font (juce::FontOptions (22.0f).withStyle ("Bold")));
    g.drawText ("BEN BOCAZ STUDIO PRO", header.reduced (14, 10).removeFromTop (28), juce::Justification::centredLeft);

    g.setColour (juce::Colour (0xffe5a94c));
    g.setFont (juce::Font (juce::FontOptions (10.0f).withStyle ("Bold")));
    g.drawText ("RUSTIC ANALOG ROOMS  •  3D 4-STAND RE-MIC  •  VOCAL MIC MOD",
                header.reduced (14, 0).withTrimmedTop (42), juce::Justification::centredLeft);

    auto drawPanel = [&] (juce::Rectangle<int> r, const juce::String& title)
    {
        g.setColour (juce::Colour (0xff1a1511).withAlpha (0.76f));
        g.fillRoundedRectangle (r.toFloat(), 6.0f);
        g.setColour (juce::Colour (0xff785f44).withAlpha (0.85f));
        g.drawRoundedRectangle (r.toFloat(), 6.0f, 1.15f);

        g.setColour (juce::Colour (0xffa6885f));
        g.fillEllipse (static_cast<float> (r.getX() + 6), static_cast<float> (r.getY() + 6), 5.5f, 5.5f);
        g.fillEllipse (static_cast<float> (r.getRight() - 12), static_cast<float> (r.getY() + 6), 5.5f, 5.5f);

        g.setColour (juce::Colour (0xffe6ab50));
        g.setFont (juce::Font (juce::FontOptions (10.5f).withStyle ("Bold")));
        g.drawText (title, r.reduced (18, 5).removeFromTop (16), juce::Justification::centredLeft);
    };

    drawPanel ({ 24,   84, 260, 402 }, "VOCAL MIC MOD & SOURCE");
    drawPanel ({ 296,  84, 640, 402 }, "LIVE ROOM 3D STAGE");
    drawPanel ({ 948,  84, 268, 402 }, "ECHO CHAMBERS & IR");
    drawPanel ({ 24,  498, 592, 246 }, "ANALOG CONSOLE EQ");
    drawPanel ({ 628, 498, 412, 246 }, "OUTBOARD COMPRESSOR");
    drawPanel ({ 1052, 498, 164, 246 }, "MASTER");

    g.setColour (juce::Colour (0xff614e39).withAlpha (0.55f));
    g.drawHorizontalLine (270, 36.0f, 272.0f);
}

void BenBocazStudioProEditor::resized()
{
    modeLbl.setBounds       (384, 12, 100, 14); modeBox.setBounds       (384, 28, 104, 26);
    chainLbl.setBounds      (498, 12, 170, 14); chainBox.setBounds      (498, 28, 180, 26);
    phaseAlignBtn.setBounds (690, 11, 114, 24);
    phaseInvBtn.setBounds   (690, 39, 114, 24);
    vuMeter.setBounds       (816, 8,  290, 58);
    vuModeBtn.setBounds     (1118, 22, 92, 28);

    vocalModBtn.setBounds    (36,  110, 236, 22);
    vocalSrcLbl.setBounds    (36,  136, 114, 12); vocalSrcBox.setBounds    (36,  149, 114, 22);
    vocalTgtLbl.setBounds    (158, 136, 114, 12); vocalTgtBox.setBounds    (158, 149, 114, 22);
    vocalInLbl.setBounds     (46,  178, 94,  12); vocalInSlider.setBounds  (56,  191, 74,  72);
    vocalOutLbl.setBounds    (168, 178, 94,  12); vocalOutSlider.setBounds (178, 191, 74,  72);

    sourceLbl.setBounds   (36,  278, 114, 12); sourceBox.setBounds   (36,  291, 114, 22);
    monitorLbl.setBounds  (158, 278, 114, 12); monitorBox.setBounds  (158, 291, 114, 22);
    roomLbl.setBounds     (36,  321, 114, 12); roomBox.setBounds     (36,  334, 114, 22);
    wallsLbl.setBounds    (158, 321, 114, 12); wallsBox.setBounds    (158, 334, 114, 22);
    driveLbl.setBounds    (87,  366, 134, 12); driveSlider.setBounds (112, 380, 84,  92);

    mainStereoBtn.setBounds (716, 89, 114, 20);
    roomOnBtn.setBounds     (838, 89, 86,  20);
    roomPad.setBounds       (308, 114, 616, 274);

    mic1GainLbl.setBounds   (310, 395, 100, 13); mic1GainSlider.setBounds   (324, 409, 72, 72);
    mic2GainLbl.setBounds   (412, 395, 100, 13); mic2GainSlider.setBounds   (426, 409, 72, 72);
    mic3GainLbl.setBounds   (514, 395, 100, 13); mic3GainSlider.setBounds   (528, 409, 72, 72);
    mic4GainLbl.setBounds   (616, 395, 100, 13); mic4GainSlider.setBounds   (630, 409, 72, 72);
    pairWidthLbl.setBounds  (718, 395, 100, 13); pairWidthSlider.setBounds  (732, 409, 72, 72);
    deccaWidthLbl.setBounds (820, 395, 100, 13); deccaWidthSlider.setBounds (834, 409, 72, 72);

    chamberOnBtn.setBounds (1136, 89, 68, 20);
    chamberLbl.setBounds   (960, 116, 160, 13); chamberBox.setBounds (960, 130, 244, 24);
    loadIRBtn.setBounds    (960, 162, 112, 24); irNameLbl.setBounds  (1078, 162, 126, 24);

    chMixLbl.setBounds   (976,  206, 96, 13); chMixSlider.setBounds   (981,  221, 86, 96);
    chDecayLbl.setBounds (1094, 206, 96, 13); chDecaySlider.setBounds (1099, 221, 86, 96);
    chPreLbl.setBounds   (976,  342, 96, 13); chPreSlider.setBounds   (981,  357, 86, 96);
    chWidthLbl.setBounds (1094, 342, 96, 13); chWidthSlider.setBounds (1099, 357, 86, 96);

    eqOnBtn.setBounds   (528, 504, 76, 22);
    eqTypeLbl.setBounds (42,  534, 80, 14); eqTypeBox.setBounds (124, 530, 224, 24);

    eqHpfLbl.setBounds  (58,  576, 96, 14); eqHpfSlider.setBounds  (58,  592, 96, 114);
    eqLowLbl.setBounds  (196, 576, 96, 14); eqLowSlider.setBounds  (196, 592, 96, 114);
    eqMidLbl.setBounds  (334, 576, 96, 14); eqMidSlider.setBounds  (334, 592, 96, 114);
    eqHighLbl.setBounds (472, 576, 96, 14); eqHighSlider.setBounds (472, 592, 96, 114);

    compOnBtn.setBounds     (948, 504, 80, 22);
    compLbl.setBounds       (650, 574, 185, 14); compBox.setBounds       (650, 592, 196, 28);
    compThreshLbl.setBounds (880, 576, 106, 14); compThreshSlider.setBounds (885, 592, 96, 114);

    outGainLbl.setBounds    (1086, 576, 96, 14); outGainSlider.setBounds (1086, 592, 96, 114);
}
