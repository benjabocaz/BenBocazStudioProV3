#include "PluginEditor.h"
#if HAS_CUSTOM_ASSETS
    #include "BinaryData.h"
#endif

//==============================================================================
AndesLookAndFeel::AndesLookAndFeel()
{
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff1b1511));
    setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff6b5239));
    setColour(juce::ComboBox::textColourId, juce::Colour(0xfff4e6cf));
    setColour(juce::ComboBox::arrowColourId, juce::Colour(0xffe0a852));
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff19130f));
    setColour(juce::PopupMenu::textColourId, juce::Colour(0xfff4e6cf));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff996129));
    setColour(juce::TextButton::buttonColourId, juce::Colour(0xff241c16));
    setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffd99b43));
    setColour(juce::TextButton::textColourOffId, juce::Colour(0xffeadbc2));
    setColour(juce::TextButton::textColourOnId, juce::Colour(0xff140f0b));
    setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xfff0dfc4));
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

void AndesLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, const float rotaryStartAngle, const float rotaryEndAngle,
                                        juce::Slider&)
{
    const float size = static_cast<float>(juce::jmin(width, height));
    const float radius = size * 0.34f;
    const float cx = static_cast<float>(x) + static_cast<float>(width) * 0.5f;
    const float cy = static_cast<float>(y) + static_cast<float>(height) * 0.5f;
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    const int numTicks = 11;
    for (int i = 0; i < numTicks; ++i)
    {
        const float frac = static_cast<float>(i) / static_cast<float>(numTicks - 1);
        const float tickAngle = rotaryStartAngle + frac * (rotaryEndAngle - rotaryStartAngle);
        const bool isActive = frac <= sliderPos + 0.001f;
        const float r1 = radius + 4.0f;
        const float r2 = radius + ((i == 0 || i == numTicks - 1 || i == numTicks / 2) ? 8.0f : 6.5f);

        g.setColour(isActive ? juce::Colour(0xffe0b060) : juce::Colour(0xff594737));
        g.drawLine(cx + std::sin(tickAngle) * r1, cy - std::cos(tickAngle) * r1,
                   cx + std::sin(tickAngle) * r2, cy - std::cos(tickAngle) * r2, 1.3f);
    }

    g.setColour(juce::Colour(0xa0050302));
    g.fillEllipse(cx - radius - 1.0f, cy - radius + 3.0f, (radius + 1.0f) * 2.0f, (radius + 1.0f) * 2.0f);

    juce::ColourGradient outerGrad(juce::Colour(0xff5e4d3e), cx - radius * 0.7f, cy - radius * 0.7f,
                                   juce::Colour(0xff140f0c), cx + radius * 0.8f, cy + radius * 0.8f, false);
    g.setGradientFill(outerGrad);
    g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

    const float innerR = radius * 0.86f;
    juce::ColourGradient bodyGrad(juce::Colour(0xff3c3026), cx - innerR * 0.4f, cy - innerR * 0.5f,
                                  juce::Colour(0xff120d0a), cx + innerR * 0.6f, cy + innerR * 0.6f, true);
    g.setGradientFill(bodyGrad);
    g.fillEllipse(cx - innerR, cy - innerR, innerR * 2.0f, innerR * 2.0f);

    g.setColour(juce::Colour(0x55d4b287));
    g.drawEllipse(cx - innerR, cy - innerR, innerR * 2.0f, innerR * 2.0f, 0.9f);

    juce::Path ptr;
    ptr.addRoundedRectangle(-1.3f, -innerR + 2.0f, 2.6f, innerR * 0.55f, 1.1f);
    ptr.applyTransform(juce::AffineTransform::rotation(angle).translated(cx, cy));
    g.setColour(juce::Colour(0xfff7e8cc));
    g.fillPath(ptr);
}

void AndesLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                        bool isHover, bool)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
    const bool on = button.getToggleState();

    if (bounds.getWidth() <= 38.0f)
    {
        g.setColour(on ? juce::Colour(0xffd99b43) : juce::Colour(0xff211a14));
        g.fillRoundedRectangle(bounds, 3.0f);
        g.setColour(on ? juce::Colour(0xffffd485) : (isHover ? juce::Colour(0xff8c6d4e) : juce::Colour(0xff54402e)));
        g.drawRoundedRectangle(bounds, 3.0f, 1.1f);

        g.setColour(on ? juce::Colour(0xff140f0b) : juce::Colour(0xffd6c3a8));
        g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
        g.drawFittedText(button.getButtonText(), bounds.toNearestInt(), juce::Justification::centred, 1);
        return;
    }

    juce::ColourGradient bg(on ? juce::Colour(0xff47321d) : juce::Colour(0xff241c16),
                            bounds.getX(), bounds.getY(),
                            on ? juce::Colour(0xff291c10) : juce::Colour(0xff18120e),
                            bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill(bg);
    g.fillRoundedRectangle(bounds, 4.0f);

    g.setColour(on ? juce::Colour(0xffe6a84c) : (isHover ? juce::Colour(0xff8c6d4e) : juce::Colour(0xff54402e)));
    g.drawRoundedRectangle(bounds, 4.0f, 1.1f);

    const float ledR = 3.8f;
    const float ledX = bounds.getX() + 9.5f;
    const float ledY = bounds.getCentreY();

    if (on)
    {
        g.setColour(juce::Colour(0x55ffbe4d));
        g.fillEllipse(ledX - ledR - 2.2f, ledY - ledR - 2.2f, (ledR + 2.2f) * 2.0f, (ledR + 2.2f) * 2.0f);
    }

    g.setColour(on ? juce::Colour(0xffffc45e) : juce::Colour(0xff4a3b2e));
    g.fillEllipse(ledX - ledR, ledY - ledR, ledR * 2.0f, ledR * 2.0f);

    g.setColour(on ? juce::Colour(0xfffff0d6) : juce::Colour(0xffc9b69c));
    g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
    g.drawFittedText(button.getButtonText(),
                     bounds.withTrimmedLeft(19.0f).withTrimmedRight(4.0f).toNearestInt(),
                     juce::Justification::centredLeft, 1);
}

