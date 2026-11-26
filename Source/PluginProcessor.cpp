#include "PluginProcessor.h"
#include "PluginEditor.h"

BenBocazStudioProAudioProcessor::BenBocazStudioProAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout BenBocazStudioProAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    p.push_back(std::make_unique<juce::AudioParameterChoice>("mode", "Mode", juce::StringArray{ "Re-Mic", "Insert" }, 0));
    p.push_back(std::make_unique<juce::AudioParameterChoice>("routing", "Routing", juce::StringArray{ "EQ -> Comp -> Chamber", "Chamber -> EQ -> Comp" }, 0));
    p.push_back(std::make_unique<juce::AudioParameterBool>("hqMode", "HQ 4x Mode", true));
    p.push_back(std::make_unique<juce::AudioParameterBool>("phaseSafe", "Phase Safe", true));
    p.push_back(std::make_unique<juce::AudioParameterBool>("polarityInv", "Polarity Inv", false));
    p.push_back(std::make_unique<juce::AudioParameterBool>("snapCenter", "Snap Center", true));
    p.push_back(std::make_unique<juce::AudioParameterBool>("link34", "Link 3+4", true));
    p.push_back(std::make_unique<juce::AudioParameterBool>("mainStereo", "Main Stereo", false));
    p.push_back(std::make_unique<juce::AudioParameterBool>("roomOn", "Room On", true));

    p.push_back(std::make_unique<juce::AudioParameterBool>("vocalModOn", "Vocal Mic Mod", false));
    p.push_back(std::make_unique<juce::AudioParameterChoice>("vocalInMic", "Input Mic", juce::StringArray{ "Shure SM7B", "Neumann U87", "Condenser", "Shure SM58" }, 0));
    p.push_back(std::make_unique<juce::AudioParameterChoice>("vocalTargetMic", "Target Mic", juce::StringArray{ "Sony C-800G", "Neumann U47", "Telefunken 251", "U87 Vintage" }, 0));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("vocalInGain", "Mic In", juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("vocalOutGain", "Mic Out", juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));

    p.push_back(std::make_unique<juce::AudioParameterChoice>("source", "Room Source",
        juce::StringArray{ "Vocals", "Drums", "Bass Amp", "Guitar Cab Mono", "Guitar Cab Stereo", "Ensemble", "Monitors" }, 0));
    p.push_back(std::make_unique<juce::AudioParameterChoice>("monitorSpeaker", "Monitor Speaker", juce::StringArray{ "Altec Main", "NS-10M", "Auratone" }, 0));
    p.push_back(std::make_unique<juce::AudioParameterChoice>("room", "Studio Room", juce::StringArray{ "Volcano Hall", "Shangri-La", "Canyon Room" }, 0));
    p.push_back(std::make_unique<juce::AudioParameterChoice>("walls", "Room Walls", juce::StringArray{ "Gates Open", "Gates Closed" }, 1));

    p.push_back(std::make_unique<juce::AudioParameterChoice>("circuit", "Tube Circuit", juce::StringArray{ "Neve Class-A", "API Discrete", "Ampex Tape" }, 0));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("drive", "Tube Drive", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.20f));

    p.push_back(std::make_unique<juce::AudioParameterFloat>("mic1Gain", "1: Main", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.00f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("mic2Gain", "2: Room/Air", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.25f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("mic3Gain", "3: Far L", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.00f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("mic4Gain", "4: Far R", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.00f));

    p.push_back(std::make_unique<juce::AudioParameterBool>("mic1Solo", "Mic 1 Solo", false));
    p.push_back(std::make_unique<juce::AudioParameterBool>("mic2Solo", "Mic 2 Solo", false));
    p.push_back(std::make_unique<juce::AudioParameterBool>("mic3Solo", "Mic 3 Solo", false));
    p.push_back(std::make_unique<juce::AudioParameterBool>("mic4Solo", "Mic 4 Solo", false));

    p.push_back(std::make_unique<juce::AudioParameterFloat>("mic1X", "Mic 1 X", 0.05f, 0.95f, 0.50f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("mic1Y", "Mic 1 Y", 0.05f, 0.95f, 0.14f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("mic2X", "Mic 2 X", 0.05f, 0.95f, 0.50f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("mic2Y", "Mic 2 Y", 0.05f, 0.95f, 0.42f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("mic3X", "Mic 3 X", 0.05f, 0.95f, 0.14f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("mic3Y", "Mic 3 Y", 0.05f, 0.95f, 0.86f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("mic4X", "Mic 4 X", 0.05f, 0.95f, 0.86f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("mic4Y", "Mic 4 Y", 0.05f, 0.95f, 0.86f));

    p.push_back(std::make_unique<juce::AudioParameterFloat>("pairWide", "Pair Wide", 0.0f, 1.0f, 1.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("farWide", "Far Wide", 0.0f, 1.0f, 1.0f));

    p.push_back(std::make_unique<juce::AudioParameterBool>("eqOn", "EQ On", true));
    p.push_back(std::make_unique<juce::AudioParameterChoice>("eqModel", "EQ Model", juce::StringArray{ "Neve 1073 (British)", "API 550 (American)", "Pultec (Tube)" }, 0));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("hpf", "HPF", juce::NormalisableRange<float>(20.0f, 300.0f, 1.0f, 0.45f), 20.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("lowGain", "Low Gain", juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterChoice>("lowFreq", "Low Freq", juce::StringArray{ "60 Hz", "110 Hz", "220 Hz" }, 1));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("midGain", "Mid Gain", juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterChoice>("midFreq", "Mid Freq", juce::StringArray{ "700 Hz", "1.6 kHz", "3.2 kHz", "4.8 kHz" }, 1));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("highGain", "High Gain", juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterChoice>("highFreq", "High Freq", juce::StringArray{ "8 kHz", "12 kHz", "16 kHz" }, 1));

    p.push_back(std::make_unique<juce::AudioParameterBool>("compOn", "Comp On", true));
    p.push_back(std::make_unique<juce::AudioParameterChoice>("compModel", "Comp Model", juce::StringArray{ "1176LN (FET)", "LA-2A (Opto)", "SSL (VCA)", "Fairchild (Tube)" }, 0));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("compThresh", "Threshold", juce::NormalisableRange<float>(-40.0f, 0.0f, 0.1f), -16.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("compMix", "Comp Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f));
    p.push_back(std::make_unique<juce::AudioParameterBool>("scLink", "SC Link", true));
    p.push_back(std::make_unique<juce::AudioParameterBool>("scFilter", "SC Filter", false));
    p.push_back(std::make_unique<juce::AudioParameterBool>("parCrush", "Par Crush", false));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("parBlend", "Par Blend", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.35f));
    p.push_back(std::make_unique<juce::AudioParameterChoice>("parRoute", "Par Route", juce::StringArray{ "Room Mics", "Full Mix" }, 0));

    p.push_back(std::make_unique<juce::AudioParameterBool>("chamberOn", "Chamber On", true));
    p.push_back(std::make_unique<juce::AudioParameterChoice>("chamberModel", "Echo Chamber", juce::StringArray{ "Chamber 1 (Tight)", "Chamber 2 (Warm)", "Chamber 3 (Classic)" }, 2));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("chamberSend", "Send", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.18f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("chamberDecay", "Decay", juce::NormalisableRange<float>(0.10f, 1.0f, 0.01f), 0.72f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("chamberPreDelay", "Pre-Delay", juce::NormalisableRange<float>(0.0f, 100.0f, 0.5f), 22.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("chamberWidth", "Width", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f));

    p.push_back(std::make_unique<juce::AudioParameterFloat>("outputGain", "Output", juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f), 0.0f));

    return { p.begin(), p.end() };
}

void BenBocazStudioProAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    const int safeBlockSize = std::max(samplesPerBlock, 4096);

    vocalModeler.prepare(sampleRate, safeBlockSize);
    sourceModeler.prepare(sampleRate);
    consoleChain.prepare(sampleRate, safeBlockSize);
    acousticEngine.prepare(sampleRate, safeBlockSize);
    parallelComp.prepare(sampleRate, safeBlockSize);

    roomOnlyBuffer.setSize(2, safeBlockSize, false, true, false);
    parCrushBuffer.setSize(2, safeBlockSize, false, true, false);

    outGainSmooth.reset(sampleRate, 0.02);
    outGainSmooth.setCurrentAndTargetValue(1.0f);

    const bool hqOn = apvts.getRawParameterValue("hqMode")->load() > 0.5f;
    lastReportedLatency = consoleChain.getLatencySamples(hqOn);
    setLatencySamples(lastReportedLatency);
}

void BenBocazStudioProAudioProcessor::releaseResources()
{
    vocalModeler.reset();
    sourceModeler.reset();
    consoleChain.reset();
    acousticEngine.reset();
    parallelComp.reset();
}

bool BenBocazStudioProAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void BenBocazStudioProAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    if (numSamples == 0) return;

    auto getVal = [this](const char* id) { return apvts.getRawParameterValue(id)->load(); };

    const bool hq4x = getVal("hqMode") > 0.5f;
    const int targetLatency = consoleChain.getLatencySamples(hq4x);
    if (targetLatency != lastReportedLatency)
    {
        lastReportedLatency = targetLatency;
        setLatencySamples(targetLatency);
    }

    const bool vocalOn = getVal("vocalModOn") > 0.5f;
    vocalModeler.setParameters(vocalOn,
                               static_cast<int>(getVal("vocalInMic")),
                               static_cast<int>(getVal("vocalTargetMic")),
                               getVal("vocalInGain"),
                               getVal("vocalOutGain"));
    vocalModeler.processEQ(buffer);

    const bool isReMic = (static_cast<int>(getVal("mode")) == 0);
    const bool roomActive = isReMic && (getVal("roomOn") > 0.5f);

    sourceModeler.setParameters(static_cast<int>(getVal("source")),
                                static_cast<int>(getVal("monitorSpeaker")));
    sourceModeler.process(buffer, roomActive);

    std::array<BenBocazDSP::MicState, 4> mics;
    mics[0] = { getVal("mic1X"), getVal("mic1Y"), getVal("mic1Gain"), getVal("mic1Solo") > 0.5f };
    mics[1] = { getVal("mic2X"), getVal("mic2Y"), getVal("mic2Gain"), getVal("mic2Solo") > 0.5f };
    mics[2] = { getVal("mic3X"), getVal("mic3Y"), getVal("mic3Gain"), getVal("mic3Solo") > 0.5f };
    mics[3] = { getVal("mic4X"), getVal("mic4Y"), getVal("mic4Gain"), getVal("mic4Solo") > 0.5f };

    acousticEngine.processRoom(buffer, roomOnlyBuffer,
                               roomActive,
                               static_cast<int>(getVal("room")),
                               static_cast<int>(getVal("walls")),
                               getVal("mainStereo") > 0.5f,
                               getVal("phaseSafe") > 0.5f,
                               getVal("pairWide"),
                               getVal("farWide"),
                               mics);

    if (getVal("parCrush") > 0.5f && getVal("parBlend") > 0.001f && numSamples <= parCrushBuffer.getNumSamples())
    {
        const bool routeRoom = (static_cast<int>(getVal("parRoute")) == 0);
        auto& srcBuf = routeRoom ? roomOnlyBuffer : buffer;

        parCrushBuffer.copyFrom(0, 0, srcBuf, 0, 0, numSamples);
        if (srcBuf.getNumChannels() > 1)
            parCrushBuffer.copyFrom(1, 0, srcBuf, 1, 0, numSamples);
        else
            parCrushBuffer.copyFrom(1, 0, srcBuf, 0, 0, numSamples);

        juce::AudioBuffer<float> parView(parCrushBuffer.getArrayOfWritePointers(), 2, numSamples);
        parallelComp.setParameters(BenBocazDSP::VintageCompressor::FET_1176, -28.0f, 1.0f, true, true, true);
        parallelComp.process(parView, true);

        const float blend = getVal("parBlend") * 0.55f;
        buffer.addFrom(0, 0, parCrushBuffer, 0, 0, numSamples, blend);
        if (buffer.getNumChannels() > 1)
            buffer.addFrom(1, 0, parCrushBuffer, 1, 0, numSamples, blend);
    }

    static constexpr float loHzTbl[3]  = { 60.0f, 110.0f, 220.0f };
    static constexpr float midHzTbl[4] = { 700.0f, 1600.0f, 3200.0f, 4800.0f };
    static constexpr float hiHzTbl[3]  = { 8000.0f, 12000.0f, 16000.0f };

    const int routing = static_cast<int>(getVal("routing"));
    auto runChamber = [&]()
    {
        acousticEngine.processChamber(buffer,
                                      getVal("chamberOn") > 0.5f,
                                      static_cast<int>(getVal("chamberModel")),
                                      getVal("chamberSend"),
                                      getVal("chamberDecay"),
                                      getVal("chamberPreDelay"),
                                      getVal("chamberWidth"));
    };

    if (routing == 1) runChamber();

    consoleChain.process(buffer, hq4x,
                         getVal("eqOn") > 0.5f,
                         static_cast<int>(getVal("eqModel")),
                         getVal("hpf"),
                         getVal("lowGain"),  loHzTbl[std::clamp(static_cast<int>(getVal("lowFreq")), 0, 2)],
                         getVal("midGain"),  midHzTbl[std::clamp(static_cast<int>(getVal("midFreq")), 0, 3)],
                         getVal("highGain"), hiHzTbl[std::clamp(static_cast<int>(getVal("highFreq")), 0, 2)],
                         getVal("compOn") > 0.5f,
                         static_cast<int>(getVal("compModel")),
                         getVal("compThresh"),
                         getVal("compMix"),
                         getVal("scLink") > 0.5f,
                         getVal("scFilter") > 0.5f,
                         static_cast<int>(getVal("circuit")),
                         getVal("drive"),
                         vocalModeler.isActive(),
                         vocalModeler.getSaturationDrive());

    if (routing == 0) runChamber();

    const float pol = (getVal("polarityInv") > 0.5f) ? -1.0f : 1.0f;
    outGainSmooth.setTargetValue(juce::Decibels::decibelsToGain(getVal("outputGain")));

    float peak = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        const float g = outGainSmooth.getNextValue() * pol;
        for (int c = 0; c < buffer.getNumChannels(); ++c)
        {
            const float s = buffer.getSample(c, i) * g;
            buffer.setSample(c, i, s);
            peak = std::max(peak, std::abs(s));
        }
    }

    vuOutDb.store(juce::Decibels::gainToDecibels(peak + 1e-6f, -60.0f), std::memory_order_relaxed);
    vuGrDb.store(consoleChain.getGainReductionDb(), std::memory_order_relaxed);
}

const juce::String BenBocazStudioProAudioProcessor::getProgramName(int index)
{
    static const juce::StringArray names {
        "01. Volcano Clean Default",
        "02. Volcano Epic Drums & Gates Open",
        "03. Volcano Ensemble Wide",
        "04. Shangri-La Vocal Warm & Air",
        "05. Canyon Stereo Guitar Cab",
        "06. Canyon Tight Bass / Monitors"
    };
    return names[std::clamp(index, 0, names.size() - 1)];
}

void BenBocazStudioProAudioProcessor::setCurrentProgram(int index)
{
    applyFactoryPreset(index);
}

void BenBocazStudioProAudioProcessor::applyFactoryPreset(int presetIndex)
{
    currentPresetIndex = std::clamp(presetIndex, 0, 5);
    auto setP = [this](const char* id, float val)
    {
        if (auto* param = apvts.getParameter(id))
            param->setValueNotifyingHost(param->convertTo0to1(val));
    };

    setP("mode", 0.0f); setP("roomOn", 1.0f); setP("phaseSafe", 1.0f); setP("link34", 1.0f);
    setP("vocalModOn", 0.0f); setP("parCrush", 0.0f); setP("lowGain", 0.0f); setP("midGain", 0.0f); setP("highGain", 0.0f);
    setP("mic1Solo", 0.0f); setP("mic2Solo", 0.0f); setP("mic3Solo", 0.0f); setP("mic4Solo", 0.0f);
    setP("outputGain", 0.0f);

    switch (currentPresetIndex)
    {
        case 0:
            setP("room", 0.0f); setP("walls", 1.0f); setP("source", 0.0f); setP("mainStereo", 0.0f);
            setP("mic1Gain", 1.00f); setP("mic2Gain", 0.25f); setP("mic3Gain", 0.00f); setP("mic4Gain", 0.00f);
            setP("mic1X", 0.50f); setP("mic1Y", 0.14f); setP("mic2X", 0.50f); setP("mic2Y", 0.42f);
            setP("mic3X", 0.14f); setP("mic3Y", 0.86f); setP("mic4X", 0.86f); setP("mic4Y", 0.86f);
            setP("chamberSend", 0.18f); setP("compThresh", -16.0f);
            break;
        case 1:
            setP("room", 0.0f); setP("walls", 0.0f); setP("source", 1.0f); setP("mainStereo", 1.0f);
            setP("mic1Gain", 1.00f); setP("mic2Gain", 0.42f); setP("mic3Gain", 0.55f); setP("mic4Gain", 0.55f);
            setP("mic1X", 0.50f); setP("mic1Y", 0.18f); setP("mic2X", 0.50f); setP("mic2Y", 0.48f);
            setP("mic3X", 0.10f); setP("mic3Y", 0.90f); setP("mic4X", 0.90f); setP("mic4Y", 0.90f);
            setP("parCrush", 1.0f); setP("parBlend", 0.40f); setP("parRoute", 0.0f);
            setP("chamberSend", 0.12f); setP("compThresh", -18.0f);
            break;
        case 2:
            setP("room", 0.0f); setP("walls", 0.0f); setP("source", 5.0f); setP("mainStereo", 1.0f);
            setP("mic1Gain", 0.90f); setP("mic2Gain", 0.48f); setP("mic3Gain", 0.68f); setP("mic4Gain", 0.68f);
            setP("mic1X", 0.50f); setP("mic1Y", 0.24f); setP("mic2X", 0.50f); setP("mic2Y", 0.55f);
            setP("mic3X", 0.08f); setP("mic3Y", 0.92f); setP("mic4X", 0.92f); setP("mic4Y", 0.92f);
            setP("chamberSend", 0.28f); setP("chamberDecay", 0.82f); setP("compThresh", -14.0f);
            break;
        case 3:
            setP("room", 1.0f); setP("walls", 0.0f); setP("source", 0.0f); setP("mainStereo", 0.0f);
            setP("vocalModOn", 1.0f); setP("vocalInMic", 0.0f); setP("vocalTargetMic", 0.0f);
            setP("mic1Gain", 1.00f); setP("mic2Gain", 0.30f); setP("mic3Gain", 0.18f); setP("mic4Gain", 0.18f);
            setP("mic1X", 0.50f); setP("mic1Y", 0.12f); setP("mic2X", 0.50f); setP("mic2Y", 0.40f);
            setP("mic3X", 0.18f); setP("mic3Y", 0.78f); setP("mic4X", 0.82f); setP("mic4Y", 0.78f);
            setP("compModel", 1.0f); setP("compThresh", -18.0f); setP("chamberSend", 0.24f);
            break;
        case 4:
            setP("room", 2.0f); setP("walls", 0.0f); setP("source", 4.0f); setP("mainStereo", 1.0f);
            setP("mic1Gain", 1.00f); setP("mic2Gain", 0.22f); setP("mic3Gain", 0.38f); setP("mic4Gain", 0.38f);
            setP("mic1X", 0.50f); setP("mic1Y", 0.14f); setP("mic2X", 0.50f); setP("mic2Y", 0.36f);
            setP("mic3X", 0.16f); setP("mic3Y", 0.72f); setP("mic4X", 0.84f); setP("mic4Y", 0.72f);
            setP("drive", 0.35f); setP("chamberSend", 0.15f);
            break;
        case 5:
        default:
            setP("room", 2.0f); setP("walls", 1.0f); setP("source", 2.0f); setP("mainStereo", 0.0f);
            setP("mic1Gain", 1.00f); setP("mic2Gain", 0.12f); setP("mic3Gain", 0.00f); setP("mic4Gain", 0.00f);
            setP("mic1X", 0.50f); setP("mic1Y", 0.10f); setP("mic2X", 0.50f); setP("mic2Y", 0.32f);
            setP("compModel", 0.0f); setP("compThresh", -20.0f); setP("chamberSend", 0.05f);
            break;
    }
}

void BenBocazStudioProAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary(*xml, destData);
}

void BenBocazStudioProAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* BenBocazStudioProAudioProcessor::createEditor()
{
    return new BenBocazStudioProAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BenBocazStudioProAudioProcessor();
}
