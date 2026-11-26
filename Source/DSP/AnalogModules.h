#pragma once
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <algorithm>

namespace BenBocazDSP
{

inline float softClipTanh(float x) noexcept
{
    return std::tanh(std::clamp(x, -10.0f, 10.0f));
}

//==============================================================================
// 0. SOURCE & MONITOR SPEAKER MODELER (Radiacion fisica antes de la sala)
class SourceAndMonitorModeler
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
            body[c].reset();
            mid[c].reset();
            lpf[c].reset();
        }
    }

    void setParameters(int sourceIndex, int monitorIndex)
    {
        if (src != sourceIndex || mon != monitorIndex)
        {
            src = std::clamp(sourceIndex, 0, 6);
            mon = std::clamp(monitorIndex, 0, 2);
            dirty = true;
        }
    }

    void process(juce::AudioBuffer<float>& buffer, bool enabled)
    {
        if (!enabled || buffer.getNumSamples() == 0)
            return;

        if (dirty)
            updateCurves();

        const int numCh = std::min(2, buffer.getNumChannels());
        const int numSamples = buffer.getNumSamples();

        for (int c = 0; c < numCh; ++c)
        {
            auto* data = buffer.getWritePointer(c);
            for (int i = 0; i < numSamples; ++i)
            {
                float s = data[i];
                if (useHpf) s = hpf[c].processSample(s);
                s = body[c].processSample(s);
                s = mid[c].processSample(s);
                if (useLpf) s = lpf[c].processSample(s);

                if (satDrive > 0.001f)
                    s = softClipTanh(s * (1.0f + satDrive)) / (1.0f + satDrive * 0.65f);

                data[i] = s;
            }
        }
    }

private:
    void updateCurves()
    {
        useHpf = true;
        useLpf = false;
        satDrive = 0.0f;

        float hpHz = 40.0f, lpHz = 18000.0f;
        float bodyHz = 120.0f, bodyQ = 0.8f, bodyDb = 0.0f;
        float midHzL = 2000.0f, midHzR = 2000.0f, midQ = 1.0f, midDb = 0.0f;

        // 0=Vocals, 1=Drums, 2=Bass Amp, 3=Guitar Cab Mono, 4=Guitar Cab Stereo, 5=Ensemble, 6=Monitors
        switch (src)
        {
            case 0: // Vocals
                hpHz = 75.0f; bodyHz = 220.0f; bodyDb = 0.4f; midHzL = midHzR = 3500.0f; midDb = 0.8f;
                break;
            case 1: // Drums
                hpHz = 32.0f; bodyHz = 65.0f; bodyDb = 1.2f; midHzL = midHzR = 420.0f; midDb = -1.0f;
                break;
            case 2: // Bass Amp (8x10 Cabinet)
                hpHz = 42.0f; useLpf = true; lpHz = 4200.0f;
                bodyHz = 78.0f; bodyQ = 1.1f; bodyDb = 2.8f;
                midHzL = midHzR = 240.0f; midQ = 1.2f; midDb = 1.6f;
                satDrive = 0.22f;
                break;
            case 3: // Guitar Cab Mono (4x12 Celestion)
                hpHz = 85.0f; useLpf = true; lpHz = 5200.0f;
                bodyHz = 125.0f; bodyQ = 1.2f; bodyDb = 2.2f;
                midHzL = midHzR = 2400.0f; midQ = 1.4f; midDb = 3.0f;
                satDrive = 0.28f;
                break;
            case 4: // Guitar Cab Stereo (Dual 4x12 con ligero offset L/R)
                hpHz = 85.0f; useLpf = true; lpHz = 5400.0f;
                bodyHz = 120.0f; bodyQ = 1.1f; bodyDb = 2.0f;
                midHzL = 2250.0f; midHzR = 2650.0f; midQ = 1.3f; midDb = 2.8f;
                satDrive = 0.25f;
                break;
            case 5: // Ensemble
                hpHz = 55.0f; bodyHz = 250.0f; bodyDb = 0.9f; midHzL = midHzR = 4500.0f; midDb = 0.5f;
                break;
            case 6: // Monitors (0=Altec Main, 1=NS-10M, 2=Auratone)
            default:
                if (mon == 0) // Altec Main
                {
                    hpHz = 38.0f; useLpf = true; lpHz = 15500.0f;
                    bodyHz = 62.0f; bodyDb = 1.6f; midHzL = midHzR = 2200.0f; midDb = 1.4f;
                    satDrive = 0.10f;
                }
                else if (mon == 1) // Yamaha NS-10M
                {
                    hpHz = 95.0f; useLpf = true; lpHz = 11500.0f;
                    bodyHz = 150.0f; bodyDb = -1.2f; midHzL = midHzR = 1800.0f; midQ = 1.1f; midDb = 3.2f;
                    satDrive = 0.12f;
                }
                else // Auratone 5C
                {
                    hpHz = 160.0f; useLpf = true; lpHz = 6500.0f;
                    bodyHz = 240.0f; bodyDb = -1.5f; midHzL = midHzR = 1200.0f; midQ = 0.9f; midDb = 2.8f;
                    satDrive = 0.18f;
                }
                break;
        }

        auto hpC  = juce::dsp::IIR::Coefficients<float>::makeHighPass(fs, hpHz, 0.707f);
        auto lpC  = juce::dsp::IIR::Coefficients<float>::makeLowPass(fs, lpHz, 0.75f);
        auto bC   = juce::dsp::IIR::Coefficients<float>::makePeakFilter(fs, bodyHz, bodyQ, juce::Decibels::decibelsToGain(bodyDb));
        auto mCL  = juce::dsp::IIR::Coefficients<float>::makePeakFilter(fs, midHzL, midQ, juce::Decibels::decibelsToGain(midDb));
        auto mCR  = juce::dsp::IIR::Coefficients<float>::makePeakFilter(fs, midHzR, midQ, juce::Decibels::decibelsToGain(midDb));

        for (int c = 0; c < 2; ++c)
        {
            hpf[c].coefficients  = hpC;
            lpf[c].coefficients  = lpC;
            body[c].coefficients = bC;
        }
        mid[0].coefficients = mCL;
        mid[1].coefficients = mCR;
        dirty = false;
    }

    double fs = 48000.0;
    int src = 0, mon = 0;
    bool dirty = true, useHpf = true, useLpf = false;
    float satDrive = 0.0f;
    juce::dsp::IIR::Filter<float> hpf[2], body[2], mid[2], lpf[2];
};

