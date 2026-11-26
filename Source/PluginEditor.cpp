#include "PluginEditor.h"

#if HAS_CUSTOM_ASSETS
 #include "BinaryData.h"
#endif

namespace
{
    void removeGreenScreen (juce::Image& img, bool trimBottomBand = false)
    {
        if (! img.isValid()) return;

        juce::Image argb = img.convertedToFormat (juce::Image::ARGB);
        const int w = argb.getWidth();
        const int h = argb.getHeight();
        const int effectiveBottom = trimBottomBand ? static_cast<int> (h * 0.84f) : h;

        juce::Image::BitmapData data (argb, juce::Image::BitmapData::readWrite);

        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                if (y >= effectiveBottom)
                {
                    data.setPixelColour (x, y, juce::Colour (0x00000000));
                    continue;
                }

                const auto px = data.getPixelColour (x, y);
                const int r = static_cast<int> (px.getRed());
                const int g = static_cast<int> (px.getGreen());
                const int b = static_cast<int> (px.getBlue());

                const int excessGreen = g - std::max (r, b);

                if (g > 85 && excessGreen > 34)
                {
                    data.setPixelColour (x, y, juce::Colour (0x00000000));
                }
                else if (g > 70 && excessGreen > 14)
                {
                    const float spill = std::clamp ((static_cast<float> (excessGreen) - 14.0f) / 22.0f, 0.0f, 1.0f);
                    const int newAlpha = static_cast<int> (std::round (px.getAlpha() * (1.0f - spill)));
                    const int clampedG = std::min (g, std::max (r, b) + 12);
                    data.setPixelColour (x, y, juce::Colour::fromRGBA (static_cast<juce::uint8> (r),
                                                                      static_cast<juce::uint8> (clampedG),
                                                                      static_cast<juce::uint8> (b),
                                                                      static_cast<juce::uint8> (std::clamp (newAlpha, 0, 255))));
                }
            }
        }

        img = argb;
    }
}

//==============================================================================
// RusticStudioLookAndFeel (Perillas Hardware Vintage en Baquelita + Bronce)
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

    constexpr int numTicks = 11;
    const float tickR = radius + 4.2f;
    for (int i = 0; i < numTicks; ++i)
    {
        const float frac = static_cast<float> (i) / static_cast<float> (numTicks - 1);
        const float a = rotaryStartAngle + frac * (rotaryEndAngle - rotaryStartAngle);
        const float tx = cx + std::sin (a) * tickR;
        const float ty = cy - std::cos (a) * tickR;
        const bool lit = frac <= sliderPos + 0.01f;
        const float dotR = (i == 0 || i == 5 || i == 10) ? 1.55f : 1.15f;
        g.setColour (lit ? juce::Colour (0xffe5b05c) : juce::Colour (0xff594a3a));
        g.fillEllipse (tx - dotR, ty - dotR, dotR * 2.0f, dotR * 2.0f);
    }

    g.setColour (juce::Colours::black.withAlpha (0.72f));
    g.fillEllipse (cx - radius + 1.5f, cy - radius + 3.8f, radius * 2.0f, radius * 2.0f);

    juce::ColourGradient skirt (juce::Colour (0xff362f29), cx - radius * 0.55f, cy - radius * 0.55f,
                                juce::Colour (0xff0f0c0a), cx + radius * 0.65f, cy + radius * 0.65f, true);
    g.setGradientFill (skirt);
    g.fillEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

    g.setColour (juce::Colour (0xff6e5a42).withAlpha (0.65f));
    g.drawEllipse (cx - radius + 0.5f, cy - radius + 0.5f, radius * 2.0f - 1.0f, radius * 2.0f - 1.0f, 1.0f);

    const float capR = radius * 0.68f;
    juce::ColourGradient cap (juce::Colour (0xff9e7a47), cx - capR * 0.45f, cy - capR * 0.50f,
                              juce::Colour (0xff2b2014), cx + capR * 0.55f, cy + capR * 0.55f, true);
    g.setGradientFill (cap);
    g.fillEllipse (cx - capR, cy - capR, capR * 2.0f, capR * 2.0f);

    g.setColour (juce::Colour (0xffd9ab62).withAlpha (0.72f));
    g.drawEllipse (cx - capR, cy - capR, capR * 2.0f, capR * 2.0f, 1.05f);

    juce::Path pointer;
    pointer.addRoundedRectangle (-1.5f, -radius * 0.88f, 3.0f, radius * 0.52f, 1.4f);
    pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (cx, cy));
    g.setColour (juce::Colour (0xfffaf0dc));
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
// Studio4MicStagePad
//==============================================================================
Studio4MicStagePad::Studio4MicStagePad (BenBocazStudioProProcessor& proc)
    : processor (proc), apvts (proc.apvts)
{
    startTimerHz (24);
}

void Studio4MicStagePad::setSourceImage (int idx, const juce::Image& img)
{
    if (idx >= 0 && idx < 7)
    {
        juce::Image copy = img;
        removeGreenScreen (copy, idx == 5);
        sourceImgs[static_cast<size_t> (idx)] = copy;
        repaint();
    }
}

void Studio4MicStagePad::setMicImage (int idx, const juce::Image& img)
{
    if (idx >= 0 && idx < 4)
    {
        juce::Image copy = img;
        removeGreenScreen (copy, false);
        micImgs[static_cast<size_t> (idx)] = copy;
        repaint();
    }
}

