#include "PluginEditor.h"
#if HAS_CUSTOM_ASSETS
    #include "BinaryData.h"
#endif

//==============================================================================
// Look & Feel
AndesLookAndFeel::AndesLookAndFeel()
{
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff1c1612));
    setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff5a4634));
    setColour(juce::ComboBox::textColourId, juce::Colour(0xfff0e2cc));
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff1a1410));
    setColour(juce::PopupMenu::textColourId, juce::Colour(0xfff0e2cc));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff8c5a2b));
    setColour(juce::TextButton::buttonColourId, juce::Colour(0xff261e18));
    setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffc98a3b));
    setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe8d9c0));
    setColour(juce::TextButton::textColourOnId, juce::Colour(0xff140f0b));
    setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xfff0e2cc));
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

void AndesLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                        juce::Slider&)
{
    const float radius = static_cast<float>(juce::jmin(width, height)) * 0.42f;
    const float cx = static_cast<float>(x) + static_cast<float>(width) * 0.5f;
    const float cy = static_cast<float>(y) + static_cast<float>(height) * 0.5f;
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // Anillo exterior
    juce::Path track;
    track.addCentredArc(cx, cy, radius + 3.0f, radius + 3.0f, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(juce::Colour(0xff3a2c20));
    g.strokePath(track, juce::PathStrokeType(2.2f));

    juce::Path active;
    active.addCentredArc(cx, cy, radius + 3.0f, radius + 3.0f, 0.0f, rotaryStartAngle, angle, true);
    g.setColour(juce::Colour(0xffd99b43));
    g.strokePath(active, juce::PathStrokeType(2.6f));

    // Cuerpo metalico
    juce::ColourGradient grad(juce::Colour(0xff4a3b2e), cx - radius * 0.4f, cy - radius * 0.4f,
                              juce::Colour(0xff18120e), cx + radius * 0.6f, cy + radius * 0.6f, true);
    g.setGradientFill(grad);
    g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

    g.setColour(juce::Colour(0xff6b543f));
    g.drawEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 1.2f);

    // Indicador
    juce::Path ptr;
    ptr.addRoundedRectangle(-1.6f, -radius + 3.0f, 3.2f, radius * 0.55f, 1.2f);
    ptr.applyTransform(juce::AffineTransform::rotation(angle).translated(cx, cy));
    g.setColour(juce::Colour(0xfff5d79b));
    g.fillPath(ptr);
}

void AndesLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                        bool isHover, bool)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
    const bool on = button.getToggleState();

    g.setColour(on ? juce::Colour(0xff3d2c1b) : juce::Colour(0xff1b1511));
    g.fillRoundedRectangle(bounds, 4.0f);
    g.setColour(on ? juce::Colour(0xffd99b43) : (isHover ? juce::Colour(0xff7a6148) : juce::Colour(0xff473729)));
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

    const float ledR = 3.6f;
    const float ledX = bounds.getX() + 9.0f;
    const float ledY = bounds.getCentreY();
    g.setColour(on ? juce::Colour(0xffffb84d) : juce::Colour(0xff44382d));
    g.fillEllipse(ledX - ledR, ledY - ledR, ledR * 2.0f, ledR * 2.0f);

    g.setColour(on ? juce::Colour(0xffffecd0) : juce::Colour(0xffbba992));
    g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
    g.drawFittedText(button.getButtonText(),
                     bounds.withTrimmedLeft(18.0f).withTrimmedRight(4.0f).toNearestInt(),
                     juce::Justification::centredLeft, 1);
}

