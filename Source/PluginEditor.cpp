#pragma once
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <algorithm>
#include <array>

namespace BenBocazDSP
{

class VocalMicModeler
{
public:
    VocalMicModeler()
        : oversampler(2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true)
    {}

    void prepare(double sampleRate, int samplesPerBlock = 4096)
    {
        fs = std::max(8000.0, sampleRate);
        maxBlock = std::max(samplesPerBlock, 4096);

        oversampler.initProcessing(static_cast<size_t>(maxBlock));
        dryWorkBuffer.setSize(2, maxBlock, false, true, false);

        inGainSmooth.reset(fs, 0.025);
        outGainSmooth.reset(fs, 0.025);
        mixSmooth.reset(fs, 0.030);

        inGainSmooth.setCurrentAndTargetValue(1.0f);
        outGainSmooth.setCurrentAndTargetValue(1.0f);
        mixSmooth.setCurrentAndTargetValue(0.0f);

        reset();
        dirty = true;
    }

    void reset()
    {
        oversampler.reset();
        for (int c = 0; c < 2; ++c)
        {
            lowShelf[c].reset();
            bodyPeak[c].reset();
            midPeak[c].reset();
            airShelf[c].reset();
            deHarshLp[c].reset();
        }
    }

    void setParameters(bool enabled, int inputMicIndex, int targetMicIndex,
                       float inGainDb, float outGainDb)
    {
        const int clampedIn  = std::clamp(inputMicIndex, 0, 3);
        const int clampedOut = std::clamp(targetMicIndex, 0, 3);

        if (inMic != clampedIn || targetMic != clampedOut)
        {
            inMic = clampedIn;
            targetMic = clampedOut;
            dirty = true;
        }

        isEnabled = enabled;
        inGainSmooth.setTargetValue(juce::Decibels::decibelsToGain(inGainDb));
        outGainSmooth.setTargetValue(juce::Decibels::decibelsToGain(outGainDb));
        mixSmooth.setTargetValue(enabled ? 1.0f : 0.0f);

        const float gainDrive = std::clamp((inGainDb + 6.0f) / 20.0f, 0.0f, 0.65f);
        currentDrive = enabled ? std::clamp(targetSatAmount * 0.65f + gainDrive * 0.35f, 0.04f, 0.75f) : 0.0f;
    }

    void processEQ(juce::AudioBuffer<float>& buffer)
    {
        const int numSamples = buffer.getNumSamples();
        if (numSamples == 0 || numSamples > dryWorkBuffer.getNumSamples())
            return;

        if (!isEnabled && !mixSmooth.isSmoothing() && mixSmooth.getCurrentValue() < 0.0001f)
            return;

        if (dirty)
            updateMicCurves();

        const int numCh = std::min(2, buffer.getNumChannels());

        dryWorkBuffer.copyFrom(0, 0, buffer, 0, 0, numSamples);
        if (numCh > 1)
            dryWorkBuffer.copyFrom(1, 0, buffer, 1, 0, numSamples);
        else
            dryWorkBuffer.copyFrom(1, 0, buffer, 0, 0, numSamples);

        for (int i = 0; i < numSamples; ++i)
        {
            const float gIn = inGainSmooth.getNextValue();
            for (int c = 0; c < numCh; ++c)
            {
                float s = buffer.getSample(c, i) * gIn;
                s = lowShelf[c].processSample(s);
                s = bodyPeak[c].processSample(s);
                s = midPeak[c].processSample(s);
                s = airShelf[c].processSample(s);
                if (useDeHarsh)
                    s = deHarshLp[c].processSample(s);
                buffer.setSample(c, i, s);
            }
        }

        if (targetSatAmount > 0.001f)
        {
            juce::dsp::AudioBlock<float> baseBlock(buffer);
            auto osBlock = oversampler.processSamplesUp(baseBlock);

            const int osCh = static_cast<int>(osBlock.getNumChannels());
            const int osSamples = static_cast<int>(osBlock.getNumSamples());
            const float drive = 1.0f + targetSatAmount * 1.65f;
            const float asym = targetAsym;
            const float norm = 1.0f / std::tanh(drive + std::abs(asym));

            for (int c = 0; c < osCh; ++c)
            {
                auto* ptr = osBlock.getChannelPointer(static_cast<size_t>(c));
                for (int i = 0; i < osSamples; ++i)
                {
                    const float x = ptr[i] * drive + asym;
                    ptr[i] = (std::tanh(x) - std::tanh(asym)) * norm;
                }
            }

            oversampler.processSamplesDown(baseBlock);
        }

        for (int i = 0; i < numSamples; ++i)
        {
            const float gOut = outGainSmooth.getNextValue();
            const float mix  = mixSmooth.getNextValue();

            for (int c = 0; c < numCh; ++c)
            {
                const float dry = dryWorkBuffer.getSample(c, i);
                const float wet = buffer.getSample(c, i) * gOut;
                buffer.setSample(c, i, dry + mix * (wet - dry));
            }
        }
    }

