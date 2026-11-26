#pragma once
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <algorithm>

namespace BenBocazDSP
{

//==============================================================================
// Utilidad de saturacion suave sin NaN
inline float softClipTanh(float x) noexcept
{
    return std::tanh(std::clamp(x, -10.0f, 10.0f));
}

//==============================================================================
// 1. VINTAGE COMPRESSOR CON AUTO-GAIN SUAVIZADO
class VintageCompressor
{
public:
    enum Model { FET_1176 = 0, OPTO_LA2A, VCA_SSL, TUBE_FAIRCHILD };

    void prepare(double sampleRate, int /*maxBlockSize*/)
    {
        fs = std::max(8000.0, sampleRate);
        reset();
        updateCoefficients(true);
    }

    void reset()
    {
        env = 0.0f;
        autoGainEnvDb = 0.0f;
        scHpL.reset();
        scHpR.reset();
        lastGrDb = 0.0f;
    }

    void setParameters(int modelIndex, float thresholdDb, float mix01,
                       bool scLink, bool scFilterOn, bool autoGainOn = true)
    {
        const bool changed = (model != modelIndex) ||
                             (std::abs(threshDb - thresholdDb) > 1e-4f) ||
                             (scFilter != scFilterOn);

        model = std::clamp(modelIndex, 0, 3);
        threshDb = thresholdDb;
        mix = std::clamp(mix01, 0.0f, 1.0f);
        link = scLink;
        scFilter = scFilterOn;
        autoGain = autoGainOn;

        if (changed)
            updateCoefficients(false);
    }

    void process(juce::AudioBuffer<float>& buffer, bool enabled)
    {
        if (!enabled || buffer.getNumSamples() == 0)
        {
            lastGrDb = 0.0f;
            return;
        }

        const int numCh = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();
        auto* chL = buffer.getWritePointer(0);
        auto* chR = (numCh > 1) ? buffer.getWritePointer(1) : nullptr;

        float peakGr = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            const float dryL = chL[i];
            const float dryR = chR ? chR[i] : dryL;

            // Sidechain HPF (90 Hz) si esta activo
            float scL = scFilter ? scHpL.processSample(dryL) : dryL;
            float scR = scFilter ? scHpR.processSample(dryR) : dryR;

            float det = link ? std::max(std::abs(scL), std::abs(scR))
                             : 0.5f * (std::abs(scL) + std::abs(scR));

            const float detDb = juce::Decibels::gainToDecibels(det + 1e-6f, -96.0f);

            // Curva de compresion con soft knee
            float grDb = 0.0f;
            const float overDb = detDb - threshDb;
            if (overDb > -kneeHalf)
            {
                if (overDb >= kneeHalf)
                    grDb = overDb * (1.0f - 1.0f / ratio);
                else
                {
                    const float x = overDb + kneeHalf;
                    grDb = (x * x) / (4.0f * kneeHalf) * (1.0f - 1.0f / ratio);
                }
            }

            // Envolvente ataque / release
            const float coeff = (grDb > env) ? attackCoeff : releaseCoeff;
            env = grDb + coeff * (env - grDb);

            // Auto-Gain interno suavizado (compensa ~70% de la reduccion media sin bombear)
            const float targetAutoDb = autoGain ? std::clamp(env * autoGainFactor, 0.0f, 12.0f) : baseMakeupDb;
            autoGainEnvDb = targetAutoDb + autoGainSmoothCoeff * (autoGainEnvDb - targetAutoDb);

            const float totalGain = juce::Decibels::decibelsToGain(-env + autoGainEnvDb);

            float wetL = dryL * totalGain;
            float wetR = dryR * totalGain;

            // Coloracion armonica sutil segun modelo
            if (model == FET_1176 || model == TUBE_FAIRCHILD)
            {
                wetL = softClipTanh(wetL * colorDrive) / colorDrive;
                wetR = softClipTanh(wetR * colorDrive) / colorDrive;
            }

            chL[i] = dryL + mix * (wetL - dryL);
            if (chR)
                chR[i] = dryR + mix * (wetR - dryR);

            if (env > peakGr)
                peakGr = env;
        }

        lastGrDb = peakGr;
    }