//==============================================================================
// AnalogVUMeter
void AnalogVUMeter::setLevelDb(float db, bool isGainReduction)
{
    grMode = isGainReduction;
    float norm = 0.0f;
    if (grMode)
        norm = std::clamp(1.0f - (db / 18.0f), 0.0f, 1.0f);
    else
        norm = std::clamp((db + 36.0f) / 39.0f, 0.0f, 1.0f);

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
    juce::ColourGradient bg(juce::Colour(0xffe5b869), b.getCentreX(), b.getY(),
                            juce::Colour(0xffb57c35), b.getCentreX(), b.getBottom(), false);
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
// Studio4MicStagePad
Studio4MicStagePad::Studio4MicStagePad(juce::AudioProcessorValueTreeState& state)
    : apvts(state)
{
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

            if (greenExcess > 48 && g > 95)
            {
                bd.setPixelColour(x, y, juce::Colours::transparentBlack);
            }
            else if (greenExcess > 18 && g > 70)
            {
                const float alpha = 1.0f - std::clamp(static_cast<float>(greenExcess - 18) / 30.0f, 0.0f, 1.0f);
                const uint8_t cleanG = static_cast<uint8_t>(std::min(g, maxRB + 12));
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

        if (name.containsIgnoreCase("volcano") && name.containsIgnoreCase("open"))         imgVolcanoOpen = loadImg();
        else if (name.containsIgnoreCase("volcano") && name.containsIgnoreCase("closed"))  imgVolcanoClosed = loadImg();
        else if (name.containsIgnoreCase("shangri"))                                       imgShangrila = loadImg();
        else if (name.containsIgnoreCase("canyon"))                                        imgCanyon = loadImg();
        else if (name.containsIgnoreCase("mic_main_mono"))                                 imgMicMono = removeGreenScreen(loadImg());
        else if (name.containsIgnoreCase("mic_main_xy"))                                   imgMicXY = removeGreenScreen(loadImg());
        else if (name.containsIgnoreCase("mic_stereo_pair"))                               imgMicPair = removeGreenScreen(loadImg());
        else if (name.containsIgnoreCase("mic_far_l"))                                     imgMicFarL = removeGreenScreen(loadImg());
        else if (name.containsIgnoreCase("mic_far_r"))                                     imgMicFarR = removeGreenScreen(loadImg());
        else if (name.containsIgnoreCase("src_drums"))                                     imgSrcDrums = removeGreenScreen(loadImg());
        else if (name.containsIgnoreCase("src_bass"))                                      imgSrcBass = removeGreenScreen(loadImg());
        else if (name.containsIgnoreCase("src_ensemble"))                                  imgSrcEnsemble = removeGreenScreen(loadImg());
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
        setMicPos(2, 0.14f, 0.86f);
        setMicPos(3, 0.86f, 0.86f);
    }
    else
    {
        const char* yId = (selectedMic == 0) ? "mic1Y" : "mic2Y";
        setMicPos(selectedMic, 0.50f, apvts.getRawParameterValue(yId)->load());
    }
    repaint();
}

juce::Point<float> Studio4MicStagePad::normToScreen(float nx, float ny, juce::Rectangle<float> area) const
{
    // Perspectiva levemente elevada: ny=0 cerca del fondo (fuente), ny=1 frente/extremo
    const float yTop = area.getY() + area.getHeight() * 0.56f;
    const float yBot = area.getBottom() - area.getHeight() * 0.05f;
    const float sy = yTop + ny * (yBot - yTop);

    const float spread = 0.52f + 0.44f * ny;
    const float sx = area.getCentreX() + (nx - 0.5f) * area.getWidth() * spread;
    return { sx, sy };
}

juce::Point<float> Studio4MicStagePad::screenToNorm(juce::Point<float> pt, juce::Rectangle<float> area) const
{
    const float yTop = area.getY() + area.getHeight() * 0.56f;
    const float yBot = area.getBottom() - area.getHeight() * 0.05f;
    const float ny = std::clamp((pt.y - yTop) / std::max(1.0f, yBot - yTop), 0.05f, 0.95f);

    const float spread = 0.52f + 0.44f * ny;
    const float nx = std::clamp(0.5f + (pt.x - area.getCentreX()) / std::max(1.0f, area.getWidth() * spread), 0.05f, 0.95f);
    return { nx, ny };
}

void Studio4MicStagePad::setMicPos(int idx, float nx, float ny)
{
    static constexpr const char* xIds[4] = { "mic1X", "mic2X", "mic3X", "mic4X" };
    static constexpr const char* yIds[4] = { "mic1Y", "mic2Y", "mic3Y", "mic4Y" };

    if (auto* px = apvts.getParameter(xIds[idx])) px->setValue(px->convertTo0to1(nx));
    if (auto* py = apvts.getParameter(yIds[idx])) py->setValue(py->convertTo0to1(ny));
}

void Studio4MicStagePad::paint(juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat().reduced(2.0f);
    const int roomIdx   = static_cast<int>(apvts.getRawParameterValue("room")->load());
    const int wallsIdx  = static_cast<int>(apvts.getRawParameterValue("walls")->load());
    const int srcIdx    = static_cast<int>(apvts.getRawParameterValue("source")->load());
    const bool mainSt   = apvts.getRawParameterValue("mainStereo")->load() > 0.5f;
    const bool phaseOk  = apvts.getRawParameterValue("phaseSafe")->load() > 0.5f;

    // 1. Fondo de sala
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

    // 2. Fuente al fondo (Drums / Bass / Ensemble)
    const juce::Image* srcImg = (srcIdx == 1) ? &imgSrcDrums : (srcIdx == 2 ? &imgSrcBass : (srcIdx == 4 ? &imgSrcEnsemble : nullptr));
    if (srcImg && srcImg->isValid())
    {
        const float sh = area.getHeight() * 0.30f;
        const float sw = sh * (static_cast<float>(srcImg->getWidth()) / std::max(1, srcImg->getHeight()));
        g.drawImage(*srcImg, juce::Rectangle<float>(area.getCentreX() - sw * 0.5f, area.getY() + area.getHeight() * 0.34f, sw, sh));
    }

    // 3. Dibujar 4 atriles ordenados por profundidad (Y)
    static constexpr const char* xIds[4] = { "mic1X", "mic2X", "mic3X", "mic4X" };
    static constexpr const char* yIds[4] = { "mic1Y", "mic2Y", "mic3Y", "mic4Y" };
    static constexpr const char* gIds[4] = { "mic1Gain", "mic2Gain", "mic3Gain", "mic4Gain" };
    static constexpr const char* labels[4] = { "1:MAIN", "2:ROOM", "3:FAR L", "4:FAR R" };

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

        const float scale = 0.24f + 0.26f * ny;
        const float micH = area.getHeight() * scale;

        const juce::Image* micImg = nullptr;
        if (idx == 0)      micImg = mainSt ? &imgMicXY : &imgMicMono;
        else if (idx == 1) micImg = &imgMicPair;
        else if (idx == 2) micImg = imgMicFarL.isValid() ? &imgMicFarL : &imgMicMono;
        else               micImg = imgMicFarR.isValid() ? &imgMicFarR : &imgMicMono;

        const float alpha = (gain > 0.01f) ? 1.0f : 0.45f;
        g.setOpacity(alpha);

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

        // Anillo de seleccion en la base
        if (idx == selectedMic)
        {
            g.setColour(juce::Colour(0xffffb84d));
            g.drawEllipse(pos.x - 16.0f, pos.y - 6.0f, 32.0f, 11.0f, 2.0f);
        }

        // Etiqueta flotante con distancia en metros
        const float distM = BenBocazDSP::AcousticEngine::computeDistanceMeters(roomIdx, nx, ny);
        const juce::String tag = juce::String(labels[idx]) + " " + juce::String(distM, 1) + "m";
        const juce::Rectangle<float> tagRect(pos.x - 44.0f, pos.y - micH - 18.0f, 88.0f, 16.0f);

        g.setColour(idx == selectedMic ? juce::Colour(0xd9261b12) : juce::Colour(0xb0140e0a));
        g.fillRoundedRectangle(tagRect, 3.0f);
        g.setColour(idx == selectedMic ? juce::Colour(0xffffb84d) : juce::Colour(0xff8c7258));
        g.drawRoundedRectangle(tagRect, 3.0f, 1.0f);
        g.setColour(juce::Colour(0xfff5e6d0));
        g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
        g.drawFittedText(tag, tagRect.toNearestInt(), juce::Justification::centred, 1);
    }

    // 4. Barra superior de estado dentro del escenario
    const juce::String roomTitle = (roomIdx == 0) ? (wallsIdx == 0 ? "VOLCANO HALL | GATES OPEN" : "VOLCANO HALL | GATES CLOSED")
                                 : (roomIdx == 1 ? "SHANGRI-LA | LIVE WOOD" : "CANYON ROOM | STUDIO WOOD");
    g.setColour(juce::Colour(0xb0120c08));
    g.fillRect(area.withHeight(20.0f));
    g.setColour(juce::Colour(0xffe6d2b5));
    g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
    g.drawText(roomTitle, area.withHeight(20.0f).reduced(8.0f, 0.0f), juce::Justification::centredLeft);

    g.setColour(phaseOk ? juce::Colour(0xff9be08a) : juce::Colour(0xffe0a85c));
    g.drawText(phaseOk ? "[OK] PHASE LOCKED" : "NATURAL TIME-OF-FLIGHT",
               area.withHeight(20.0f).reduced(8.0f, 0.0f), juce::Justification::centredRight);

    g.setColour(juce::Colour(0xff5a4634));
    g.drawRoundedRectangle(area, 4.0f, 1.5f);
}