//==============================================================================
// 1. VINTAGE COMPRESSOR (1176 FET, LA-2A Dual-Release Opto, SSL VCA, Fairchild Vari-Mu)
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
        optoSlowEnv = 0.0f;
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

            float scL = scFilter ? scHpL.processSample(dryL) : dryL;
            float scR = scFilter ? scHpR.processSample(dryR) : dryR;

            float det = link ? std::max(std::abs(scL), std::abs(scR))
                             : 0.5f * (std::abs(scL) + std::abs(scR));

            const float detDb = juce::Decibels::gainToDecibels(det + 1e-6f, -96.0f);
            const float overDb = detDb - threshDb;

            // Vari-Mu progresivo en Fairchild (ratio sube suavemente con el nivel)
            float effRatio = ratio;
            if (model == TUBE_FAIRCHILD && overDb > 0.0f)
                effRatio = std::clamp(2.0f + overDb * 0.25f, 2.0f, 8.0f);

            float grDb = 0.0f;
            if (overDb > -kneeHalf)
            {
                if (overDb >= kneeHalf)
                    grDb = overDb * (1.0f - 1.0f / effRatio);
                else
                {
                    const float x = overDb + kneeHalf;
                    grDb = (x * x) / (4.0f * kneeHalf) * (1.0f - 1.0f / effRatio);
                }
            }

            if (model == OPTO_LA2A)
            {
                // Celda optica T4B: release en 2 etapas (rapida inicial + cola lenta)
                const float cFast = (grDb > env) ? attackCoeff : releaseCoeff;
                env = grDb + cFast * (env - grDb);
                optoSlowEnv = std::max(env, env + optoSlowCoeff * (optoSlowEnv - env));
                env = 0.65f * env + 0.35f * optoSlowEnv;
            }
            else
            {
                const float coeff = (grDb > env) ? attackCoeff : releaseCoeff;
                env = grDb + coeff * (env - grDb);
            }

            const float targetAutoDb = autoGain ? std::clamp(env * autoGainFactor, 0.0f, 12.0f) : baseMakeupDb;
            autoGainEnvDb = targetAutoDb + autoGainSmoothCoeff * (autoGainEnvDb - targetAutoDb);

            const float totalGain = juce::Decibels::decibelsToGain(-env + autoGainEnvDb);
            float wetL = dryL * totalGain;
            float wetR = dryR * totalGain;

            if (model == FET_1176)
            {
                wetL = (softClipTanh(wetL * colorDrive + 0.02f) - 0.02f) / colorDrive;
                wetR = (softClipTanh(wetR * colorDrive + 0.02f) - 0.02f) / colorDrive;
            }
            else if (model == TUBE_FAIRCHILD || model == OPTO_LA2A)
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
        float atkMs = 5.0f, relMs = 80.0f;

        switch (model)
        {
            case FET_1176:
                ratio = 4.0f; kneeHalf = 2.5f; atkMs = 0.4f; relMs = 55.0f;
                baseMakeupDb = 2.0f; autoGainFactor = 0.72f; colorDrive = 1.24f;
                break;
            case OPTO_LA2A:
                ratio = 3.2f; kneeHalf = 6.5f; atkMs = 10.0f; relMs = 65.0f;
                baseMakeupDb = 1.5f; autoGainFactor = 0.68f; colorDrive = 1.08f;
                break;
            case VCA_SSL:
                ratio = 4.0f; kneeHalf = 2.0f; atkMs = 3.0f; relMs = 100.0f;
                baseMakeupDb = 1.0f; autoGainFactor = 0.70f; colorDrive = 1.02f;
                break;
            case TUBE_FAIRCHILD:
            default:
                ratio = 2.5f; kneeHalf = 9.0f; atkMs = 5.0f; relMs = 170.0f;
                baseMakeupDb = 2.5f; autoGainFactor = 0.74f; colorDrive = 1.28f;
                break;
        }

        attackCoeff  = std::exp(-1.0f / (static_cast<float>(fs) * (atkMs * 0.001f)));
        releaseCoeff = std::exp(-1.0f / (static_cast<float>(fs) * (relMs * 0.001f)));
        optoSlowCoeff = std::exp(-1.0f / (static_cast<float>(fs) * 0.45f));
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
    float threshDb = -16.0f, mix = 1.0f;
    bool link = true, scFilter = false, autoGain = true;

    float ratio = 4.0f, kneeHalf = 3.0f, baseMakeupDb = 2.0f, autoGainFactor = 0.72f, colorDrive = 1.15f;
    float attackCoeff = 0.99f, releaseCoeff = 0.999f, optoSlowCoeff = 0.9997f, autoGainSmoothCoeff = 0.9995f;
    float env = 0.0f, optoSlowEnv = 0.0f, autoGainEnvDb = 0.0f, lastGrDb = 0.0f;

    juce::dsp::IIR::Filter<float> scHpL, scHpR;
};