    float getGainReductionDb() const noexcept { return lastGrDb; }

private:
    void updateCoefficients(bool force)
    {
        float atkMs = 5.0f;
        float relMs = 80.0f;

        switch (model)
        {
            case FET_1176:
                ratio = 4.0f; kneeHalf = 3.0f; atkMs = 0.8f; relMs = 60.0f;
                baseMakeupDb = 2.0f; autoGainFactor = 0.72f; colorDrive = 1.18f;
                break;
            case OPTO_LA2A:
                ratio = 3.0f; kneeHalf = 6.0f; atkMs = 10.0f; relMs = 220.0f;
                baseMakeupDb = 1.5f; autoGainFactor = 0.68f; colorDrive = 1.05f;
                break;
            case VCA_SSL:
                ratio = 4.0f; kneeHalf = 2.5f; atkMs = 3.0f; relMs = 100.0f;
                baseMakeupDb = 1.0f; autoGainFactor = 0.70f; colorDrive = 1.02f;
                break;
            case TUBE_FAIRCHILD:
            default:
                ratio = 2.5f; kneeHalf = 8.0f; atkMs = 6.0f; relMs = 180.0f;
                baseMakeupDb = 2.5f; autoGainFactor = 0.74f; colorDrive = 1.25f;
                break;
        }

        attackCoeff  = std::exp(-1.0f / (static_cast<float>(fs) * (atkMs * 0.001f)));
        releaseCoeff = std::exp(-1.0f / (static_cast<float>(fs) * (relMs * 0.001f)));
        // Suavizado lento (~350ms) para que el auto-gain no bombee con transientes
        autoGainSmoothCoeff = std::exp(-1.0f / (static_cast<float>(fs) * 0.35f));

        if (force || scFilter)
        {
            auto hpCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(fs, 90.0f, 0.707f);
            scHpL.coefficients = hpCoeffs;
            scHpR.coefficients = hpCoeffs;
        }
    }

    double fs = 48000.0;
    int model = FET_1176;
    float threshDb = -16.0f;
    float mix = 1.0f;
    bool link = true;
    bool scFilter = false;
    bool autoGain = true;

    float ratio = 4.0f;
    float kneeHalf = 3.0f;
    float baseMakeupDb = 2.0f;
    float autoGainFactor = 0.72f;
    float colorDrive = 1.15f;

    float attackCoeff = 0.99f;
    float releaseCoeff = 0.999f;
    float autoGainSmoothCoeff = 0.9995f;

    float env = 0.0f;
    float autoGainEnvDb = 0.0f;
    float lastGrDb = 0.0f;

    juce::dsp::IIR::Filter<float> scHpL, scHpR;
};

//==============================================================================
// 2. ANALOG CONSOLE EQ CON CACHE DE FILTROS (DIRTY FLAGS)
class AnalogConsoleEQ
{
public:
    void prepare(double sampleRate)
    {
        fs = std::max(8000.0, sampleRate);
        reset();
        dirty = true;
    }

    void reset()
    {
        for (int c = 0; c < 2; ++c)
        {
            hpf[c].reset();
            low[c].reset();
            mid[c].reset();
            high[c].reset();
        }
    }

    void setParameters(int eqModel, float hpfHz,
                       float lowDb, float lowHz,
                       float midDb, float midHz,
                       float highDb, float highHz)
    {
        if (model != eqModel ||
            std::abs(hpfFreq - hpfHz) > 0.05f ||
            std::abs(lowGainDb - lowDb) > 1e-3f || std::abs(lowFreq - lowHz) > 0.1f ||
            std::abs(midGainDb - midDb) > 1e-3f || std::abs(midFreq - midHz) > 0.1f ||
            std::abs(highGainDb - highDb) > 1e-3f || std::abs(highFreq - highHz) > 0.1f)
        {
            model = eqModel;
            hpfFreq = std::clamp(hpfHz, 20.0f, 350.0f);
            lowGainDb = lowDb;   lowFreq = std::clamp(lowHz, 35.0f, 300.0f);
            midGainDb = midDb;   midFreq = std::clamp(midHz, 250.0f, 7000.0f);
            highGainDb = highDb; highFreq = std::clamp(highHz, 4000.0f, 18000.0f);
            dirty = true;
        }
    }