//==============================================================================
void AnalogVUMeter::setLevelDb(float db, bool isGainReduction)
{
    grMode = isGainReduction;
    float norm = grMode ? std::clamp(1.0f - (db / 18.0f), 0.0f, 1.0f)
                        : std::clamp((db + 36.0f) / 39.0f, 0.0f, 1.0f);

    const float next = smoothedNeedle * 0.72f + norm * 0.28f;
    if (std::abs(next - smoothedNeedle) > 0.0005f)
    {
        smoothedNeedle = next;
        repaint();
    }
}

void AnalogVUMeter::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced(1.0f);
    juce::ColourGradient bg(juce::Colour(0xffedd08c), b.getCentreX(), b.getY(),
                            juce::Colour(0xffbd833a), b.getCentreX(), b.getBottom(), false);
    g.setGradientFill(bg);
    g.fillRoundedRectangle(b, 4.0f);

    g.setColour(juce::Colour(0xff2b1e14));
    g.drawRoundedRectangle(b, 4.0f, 1.5f);

    g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
    g.drawText("-20   -10   -5   0   +3", b.withTrimmedTop(3.0f).withHeight(14.0f), juce::Justification::centred);
    g.drawText(grMode ? "VU GR" : "VU OUT", b.withTrimmedLeft(6.0f).withTrimmedBottom(2.0f), juce::Justification::bottomLeft);

    const float angle = juce::jmap(smoothedNeedle, -0.68f, 0.68f);
    const float px = b.getCentreX();
    const float py = b.getBottom() + 10.0f;
    const float len = b.getHeight() * 0.92f;

    g.setColour(juce::Colour(0xff1a120c));
    g.drawLine(px, py, px + std::sin(angle) * len, py - std::cos(angle) * len, 1.8f);
}

//==============================================================================
Studio4MicStagePad::Studio4MicStagePad(juce::AudioProcessorValueTreeState& state)
    : apvts(state)
{
    setInterceptsMouseClicks(true, false);
    loadEmbeddedImages();
}

juce::Image Studio4MicStagePad::removeGreenScreen(const juce::Image& src)
{
    if (!src.isValid()) return {};
    juce::Image out = src.convertedToFormat(juce::Image::ARGB);
    juce::Image::BitmapData bd(out, juce::Image::BitmapData::readWrite);

    for (int y = 0; y < out.getHeight(); ++y)
    {
        for (int x = 0; x < out.getWidth(); ++x)
        {
            auto c = bd.getPixelColour(x, y);
            const int r = c.getRed(), g = c.getGreen(), b = c.getBlue();
            const int maxRB = std::max(r, b);
            const int greenExcess = g - maxRB;

            if (greenExcess > 55 && g > 110)
                bd.setPixelColour(x, y, juce::Colours::transparentBlack);
            else if (greenExcess > 32 && g > 95)
            {
                const float alpha = 1.0f - std::clamp(static_cast<float>(greenExcess - 32) / 23.0f, 0.0f, 1.0f);
                const uint8_t cleanG = static_cast<uint8_t>(std::min(g, maxRB + 10));
                bd.setPixelColour(x, y, juce::Colour::fromRGBA(static_cast<uint8_t>(r), cleanG, static_cast<uint8_t>(b),
                                                               static_cast<uint8_t>(alpha * 255.0f)));
            }
        }
    }
    return out;
}

void Studio4MicStagePad::loadEmbeddedImages()
{
#if HAS_CUSTOM_ASSETS
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        const juce::String name(BinaryData::namedResourceList[i]);
        int dataSize = 0;
        const char* data = BinaryData::getNamedResource(BinaryData::namedResourceList[i], dataSize);
        if (!data || dataSize <= 0) continue;

        auto loadImg = [&]() { return juce::ImageFileFormat::loadFrom(data, static_cast<size_t>(dataSize)); };

        if (name.containsIgnoreCase("room_volcano_open"))        imgVolcanoOpen     = loadImg();
        else if (name.containsIgnoreCase("room_volcano_closed")) imgVolcanoClosed   = loadImg();
        else if (name.containsIgnoreCase("room_shangrila"))      imgShangrila       = loadImg();
        else if (name.containsIgnoreCase("room_canyon"))         imgCanyon          = loadImg();
        else if (name.containsIgnoreCase("mic_main_mono"))       imgMicMono         = removeGreenScreen(loadImg());
        else if (name.containsIgnoreCase("mic_main_xy"))         imgMicXY           = removeGreenScreen(loadImg());
        else if (name.containsIgnoreCase("mic_stereo_pair"))     imgMicPair         = removeGreenScreen(loadImg());
        else if (name.containsIgnoreCase("mic_far_l"))           imgMicFarL         = removeGreenScreen(loadImg());
        else if (name.containsIgnoreCase("mic_far_r"))           imgMicFarR         = removeGreenScreen(loadImg());
        else if (name.containsIgnoreCase("src_vocals"))          imgSrcVocals       = removeGreenScreen(loadImg());
        else if (name.containsIgnoreCase("src_drums"))           imgSrcDrums        = removeGreenScreen(loadImg());
        else if (name.containsIgnoreCase("src_bass"))            imgSrcBass         = removeGreenScreen(loadImg());
        else if (name.containsIgnoreCase("src_guitar_stereo"))   imgSrcGuitarStereo = removeGreenScreen(loadImg());
        else if (name.containsIgnoreCase("src_guitar_mono"))     imgSrcGuitarMono   = removeGreenScreen(loadImg());
        else if (name.containsIgnoreCase("src_ensemble"))        imgSrcEnsemble     = removeGreenScreen(loadImg());
        else if (name.containsIgnoreCase("src_monitors"))        imgSrcMonitors     = removeGreenScreen(loadImg());
    }
