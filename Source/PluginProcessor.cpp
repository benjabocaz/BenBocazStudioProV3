#include "PluginProcessor.h"
#include "PluginEditor.h"

BenBocazStudioProProcessor::BenBocazStudioProProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout BenBocazStudioProProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    p.push_back (std::make_unique<juce::AudioParameterChoice> (
        "mode", "Mode", juce::StringArray { "Reverb", "Re-Mic" }, 1));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (
        "chainOrder", "Routing", juce::StringArray { "EQ -> Comp -> Chamber", "Comp -> EQ -> Chamber", "Chamber -> Comp -> EQ" }, 0));
    p.push_back (std::make_unique<juce::AudioParameterBool> ("phaseAlign", "Phase Safe Auto-Align", true));
    p.push_back (std::make_unique<juce::AudioParameterBool> ("phaseInvert", "Polarity Invert", false));

    // Emulador de Micrófonos Vocales con Input y Output independientes (OFF por defecto)
    p.push_back (std::make_unique<juce::AudioParameterBool> ("vocalModOn", "Vocal Mic Mod ON", false));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (
        "vocalSrcMic", "Input Mic", juce::StringArray { "Shure SM27", "Shure SM57", "Shure SM7B", "Generic LDC" }, 2));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (
        "vocalTgtMic", "Target Mic", juce::StringArray { "Sony C-800G", "Neumann TLM 103", "Aston Origin" }, 0));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("vocalInGain",  "Mic Mod Input (dB)",  -18.0f, 18.0f, 0.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("vocalOutGain", "Mic Mod Output (dB)", -18.0f, 18.0f, 0.0f));

    // Sala Rústica independiente (ON/OFF), Fuente, Monitores y Estéreo
    p.push_back (std::make_unique<juce::AudioParameterBool> ("roomOn", "Room Stage Power", true));
    p.push_back (std::make_unique<juce::AudioParameterBool> ("mainStereo", "Main Mic Stereo", false));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("pairSpread", "Mid/Air Pair Width", 0.0f, 1.5f, 1.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("deccaWidth", "Decca Tree Spread",  0.2f, 1.5f, 1.0f));

    p.push_back (std::make_unique<juce::AudioParameterChoice> (
        "source", "Source", juce::StringArray { "Vocals", "Drums", "Ensemble", "Monitors (Swedien)" }, 0));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (
        "monitorType", "Monitor", juce::StringArray { "Altec Main", "NS-10 Nearfield", "JBL PA Wedge" }, 0));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (
        "room", "Studio Room", juce::StringArray { "Studio A (Shangri-La)", "Studio B (Canyon)" }, 0));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (
        "walls", "Room Walls", juce::StringArray { "Tight / Damped", "Live / Cedar Wood" }, 1));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("consoleDrive", "Tube / Tape Drive", 0.0f, 1.0f, 0.20f));

    // 4 Atriles de Micrófono Movibles en 2.5D (MAIN, MID, AIR, DECCA)
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("mic1X", "Main Stand X", -1.0f, 1.0f,  0.00f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("mic1Y", "Main Stand Y",  0.0f, 1.0f,  0.18f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("mic1Gain", "Main Level", 0.0f, 1.5f,  1.00f));

    p.push_back (std::make_unique<juce::AudioParameterFloat> ("mic2X", "Mid Stand X",  -1.0f, 1.0f, -0.42f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("mic2Y", "Mid Stand Y",   0.0f, 1.0f,  0.54f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("mic2Gain", "Mid Level",  0.0f, 1.5f,  0.45f));

    p.push_back (std::make_unique<juce::AudioParameterFloat> ("mic3X", "Air Stand X",  -1.0f, 1.0f,  0.42f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("mic3Y", "Air Stand Y",   0.0f, 1.0f,  0.76f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("mic3Gain", "Air Level",  0.0f, 1.5f,  0.35f));

    p.push_back (std::make_unique<juce::AudioParameterFloat> ("mic4X", "Decca Stand X", -1.0f, 1.0f, 0.05f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("mic4Y", "Decca Stand Y",  0.0f, 1.0f, 0.86f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("mic4Gain", "Decca Level", 0.0f, 1.5f, 0.30f));

    // Cámaras Subterráneas con ON/OFF
    p.push_back (std::make_unique<juce::AudioParameterBool> ("chamberOn", "Chamber Power", true));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (
        "chamberType", "Chamber", juce::StringArray {
            "Chamber 1 (Bright)", "Chamber 3 (Classic)", "Chamber 5 (Deep)", "Chamber 8 (Dense)", "Custom WAV IR"
        }, 1));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("chamberMix", "Chamber Send", 0.0f, 1.0f, 0.24f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("chamberDecay", "Chamber Decay", 0.15f, 1.0f, 0.72f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("chamberPreDelay", "Pre-Delay (ms)", 0.0f, 150.0f, 22.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("chamberWidth", "Chamber Width", 0.0f, 1.5f, 1.0f));

    // Ecualizador de Consola con ON/OFF y 4 Modelos
    p.push_back (std::make_unique<juce::AudioParameterBool> ("eqOn", "EQ Power", true));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (
        "eqType", "EQ Model", juce::StringArray {
            "Neve 1073 (British)", "Pultec EQP-1A (Tube)", "API 550A (Punch)", "SSL 4000E (Clean)"
        }, 0));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("eqHpf", "HPF (Hz)", 20.0f, 300.0f, 20.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("eqLow", "Low (dB)", -12.0f, 12.0f, 0.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("eqMid", "Mid (dB)", -12.0f, 12.0f, 0.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("eqHigh", "High (dB)", -12.0f, 12.0f, 0.0f));

    // Compresor Outboard con ON/OFF
    p.push_back (std::make_unique<juce::AudioParameterBool> ("compOn", "Compressor Power", true));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (
        "compType", "Comp Model", juce::StringArray { "1176LN (FET)", "LA-2A (Opto)", "Mastering (Tube)" }, 0));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("compThresh", "Threshold (dB)", -40.0f, 0.0f, -16.0f));

    // Master Output
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("outputGain", "Output (dB)", -18.0f, 12.0f, 0.0f));

    return { p.begin(), p.end() };
}

void BenBocazStudioProProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    srcHpf.prepare (sampleRate); srcLpf.prepare (sampleRate); srcMid.prepare (sampleRate);
    eqHpf.prepare (sampleRate);  eqLow.prepare (sampleRate);  eqPultecDip.prepare (sampleRate);
    eqMid.prepare (sampleRate);  eqHigh.prepare (sampleRate);

    vocalMicModeler.prepare (sampleRate);
    analogStage.prepare (samplesPerBlock);
    compressor.prepare (sampleRate);
    acousticEngine.prepare (sampleRate, samplesPerBlock);
    chamberScratch.setSize (2, samplesPerBlock);

    setLatencySamples (analogStage.getLatencySamples());
    updateFilters();
}

void BenBocazStudioProProcessor::releaseResources() {}

bool BenBocazStudioProProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    const auto inSet = layouts.getMainInputChannelSet();
    return inSet == juce::AudioChannelSet::stereo() || inSet == juce::AudioChannelSet::mono();
}

void BenBocazStudioProProcessor::updateFilters()
{
    const int vSrc = static_cast<int> (apvts.getRawParameterValue ("vocalSrcMic")->load());
    const int vTgt = static_cast<int> (apvts.getRawParameterValue ("vocalTgtMic")->load());
    vocalMicModeler.updateCurves (vSrc, vTgt);

    const int srcIdx = static_cast<int> (apvts.getRawParameterValue ("source")->load());
    const int monIdx = static_cast<int> (apvts.getRawParameterValue ("monitorType")->load());
    if (srcIdx == 3)
    {
        if (monIdx == 0)      { srcHpf.setHighPass (48.0f);  srcLpf.setLowPass (16500.0f); srcMid.setPeak (2200.0f, 1.5f, 1.0f); }
        else if (monIdx == 1) { srcHpf.setHighPass (115.0f); srcLpf.setLowPass (10200.0f); srcMid.setPeak (1750.0f, 4.0f, 1.1f); }
        else                  { srcHpf.setHighPass (82.0f);  srcLpf.setLowPass (13800.0f); srcMid.setPeak (3100.0f, 3.0f, 1.3f); }
    }
    else
    {
        srcHpf.setBypass(); srcLpf.setBypass(); srcMid.setBypass();
    }

    const int eqType = static_cast<int> (apvts.getRawParameterValue ("eqType")->load());
    const float hpf  = apvts.getRawParameterValue ("eqHpf")->load();
    const float low  = apvts.getRawParameterValue ("eqLow")->load();
    const float mid  = apvts.getRawParameterValue ("eqMid")->load();
    const float high = apvts.getRawParameterValue ("eqHigh")->load();

    if (hpf > 22.0f) eqHpf.setHighPass (hpf, 0.7071f);
    else             eqHpf.setBypass();

    eqPultecDip.setBypass();

    if (eqType == 0)
    {
        eqLow.setLowShelf (110.0f, low);
        eqMid.setPeak (1600.0f, mid, 0.85f);
        eqHigh.setHighShelf (12000.0f, high);
    }
    else if (eqType == 1)
    {
        eqLow.setLowShelf (65.0f, low * 1.08f);
        if (low > 0.5f)
            eqPultecDip.setPeak (185.0f, -low * 0.38f, 1.15f);
        eqMid.setPeak (1000.0f, mid, 0.65f);
        eqHigh.setHighShelf (10000.0f, high);
    }
    else if (eqType == 2)
    {
        const float propQ = 0.9f + std::abs (mid) * 0.08f;
        eqLow.setLowShelf (100.0f, low);
        eqMid.setPeak (3000.0f, mid, propQ);
        eqHigh.setHighShelf (7500.0f, high);
    }
    else
    {
        eqLow.setLowShelf (80.0f, low);
        eqMid.setPeak (2200.0f, mid, 1.55f);
        eqHigh.setHighShelf (8500.0f, high);
    }
}

void BenBocazStudioProProcessor::processConsoleEQ (juce::AudioBuffer<float>& buffer)
{
    if (apvts.getRawParameterValue ("eqOn")->load() < 0.5f) return;

    eqHpf.processBuffer (buffer);
    eqLow.processBuffer (buffer);
    eqPultecDip.processBuffer (buffer);
    eqMid.processBuffer (buffer);
    eqHigh.processBuffer (buffer);
}

void BenBocazStudioProProcessor::processCompressorStage (juce::AudioBuffer<float>& buffer)
{
    const bool enabled = apvts.getRawParameterValue ("compOn")->load() >= 0.5f;
    const int compType = static_cast<int> (apvts.getRawParameterValue ("compType")->load());
    const float thresh = apvts.getRawParameterValue ("compThresh")->load();
    compressor.process (buffer, enabled, compType, thresh);
}

void BenBocazStudioProProcessor::processChamberStage (juce::AudioBuffer<float>& buffer)
{
    const bool enabled = apvts.getRawParameterValue ("chamberOn")->load() >= 0.5f;
    const int chType   = static_cast<int> (apvts.getRawParameterValue ("chamberType")->load());
    const float chMix  = apvts.getRawParameterValue ("chamberMix")->load();
    const float chDec  = apvts.getRawParameterValue ("chamberDecay")->load();
    const float chPre  = apvts.getRawParameterValue ("chamberPreDelay")->load();
    const float chWid  = apvts.getRawParameterValue ("chamberWidth")->load();

    chamberScratch.setSize (2, buffer.getNumSamples(), false, false, true);
    acousticEngine.processChamber (buffer, chamberScratch, enabled, chType, chMix, chDec, chPre, chWid);
}

void BenBocazStudioProProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    if (getTotalNumInputChannels() == 1 && buffer.getNumChannels() >= 2)
        buffer.copyFrom (1, 0, buffer, 0, 0, buffer.getNumSamples());

    updateFilters();

    const bool vocalModOn = apvts.getRawParameterValue ("vocalModOn")->load() >= 0.5f;
    const float vocalIn   = apvts.getRawParameterValue ("vocalInGain")->load();
    const float vocalOut  = apvts.getRawParameterValue ("vocalOutGain")->load();
    vocalMicModeler.process (buffer, vocalModOn, vocalIn, vocalOut);

    const bool roomOn   = apvts.getRawParameterValue ("roomOn")->load() >= 0.5f;
    const int sourceIdx = static_cast<int> (apvts.getRawParameterValue ("source")->load());

    if (roomOn)
    {
        srcHpf.processBuffer (buffer);
        srcMid.processBuffer (buffer);
        srcLpf.processBuffer (buffer);

        MultiMicRoomConfig cfg;
        cfg.modeIdx     = static_cast<int> (apvts.getRawParameterValue ("mode")->load());
        cfg.sourceIdx   = sourceIdx;
        cfg.roomIdx     = static_cast<int> (apvts.getRawParameterValue ("room")->load());
        cfg.wallsLive   = static_cast<int> (apvts.getRawParameterValue ("walls")->load());
        cfg.phaseSafe   = apvts.getRawParameterValue ("phaseAlign")->load() >= 0.5f;
        cfg.phaseInvert = apvts.getRawParameterValue ("phaseInvert")->load() >= 0.5f;
        cfg.mainStereo  = apvts.getRawParameterValue ("mainStereo")->load() >= 0.5f;
        cfg.pairSpread  = apvts.getRawParameterValue ("pairSpread")->load();
        cfg.deccaWidth  = apvts.getRawParameterValue ("deccaWidth")->load();

        cfg.micX    = { apvts.getRawParameterValue ("mic1X")->load(), apvts.getRawParameterValue ("mic2X")->load(),
                        apvts.getRawParameterValue ("mic3X")->load(), apvts.getRawParameterValue ("mic4X")->load() };
        cfg.micY    = { apvts.getRawParameterValue ("mic1Y")->load(), apvts.getRawParameterValue ("mic2Y")->load(),
                        apvts.getRawParameterValue ("mic3Y")->load(), apvts.getRawParameterValue ("mic4Y")->load() };
        cfg.micGain = { apvts.getRawParameterValue ("mic1Gain")->load(), apvts.getRawParameterValue ("mic2Gain")->load(),
                        apvts.getRawParameterValue ("mic3Gain")->load(), apvts.getRawParameterValue ("mic4Gain")->load() };

        acousticEngine.processMultiMicRoom (buffer, cfg);
    }

    const float drive = apvts.getRawParameterValue ("consoleDrive")->load();
    analogStage.process (buffer, drive, roomOn && sourceIdx == 3);

    const int order = static_cast<int> (apvts.getRawParameterValue ("chainOrder")->load());
    if (order == 0)      { processConsoleEQ (buffer); processCompressorStage (buffer); processChamberStage (buffer); }
    else if (order == 1) { processCompressorStage (buffer); processConsoleEQ (buffer); processChamberStage (buffer); }
    else                 { processChamberStage (buffer); processCompressorStage (buffer); processConsoleEQ (buffer); }

    const float outGain = juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("outputGain")->load());
    buffer.applyGain (outGain);

    const float rms = 0.5f * (buffer.getRMSLevel (0, 0, buffer.getNumSamples())
                            + (buffer.getNumChannels() > 1 ? buffer.getRMSLevel (1, 0, buffer.getNumSamples()) : 0.0f));
    outputVuDb.store (juce::Decibels::gainToDecibels (rms + 1e-6f));
}

juce::AudioProcessorEditor* BenBocazStudioProProcessor::createEditor()
{
    return new BenBocazStudioProEditor (*this);
}

void BenBocazStudioProProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void BenBocazStudioProProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState && xmlState->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BenBocazStudioProProcessor();
}