//==============================================================================
// 2. ANALOG CONSOLE EQ (Neve 1073, API 550 Proportional-Q, Pultec Boost+Dip)
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
            hpf[c].reset(); low[c].reset(); pultecDip[c].reset(); mid[c].reset(); high[c].reset();
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
        const bool usePultecDip = (model == 2 && lowGainDb > 0.5f);

        for (int c = 0; c < numCh; ++c)
        {
            auto* data = buffer.getWritePointer(c);
            for (int i = 0; i < numSamples; ++i)
            {
                float s = data[i];
                if (useHpf) s = hpf[c].processSample(s);
                s = low[c].processSample(s);
                if (usePultecDip) s = pultecDip[c].processSample(s);
                s = mid[c].processSample(s);
                s = high[c].processSample(s);
                data[i] = s;
            }
        }
    }

private:
    void updateFilters()
    {
        // API 550 (model == 1): Proportional-Q real (mas ganancia = campana mas enfocada)
        float qMid = 0.85f;
        if (model == 1)      qMid = 0.70f + std::abs(midGainDb) * 0.14f;
        else if (model == 2) qMid = 0.55f; // Pultec musical y ancho

        const float qShelf = (model == 0) ? 0.78f : (model == 2 ? 0.58f : 0.707f);

        auto hpC  = juce::dsp::IIR::Coefficients<float>::makeHighPass(fs, hpfFreq, 0.707f);
        auto loC  = juce::dsp::IIR::Coefficients<float>::makeLowShelf(fs, lowFreq, qShelf, juce::Decibels::decibelsToGain(lowGainDb));
        auto dipC = juce::dsp::IIR::Coefficients<float>::makePeakFilter(fs, std::min(lowFreq * 2.1f, 450.0f), 1.1f,
                                                                        juce::Decibels::decibelsToGain(-lowGainDb * 0.28f));
        auto midC = juce::dsp::IIR::Coefficients<float>::makePeakFilter(fs, midFreq, qMid, juce::Decibels::decibelsToGain(midGainDb));
        auto hiC  = juce::dsp::IIR::Coefficients<float>::makeHighShelf(fs, highFreq, qShelf, juce::Decibels::decibelsToGain(highGainDb));

        for (int c = 0; c < 2; ++c)
        {
            hpf[c].coefficients       = hpC;
            low[c].coefficients       = loC;
            pultecDip[c].coefficients = dipC;
            mid[c].coefficients       = midC;
            high[c].coefficients      = hiC;
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

    juce::dsp::IIR::Filter<float> hpf[2], low[2], pultecDip[2], mid[2], high[2];
};

//==============================================================================
// 3. TUBE / TAPE DRIVE (Neve Class-A, API Discrete, Ampex Tape)
class TubeTapeDrive
{
public:
    void prepare(double sampleRate)
    {
        fs = std::max(8000.0, sampleRate);
        driveSmooth.reset(fs, 0.025);
        driveSmooth.setCurrentAndTargetValue(0.0f);
        auto tapeC = juce::dsp::IIR::Coefficients<float>::makeHighShelf(fs, 9500.0f, 0.707f, juce::Decibels::decibelsToGain(-1.5f));
        tapeLpf[0].coefficients = tapeLpf[1].coefficients = tapeC;
        tapeLpf[0].reset(); tapeLpf[1].reset();
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

        const int numCh = std::min(2, buffer.getNumChannels());
        const int numSamples = buffer.getNumSamples();

        for (int i = 0; i < numSamples; ++i)
        {
            const float d = driveSmooth.getNextValue();
            if (d < 0.001f)
                continue;

            const float preGain = 1.0f + d * (circuit == 0 ? 2.4f : (circuit == 1 ? 3.2f : 2.0f));
            const float asym = (circuit == 0) ? 0.06f * d : 0.0f; // Neve: 2do armonico par
            const float compGain = 1.0f / (1.0f + 0.58f * d);

            for (int c = 0; c < numCh; ++c)
            {
                float x = buffer.getSample(c, i);
                float y = softClipTanh(x * preGain + asym) - softClipTanh(asym);
                if (circuit == 2) // Ampex Tape: suavizado sutil de agudos con el drive
                    y = y * (1.0f - 0.5f * d) + tapeLpf[c].processSample(y) * (0.5f * d);
                buffer.setSample(c, i, y * compGain);
            }
        }
    }

private:
    double fs = 48000.0;
    int circuit = 0;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> driveSmooth;
    juce::dsp::IIR::Filter<float> tapeLpf[2];
};

//==============================================================================
// 4. OVERSAMPLED CONSOLE CHAIN (TRACK 0ms 1x vs MIXING HQ 4x)
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
        eq1x.prepare(baseFs);   comp1x.prepare(baseFs, maxBlockSize);   drive1x.prepare(baseFs);
        eq4x.prepare(osFs);     comp4x.prepare(osFs, maxBlockSize * 4); drive4x.prepare(osFs);
    }

    void reset()
    {
        oversampler.reset();
        eq1x.reset(); comp1x.reset();
        eq4x.reset(); comp4x.reset();
    }

    int getLatencySamples(bool hq4xEnabled) const noexcept
    {
        return hq4xEnabled ? static_cast<int>(std::round(oversampler.getLatencyInSamples())) : 0;
    }

    void process(juce::AudioBuffer<float>& buffer,
                 bool hq4xEnabled,
                 bool eqOn, int eqModel, float hpfHz, float loDb, float loHz, float midDb, float midHz, float hiDb, float hiHz,
                 bool compOn, int compModel, float compThresh, float compMix, bool scLink, bool scFilt,
                 int circuitModel, float drive01,
                 bool vocalSatOn, float vocalSatDrive)
    {
        if (!hq4xEnabled)
        {
            if (vocalSatOn && vocalSatDrive > 0.001f) applyVocalSaturation(buffer, vocalSatDrive);
            drive1x.setParameters(circuitModel, drive01); drive1x.process(buffer);
            eq1x.setParameters(eqModel, hpfHz, loDb, loHz, midDb, midHz, hiDb, hiHz); eq1x.process(buffer, eqOn);
            comp1x.setParameters(compModel, compThresh, compMix, scLink, scFilt, true); comp1x.process(buffer, compOn);
            lastGrDb = comp1x.getGainReductionDb();
            return;
        }

        juce::dsp::AudioBlock<float> baseBlock(buffer);
        auto osBlock = oversampler.processSamplesUp(baseBlock);

        const int osChannels = static_cast<int>(osBlock.getNumChannels());
        const int osSamples  = static_cast<int>(osBlock.getNumSamples());
        float* ptrs[2] = { osBlock.getChannelPointer(0), osChannels > 1 ? osBlock.getChannelPointer(1) : osBlock.getChannelPointer(0) };
        juce::AudioBuffer<float> osBuffer(ptrs, osChannels, osSamples);

        if (vocalSatOn && vocalSatDrive > 0.001f) applyVocalSaturation(osBuffer, vocalSatDrive);
        drive4x.setParameters(circuitModel, drive01); drive4x.process(osBuffer);
        eq4x.setParameters(eqModel, hpfHz, loDb, loHz, midDb, midHz, hiDb, hiHz); eq4x.process(osBuffer, eqOn);
        comp4x.setParameters(compModel, compThresh, compMix, scLink, scFilt, true); comp4x.process(osBuffer, compOn);
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