#endif
}

void Studio4MicStagePad::setSelectedMic(int idx)
{
    selectedMic = std::clamp(idx, 0, 3);
    repaint();
}

void Studio4MicStagePad::centerSelectedMic()
{
    if (selectedMic == 2 || selectedMic == 3)
    {
        beginMicGesture(2); beginMicGesture(3);
        setMicPos(2, 0.14f, 0.86f);
        setMicPos(3, 0.86f, 0.86f);
        endMicGesture(2); endMicGesture(3);
    }
    else
    {
        const char* yId = (selectedMic == 0) ? "mic1Y" : "mic2Y";
        beginMicGesture(selectedMic);
        setMicPos(selectedMic, 0.50f, apvts.getRawParameterValue(yId)->load());
        endMicGesture(selectedMic);
    }
    repaint();
}

juce::Point<float> Studio4MicStagePad::normToScreen(float nx, float ny, juce::Rectangle<float> area) const
{
    const float yTop = area.getY() + area.getHeight() * 0.56f;
    const float yBot = area.getBottom() - area.getHeight() * 0.04f;
    const float sy = yTop + ny * (yBot - yTop);
    const float spread = 0.52f + 0.44f * ny;
    return { area.getCentreX() + (nx - 0.5f) * area.getWidth() * spread, sy };
}

juce::Point<float> Studio4MicStagePad::screenToNorm(juce::Point<float> pt, juce::Rectangle<float> area) const
{
    const float yTop = area.getY() + area.getHeight() * 0.56f;
    const float yBot = area.getBottom() - area.getHeight() * 0.04f;
    const float ny = std::clamp((pt.y - yTop) / std::max(1.0f, yBot - yTop), 0.05f, 0.95f);
    const float spread = 0.52f + 0.44f * ny;
    const float nx = std::clamp(0.5f + (pt.x - area.getCentreX()) / std::max(1.0f, area.getWidth() * spread), 0.05f, 0.95f);
    return { nx, ny };
}

void Studio4MicStagePad::beginMicGesture(int idx)
{
    static constexpr const char* xIds[4] = { "mic1X", "mic2X", "mic3X", "mic4X" };
    static constexpr const char* yIds[4] = { "mic1Y", "mic2Y", "mic3Y", "mic4Y" };
    if (auto* px = apvts.getParameter(xIds[idx])) px->beginChangeGesture();
    if (auto* py = apvts.getParameter(yIds[idx])) py->beginChangeGesture();
}

void Studio4MicStagePad::endMicGesture(int idx)
{
    static constexpr const char* xIds[4] = { "mic1X", "mic2X", "mic3X", "mic4X" };
    static constexpr const char* yIds[4] = { "mic1Y", "mic2Y", "mic3Y", "mic4Y" };
    if (auto* px = apvts.getParameter(xIds[idx])) px->endChangeGesture();
    if (auto* py = apvts.getParameter(yIds[idx])) py->endChangeGesture();
}

void Studio4MicStagePad::setMicPos(int idx, float nx, float ny)
{
    static constexpr const char* xIds[4] = { "mic1X", "mic2X", "mic3X", "mic4X" };
    static constexpr const char* yIds[4] = { "mic1Y", "mic2Y", "mic3Y", "mic4Y" };
    if (auto* px = apvts.getParameter(xIds[idx])) px->setValueNotifyingHost(px->convertTo0to1(nx));
    if (auto* py = apvts.getParameter(yIds[idx])) py->setValueNotifyingHost(py->convertTo0to1(ny));
}