void Studio4MicStagePad::mouseDown(const juce::MouseEvent& e)
{
    auto area = getLocalBounds().toFloat().reduced(2.0f);
    static constexpr const char* xIds[4] = { "mic1X", "mic2X", "mic3X", "mic4X" };
    static constexpr const char* yIds[4] = { "mic1Y", "mic2Y", "mic3Y", "mic4Y" };

    float bestDist = std::max(44.0f, area.getHeight() * 0.14f);
    int bestIdx = -1;
    for (int i = 0; i < 4; ++i)
    {
        const float nx = apvts.getRawParameterValue(xIds[i])->load();
        const float ny = apvts.getRawParameterValue(yIds[i])->load();
        const auto p = normToScreen(nx, ny, area);
        const float micH = area.getHeight() * (0.24f + 0.26f * ny);
        const float d = e.position.getDistanceFrom(p.translated(0.0f, -micH * 0.45f));
        if (d < bestDist)
        {
            bestDist = d;
            bestIdx = i;
        }
    }

    if (bestIdx >= 0)
    {
        selectedMic = bestIdx;
        draggingMic = bestIdx;
        repaint();
    }
}

void Studio4MicStagePad::mouseDrag(const juce::MouseEvent& e)
{
    if (draggingMic < 0) return;

    auto area = getLocalBounds().toFloat().reduced(2.0f);
    auto norm = screenToNorm(e.position, area);

    const bool snap = apvts.getRawParameterValue("snapCenter")->load() > 0.5f;
    const bool link34 = apvts.getRawParameterValue("link34")->load() > 0.5f;

    if (snap && draggingMic < 2 && std::abs(norm.x - 0.5f) < 0.045f)
        norm.x = 0.5f;

    setMicPos(draggingMic, norm.x, norm.y);

    // Movimiento en espejo para FAR L (2) y FAR R (3)
    if (link34 && (draggingMic == 2 || draggingMic == 3))
    {
        const int other = (draggingMic == 2) ? 3 : 2;
        setMicPos(other, std::clamp(1.0f - norm.x, 0.05f, 0.95f), norm.y);
    }

    repaint();
}

