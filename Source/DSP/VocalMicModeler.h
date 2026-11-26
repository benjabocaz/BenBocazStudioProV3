#pragma once
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <algorithm>

namespace BenBocazDSP
{

class VocalMicModeler
{
public:
    void prepare(double sampleRate)
    {
        fs = std::max(8000.0, sampleRate);
        inGainSmooth.reset(fs, 0.02);
        outGainSmooth.reset(fs, 0.02);
        mixSmooth.reset(fs, 0.02);

        inGainSmooth.setCurrentAndTargetValue(1.0f);
        outGainSmooth.setCurrentAndTargetValue(1.0f);
        mixSmooth.setCurrentAndTargetValue(0.0f);

        reset();
        dirty = true;
    }

    void reset()
    {
        for (int c = 0; c < 2; ++c)
        {
            lowShelf[c].reset();
            midPeak[c].reset();
            airShelf[c].reset();
        }
    }

    void setParameters(bool enabled, int inputMicIndex, int targetMicIndex,
                       float inGainDb, float outGainDb)
    {
        if (inMic != inputMicIndex || targetMic != targetMicIndex)
        {
            inMic = inputMicIndex;
            targetMic = targetMicIndex;
            dirty = true;
        }

        isEnabled = enabled;
        inGainSmooth.setTargetValue(juce::Decibels::decibelsToGain(inGainDb));
        outGainSmooth.setTargetValue(juce::Decibels::decibelsToGain(outGainDb));
        mixSmooth.setTargetValue(enabled ? 1.0f : 0.0f);
        currentDrive = enabled ? std::clamp((inGainDb + 6.0f) / 18.0f, 0.05f, 0.85f) : 0.0f;
    }

    void processEQ(juce::AudioBuffer<float>& buffer)
    {
        if (buffer.getNumSamples() == 0)
            return;

        if (!isEnabled && !mixSmooth.isSmoothing() && mixSmooth.getCurrentValue() < 0.0001f)
            return;

        if (dirty)
            updateMicCurves();

        const int numCh = std::min(2, buffer.getNumChannels());
        const int numSamples = buffer.getNumSamples();

        for (int i = 0; i < numSamples; ++i)
        {
            const float gIn  = inGainSmooth.getNextValue();
            const float gOut = outGainSmooth.getNextValue();
            const float mix  = mixSmooth.getNextValue();

            for (int c = 0; c < numCh; ++c)
            {
                const float dry = buffer.getSample(c, i);
                float wet = dry * gIn;
                wet = lowShelf[c].processSample(wet);
                wet = midPeak[c].processSample(wet);
                wet = airShelf[c].processSample(wet);
                wet *= gOut;
                buffer.setSample(c, i, dry + mix * (wet - dry));
            }
        }
    }

    float getSaturationDrive() const noexcept { return currentDrive; }
    bool isActive() const noexcept { return isEnabled; }

private:
    void updateMicCurves()
    {
        float loDb = 0.0f, midDb = 0.0f, airDb = 0.0f;
        float midHz = 3200.0f, airHz = 10000.0f;

        // Entrada: 0=SM7B, 1=U87, 2=Condenser, 3=SM58
        if (inMic == 0)      { loDb -= 1.2f; midDb += 1.5f; airDb += 2.5f; }
        else if (inMic == 3) { loDb -= 2.0f; midDb -= 1.0f; airDb += 3.2f; }

        // Destino: 0=Sony C-800G, 1=Neumann U47, 2=Telefunken 251, 3=U87 Vintage
        switch (targetMic)
        {
            case 0: loDb += 0.8f; midDb += 1.2f; airDb += 3.5f; midHz = 4200.0f; airHz = 11500.0f; break;
            case 1: loDb += 2.2f; midDb += 1.5f; airDb -= 0.5f; midHz = 1800.0f; airHz = 9000.0f;  break;
            case 2: loDb += 1.0f; midDb += 0.6f; airDb += 2.8f; midHz = 3500.0f; airHz = 12000.0f; break;
            default: loDb += 0.5f; midDb += 1.8f; airDb += 0.8f; midHz = 2800.0f; airHz = 10000.0f; break;
        }

        auto loC  = juce::dsp::IIR::Coefficients<float>::makeLowShelf(fs, 140.0f, 0.707f, juce::Decibels::decibelsToGain(loDb));
        auto midC = juce::dsp::IIR::Coefficients<float>::makePeakFilter(fs, midHz, 0.9f, juce::Decibels::decibelsToGain(midDb));
        auto airC = juce::dsp::IIR::Coefficients<float>::makeHighShelf(fs, airHz, 0.707f, juce::Decibels::decibelsToGain(airDb));

        for (int c = 0; c < 2; ++c)
        {
            lowShelf[c].coefficients = loC;
            midPeak[c].coefficients  = midC;
            airShelf[c].coefficients = airC;
        }
        dirty = false;
    }

    double fs = 48000.0;
    bool isEnabled = false;
    int inMic = 0;
    int targetMic = 0;
    float currentDrive = 0.0f;
    bool dirty = true;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> inGainSmooth, outGainSmooth;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmooth;

    juce::dsp::IIR::Filter<float> lowShelf[2], midPeak[2], airShelf[2];
};

} // namespace BenBocazDSP