void Studio4MicStagePad::paint(juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat().reduced(2.0f);
    const int roomIdx   = static_cast<int>(apvts.getRawParameterValue("room")->load());
    const int wallsIdx  = static_cast<int>(apvts.getRawParameterValue("walls")->load());
    const int srcIdx    = static_cast<int>(apvts.getRawParameterValue("source")->load());
    const bool mainSt   = apvts.getRawParameterValue("mainStereo")->load() > 0.5f;
    const bool phaseOk  = apvts.getRawParameterValue("phaseSafe")->load() > 0.5f;

    const juce::Image* roomImg = nullptr;
    if (roomIdx == 0)      roomImg = (wallsIdx == 0) ? &imgVolcanoOpen : &imgVolcanoClosed;
    else if (roomIdx == 1) roomImg = &imgShangrila;
    else                   roomImg = &imgCanyon;

    if (roomImg && roomImg->isValid())
        g.drawImageWithin(*roomImg, area.toNearestInt().getX(), area.toNearestInt().getY(),
                          area.toNearestInt().getWidth(), area.toNearestInt().getHeight(),
                          juce::RectanglePlacement::fillDestination);
    else
    {
        g.setColour(juce::Colour(0xff231913));
        g.fillRoundedRectangle(area, 4.0f);
    }

    const juce::Image* srcImg = nullptr;
    if (srcIdx == 0)      srcImg = &imgSrcVocals;
    else if (srcIdx == 1) srcImg = &imgSrcDrums;
    else if (srcIdx == 2) srcImg = &imgSrcBass;
    else if (srcIdx == 3) srcImg = (mainSt && imgSrcGuitarStereo.isValid()) ? &imgSrcGuitarStereo : &imgSrcGuitarMono;
    else if (srcIdx == 4) srcImg = &imgSrcEnsemble;
    else if (srcIdx == 5) srcImg = &imgSrcMonitors;

    const float floorLineY = area.getY() + area.getHeight() * 0.64f;
    if (srcImg && srcImg->isValid())
    {
        const float heightRatio = (srcIdx == 1) ? 0.35f : (srcIdx == 4 ? 0.34f : 0.30f);
        const float sh = area.getHeight() * heightRatio;
        const float sw = sh * (static_cast<float>(srcImg->getWidth()) / std::max(1, srcImg->getHeight()));
        const float sx = area.getCentreX() - sw * 0.5f;
        const float sy = floorLineY - sh;

        g.setOpacity(1.0f);
        g.setColour(juce::Colour(0x660b0705));
        g.fillEllipse(area.getCentreX() - sw * 0.45f, floorLineY - 5.0f, sw * 0.90f, 11.0f);
        g.drawImage(*srcImg, juce::Rectangle<float>(sx, sy, sw, sh));
    }

    static constexpr const char* xIds[4] = { "mic1X", "mic2X", "mic3X", "mic4X" };
    static constexpr const char* yIds[4] = { "mic1Y", "mic2Y", "mic3Y", "mic4Y" };
    static constexpr const char* gIds[4] = { "mic1Gain", "mic2Gain", "mic3Gain", "mic4Gain" };
    static constexpr const char* labels[4] = { "1:MAIN", "2:ROOM/AIR", "3:FAR L", "4:FAR R" };

    std::array<int, 4> order { 0, 1, 2, 3 };
    std::sort(order.begin(), order.end(), [&](int a, int b)
    {
        return apvts.getRawParameterValue(yIds[a])->load() < apvts.getRawParameterValue(yIds[b])->load();
    });

    for (int idx : order)
    {
        const float nx = apvts.getRawParameterValue(xIds[idx])->load();
        const float ny = apvts.getRawParameterValue(yIds[idx])->load();
        const float gain = apvts.getRawParameterValue(gIds[idx])->load();
        const auto pos = normToScreen(nx, ny, area);
        const float micH = area.getHeight() * (0.22f + 0.25f * ny);

        const juce::Image* micImg = nullptr;
        if (idx == 0)      micImg = mainSt ? &imgMicXY : &imgMicMono;
        else if (idx == 1) micImg = &imgMicPair;
        else if (idx == 2) micImg = imgMicFarL.isValid() ? &imgMicFarL : &imgMicMono;
        else               micImg = imgMicFarR.isValid() ? &imgMicFarR : &imgMicMono;

        g.setOpacity((gain > 0.01f) ? 1.0f : 0.45f);
        if (micImg && micImg->isValid())
        {
            const float micW = micH * (static_cast<float>(micImg->getWidth()) / std::max(1, micImg->getHeight()));
            g.drawImage(*micImg, juce::Rectangle<float>(pos.x - micW * 0.5f, pos.y - micH, micW, micH));
        }
        else
        {
            g.setColour(juce::Colour(0xffd99b43));
            g.fillEllipse(pos.x - 8.0f, pos.y - 8.0f, 16.0f, 16.0f);
        }
        g.setOpacity(1.0f);

        if (idx == selectedMic)
        {
            const float ringW = 24.0f + 16.0f * ny;
            const float ringH = 8.0f + 5.0f * ny;
            g.setColour(juce::Colour(0xffffb84d));
            g.drawEllipse(pos.x - ringW * 0.5f, pos.y - ringH * 0.5f, ringW, ringH, 2.0f);
        }

        const float distM = BenBocazDSP::AcousticEngine::computeDistanceMeters(roomIdx, nx, ny);
        const juce::String tag = juce::String(labels[idx]) + " " + juce::String(distM, 1) + "m";
        const juce::Rectangle<float> tagRect(pos.x - 48.0f, pos.y - micH - 18.0f, 96.0f, 16.0f);

        g.setColour(idx == selectedMic ? juce::Colour(0xd9261b12) : juce::Colour(0xb0140e0a));
        g.fillRoundedRectangle(tagRect, 3.0f);
        g.setColour(idx == selectedMic ? juce::Colour(0xffffb84d) : juce::Colour(0xff8c7258));
        g.drawRoundedRectangle(tagRect, 3.0f, 1.0f);
        g.setColour(juce::Colour(0xfff5e6d0));
        g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
        g.drawFittedText(tag, tagRect.toNearestInt(), juce::Justification::centred, 1);
    }

    const juce::String roomTitle = (roomIdx == 0) ? (wallsIdx == 0 ? "VOLCANO HALL | GATES OPEN" : "VOLCANO HALL | GATES CLOSED")
                                 : (roomIdx == 1 ? "SHANGRI-LA | LIVE WOOD" : "CANYON ROOM | STUDIO WOOD");
    g.setColour(juce::Colour(0xb8120c08));
    g.fillRect(area.withHeight(20.0f));
    g.setColour(juce::Colour(0xffe6d2b5));
    g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
    g.drawText(roomTitle, area.withHeight(20.0f).reduced(8.0f, 0.0f), juce::Justification::centredLeft);

    g.setColour(phaseOk ? juce::Colour(0xff9be08a) : juce::Colour(0xffe0a85c));
    g.drawText(phaseOk ? "[OK] PHASE LOCKED" : "NATURAL TIME-OF-FLIGHT",
               area.withHeight(20.0f).reduced(8.0f, 0.0f), juce::Justification::centredRight);

    const float selX = apvts.getRawParameterValue(xIds[selectedMic])->load();
    const float selY = apvts.getRawParameterValue(yIds[selectedMic])->load();
    const float selDist = BenBocazDSP::AcousticEngine::computeDistanceMeters(roomIdx, selX, selY);
    const int panPct = static_cast<int>(std::round((selX - 0.5f) * 200.0f));
    const juce::String panStr = (panPct == 0) ? "CENTER" : (panPct < 0 ? juce::String(std::abs(panPct)) + "% L" : juce::String(panPct) + "% R");

    const juce::Rectangle<float> infoCard(area.getRight() - 188.0f, area.getBottom() - 40.0f, 180.0f, 32.0f);
    g.setColour(juce::Colour(0xd917110d));
    g.fillRoundedRectangle(infoCard, 4.0f);
    g.setColour(juce::Colour(0xff7d6245));
    g.drawRoundedRectangle(infoCard, 4.0f, 1.0f);

    g.setColour(juce::Colour(0xffffc45e));
    g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
    g.drawText("SELECTED: " + juce::String(labels[selectedMic]), infoCard.getX() + 7.0f, infoCard.getY() + 3.0f, 166.0f, 12.0f, juce::Justification::centredLeft);

    g.setColour(juce::Colour(0xffe8d7be));
    g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
    g.drawText("DIST: " + juce::String(selDist, 1) + "m  |  PAN: " + panStr, infoCard.getX() + 7.0f, infoCard.getY() + 16.0f, 166.0f, 12.0f, juce::Justification::centredLeft);

    g.setColour(juce::Colour(0xff6b5239));
    g.drawRoundedRectangle(area, 4.0f, 1.5f);
}