void Studio4MicStagePad::mouseUp(const juce::MouseEvent&)
{
    draggingMic = -1;
}

//==============================================================================
// Editor Principal
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

    // Top bar
    addCombo(cbMode, "mode");
    addCombo(cbRouting, "routing");
    addToggle(btHqMode, "hqMode");
    btHqMode.onClick = [this]() { updateHqButtonText(); };
    updateHqButtonText();

    addToggle(btPhaseSafe, "phaseSafe");
    addToggle(btPolarity, "polarityInv");
    addToggle(btSnapCenter, "snapCenter");
    addToggle(btLink34, "link34");

    for (int i = 0; i < processor.getNumPrograms(); ++i)
        cbPresets.addItem(processor.getProgramName(i), i + 1);
    cbPresets.setSelectedId(processor.getCurrentProgram() + 1, juce::dontSendNotification);
    cbPresets.onChange = [this]()
    {
        const int idx = cbPresets.getSelectedId() - 1;
        if (idx >= 0)
        {
            processor.applyFactoryPreset(idx);
            stagePad.repaint();
        }
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

    // Stage
    addToggle(btMainStereo, "mainStereo");
    addToggle(btRoomOn, "roomOn");
    addAndMakeVisible(stagePad);

    // Left Panel
    addToggle(btVocalMod, "vocalModOn");
    addCombo(cbVocalIn, "vocalInMic");
    addCombo(cbVocalTarget, "vocalTargetMic");
    addKnob(slVocalIn, "vocalInGain");   setupLabel(lbVocalIn, "MIC IN");
    addKnob(slVocalOut, "vocalOutGain"); setupLabel(lbVocalOut, "MIC OUT");
    addCombo(cbSource, "source");
    addCombo(cbMonitor, "monitorSpeaker");
    addCombo(cbRoom, "room");
    addCombo(cbWalls, "walls");
    addCombo(cbCircuit, "circuit");
    addKnob(slDrive, "drive");           setupLabel(lbDrive, "TUBE / TAPE DRIVE");

    // 4 Mics strip
    static constexpr const char* micBtnNames[4] = { "1: MAIN", "2: ROOM/AIR", "3: FAR L", "4: FAR R" };
    static constexpr const char* micGainIds[4]  = { "mic1Gain", "mic2Gain", "mic3Gain", "mic4Gain" };
    static constexpr const char* micSoloIds[4]  = { "mic1Solo", "mic2Solo", "mic3Solo", "mic4Solo" };

    for (int i = 0; i < 4; ++i)
    {
        btMicSelect[i].setButtonText(micBtnNames[i]);
        btMicSelect[i].onClick = [this, i]() { stagePad.setSelectedMic(i); };
        addAndMakeVisible(btMicSelect[i]);
        addKnob(slMicGain[i], micGainIds[i]);
        btMicSolo[i].setButtonText("S");
        addToggle(btMicSolo[i], micSoloIds[i]);
    }
    addKnob(slPairWide, "pairWide"); setupLabel(lbPairWide, "ROOM WIDE");
    addKnob(slFarWide, "farWide");   setupLabel(lbFarWide, "FAR WIDE");

    // Right Panel: Echo Chambers
    addToggle(btChamberOn, "chamberOn");
    addCombo(cbChamberModel, "chamberModel");
    addKnob(slChamberSend, "chamberSend");       setupLabel(lbChSend, "SEND");
    addKnob(slChamberDecay, "chamberDecay");     setupLabel(lbChDecay, "DECAY");
    addKnob(slChamberPre, "chamberPreDelay");    setupLabel(lbChPre, "PRE-DELAY");
    addKnob(slChamberWidth, "chamberWidth");     setupLabel(lbChWidth, "WIDTH");

    // Bottom Left: Console EQ
    addToggle(btEqOn, "eqOn");
    addCombo(cbEqModel, "eqModel");
    addKnob(slHpf, "hpf");           setupLabel(lbHpf, "HPF");
    addKnob(slLowGain, "lowGain");   setupLabel(lbLow, "LOW");
    addKnob(slMidGain, "midGain");   setupLabel(lbMid, "MID");
    addKnob(slHighGain, "highGain"); setupLabel(lbHigh, "HIGH");
    addCombo(cbLowFreq, "lowFreq");
    addCombo(cbMidFreq, "midFreq");
    addCombo(cbHighFreq, "highFreq");

    // Bottom Mid: Outboard & Parallel Comp
    addToggle(btCompOn, "compOn");
    addCombo(cbCompModel, "compModel");
    addKnob(slCompThresh, "compThresh"); setupLabel(lbCompThresh, "THRESHOLD");
    addKnob(slCompMix, "compMix");       setupLabel(lbCompMix, "MIX");
    addToggle(btScLink, "scLink");
    addToggle(btScFilter, "scFilter");
    addToggle(btParCrush, "parCrush");
    addKnob(slParBlend, "parBlend");     setupLabel(lbParBlend, "PAR BLEND");
    addCombo(cbParRoute, "parRoute");

    // Bottom Right: Master
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
    // Evita el clasico "-0.00" en punto flotante
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
    l.setColour(juce::Label::textColourId, juce::Colour(0xffc9b296));
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

    // Velo oscuro sutil para legibilidad
    g.setColour(juce::Colour(0x88120c08));
    g.fillRect(getLocalBounds());

    // Header
    g.setColour(juce::Colour(0xd917110d));
    g.fillRect(0, 0, getWidth(), 102);
    g.setColour(juce::Colour(0xff5a4634));
    g.drawHorizontalLine(102, 0.0f, static_cast<float>(getWidth()));

    // Titulo del chasis: LOS ANDES CAPITAL
    g.setColour(juce::Colour(0xfff5e6d0));
    g.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    g.drawText("LOS ANDES CAPITAL", 24, 10, 320, 30, juce::Justification::centredLeft);

    g.setColour(juce::Colour(0xffc99a5b));
    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.drawText("VOLCANO HALL  |  3D 4-STAND RE-MIC  |  ANALOG CONSOLE", 24, 40, 360, 16, juce::Justification::centredLeft);
}