void Studio4MicStagePad::setSelectedStand (int idx)
{
    const int clamped = std::clamp (idx, 0, 3);
    if (selectedStand != clamped)
    {
        selectedStand = clamped;
        if (onSelectedStandChanged) onSelectedStandChanged (selectedStand);
        repaint();
    }
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
    const int roomIdx = std::clamp (static_cast<int> (apvts.getRawParameterValue ("room")->load()), 0, 2);

    // En Volcano (roomIdx == 2) la fuente va al fondo, así que permitimos llevar los mics más atrás (yTopRatio = 0.61f)
    // con un trapecio lateral ajustado a la perspectiva del salón
    const float yTopRatio = (roomIdx == 2) ? 0.61f : 0.75f;
    const float yBotPad   = (roomIdx == 2) ? 12.0f : 14.0f;
    const float backHalfW = (roomIdx == 2) ? 0.17f : 0.30f;
    const float frontHalfW = 0.43f;

    const float yTop  = area.getY() + area.getHeight() * yTopRatio;
    const float yBot  = area.getBottom() - yBotPad;
    const float sy    = juce::jmap (my, 0.0f, 1.0f, yTop, yBot);
    const float halfW = juce::jmap (my, 0.0f, 1.0f, area.getWidth() * backHalfW, area.getWidth() * frontHalfW);
    const float sx    = area.getCentreX() + mx * halfW;
    return { sx, sy };
}

float Studio4MicStagePad::getStandScale (int idx) const
{
    const float my = apvts.getRawParameterValue ("mic" + juce::String (idx + 1) + "Y")->load();
    const int roomIdx = std::clamp (static_cast<int> (apvts.getRawParameterValue ("room")->load()), 0, 2);
    const float minScale = (roomIdx == 2) ? 0.62f : 0.78f;
    const float maxScale = (roomIdx == 2) ? 1.18f : 1.20f;
    return juce::jmap (my, 0.0f, 1.0f, minScale, maxScale);
}

float Studio4MicStagePad::getStandVisualHeight (int idx, float scale) const
{
    // Alturas diferenciadas:
    // 0: MAIN, 1: ROOM PAIR (par estéreo de sala), 2: FAR L, 3: FAR R
    static constexpr std::array<float, 4> baseHeights { 116.0f, 146.0f, 134.0f, 134.0f };
    const float baseH = baseHeights[static_cast<size_t> (std::clamp (idx, 0, 3))];

    const float padFactor = std::clamp (static_cast<float> (getHeight()) / 360.0f, 0.88f, 1.48f);
    return baseH * scale * padFactor;
}

juce::Rectangle<float> Studio4MicStagePad::getStandTagRect (int idx, juce::Rectangle<float> area) const
{
    const auto base = getStandBasePos (idx, area);
    const float sc  = getStandScale (idx);
    const float vh  = getStandVisualHeight (idx, sc);
    const float tagW = (idx == 0) ? 102.0f : 86.0f;
    const float tagY = std::max (area.getY() + 26.0f, base.y - vh - 14.0f);
    return { base.x - tagW * 0.5f, tagY, tagW, 15.0f };
}