    float getSaturationDrive() const noexcept { return currentDrive; }
    bool isActive() const noexcept { return isEnabled; }

private:
    struct MicProfile
    {
        float lowDb;
        float lowHz;
        float bodyDb;
        float bodyHz;
        float midDb;
        float midHz;
        float midQ;
        float airDb;
        float airHz;
        float sat;
        float asym;
        float topLimitHz;
    };

    void updateMicCurves()
    {
        static constexpr std::array<MicProfile, 4> inputProfiles {{
            {  1.8f, 130.0f,  1.4f, 260.0f,  0.5f, 3000.0f, 0.85f, -2.2f, 10500.0f, 0.22f,  0.03f, 15500.0f },
            {  0.2f, 120.0f,  0.3f, 240.0f,  1.2f, 2700.0f, 0.85f,  0.8f, 11000.0f, 0.14f,  0.02f, 18000.0f },
            {  0.0f, 110.0f,  0.0f, 240.0f,  0.4f, 3200.0f, 0.80f,  1.0f, 11500.0f, 0.08f,  0.01f, 18500.0f },
            { -2.4f, 150.0f, -0.8f, 280.0f,  2.0f, 3600.0f, 1.05f, -3.0f, 10000.0f, 0.24f,  0.03f, 14500.0f }
        }};

        static constexpr std::array<MicProfile, 4> targetProfiles {{
            {  0.6f, 115.0f,  0.4f, 230.0f,  1.6f, 4200.0f, 0.90f,  3.2f, 12000.0f, 0.20f,  0.02f, 17500.0f },
            {  2.6f, 125.0f,  1.8f, 250.0f,  1.3f, 1900.0f, 0.80f, -0.6f,  9500.0f, 0.34f,  0.05f, 16000.0f },
            {  1.2f, 120.0f,  0.8f, 240.0f,  1.1f, 3400.0f, 0.78f,  2.6f, 12500.0f, 0.28f,  0.04f, 18000.0f },
            {  0.8f, 120.0f,  0.7f, 240.0f,  1.8f, 2800.0f, 0.88f,  1.0f, 10500.0f, 0.24f,  0.03f, 17000.0f }
        }};

        const auto& inP  = inputProfiles[static_cast<size_t>(inMic)];
        const auto& outP = targetProfiles[static_cast<size_t>(targetMic)];

        const float loDb   = std::clamp(outP.lowDb  - inP.lowDb * 0.65f,  -8.0f, 8.0f);
        const float bodyDb = std::clamp(outP.bodyDb - inP.bodyDb * 0.65f, -6.0f, 6.0f);
        const float midDb  = std::clamp(outP.midDb  - inP.midDb * 0.65f,  -6.0f, 6.0f);
        const float airDb  = std::clamp(outP.airDb  - inP.airDb * 0.65f,  -6.0f, 7.5f);

        targetSatAmount = std::clamp(outP.sat - inP.sat * 0.45f, 0.0f, 0.65f);
        targetAsym      = std::clamp(outP.asym - inP.asym * 0.40f, -0.08f, 0.08f);

        const float topHz = std::min(inP.topLimitHz, outP.topLimitHz);
        useDeHarsh = (topHz < 18200.0f);

        auto loC   = juce::dsp::IIR::Coefficients<float>::makeLowShelf(fs, outP.lowHz, 0.707f,
                                                                      juce::Decibels::decibelsToGain(loDb));
        auto bodyC = juce::dsp::IIR::Coefficients<float>::makePeakFilter(fs, outP.bodyHz, 0.85f,
                                                                      juce::Decibels::decibelsToGain(bodyDb));
        auto midC  = juce::dsp::IIR::Coefficients<float>::makePeakFilter(fs, outP.midHz, outP.midQ,
                                                                      juce::Decibels::decibelsToGain(midDb));
        auto airC  = juce::dsp::IIR::Coefficients<float>::makeHighShelf(fs, outP.airHz, 0.707f,
                                                                       juce::Decibels::decibelsToGain(airDb));
        auto lpC   = juce::dsp::IIR::Coefficients<float>::makeLowPass(fs, std::clamp(topHz, 9000.0f, 20000.0f), 0.707f);

        for (int c = 0; c < 2; ++c)
        {
            lowShelf[c].coefficients  = loC;
            bodyPeak[c].coefficients  = bodyC;
            midPeak[c].coefficients   = midC;
            airShelf[c].coefficients  = airC;
            deHarshLp[c].coefficients = lpC;
        }

        dirty = false;
    }

    double fs = 48000.0;
    int maxBlock = 4096;
    bool isEnabled = false;
    int inMic = 0;
    int targetMic = 0;
    bool dirty = true;
    bool useDeHarsh = false;

    float targetSatAmount = 0.0f;
    float targetAsym = 0.0f;
    float currentDrive = 0.0f;

    juce::dsp::Oversampling<float> oversampler;
    juce::AudioBuffer<float> dryWorkBuffer;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> inGainSmooth, outGainSmooth;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmooth;

    juce::dsp::IIR::Filter<float> lowShelf[2], bodyPeak[2], midPeak[2], airShelf[2], deHarshLp[2];
};

} // namespace BenBocazDSP