void Studio4MicStagePad::mouseDown(const juce::MouseEvent& e)
{
    auto area = getLocalBounds().toFloat().reduced(2.0f);
    static constexpr const char* xIds[4] = { "mic1X", "mic2X", "mic3X", "mic4X" };
    static constexpr const char* yIds[4] = { "mic1Y", "mic2Y", "mic3Y", "mic4Y" };

    float bestDist = std::max(78.0f, area.getHeight() * 0.26f);
    int bestIdx = -1;
    for (int i = 0; i < 4; ++i)
    {
        const float nx = apvts.getRawParameterValue(xIds[i])->load();
        const float ny = apvts.getRawParameterValue(yIds[i])->load();
        const auto p = normToScreen(nx, ny, area);
        const float micH = area.getHeight() * (0.22f + 0.25f * ny);

        const float d = std::min(e.position.getDistanceFrom(p),
                                 e.position.getDistanceFrom(p.translated(0.0f, -micH * 0.45f)));
        if (d < bestDist) { bestDist = d; bestIdx = i; }
    }

    if (bestIdx >= 0)
    {
        selectedMic = bestIdx;
        draggingMic = bestIdx;
        beginMicGesture(draggingMic);

        const bool linkFarLR = apvts.getRawParameterValue("link34")->load() > 0.5f;
        if (linkFarLR && (draggingMic == 2 || draggingMic == 3))
            beginMicGesture((draggingMic == 2) ? 3 : 2);

        repaint();
    }
}

void Studio4MicStagePad::mouseDrag(const juce::MouseEvent& e)
{
    if (draggingMic < 0) return;

    auto area = getLocalBounds().toFloat().reduced(2.0f);
    auto norm = screenToNorm(e.position, area);

    const bool snap = apvts.getRawParameterValue("snapCenter")->load() > 0.5f;
    const bool linkFarLR = apvts.getRawParameterValue("link34")->load() > 0.5f;

    if (snap && draggingMic < 2 && std::abs(norm.x - 0.5f) < 0.045f)
        norm.x = 0.5f;

    setMicPos(draggingMic, norm.x, norm.y);

    if (linkFarLR && (draggingMic == 2 || draggingMic == 3))
    {
        const int other = (draggingMic == 2) ? 3 : 2;
        setMicPos(other, std::clamp(1.0f - norm.x, 0.05f, 0.95f), norm.y);
    }

    repaint();
}

void Studio4MicStagePad::mouseUp(const juce::MouseEvent&)
{
    if (draggingMic >= 0)
    {
        endMicGesture(draggingMic);
        const bool linkFarLR = apvts.getRawParameterValue("link34")->load() > 0.5f;
        if (linkFarLR && (draggingMic == 2 || draggingMic == 3))
            endMicGesture((draggingMic == 2) ? 3 : 2);
    }
    draggingMic = -1;
}

//==============================================================================
BenBocazStudioProAudioProcessorEditor::BenBocazStudioProAudioProcessorEditor(BenBocazStudioProAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p), stagePad(p.apvts)
{
    setLookAndFeel(&lnf);
    setSize(1260, 790);

#if HAS_CUSTOM_ASSETS
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        if (juce::String(BinaryData::namedResourceList[i]).containsIgnoreCase("chassis_wood"))
        {
            int sz = 0;
            if (const char* d = BinaryData::getNamedResource(BinaryData::namedResourceList[i], sz))
                imgChassisWood = juce::ImageFileFormat::loadFrom(d, static_cast<size_t>(sz));
            break;
        }
    }