int Studio4MicStagePad::findTargetStand (juce::Point<float> pt, juce::Rectangle<float> area, float& outMinDist, bool preferSelected) const
{
    for (int i = 3; i >= 0; --i)
    {
        const float g = apvts.getRawParameterValue ("mic" + juce::String (i + 1) + "Gain")->load();
        if (g <= 0.005f) continue;

        if (getStandTagRect (i, area).expanded (3.0f).contains (pt))
        {
            outMinDist = 0.0f;
            return i;
        }
    }

    auto distToStand = [&] (int i)
    {
        const auto base = getStandBasePos (i, area);
        const float sc  = getStandScale (i);
        const float vh  = getStandVisualHeight (i, sc);
        const juce::Point<float> topPt (base.x, base.y - vh * 0.82f);
        const juce::Point<float> midPt (base.x, base.y - vh * 0.42f);
        return std::min ({ pt.getDistanceFrom (base), pt.getDistanceFrom (topPt), pt.getDistanceFrom (midPt) });
    };

    if (preferSelected)
    {
        const float selGain = apvts.getRawParameterValue ("mic" + juce::String (selectedStand + 1) + "Gain")->load();
        if (selGain > 0.005f)
        {
            const float selDist = distToStand (selectedStand);
            if (selDist < 36.0f)
            {
                outMinDist = selDist;
                return selectedStand;
            }
        }
    }

    outMinDist = 1e9f;
    int best = selectedStand;
    for (int i = 0; i < 4; ++i)
    {
        const float g = apvts.getRawParameterValue ("mic" + juce::String (i + 1) + "Gain")->load();
        if (g <= 0.005f) continue;

        const float d = distToStand (i);
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

    if (sourceIdx == 4)
        drawCabinet (cx, floorY, 64.0f * scale, 54.0f * scale, juce::Colour (0xff241c16), juce::Colour (0xff8a7152), 1);
    else if (sourceIdx == 5)
    {
        drawCabinet (cx - 42.0f * scale, floorY, 58.0f * scale, 48.0f * scale, juce::Colour (0xff241c16), juce::Colour (0xff8a7152), 1);
        drawCabinet (cx + 42.0f * scale, floorY, 58.0f * scale, 48.0f * scale, juce::Colour (0xff241c16), juce::Colour (0xff8a7152), 1);
    }
    else
    {
        drawCabinet (cx, floorY, 72.0f * scale, 66.0f * scale, juce::Colour (0xff191716), juce::Colour (0xff3a3836), 2);
        g.setColour (juce::Colour (0xff2b2927).withAlpha (alpha));
        g.fillRoundedRectangle (cx - 32.0f * scale, floorY - 82.0f * scale, 64.0f * scale, 16.0f * scale, 2.0f);
        g.setColour (juce::Colour (0xffe5a94c).withAlpha (0.85f * alpha));
        g.drawRect (cx - 27.0f * scale, floorY - 77.0f * scale, 54.0f * scale, 6.0f * scale, 1.0f);
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
        "1. Single Mic Anchor: 1:MAIN Mono centered close, ROOM PAIR & FAR at 0.\n"
        "2. Room & Depth: 1:MAIN close + 2:ROOM PAIR at mid distance for natural air.\n"
        "3. Wide Hall Depth: Move 1:MAIN back + enable LINK 3+4 with FAR L/R.",

        "1. Coincident Overhead: 1:MAIN X-Y centered for punchy kit focus.\n"
        "2. Studio Room Punch: 2:ROOM PAIR with PAIR WIDE open for cymbals.\n"
        "3. Huge Live Room: Add LINK 3+4 (FAR L/R) + Parallel Crush on Room Mics.",

        "1. Single Acoustic Photo: 1:MAIN X-Y centered at medium distance.\n"
        "2. Wood Width: 1:MAIN anchor + 2:ROOM PAIR spaced for warmth.\n"
        "3. Deep Hall Tail: 1:MAIN + LINK 3+4 (FAR L/R) wide at the sides.",

        "1. Reference Focus: 1:MAIN centered to capture direct speaker punch.\n"
        "2. Controlled Room: 1:MAIN X-Y + 2:ROOM PAIR low for natural wall bounce.\n"
        "3. Wide Stage: Push 2:ROOM PAIR + LINK 3+4 (FAR L/R) for lush spread.",

        "1. Cone Edge Bite: 1:MAIN Mono close & centered for tight riffs.\n"
        "2. Cab + Room Air: 1:MAIN close + 2:ROOM PAIR for organic wood body.\n"
        "3. Symmetrical Depth: Enable LINK 3+4 with FAR L/R mirrored.",

        "1. Stereo Cab Capture: 1:MAIN X-Y centered to preserve chorus/delay.\n"
        "2. Wide Pair Stage: 2:ROOM PAIR with PAIR WIDE open on both cabs.\n"
        "3. Huge Live Wall: Add LINK 3+4 (FAR L/R) + Chamber 3 for 3D width.",

        "1. Sub Core Anchor: 1:MAIN Mono close/center, ambient mics low.\n"
        "2. Growl + Wood Body: 1:MAIN + 2:ROOM PAIR with EQ HPF around 40-50Hz.\n"
        "3. Warm Room Weight: Subtle LINK 3+4 (FAR L/R) with PHASE SAFE locked."
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
    const int roomIdx     = std::clamp (static_cast<int> (apvts.getRawParameterValue ("room")->load()), 0, 2);
    const int wallsLive   = static_cast<int> (apvts.getRawParameterValue ("walls")->load());
    const int sourceIdx   = static_cast<int> (apvts.getRawParameterValue ("source")->load());
    const int monitorIdx  = static_cast<int> (apvts.getRawParameterValue ("monitorType")->load());
    const bool phaseOk    = apvts.getRawParameterValue ("phaseAlign")->load() >= 0.5f;
    const bool mainStereo = apvts.getRawParameterValue ("mainStereo")->load() >= 0.5f;
    const float pairW     = apvts.getRawParameterValue ("pairSpread")->load();
    const float farW      = apvts.getRawParameterValue ("deccaWidth")->load();

    juce::Path clip;
    clip.addRoundedRectangle (area, 6.0f);
    g.saveState();
    g.reduceClipRegion (clip);

    juce::Image activeRoom;
    if (roomIdx == 2)
    {
        if (wallsLive != 0)
            activeRoom = volcanoOpenImg.isValid() ? volcanoOpenImg : volcanoClosedImg;
        else
            activeRoom = volcanoClosedImg.isValid() ? volcanoClosedImg : volcanoOpenImg;
    }
    else if (roomIdx == 1)
    {
        activeRoom = canyonImg.isValid() ? canyonImg : shangriLaImg;
    }
    else
    {
        activeRoom = shangriLaImg.isValid() ? shangriLaImg : canyonImg;
    }

    if (activeRoom.isValid())
    {
        g.drawImage (activeRoom, area, juce::RectanglePlacement::fillDestination);
        const float dimAlpha = ! roomIsOn ? 0.72f : (wallsLive ? 0.08f : 0.30f);
        g.setColour (juce::Colour (0xff140e09).withAlpha (dimAlpha));
        g.fillRect (area);
    }
    else
    {
        juce::ColourGradient wood (roomIdx == 2 ? juce::Colour (0xff3d2414) : (roomIdx == 0 ? juce::Colour (0xff4a311e) : juce::Colour (0xff2e1c11)),
                                   area.getTopLeft(), juce::Colour (0xff170f0a), area.getBottomRight(), false);
        g.setGradientFill (wood);
        g.fillRect (area);
    }

    const float stageAlpha = roomIsOn ? 1.0f : 0.35f;

    const float srcFloorX = area.getCentreX();
    const bool isCabinetSource = (sourceIdx >= 4);
    const bool isCanyon  = (roomIdx == 1);
    const bool isVolcano = (roomIdx == 2);

    // En Volcano (Open y Closed) llevamos los instrumentos más al fondo del salón
    float floorRatio = 0.70f;
    if (isCanyon)
        floorRatio = isCabinetSource ? 0.725f : 0.735f;
    else if (isVolcano)
        floorRatio = isCabinetSource ? 0.580f : 0.590f;
    else
        floorRatio = isCabinetSource ? 0.690f : 0.700f;

    const float srcFloorY = area.getY() + area.getHeight() * floorRatio;
    const float roomScale = isCanyon ? 0.88f : (isVolcano ? 0.66f : 1.04f);

    g.setColour (juce::Colours::black.withAlpha (0.48f * stageAlpha));
    g.fillEllipse (srcFloorX - 66.0f * roomScale, srcFloorY - 7.0f, 132.0f * roomScale, 16.0f);

    const size_t sIdx = static_cast<size_t> (std::clamp (sourceIdx, 0, 6));
    const auto& srcImg = sourceImgs[sIdx];
    if (srcImg.isValid())
    {
        // Solo guitar_mono (idx 4) se reduce respecto al tamaño anterior; guitar_stereo (idx 5) mantiene su escala
        static constexpr std::array<float, 7> srcHeightRatios { 0.33f, 0.36f, 0.37f, 0.31f, 0.17f, 0.23f, 0.29f };
        const float drawH  = area.getHeight() * srcHeightRatios[sIdx] * roomScale;
        const float aspect = static_cast<float> (srcImg.getWidth()) / std::max (1.0f, static_cast<float> (srcImg.getHeight()));
        const float drawW  = drawH * aspect;
        float yNudge = isCabinetSource ? 6.0f : 8.0f;
        if (sourceIdx == 5)
            yNudge += drawH * 0.16f; // Compensa el 16% recortado en la base de Guitar Cab (Stereo)

        juce::Rectangle<float> srcDest (srcFloorX - drawW * 0.5f, srcFloorY - drawH + yNudge, drawW, drawH);
        g.setOpacity (stageAlpha);
        g.drawImage (srcImg, srcDest, juce::RectanglePlacement::stretchToFit);
        g.setOpacity (1.0f);
    }
    else if (sourceIdx >= 4)
    {
        drawAmpFallback (g, srcFloorX, srcFloorY + 12.0f, sourceIdx, roomScale, stageAlpha);
    }

    static const std::array<juce::String, 7> srcLabels {
        "SOURCE: VOCALS", "SOURCE: DRUMS", "SOURCE: ENSEMBLE", "MONITORS",
        "GUITAR CAB (MONO)", "GUITAR CAB (STEREO)", "BASS AMP (LOW END)"
    };
    juce::String srcLbl = srcLabels[sIdx];
    if (sourceIdx == 3)
        srcLbl = (monitorIdx == 0 ? "MONITORS: ALTEC" : (monitorIdx == 1 ? "MONITORS: NS-10" : "MONITORS: JBL PA"));

    g.setColour (juce::Colour (0xff120d09).withAlpha (0.86f * stageAlpha));
    g.fillRoundedRectangle (srcFloorX - 70.0f, area.getY() + 28.0f, 140.0f, 15.5f, 3.5f);
    g.setColour (juce::Colour (0xfff2e6d0).withAlpha (0.92f * stageAlpha));
    g.drawRoundedRectangle (srcFloorX - 70.0f, area.getY() + 28.0f, 140.0f, 15.5f, 3.5f, 0.9f);
    g.setFont (juce::Font (juce::FontOptions (8.5f).withStyle ("Bold")));
    g.drawText (srcLbl, static_cast<int> (srcFloorX - 70), static_cast<int> (area.getY() + 28), 140, 16, juce::Justification::centred);

    if (linkAirDecca && roomIsOn)
    {
        const float g3 = apvts.getRawParameterValue ("mic3Gain")->load();
        const float g4 = apvts.getRawParameterValue ("mic4Gain")->load();
        if (g3 > 0.005f && g4 > 0.005f)
        {
            const auto p3 = getStandBasePos (2, area);
            const auto p4 = getStandBasePos (3, area);
            g.setColour (juce::Colour (0xff8fc7e8).withAlpha (0.52f));
            float dash[2] = { 4.0f, 4.0f };
            g.drawDashedLine (juce::Line<float> (p3, p4), dash, 2, 1.3f);
        }
    }

    std::array<int, 4> order { 0, 1, 2, 3 };
    std::sort (order.begin(), order.end(), [this] (int a, int b)
    {
        const float ya = apvts.getRawParameterValue ("mic" + juce::String (a + 1) + "Y")->load();
        const float yb = apvts.getRawParameterValue ("mic" + juce::String (b + 1) + "Y")->load();
        return ya < yb;
    });

    static const std::array<juce::String, 4> baseNames { "MAIN", "ROOM PAIR", "FAR L", "FAR R" };
    static const std::array<juce::Colour, 4> cols {
        juce::Colour (0xfff2b24c),
        juce::Colour (0xffe07a52),
        juce::Colour (0xff8fc7e8),
        juce::Colour (0xfff5ebd6)
    };

    const float roomD = (roomIdx == 0) ? 12.5f : ((roomIdx == 1) ? 8.5f : 16.0f);
    const int soloIdx = processor.getSoloStand();

    for (int idx : order)
    {
        const float gain = apvts.getRawParameterValue ("mic" + juce::String (idx + 1) + "Gain")->load();
        const bool mutedBySolo = (soloIdx >= 0 && soloIdx != idx);

        if (gain <= 0.005f || mutedBySolo)
            continue;

        const auto base   = getStandBasePos (idx, area);
        const float sc    = getStandScale (idx);
        const float vh    = getStandVisualHeight (idx, sc);
        const float headX = base.x;
        const float headY = base.y - vh * 0.84f;

        const float my    = apvts.getRawParameterValue ("mic" + juce::String (idx + 1) + "Y")->load();
        const float distM = juce::jmap (my, 0.0f, 1.0f, 0.35f, roomD - 1.5f);
        const float alpha = stageAlpha;
        const auto col    = cols[static_cast<size_t> (idx)];
        const bool active = (idx == selectedStand || idx == hoveredStand) && roomIsOn;

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
        {
            micSpriteIdx = mainStereo ? 1 : 0;
        }
        else if (idx == 1)
        {
            micSpriteIdx = (pairW < 0.08f) ? 0 : 2;
            widthStretch = (pairW < 0.08f) ? 0.92f : juce::jmap (pairW, 0.08f, 1.5f, 0.68f, 1.32f);
        }
        else
        {
            // FAR L y FAR R son dos micrófonos individuales lejanos
            micSpriteIdx = 0;
            widthStretch = 0.95f;
        }

        const auto& mImg = micImgs[static_cast<size_t> (micSpriteIdx)];

        const float ringR = (idx >= 2 ? 15.0f : 16.0f) * sc;
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

        juce::String tag = juce::String (idx + 1) + ":" + baseNames[static_cast<size_t> (idx)];
        if (idx == 0) tag += (mainStereo ? " X-Y" : " MONO");
        tag += " " + juce::String (distM, 1) + "m";

        const auto tagRect = getStandTagRect (idx, area);
        g.setColour (juce::Colour (0xff120d0a).withAlpha (0.92f * stageAlpha));
        g.fillRoundedRectangle (tagRect, 3.0f);
        g.setColour (col.withAlpha (active ? 1.0f : 0.90f * alpha));
        g.drawRoundedRectangle (tagRect, 3.0f, active ? 1.4f : 0.85f);
        g.setFont (juce::Font (juce::FontOptions (8.5f).withStyle ("Bold")));
        g.drawText (tag, tagRect.toNearestInt(), juce::Justification::centred);
    }

    if (roomIsOn)
    {
        const juce::String selPfx = "mic" + juce::String (selectedStand + 1);
        const float selX = apvts.getRawParameterValue (selPfx + "X")->load();
        const float selY = apvts.getRawParameterValue (selPfx + "Y")->load();
        const float selDist = juce::jmap (selY, 0.0f, 1.0f, 0.35f, roomD - 1.5f);

        juce::String panStr = "CENTER (0.00)";
        if (selX < -0.02f) panStr = juce::String (std::abs (selX) * 100.0f, 0) + "% L";
        else if (selX > 0.02f) panStr = juce::String (selX * 100.0f, 0) + "% R";

        juce::Rectangle<float> hud (area.getRight() - 190.0f, area.getBottom() - 42.0f, 182.0f, 35.0f);
        g.setColour (juce::Colour (0xff120d09).withAlpha (0.88f));
        g.fillRoundedRectangle (hud, 4.0f);
        g.setColour (cols[static_cast<size_t> (selectedStand)].withAlpha (0.85f));
        g.drawRoundedRectangle (hud, 4.0f, 1.0f);

        g.setFont (juce::Font (juce::FontOptions (8.6f).withStyle ("Bold")));
        g.drawText ("SELECTED: " + juce::String (selectedStand + 1) + ":" + baseNames[static_cast<size_t> (selectedStand)],
                    hud.reduced (7.0f, 3.0f).removeFromTop (14.0f), juce::Justification::centredLeft);
        g.setColour (juce::Colour (0xffe8dcc6));
        g.drawText ("DIST: " + juce::String (selDist, 1) + "m  |  PAN: " + panStr,
                    hud.reduced (7.0f, 3.0f).removeFromBottom (14.0f), juce::Justification::centredLeft);
    }

    if (showTipsOverlay)
        drawTipsOverlay (g, area, sourceIdx);

    g.restoreState();

    g.setColour ((roomIsOn && wallsLive) ? juce::Colour (0xffd9a04a) : juce::Colour (0xff5c4f41));
    g.drawRoundedRectangle (area, 6.0f, (roomIsOn && wallsLive) ? 2.0f : 1.4f);

    auto topBar = area.reduced (8.0f).removeFromTop (16.0f);
    g.setColour (juce::Colour (0xff140f0b).withAlpha (0.85f));
    g.fillRoundedRectangle (topBar.expanded (4.0f, 2.0f), 3.0f);

    g.setColour (juce::Colour (0xfff4d08c).withAlpha (roomIsOn ? 1.0f : 0.55f));
    g.setFont (juce::Font (juce::FontOptions (9.8f).withStyle ("Bold")));
    juce::String roomTitle = (roomIdx == 0 ? "STUDIO A (SHANGRI-LA)" : ((roomIdx == 1) ? "STUDIO B (CANYON)" : "VOLCANO"));
    roomTitle += (wallsLive ? "  |  LIVE WOOD" : "  |  TIGHT");
    g.drawText (roomTitle, topBar, juce::Justification::centredLeft);

    if (! roomIsOn)
    {
        g.setColour (juce::Colour (0xffc99e6b));
        g.drawText ("ROOM BYPASSED", topBar, juce::Justification::centredRight);
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
    const int nearest = findTargetStand (e.position, area, dist, true);
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
    const int target = findTargetStand (e.position, area, dist, true);
    setSelectedStand (target);
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
    const int nearest = findTargetStand (e.position, area, dist, true);
    if (dist < 48.0f)
    {
        setSelectedStand (nearest);
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
    const int roomIdx = std::clamp (static_cast<int> (apvts.getRawParameterValue ("room")->load()), 0, 2);
    const float yTopRatio = (roomIdx == 2) ? 0.61f : 0.75f;
    const float yBotPad   = (roomIdx == 2) ? 12.0f : 14.0f;
    const float backHalfW = (roomIdx == 2) ? 0.17f : 0.30f;
    const float frontHalfW = 0.43f;

    const float yTop = area.getY() + area.getHeight() * yTopRatio;
    const float yBot = area.getBottom() - yBotPad;

    const float normY = juce::jlimit (0.0f, 1.0f, (targetBase.y - yTop) / std::max (1.0f, yBot - yTop));
    const float halfW = juce::jmap (normY, 0.0f, 1.0f, area.getWidth() * backHalfW, area.getWidth() * frontHalfW);
    float normX = juce::jlimit (0.0f, 1.0f, 0.5f + 0.5f * ((targetBase.x - area.getCentreX()) / std::max (1.0f, halfW)));

    if (snapToCenter && std::abs (normX - 0.5f) < 0.032f)
        normX = 0.5f;

    const juce::String pfx = "mic" + juce::String (idx + 1);
    if (auto* pX = apvts.getParameter (pfx + "X")) pX->setValueNotifyingHost (normX);
    if (auto* pY = apvts.getParameter (pfx + "Y")) pY->setValueNotifyingHost (normY);

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

    roomPad.onSelectedStandChanged = [this] (int selIdx) { updateSelectedStandButtonsUI (selIdx); };

    presetLbl.setText ("PRESETS", juce::dontSendNotification);
    presetLbl.setColour (juce::Label::textColourId, juce::Colour (0xffe5a94c));
    presetLbl.setFont (juce::Font (juce::FontOptions (9.6f).withStyle ("Bold")));
    addAndMakeVisible (presetLbl);

    presetBox.addItemList ({
        "01. Simple 2-Mic Clean (Default)",
        "02. Vocal C-800G Warm & Air",
        "03. Shangri-La Live Drums",
        "04. Canyon Guitar Stereo",
        "05. Bass Amp Low Weight",
        "06. Swedien NS-10 Re-Mic"
    }, 1);
    presetBox.setSelectedItemIndex (0, juce::dontSendNotification);
    presetBox.onChange = [this] { applyFactoryPreset (presetBox.getSelectedItemIndex()); };
    addAndMakeVisible (presetBox);

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
    setupCombo (roomBox,      { "Studio A (Shangri-La)", "Studio B (Canyon)", "Volcano" }, roomLbl, "STUDIO ROOM");
    setupCombo (wallsBox,     { "Tight / Damped", "Live / Cedar Wood" }, wallsLbl, "ROOM WALLS");
    setupCombo (eqTubeBox,    { "Neve Class-A", "Pultec 12AX7", "API 2520", "SSL Clean" }, eqTubeLbl, "TUBE CIRCUIT");
    setupCombo (chamberBox,   { "Chamber 1 (Bright)", "Chamber 3 (Classic)", "Chamber 5 (Deep)", "Chamber 8 (Dense)", "Custom WAV IR" }, chamberLbl, "ECHO CHAMBER");
    setupCombo (eqTypeBox,    { "Neve 1073 (British)", "Pultec EQP-1A (Tube)", "API 550A (Punch)", "SSL 4000E (Clean)" }, eqTypeLbl, "EQ MODEL");
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

    setupStandKnob (mic1GainSlider, mic1SelBtn, "1: MAIN",      0);
    setupStandKnob (mic2GainSlider, mic2SelBtn, "2: ROOM PAIR", 1);
    setupStandKnob (mic3GainSlider, mic3SelBtn, "3: FAR L",     2);
    setupStandKnob (mic4GainSlider, mic4SelBtn, "4: FAR R",     3);
    updateSelectedStandButtonsUI (0);

    setupKnob (pairWidthSlider,  pairWidthLbl,  "PAIR WIDE");
    setupKnob (deccaWidthSlider, deccaWidthLbl, "FAR WIDE");
    setupKnob (chMixSlider,      chMixLbl,      "SEND");
    setupKnob (chDecaySlider,    chDecayLbl,    "DECAY");
    setupKnob (chPreSlider,      chPreLbl,      "PRE-DELAY");
    setupKnob (chWidthSlider,    chWidthLbl,    "WIDTH");

    setupKnob (eqHpfSlider,      eqHpfLbl,      "HPF");
    setupKnob (eqLowSlider,      eqLowLbl,      "LOW");
    setupKnob (eqMidSlider,      eqMidLbl,      "MID");
    setupKnob (eqHighSlider,     eqHighLbl,     "HIGH");

    setupKnob (compThreshSlider, compThreshLbl, "THRESHOLD");
    setupKnob (compMixSlider,    compMixLbl,    "MIX");
    setupKnob (parAmtSlider,     parAmtLbl,     "PAR BLEND");
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

void BenBocazStudioProEditor::updateSelectedStandButtonsUI (int selIdx)
{
    mic1SelBtn.setToggleState (selIdx == 0, juce::dontSendNotification);
    mic2SelBtn.setToggleState (selIdx == 1, juce::dontSendNotification);
    mic3SelBtn.setToggleState (selIdx == 2, juce::dontSendNotification);
    mic4SelBtn.setToggleState (selIdx == 3, juce::dontSendNotification);
}

void BenBocazStudioProEditor::applyFactoryPreset (int idx)
{
    auto setParam = [this] (const juce::String& id, float plainVal)
    {
        if (auto* param = audioProcessor.apvts.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (plainVal));
    };

    if (idx == 0)
    {
        setParam ("mode", 1); setParam ("source", 0); setParam ("roomOn", 1); setParam ("mainStereo", 0);
        setParam ("mic1X", 0.0f); setParam ("mic1Y", 0.16f); setParam ("mic1Gain", 1.0f);
        setParam ("mic2X", -0.38f); setParam ("mic2Y", 0.48f); setParam ("mic2Gain", 0.25f);
        setParam ("mic3Gain", 0.0f); setParam ("mic4Gain", 0.0f);
        setParam ("chamberMix", 0.16f); setParam ("eqLow", 0.0f); setParam ("eqMid", 0.0f); setParam ("eqHigh", 0.0f);
        setParam ("parCompOn", 0);
    }
    else if (idx == 1)
    {
        setParam ("mode", 1); setParam ("source", 0); setParam ("room", 0); setParam ("walls", 1);
        setParam ("vocalModOn", 1); setParam ("vocalSrcMic", 2); setParam ("vocalTgtMic", 0);
        setParam ("mainStereo", 0); setParam ("mic1X", 0.0f); setParam ("mic1Y", 0.16f);
        setParam ("mic1Gain", 1.05f); setParam ("mic2Gain", 0.25f); setParam ("mic3Gain", 0.30f); setParam ("mic4Gain", 0.0f);
        setParam ("eqType", 1); setParam ("eqHighFreq", 2); setParam ("eqHigh", 2.0f);
        setParam ("compType", 1); setParam ("compMix", 0.85f); setParam ("chamberMix", 0.22f);
    }
    else if (idx == 2)
    {
        setParam ("mode", 1); setParam ("source", 1); setParam ("room", 0); setParam ("walls", 1);
        setParam ("mainStereo", 1); setParam ("mic1X", 0.0f); setParam ("mic1Y", 0.24f);
        setParam ("mic1Gain", 0.95f); setParam ("mic2Gain", 0.65f); setParam ("mic3Gain", 0.55f); setParam ("mic4Gain", 0.50f);
        setParam ("pairSpread", 1.15f); setParam ("deccaWidth", 1.20f);
        setParam ("eqType", 2); setParam ("parCompOn", 1); setParam ("parCompRoute", 1); setParam ("parCompAmt", 0.55f);
    }
    else if (idx == 3)
    {
        setParam ("mode", 1); setParam ("source", 5); setParam ("room", 1); setParam ("walls", 1);
        setParam ("mainStereo", 1); setParam ("mic1X", 0.0f); setParam ("mic1Y", 0.20f);
        setParam ("mic1Gain", 1.0f); setParam ("mic2Gain", 0.50f); setParam ("mic3Gain", 0.45f); setParam ("mic4Gain", 0.30f);
        setParam ("eqType", 0); setParam ("eqMidFreq", 2); setParam ("eqMid", 1.8f);
    }
    else if (idx == 4)
    {
        setParam ("mode", 1); setParam ("source", 6); setParam ("room", 1); setParam ("walls", 0);
        setParam ("mainStereo", 0); setParam ("mic1X", 0.0f); setParam ("mic1Y", 0.14f);
        setParam ("mic1Gain", 1.10f); setParam ("mic2Gain", 0.32f); setParam ("mic3Gain", 0.0f); setParam ("mic4Gain", 0.22f);
        setParam ("eqType", 0); setParam ("eqLowFreq", 1); setParam ("eqLow", 2.5f);
        setParam ("compType", 0); setParam ("compScFilter", 1);
    }
    else
    {
        setParam ("mode", 1); setParam ("source", 3); setParam ("monitorType", 1); setParam ("room", 0);
        setParam ("mainStereo", 1); setParam ("mic1Gain", 0.85f); setParam ("mic2Gain", 0.55f);
        setParam ("mic3Gain", 0.45f); setParam ("mic4Gain", 0.0f); setParam ("consoleDrive", 0.32f);
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

            if (resName.containsIgnoreCase ("volcano"))
            {
                if (resName.containsIgnoreCase ("closed"))
                    roomPad.setVolcanoClosedImage (img);
                else
                    roomPad.setVolcanoOpenImage (img);
            }
            else if (resName.containsIgnoreCase ("canyon"))
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

void BenBocazStudioProEditor::setupKnob (juce::Slider& slider, juce::Label& label, const juce::String& text)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 54, 14);
    slider.setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffe0d1b8));
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (slider);

    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, juce::Colour (0xffe5b05c));
    label.setFont (juce::Font (juce::FontOptions (9.0f).withStyle ("Bold")));
    addAndMakeVisible (label);
}

void BenBocazStudioProEditor::setupStandKnob (juce::Slider& slider, juce::TextButton& selectBtn, const juce::String& text, int standIdx)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 54, 14);
    slider.setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffe0d1b8));
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider.onDragStart = [this, standIdx] { roomPad.setSelectedStand (standIdx); };
    addAndMakeVisible (slider);

    selectBtn.setButtonText (text);
    selectBtn.setClickingTogglesState (false);
    selectBtn.onClick = [this, standIdx] { roomPad.setSelectedStand (standIdx); };
    addAndMakeVisible (selectBtn);
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
        drawPanel ({ 24,   108, 256, 450 }, "VOCAL MIC MOD & SOURCE");
        drawPanel ({ 292,  108, 656, 450 }, "LIVE ROOM 3D STAGE");
        drawPanel ({ 960,  108, 276, 450 }, "ECHO CHAMBERS & IR");
        drawPanel ({ 24,   568, 576, 208 }, "ANALOG CONSOLE EQ");
        drawPanel ({ 612,  568, 476, 208 }, "OUTBOARD & PARALLEL COMP");
        drawPanel ({ 1100, 568, 136, 208 }, "MASTER");

        g.setColour (juce::Colour (0xff614e39).withAlpha (0.55f));
        g.drawHorizontalLine (286, 36.0f, 268.0f);
        g.drawHorizontalLine (394, 36.0f, 268.0f);
        g.drawVerticalLine (916, 598.0f, 766.0f);
    }
}