void BenBocazStudioProAudioProcessorEditor::resized()
{
    // Top Row 1
    cbMode.setBounds(370, 22, 105, 26);
    cbRouting.setBounds(485, 22, 185, 26);
    btHqMode.setBounds(680, 10, 115, 22);
    btPhaseSafe.setBounds(680, 36, 115, 22);
    btPolarity.setBounds(805, 36, 115, 22);
    vuMeter.setBounds(930, 8, 210, 48);
    btVuMode.setBounds(1150, 14, 92, 34);

    // Top Row 2 (Presets & Stage tools, sin boton i TIPS)
    cbPresets.setBounds(24, 68, 285, 24);
    btSnapCenter.setBounds(322, 68, 115, 24);
    btLink34.setBounds(446, 68, 98, 24);
    btCenterMic.setBounds(554, 68, 105, 24);
    btExpandStage.setBounds(1110, 68, 132, 24);

    const bool showRacks = !expandedStage;
    auto setRacksVisible = [&](bool v)
    {
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
    };
    setRacksVisible(showRacks);

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
            btMicSolo[i].setBounds(x + 100, stripY + 42, 34, 22);
        }
        lbPairWide.setBounds(865, stripY, 100, 18);
        slPairWide.setBounds(875, stripY + 20, 82, 78);
        lbFarWide.setBounds(1015, stripY, 100, 18);
        slFarWide.setBounds(1025, stripY + 20, 82, 78);
        return;
    }

    // Vista normal completa (1260 x 790)
    // Left Rack (24 .. 288)
    btVocalMod.setBounds(28, 120, 250, 24);
    cbVocalIn.setBounds(28, 152, 120, 24);
    cbVocalTarget.setBounds(158, 152, 120, 24);
    lbVocalIn.setBounds(34, 184, 100, 16);   slVocalIn.setBounds(42, 200, 84, 78);
    lbVocalOut.setBounds(164, 184, 100, 16); slVocalOut.setBounds(172, 200, 84, 78);

    cbSource.setBounds(28, 294, 120, 24);
    cbMonitor.setBounds(158, 294, 120, 24);
    cbRoom.setBounds(28, 330, 120, 24);
    cbWalls.setBounds(158, 330, 120, 24);

    cbCircuit.setBounds(28, 386, 250, 24);
    lbDrive.setBounds(28, 420, 250, 16);
    slDrive.setBounds(110, 440, 86, 82);

    // Center Stage (304 .. 956)
    btMainStereo.setBounds(726, 114, 112, 22);
    btRoomOn.setBounds(846, 114, 106, 22);
    stagePad.setBounds(304, 142, 650, 300);

    const int micY = 450;
    for (int i = 0; i < 4; ++i)
    {
        const int x = 308 + i * 112;
        btMicSelect[i].setBounds(x, micY, 88, 20);
        slMicGain[i].setBounds(x + 4, micY + 22, 76, 74);
        btMicSolo[i].setBounds(x + 82, micY + 42, 24, 22);
    }
    lbPairWide.setBounds(762, micY, 86, 18); slPairWide.setBounds(768, micY + 20, 76, 74);
    lbFarWide.setBounds(862, micY, 86, 18);  slFarWide.setBounds(868, micY + 20, 76, 74);

    // Right Rack: Echo Chambers (972 .. 1236)
    btChamberOn.setBounds(1164, 114, 72, 22);
    cbChamberModel.setBounds(976, 146, 260, 26);
    lbChSend.setBounds(986, 204, 100, 16);  slChamberSend.setBounds(994, 222, 86, 82);
    lbChDecay.setBounds(1126, 204, 100, 16); slChamberDecay.setBounds(1134, 222, 86, 82);
    lbChPre.setBounds(986, 344, 100, 16);   slChamberPre.setBounds(994, 362, 86, 82);
    lbChWidth.setBounds(1126, 344, 100, 16); slChamberWidth.setBounds(1134, 362, 86, 82);

    // Bottom Left: Console EQ (24 .. 596)
    btEqOn.setBounds(508, 566, 82, 22);
    cbEqModel.setBounds(30, 566, 220, 24);
    lbHpf.setBounds(40, 606, 86, 16);  slHpf.setBounds(40, 624, 86, 82);
    lbLow.setBounds(174, 606, 86, 16); slLowGain.setBounds(174, 624, 86, 82); cbLowFreq.setBounds(172, 716, 90, 24);
    lbMid.setBounds(316, 606, 86, 16); slMidGain.setBounds(316, 624, 86, 82); cbMidFreq.setBounds(314, 716, 90, 24);
    lbHigh.setBounds(458, 606, 86, 16); slHighGain.setBounds(458, 624, 86, 82); cbHighFreq.setBounds(456, 716, 90, 24);

    // Bottom Mid: Comp & Parallel (614 .. 1094)
    btCompOn.setBounds(1002, 566, 88, 22);
    cbCompModel.setBounds(620, 566, 165, 24);
    btScLink.setBounds(796, 566, 94, 22);
    btScFilter.setBounds(796, 594, 94, 22);

    lbCompThresh.setBounds(624, 614, 90, 16); slCompThresh.setBounds(626, 632, 86, 82);
    lbCompMix.setBounds(748, 614, 90, 16);    slCompMix.setBounds(750, 632, 86, 82);

    btParCrush.setBounds(918, 594, 110, 22);
    lbParBlend.setBounds(924, 622, 96, 16);
    slParBlend.setBounds(930, 638, 82, 76);
    cbParRoute.setBounds(914, 718, 126, 24);

    // Bottom Right: Master (1110 .. 1236)
    lbOutput.setBounds(1130, 608, 90, 16);
    slOutput.setBounds(1132, 628, 88, 86);
}