    void process(juce::AudioBuffer<float>& buffer, bool enabled)
    {
        if (!enabled || buffer.getNumSamples() == 0)
            return;

        if (dirty)
            updateFilters();

        const int numCh = std::min(2, buffer.getNumChannels());
        const int numSamples = buffer.getNumSamples();
        const bool useHpf = (hpfFreq > 21.0f);

        for (int c = 0; c < numCh; ++c)
        {
            auto* data = buffer.getWritePointer(c);
            for (int i = 0; i < numSamples; ++i)
            {
                float s = data[i];
                if (useHpf) s = hpf[c].processSample(s);
                s = low[c].processSample(s);
                s = mid[c].processSample(s);
                s = high[c].processSample(s);
                data[i] = s;
            }
        }
    }

private:
    void updateFilters()
    {
        const float qMid = (model == 0) ? 0.85f : (model == 1 ? 1.2f : 0.65f);
        const float qShelf = (model == 2) ? 0.6f : 0.707f;

        auto hpC  = juce::dsp::IIR::Coefficients<float>::makeHighPass(fs, hpfFreq, 0.707f);
        auto loC  = juce::dsp::IIR::Coefficients<float>::makeLowShelf(fs, lowFreq, qShelf, juce::Decibels::decibelsToGain(lowGainDb));
        auto midC = juce::dsp::IIR::Coefficients<float>::makePeakFilter(fs, midFreq, qMid, juce::Decibels::decibelsToGain(midGainDb));
        auto hiC  = juce::dsp::IIR::Coefficients<float>::makeHighShelf(fs, highFreq, qShelf, juce::Decibels::decibelsToGain(highGainDb));

        for (int c = 0; c < 2; ++c)
        {
            hpf[c].coefficients  = hpC;
            low[c].coefficients  = loC;
            mid[c].coefficients  = midC;
            high[c].coefficients = hiC;
        }
        dirty = false;
    }

    double fs = 48000.0;
    int model = 0;
    float hpfFreq = 20.0f;
    float lowGainDb = 0.0f, lowFreq = 110.0f;
    float midGainDb = 0.0f, midFreq = 1600.0f;
    float highGainDb = 0.0f, highFreq = 12000.0f;
    bool dirty = true;

    juce::dsp::IIR::Filter<float> hpf[2], low[2], mid[2], high[2];
};

//==============================================================================
// 3. TUBE / TAPE DRIVE CON SUAVIZADO ANTI-CLICKS
class TubeTapeDrive
{
public:
    void prepare(double sampleRate)
    {
        driveSmooth.reset(sampleRate, 0.025);
        driveSmooth.setCurrentAndTargetValue(0.0f);
    }

    void setParameters(int circuitModel, float drive01)
    {
        circuit = circuitModel;
        driveSmooth.setTargetValue(std::clamp(drive01, 0.0f, 1.0f));
    }

    void process(juce::AudioBuffer<float>& buffer)
    {
        if (buffer.getNumSamples() == 0)
            return;

        const int numCh = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();

        for (int i = 0; i < numSamples; ++i)
        {
            const float d = driveSmooth.getNextValue();
            if (d < 0.001f)
                continue;

            const float preGain = 1.0f + d * (circuit == 0 ? 2.2f : (circuit == 1 ? 3.0f : 1.8f));
            const float asym = (circuit == 0) ? 0.04f * d : 0.0f;
            const float compGain = 1.0f / (1.0f + 0.55f * d);

            for (int c = 0; c < numCh; ++c)
            {
                float x = buffer.getSample(c, i);
                float y = softClipTanh(x * preGain + asym) - softClipTanh(asym);
                buffer.setSample(c, i, y * compGain);
            }
        }
    }

private:
    int circuit = 0;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> driveSmooth;
};

//==============================================================================
// 4. OVERSAMPLED CONSOLE CHAIN (MODO TRACK 0ms 1x vs HQ 4x + LATENCIA CONSISTENTE)
class OversampledConsoleChain
{
public:
    OversampledConsoleChain()
        : oversampler(2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true)
    {}