void BenBocazStudioProEditor::resized()
{
    modeLbl.setBounds       (390, 10, 96, 13);  modeBox.setBounds       (390, 25, 100, 25);
    chainLbl.setBounds      (500, 10, 165, 13); chainBox.setBounds      (500, 25, 176, 25);
    phaseAlignBtn.setBounds (688, 9,  112, 23);
    phaseInvBtn.setBounds   (688, 36, 112, 23);
    vuMeter.setBounds       (812, 6,  312, 56);
    vuModeBtn.setBounds     (1134, 20, 94, 26);

    presetLbl.setBounds     (32,  73, 58,  22);
    presetBox.setBounds     (92,  73, 250, 22);
    snapCenterBtn.setBounds (362, 73, 106, 22);
    link34Btn.setBounds     (476, 73, 86,  22);
    centerSelBtn.setBounds  (570, 73, 92,  22);
    tipsBtn.setBounds       (1038, 73, 74,  22);
    expandStageBtn.setBounds(1120, 73, 110, 22);

    const bool showRacks = ! stageExpanded;
    for (auto* c : std::initializer_list<juce::Component*> {
        &vocalModBtn, &vocalSrcLbl, &vocalSrcBox, &vocalTgtLbl, &vocalTgtBox,
        &vocalInLbl, &vocalInSlider, &vocalOutLbl, &vocalOutSlider,
        &sourceLbl, &sourceBox, &monitorLbl, &monitorBox, &roomLbl, &roomBox, &wallsLbl, &wallsBox,
        &eqTubeLbl, &eqTubeBox, &driveLbl, &driveSlider,
        &chamberOnBtn, &chamberLbl, &chamberBox, &loadIRBtn, &irNameLbl,
        &chMixLbl, &chMixSlider, &chDecayLbl, &chDecaySlider, &chPreLbl, &chPreSlider, &chWidthLbl, &chWidthSlider,
        &eqOnBtn, &eqTypeLbl, &eqTypeBox,
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

        const int yBtn = 678, yKnb = 698;
        mic1SelBtn.setBounds (130, yBtn, 76, 17); mic1GainSlider.setBounds (134, yKnb, 68, 68); solo1Btn.setBounds (204, yKnb + 18, 22, 22);
        mic2SelBtn.setBounds (310, yBtn, 76, 17); mic2GainSlider.setBounds (314, yKnb, 68, 68); solo2Btn.setBounds (384, yKnb + 18, 22, 22);
        mic3SelBtn.setBounds (490, yBtn, 76, 17); mic3GainSlider.setBounds (494, yKnb, 68, 68); solo3Btn.setBounds (564, yKnb + 18, 22, 22);
        mic4SelBtn.setBounds (670, yBtn, 76, 17); mic4GainSlider.setBounds (674, yKnb, 68, 68); solo4Btn.setBounds (744, yKnb + 18, 22, 22);
        pairWidthLbl.setBounds  (840, yBtn + 2, 96, 13); pairWidthSlider.setBounds  (854, yKnb, 68, 68);
        deccaWidthLbl.setBounds (1010,yBtn + 2, 96, 13); deccaWidthSlider.setBounds (1024,yKnb, 68, 68);
        return;
    }

    vocalModBtn.setBounds    (36,  132, 232, 22);
    vocalSrcLbl.setBounds    (36,  158, 112, 12); vocalSrcBox.setBounds    (36,  171, 112, 22);
    vocalTgtLbl.setBounds    (156, 158, 112, 12); vocalTgtBox.setBounds    (156, 171, 112, 22);
    vocalInLbl.setBounds     (46,  198, 92,  12); vocalInSlider.setBounds  (56,  211, 70,  68);
    vocalOutLbl.setBounds    (166, 198, 92,  12); vocalOutSlider.setBounds (176, 211, 70,  68);

    sourceLbl.setBounds   (36,  294, 112, 12); sourceBox.setBounds   (36,  307, 112, 22);
    monitorLbl.setBounds  (156, 294, 112, 12); monitorBox.setBounds  (156, 307, 112, 22);
    roomLbl.setBounds     (36,  337, 112, 12); roomBox.setBounds     (36,  350, 112, 22);
    wallsLbl.setBounds    (156, 337, 112, 12); wallsBox.setBounds    (156, 350, 112, 22);

    eqTubeLbl.setBounds   (54,  402, 196, 12); eqTubeBox.setBounds   (54,  415, 196, 22);
    driveLbl.setBounds    (85,  444, 134, 12); driveSlider.setBounds (114, 458, 76,  86);

    mainStereoBtn.setBounds (728, 113, 114, 20);
    roomOnBtn.setBounds     (850, 113, 86,  20);
    roomPad.setBounds       (304, 136, 632, 326);

    mic1SelBtn.setBounds (308, 467, 72, 16); mic1GainSlider.setBounds (310, 484, 68, 66); solo1Btn.setBounds (379, 501, 20, 20);
    mic2SelBtn.setBounds (414, 467, 72, 16); mic2GainSlider.setBounds (416, 484, 68, 66); solo2Btn.setBounds (485, 501, 20, 20);
    mic3SelBtn.setBounds (520, 467, 72, 16); mic3GainSlider.setBounds (522, 484, 68, 66); solo3Btn.setBounds (591, 501, 20, 20);
    mic4SelBtn.setBounds (626, 467, 72, 16); mic4GainSlider.setBounds (628, 484, 68, 66); solo4Btn.setBounds (697, 501, 20, 20);
    pairWidthLbl.setBounds  (728, 469, 98, 13); pairWidthSlider.setBounds  (742, 484, 68, 66);
    deccaWidthLbl.setBounds (834, 469, 98, 13); deccaWidthSlider.setBounds (848, 484, 68, 66);

    chamberOnBtn.setBounds (1154, 113, 70, 20);
    chamberLbl.setBounds   (972, 140, 160, 13); chamberBox.setBounds (972, 154, 252, 24);
    loadIRBtn.setBounds    (972, 186, 114, 24); irNameLbl.setBounds  (1092, 186, 132, 24);

    chMixLbl.setBounds   (990,  238, 96, 13); chMixSlider.setBounds   (998,  253, 80, 88);
    chDecayLbl.setBounds (1110, 238, 96, 13); chDecaySlider.setBounds (1118, 253, 80, 88);
    chPreLbl.setBounds   (990,  384, 96, 13); chPreSlider.setBounds   (998,  399, 80, 88);
    chWidthLbl.setBounds (1110, 384, 96, 13); chWidthSlider.setBounds (1118, 399, 80, 88);

    eqOnBtn.setBounds       (516, 573, 72, 21);
    eqTypeLbl.setBounds     (38,  596, 80, 12); eqTypeBox.setBounds (124, 594, 210, 21);

    eqHpfLbl.setBounds  (48,  628, 86, 13); eqHpfSlider.setBounds  (54,  642, 74, 82);
    eqLowLbl.setBounds  (184, 628, 86, 13); eqLowSlider.setBounds  (190, 642, 74, 82); eqLowFreqBox.setBounds  (187, 738, 80, 20);
    eqMidLbl.setBounds  (320, 628, 86, 13); eqMidSlider.setBounds  (326, 642, 74, 82); eqMidFreqBox.setBounds  (323, 738, 80, 20);
    eqHighLbl.setBounds (456, 628, 86, 13); eqHighSlider.setBounds (462, 642, 74, 82); eqHighFreqBox.setBounds (459, 738, 80, 20);

    compOnBtn.setBounds     (998, 573, 78, 21);
    compLbl.setBounds       (626, 595, 140, 12); compBox.setBounds       (626, 608, 152, 21);
    compScLinkBtn.setBounds (786, 595, 114, 19);
    compScFiltBtn.setBounds (786, 617, 114, 19);

    compThreshLbl.setBounds (632, 648, 86, 13); compThreshSlider.setBounds (638, 662, 74, 84);
    compMixLbl.setBounds    (762, 648, 86, 13); compMixSlider.setBounds    (768, 662, 74, 84);

    parCompOnBtn.setBounds  (930, 598, 144, 20);
    parAmtLbl.setBounds     (958, 624, 88,  12); parAmtSlider.setBounds (966, 637, 72, 78);
    parRouteLbl.setBounds   (934, 722, 136, 12); parRouteBox.setBounds  (934, 735, 136, 21);

    outGainLbl.setBounds    (1124, 636, 88, 13); outGainSlider.setBounds (1130, 650, 76, 94);
}