#endif

    auto addCombo = [this](juce::ComboBox& cb, const char* paramId)
    {
        if (auto* choice = dynamic_cast<juce::AudioParameterChoice*>(processor.apvts.getParameter(paramId)))
            cb.addItemList(choice->choices, 1);
        addAndMakeVisible(cb);
        comboAtts.push_back(std::make_unique<ComboAtt>(processor.apvts, paramId, cb));
    };

    auto addToggle = [this](juce::ToggleButton& bt, const char* paramId)
    {
        addAndMakeVisible(bt);
        buttonAtts.push_back(std::make_unique<ButtonAtt>(processor.apvts, paramId, bt));
    };

    auto addKnob = [this](juce::Slider& s, const char* paramId, const juce::String& suffix = {})
    {
        setupKnob(s, suffix);
        addAndMakeVisible(s);
        sliderAtts.push_back(std::make_unique<SliderAtt>(processor.apvts, paramId, s));
    };

    addCombo(cbMode, "mode");
    addCombo(cbRouting, "routing");
    addToggle(btHqMode, "hqMode");
    btHqMode.onClick = [this]() { updateHqButtonText(); };
    updateHqButtonText();

    addToggle(btPhaseSafe, "phaseSafe");
    addToggle(btPolarity, "polarityInv");
    addToggle(btSnapCenter, "snapCenter");
    addToggle(btLink34, "link34");
    btLink34.setButtonText("LINK FAR L/R");

    for (int i = 0; i < processor.getNumPrograms(); ++i)
        cbPresets.addItem(processor.getProgramName(i), i + 1);
    cbPresets.setSelectedId(processor.getCurrentProgram() + 1, juce::dontSendNotification);
    cbPresets.onChange = [this]()
    {
        const int idx = cbPresets.getSelectedId() - 1;
        if (idx >= 0) { processor.applyFactoryPreset(idx); stagePad.repaint(); }
    };
    addAndMakeVisible(cbPresets);

    addAndMakeVisible(btCenterMic);
    btCenterMic.onClick = [this]() { stagePad.centerSelectedMic(); };

    addAndMakeVisible(vuMeter);
    addAndMakeVisible(btVuMode);
    btVuMode.onClick = [this]()
    {
        vuGrMode = !vuGrMode;
        btVuMode.setButtonText(vuGrMode ? "VU: GR" : "VU: OUT");
    };

    addAndMakeVisible(btExpandStage);
    btExpandStage.onClick = [this]()
    {
        expandedStage = !expandedStage;
        btExpandStage.setButtonText(expandedStage ? "SHOW RACKS" : "EXPAND STAGE");
        resized();
    };

    addToggle(btMainStereo, "mainStereo");
    btMainStereo.onClick = [this]() { stagePad.repaint(); };
    addToggle(btRoomOn, "roomOn");
    addAndMakeVisible(stagePad);

    addToggle(btVocalMod, "vocalModOn");
    addCombo(cbVocalIn, "vocalInMic");
    addCombo(cbVocalTarget, "vocalTargetMic");
    addKnob(slVocalIn, "vocalInGain");   setupLabel(lbVocalIn, "MIC IN");
    addKnob(slVocalOut, "vocalOutGain"); setupLabel(lbVocalOut, "MIC OUT");
    addCombo(cbSource, "source");        cbSource.onChange = [this]() { stagePad.repaint(); };
    addCombo(cbMonitor, "monitorSpeaker");
    addCombo(cbRoom, "room");            cbRoom.onChange = [this]() { stagePad.repaint(); };
    addCombo(cbWalls, "walls");          cbWalls.onChange = [this]() { stagePad.repaint(); };
    addCombo(cbCircuit, "circuit");
    addKnob(slDrive, "drive");           setupLabel(lbDrive, "TUBE / TAPE DRIVE");

    static constexpr const char* micBtnNames[4] = { "1: MAIN", "2: ROOM/AIR", "3: FAR L", "4: FAR R" };
    static constexpr const char* micGainIds[4]  = { "mic1Gain", "mic2Gain", "mic3Gain", "mic4Gain" };
    static constexpr const char* micSoloIds[4]  = { "mic1Solo", "mic2Solo", "mic3Solo", "mic4Solo" };

    for (int i = 0; i < 4; ++i)
    {
        btMicSelect[i].setButtonText(micBtnNames[i]);
        btMicSelect[i].onClick = [this, i]() { stagePad.setSelectedMic(i); };
        addAndMakeVisible(btMicSelect[i]);
        addKnob(slMicGain[i], micGainIds[i]);
        slMicGain[i].onValueChange = [this, i]()
        {
            stagePad.setSelectedMic(i);
            if (!syncingFarGains && (i == 2 || i == 3) && btLink34.getToggleState())
            {
                syncingFarGains = true;
                const int other = (i == 2) ? 3 : 2;
                slMicGain[other].setValue(slMicGain[i].getValue(), juce::sendNotificationSync);
                syncingFarGains = false;
            }
        };
        btMicSolo[i].setButtonText("S");
        addToggle(btMicSolo[i], micSoloIds[i]);
    }
    addKnob(slPairWide, "pairWide"); setupLabel(lbPairWide, "ROOM WIDE");
    addKnob(slFarWide, "farWide");   setupLabel(lbFarWide, "FAR WIDE");

    addToggle(btChamberOn, "chamberOn");
    addCombo(cbChamberModel, "chamberModel");
    addKnob(slChamberSend, "chamberSend");       setupLabel(lbChSend, "SEND");
    addKnob(slChamberDecay, "chamberDecay");     setupLabel(lbChDecay, "DECAY");
    addKnob(slChamberPre, "chamberPreDelay");    setupLabel(lbChPre, "PRE-DELAY");
    addKnob(slChamberWidth, "chamberWidth");     setupLabel(lbChWidth, "WIDTH");

    addToggle(btEqOn, "eqOn");
    addCombo(cbEqModel, "eqModel");
    addKnob(slHpf, "hpf");           setupLabel(lbHpf, "HPF");
    addKnob(slLowGain, "lowGain");   setupLabel(lbLow, "LOW");
    addKnob(slMidGain, "midGain");   setupLabel(lbMid, "MID");
    addKnob(slHighGain, "highGain"); setupLabel(lbHigh, "HIGH");
    addCombo(cbLowFreq, "lowFreq");
    addCombo(cbMidFreq, "midFreq");
    addCombo(cbHighFreq, "highFreq");

    addToggle(btCompOn, "compOn");
    addCombo(cbCompModel, "compModel");
    addKnob(slCompThresh, "compThresh"); setupLabel(lbCompThresh, "THRESHOLD");
    addKnob(slCompMix, "compMix");       setupLabel(lbCompMix, "MIX");
    addToggle(btScLink, "scLink");
    addToggle(btScFilter, "scFilter");
    addToggle(btParCrush, "parCrush");
    addKnob(slParBlend, "parBlend");     setupLabel(lbParBlend, "PAR BLEND");
    addCombo(cbParRoute, "parRoute");

    addKnob(slOutput, "outputGain"); setupLabel(lbOutput, "OUTPUT");

    startTimerHz(20);
}