    void prepare(double sampleRate, int maxBlockSize)
    {
        baseFs = std::max(8000.0, sampleRate);
        oversampler.initProcessing(static_cast<size_t>(maxBlockSize));

        const double osFs = baseFs * static_cast<double>(oversampler.getOversamplingFactor());
        eq1x.prepare(baseFs);
        comp1x.prepare(baseFs, maxBlockSize);
        drive1x.prepare(baseFs);

        eq4x.prepare(osFs);
        comp4x.prepare(osFs, maxBlockSize * 4);
        drive4x.prepare(osFs);
    }

    void reset()
    {
        oversampler.reset();
        eq1x.reset();   comp1x.reset();
        eq4x.reset();   comp4x.reset();
    }

    int getLatencySamples(bool hq4xEnabled) const noexcept
    {
        return hq4xEnabled ? static_cast<int>(std::round(oversampler.getLatencyInSamples())) : 0;
    }

    // Procesa la cadena de consola (y opcionalmente saturacion vocal) manteniendo latencia fija en HQ 4x
    void process(juce::AudioBuffer<float>& buffer,
                 bool hq4xEnabled,
                 bool eqOn, int eqModel, float hpfHz, float loDb, float loHz, float midDb, float midHz, float hiDb, float hiHz,
                 bool compOn, int compModel, float compThresh, float compMix, bool scLink, bool scFilt,
                 int circuitModel, float drive01,
                 bool vocalSatOn, float vocalSatDrive)
    {
        if (!hq4xEnabled)
        {
            // MODO TRACK (0ms latencia, 1x)
            if (vocalSatOn && vocalSatDrive > 0.001f)
                applyVocalSaturation(buffer, vocalSatDrive);

            drive1x.setParameters(circuitModel, drive01);
            drive1x.process(buffer);

            eq1x.setParameters(eqModel, hpfHz, loDb, loHz, midDb, midHz, hiDb, hiHz);
            eq1x.process(buffer, eqOn);

            comp1x.setParameters(compModel, compThresh, compMix, scLink, scFilt, true);
            comp1x.process(buffer, compOn);
            lastGrDb = comp1x.getGainReductionDb();
            return;
        }

        // MODO HQ 4x: siempre pasa por oversampler para mantener latencia 100% constante en el DAW
        juce::dsp::AudioBlock<float> baseBlock(buffer);
        auto osBlock = oversampler.processSamplesUp(baseBlock);

        const int osChannels = static_cast<int>(osBlock.getNumChannels());
        const int osSamples  = static_cast<int>(osBlock.getNumSamples());
        float* ptrs[2] = { osBlock.getChannelPointer(0), osChannels > 1 ? osBlock.getChannelPointer(1) : osBlock.getChannelPointer(0) };
        juce::AudioBuffer<float> osBuffer(ptrs, osChannels, osSamples);

        if (vocalSatOn && vocalSatDrive > 0.001f)
            applyVocalSaturation(osBuffer, vocalSatDrive);

        drive4x.setParameters(circuitModel, drive01);
        drive4x.process(osBuffer);

        eq4x.setParameters(eqModel, hpfHz, loDb, loHz, midDb, midHz, hiDb, hiHz);
        eq4x.process(osBuffer, eqOn);

        comp4x.setParameters(compModel, compThresh, compMix, scLink, scFilt, true);
        comp4x.process(osBuffer, compOn);
        lastGrDb = comp4x.getGainReductionDb();

        oversampler.processSamplesDown(baseBlock);
    }

    float getGainReductionDb() const noexcept { return lastGrDb; }

private:
    static void applyVocalSaturation(juce::AudioBuffer<float>& buf, float drive) noexcept
    {
        const float pre = 1.0f + drive * 1.4f;
        const float post = 1.0f / (1.0f + drive * 0.45f);
        for (int c = 0; c < buf.getNumChannels(); ++c)
        {
            auto* data = buf.getWritePointer(c);
            for (int i = 0; i < buf.getNumSamples(); ++i)
                data[i] = softClipTanh(data[i] * pre) * post;
        }
    }

    double baseFs = 48000.0;
    juce::dsp::Oversampling<float> oversampler;

    AnalogConsoleEQ eq1x, eq4x;
    VintageCompressor comp1x, comp4x;
    TubeTapeDrive drive1x, drive4x;
    float lastGrDb = 0.0f;
};

} // namespace BenBocazDSP