BenBocazStudioProAudioProcessorEditor::~BenBocazStudioProAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void BenBocazStudioProAudioProcessorEditor::setupKnob(juce::Slider& s, const juce::String& suffix)
{
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 18);
    s.textFromValueFunction = [suffix](double v)
    {
        if (std::abs(v) < 0.005) v = 0.0;
        return juce::String(v, 2) + suffix;
    };
}

void BenBocazStudioProAudioProcessorEditor::setupLabel(juce::Label& l, const juce::String& text,
                                                       float fontSize, juce::Justification just)
{
    l.setText(text, juce::dontSendNotification);
    l.setFont(juce::FontOptions(fontSize, juce::Font::bold));
    l.setColour(juce::Label::textColourId, juce::Colour(0xffd4bc9c));
    l.setJustificationType(just);
    addAndMakeVisible(l);
}

void BenBocazStudioProAudioProcessorEditor::updateHqButtonText()
{
    btHqMode.setButtonText(btHqMode.getToggleState() ? "HQ 4x" : "TRACK 0ms");
}

void BenBocazStudioProAudioProcessorEditor::timerCallback()
{
    vuMeter.setLevelDb(vuGrMode ? processor.vuGrDb.load(std::memory_order_relaxed)
                                : processor.vuOutDb.load(std::memory_order_relaxed),
                       vuGrMode);
    stagePad.repaint();
}

void BenBocazStudioProAudioProcessorEditor::paint(juce::Graphics& g)
{
    if (imgChassisWood.isValid())
        g.drawImageWithin(imgChassisWood, 0, 0, getWidth(), getHeight(), juce::RectanglePlacement::fillDestination);
    else
        g.fillAll(juce::Colour(0xff1a1410));

    g.setColour(juce::Colour(0x88120c08));
    g.fillRect(getLocalBounds());

    g.setColour(juce::Colour(0xe017110d));
    g.fillRect(0, 0, getWidth(), 102);
    g.setColour(juce::Colour(0xff6b5239));
    g.drawHorizontalLine(102, 0.0f, static_cast<float>(getWidth()));

    g.setColour(juce::Colour(0xfff5e6d0));
    g.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    g.drawText("LOS ANDES CAPITAL", 24, 10, 320, 30, juce::Justification::centredLeft);

    g.setColour(juce::Colour(0xffd9a457));
    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.drawText("VOLCANO HALL  |  3D 4-STAND RE-MIC  |  ANALOG CONSOLE", 24, 40, 360, 16, juce::Justification::centredLeft);

    if (!expandedStage)
    {
        auto drawRackPanel = [&](int rx, int ry, int rw, int rh, const juce::String& title)
        {
            juce::Rectangle<float> panel(static_cast<float>(rx), static_cast<float>(ry),
                                         static_cast<float>(rw), static_cast<float>(rh));
            g.setColour(juce::Colour(0xb816100c));
            g.fillRoundedRectangle(panel, 6.0f);
            g.setColour(juce::Colour(0xff634b34));
            g.drawRoundedRectangle(panel, 6.0f, 1.2f);
            g.setColour(juce::Colour(0xd0211812));
            g.fillRoundedRectangle(panel.withHeight(22.0f), 6.0f);
            g.setColour(juce::Colour(0xffe6c795));
            g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
            g.drawText(title, rx + 10, ry + 3, rw - 20, 16, juce::Justification::centredLeft);
        };

        drawRackPanel(24, 110, 264, 434, "VOCAL MIC MOD & ROOM SETUP");
        drawRackPanel(300, 110, 660, 434, "LIVE ROOM 3D STAGE");
        drawRackPanel(972, 110, 264, 434, "ECHO CHAMBERS");
        drawRackPanel(24, 556, 572, 214, "ANALOG CONSOLE EQ");
        drawRackPanel(608, 556, 488, 214, "OUTBOARD & PARALLEL COMP");
        drawRackPanel(1108, 556, 128, 214, "MASTER");
    }
}

void BenBocazStudioProAudioProcessorEditor::resized()
{
    cbMode.setBounds(370, 22, 105, 26);
    cbRouting.setBounds(485, 22, 185, 26);
    btHqMode.setBounds(680, 10, 115, 22);
    btPhaseSafe.setBounds(680, 36, 115, 22);
    btPolarity.setBounds(805, 36, 115, 22);
    vuMeter.setBounds(930, 8, 210, 48);
    btVuMode.setBounds(1150, 14, 92, 34);

    cbPresets.setBounds(24, 68, 285, 24);
    btSnapCenter.setBounds(322, 68, 115, 24);
    btLink34.setBounds(446, 68, 112, 24);
    btCenterMic.setBounds(566, 68, 105, 24);
    btExpandStage.setBounds(1110, 68, 132, 24);

    const bool v = !expandedStage;
    btVocalMod.setVisible(v); cbVocalIn.setVisible(v); cbVocalTarget.setVisible(v);
    slVocalIn.setVisible(v); slVocalOut.setVisible(v); lbVocalIn.setVisible(v); lbVocalOut.setVisible(v);
    cbSource.setVisible(v); cbMonitor.setVisible(v); cbRoom.setVisible(v); cbWalls.setVisible(v);
    cbCircuit.setVisible(v); slDrive.setVisible(v); lbDrive.setVisible(v);
    btChamberOn.setVisible(v); cbChamberModel.setVisible(v);
    slChamberSend.setVisible(v); slChamberDecay.setVisible(v); slChamberPre.setVisible(v); slChamberWidth.setVisible(v);
    lbChSend.setVisible(v); lbChDecay.setVisible(v); lbChPre.setVisible(v); lbChWidth.setVisible(v);
    btEqOn.setVisible(v); cbEqModel.setVisible(v);
    slHpf.setVisible(v); slLowGain.setVisible(v); slMidGain.setVisible(v); slHighGain.setVisible(v);
    lbHpf.setVisible(v); lbLow.setVisible(v); lbMid.setVisible(v); lbHigh.setVisible(v);
    cbLowFreq.setVisible(v); cbMidFreq.setVisible(v); cbHighFreq.setVisible(v);
    btCompOn.setVisible(v); cbCompModel.setVisible(v);
    slCompThresh.setVisible(v); slCompMix.setVisible(v); lbCompThresh.setVisible(v); lbCompMix.setVisible(v);
    btScLink.setVisible(v); btScFilter.setVisible(v); btParCrush.setVisible(v);
    slParBlend.setVisible(v); lbParBlend.setVisible(v); cbParRoute.setVisible(v);
    slOutput.setVisible(v); lbOutput.setVisible(v);

    if (expandedStage)
    {
        btMainStereo.setBounds(990, 114, 120, 22);
        btRoomOn.setBounds(1120, 114, 110, 22);
        stagePad.setBounds(30, 144, 1200, 520);

        const int stripY = 678;
        for (int i = 0; i < 4; ++i)
        {
            const int x = 60 + i * 195;
            btMicSelect[i].setBounds(x, stripY, 105, 20);
            slMicGain[i].setBounds(x + 12, stripY + 22, 82, 78);
            btMicSolo[i].setBounds(x + 100, stripY + 42, 26, 24);
        }
        lbPairWide.setBounds(865, stripY, 100, 18);  slPairWide.setBounds(875, stripY + 20, 82, 78);
        lbFarWide.setBounds(1015, stripY, 100, 18);  slFarWide.setBounds(1025, stripY + 20, 82, 78);
        return;
    }

    btVocalMod.setBounds(32, 138, 248, 22);
    cbVocalIn.setBounds(32, 166, 118, 24);      cbVocalTarget.setBounds(160, 166, 120, 24);
    lbVocalIn.setBounds(36, 196, 100, 16);      slVocalIn.setBounds(44, 212, 82, 76);
    lbVocalOut.setBounds(166, 196, 100, 16);    slVocalOut.setBounds(174, 212, 82, 76);
    cbSource.setBounds(32, 298, 118, 24);       cbMonitor.setBounds(160, 298, 120, 24);
    cbRoom.setBounds(32, 330, 118, 24);         cbWalls.setBounds(160, 330, 120, 24);
    cbCircuit.setBounds(32, 376, 248, 24);
    lbDrive.setBounds(32, 408, 248, 16);        slDrive.setBounds(115, 426, 82, 80);

    btMainStereo.setBounds(732, 114, 112, 22);
    btRoomOn.setBounds(848, 114, 104, 22);
    stagePad.setBounds(306, 140, 648, 298);

    const int micY = 446;
    for (int i = 0; i < 4; ++i)
    {
        const int x = 310 + i * 111;
        btMicSelect[i].setBounds(x, micY, 86, 20);
        slMicGain[i].setBounds(x + 4, micY + 22, 74, 72);
        btMicSolo[i].setBounds(x + 80, micY + 42, 24, 22);
    }
    lbPairWide.setBounds(762, micY, 84, 18); slPairWide.setBounds(768, micY + 20, 74, 72);
    lbFarWide.setBounds(860, micY, 84, 18);  slFarWide.setBounds(866, micY + 20, 74, 72);

    btChamberOn.setBounds(1158, 114, 72, 22);
    cbChamberModel.setBounds(980, 142, 248, 26);
    lbChSend.setBounds(988, 196, 100, 16);   slChamberSend.setBounds(996, 214, 84, 80);
    lbChDecay.setBounds(1126, 196, 100, 16); slChamberDecay.setBounds(1134, 214, 84, 80);
    lbChPre.setBounds(988, 336, 100, 16);    slChamberPre.setBounds(996, 354, 84, 80);
    lbChWidth.setBounds(1126, 336, 100, 16); slChamberWidth.setBounds(1134, 354, 84, 80);

    btEqOn.setBounds(504, 562, 82, 22);
    cbEqModel.setBounds(32, 562, 216, 24);
    lbHpf.setBounds(42, 598, 86, 16);   slHpf.setBounds(42, 616, 84, 78);
    lbLow.setBounds(176, 598, 86, 16);  slLowGain.setBounds(176, 616, 84, 78);  cbLowFreq.setBounds(174, 706, 88, 24);
    lbMid.setBounds(316, 598, 86, 16);  slMidGain.setBounds(316, 616, 84, 78);  cbMidFreq.setBounds(314, 706, 88, 24);
    lbHigh.setBounds(456, 598, 86, 16); slHighGain.setBounds(456, 616, 84, 78); cbHighFreq.setBounds(454, 706, 88, 24);

    btCompOn.setBounds(998, 562, 88, 22);
    cbCompModel.setBounds(618, 562, 164, 24);
    btScLink.setBounds(792, 562, 92, 22);
    btScFilter.setBounds(792, 590, 92, 22);
    lbCompThresh.setBounds(622, 610, 90, 16); slCompThresh.setBounds(624, 628, 84, 78);
    lbCompMix.setBounds(744, 610, 90, 16);    slCompMix.setBounds(746, 628, 84, 78);
    btParCrush.setBounds(914, 590, 110, 22);
    lbParBlend.setBounds(920, 618, 96, 16);   slParBlend.setBounds(926, 634, 80, 72);
    cbParRoute.setBounds(910, 712, 126, 24);

    lbOutput.setBounds(1128, 604, 90, 16);
    slOutput.setBounds(1130, 624, 86, 82);
}
