#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>
#include <algorithm>
#include <atomic>

//==============================================================================
// Filtro Biquad Estéreo (cero allocations en el hilo de audio)
//==============================================================================
class StereoBiquad
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate > 0.0 ? sampleRate : 44100.0;
        reset();
    }

    void reset()
    {
        z1 = { 0.0f, 0.0f };
        z2 = { 0.0f, 0.0f };
    }

    void setBypass() { b0 = 1.0f; b1 = b2 = a1 = a2 = 0.0f; }

    void setHighPass (float freq, float q = 0.7071f)
    {
        freq = std::clamp (freq, 18.0f, static_cast<float> (sr * 0.45));
        const float w0 = juce::MathConstants<float>::twoPi * freq / static_cast<float> (sr);
        const float cosw = std::cos (w0);
        const float alpha = std::sin (w0) / (2.0f * q);
        setNormalized ((1.0f + cosw) * 0.5f, -(1.0f + cosw), (1.0f + cosw) * 0.5f,
                       1.0f + alpha, -2.0f * cosw, 1.0f - alpha);
    }

    void setLowPass (float freq, float q = 0.7071f)
    {
        freq = std::clamp (freq, 40.0f, static_cast<float> (sr * 0.45));
        const float w0 = juce::MathConstants<float>::twoPi * freq / static_cast<float> (sr);
        const float cosw = std::cos (w0);
        const float alpha = std::sin (w0) / (2.0f * q);
        setNormalized ((1.0f - cosw) * 0.5f, 1.0f - cosw, (1.0f - cosw) * 0.5f,
                       1.0f + alpha, -2.0f * cosw, 1.0f - alpha);
    }

    void setPeak (float freq, float gainDb, float q = 1.0f)
    {
        if (std::abs (gainDb) < 0.01f) { setBypass(); return; }
        freq = std::clamp (freq, 20.0f, static_cast<float> (sr * 0.45));
        const float A = std::pow (10.0f, gainDb / 40.0f);
        const float w0 = juce::MathConstants<float>::twoPi * freq / static_cast<float> (sr);
        const float cosw = std::cos (w0);
        const float alpha = std::sin (w0) / (2.0f * q);
        setNormalized (1.0f + alpha * A, -2.0f * cosw, 1.0f - alpha * A,
                       1.0f + alpha / A, -2.0f * cosw, 1.0f - alpha / A);
    }

    void setLowShelf (float freq, float gainDb)
    {
        if (std::abs (gainDb) < 0.01f) { setBypass(); return; }
        freq = std::clamp (freq, 20.0f, static_cast<float> (sr * 0.45));
        const float A = std::pow (10.0f, gainDb / 40.0f);
        const float w0 = juce::MathConstants<float>::twoPi * freq / static_cast<float> (sr);
        const float cosw = std::cos (w0);
        const float sq = std::sqrt (2.0f * A) * std::sin (w0);
        setNormalized (A * ((A + 1.0f) - (A - 1.0f) * cosw + sq),
                       2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosw),
                       A * ((A + 1.0f) - (A - 1.0f) * cosw - sq),
                       (A + 1.0f) + (A - 1.0f) * cosw + sq,
                       -2.0f * ((A - 1.0f) + (A + 1.0f) * cosw),
                       (A + 1.0f) + (A - 1.0f) * cosw - sq);
    }

    void setHighShelf (float freq, float gainDb)
    {
        if (std::abs (gainDb) < 0.01f) { setBypass(); return; }
        freq = std::clamp (freq, 20.0f, static_cast<float> (sr * 0.45));
        const float A = std::pow (10.0f, gainDb / 40.0f);
        const float w0 = juce::MathConstants<float>::twoPi * freq / static_cast<float> (sr);
        const float cosw = std::cos (w0);
        const float sq = std::sqrt (2.0f * A) * std::sin (w0);
        setNormalized (A * ((A + 1.0f) + (A - 1.0f) * cosw + sq),
                       -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosw),
                       A * ((A + 1.0f) + (A - 1.0f) * cosw - sq),
                       (A + 1.0f) - (A - 1.0f) * cosw + sq,
                       2.0f * ((A - 1.0f) - (A + 1.0f) * cosw),
                       (A + 1.0f) - (A - 1.0f) * cosw - sq);
    }

    inline float processSample (int ch, float x) noexcept
    {
        const auto c = static_cast<size_t> (ch & 1);
        const float y = b0 * x + z1[c];
        z1[c] = b1 * x - a1 * y + z2[c];
        z2[c] = b2 * x - a2 * y;
        return y;
    }

    void processBuffer (juce::AudioBuffer<float>& buffer) noexcept
    {
        const int numCh = std::min (2, buffer.getNumChannels());
        const int numSamples = buffer.getNumSamples();
        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            for (int i = 0; i < numSamples; ++i)
                data[i] = processSample (ch, data[i]);
        }
    }

private:
    void setNormalized (float nb0, float nb1, float nb2, float na0, float na1, float na2)
    {
        const float inv = 1.0f / na0;
        b0 = nb0 * inv; b1 = nb1 * inv; b2 = nb2 * inv;
        a1 = na1 * inv; a2 = na2 * inv;
    }

    double sr = 44100.0;
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    std::array<float, 2> z1 { 0.0f, 0.0f };
    std::array<float, 2> z2 { 0.0f, 0.0f };
};

//==============================================================================
// Etapa de Preamp Analógico Fidedigno (4 Arquitecturas Reales en 4x Oversampling)
//==============================================================================
class AnalogPreampStage
{
public:
    void prepare (double oversampledRate)
    {
        sr = oversampledRate > 0.0 ? oversampledRate : 176400.0;
        lowCoeff  = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 160.0f   / static_cast<float> (sr));
        midCoeff  = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 1800.0f  / static_cast<float> (sr));
        highCoeff = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 11000.0f / static_cast<float> (sr));
        sagCoeff  = 1.0f - std::exp (-1.0f / (0.018f * static_cast<float> (sr)));
        reset();
    }

    void reset()
    {
        lowState  = { 0.0f, 0.0f };
        midState  = { 0.0f, 0.0f };
        highState = { 0.0f, 0.0f };
        sagEnv    = { 0.0f, 0.0f };
        dcIn      = { 0.0f, 0.0f };
        dcOut     = { 0.0f, 0.0f };
    }

    inline float processSample (int ch, float x, int circuit, float drive, bool speakerBreakup) noexcept
    {
        const auto c = static_cast<size_t> (ch & 1);
        const float d = std::clamp (drive, 0.0f, 1.0f);

        lowState[c]  += lowCoeff  * (x - lowState[c]);
        midState[c]  += midCoeff  * (x - midState[c]);
        highState[c] += highCoeff * (x - highState[c]);

        const float lows  = lowState[c];
        const float mids  = midState[c] - lowState[c];
        const float highs = x - highState[c];

        sagEnv[c] += sagCoeff * (std::abs (x) - sagEnv[c]);

        float y = x;

        if (circuit == 0) // NEVE CLASS-A (1073 + Transformador Marinair)
        {
            const float pre = 1.0f + d * 2.8f;
            const float xFed = (x + lows * (0.42f * d)) * pre;
            const float bias = 0.09f * d + 0.04f * d * sagEnv[c];
            const float shaped = std::tanh (xFed + bias) - std::tanh (bias);
            const float warm = shaped - highs * (0.12f * d);
            y = warm / (1.0f + d * 1.35f);
        }
        else if (circuit == 1) // PULTEC 12AX7 (Triodo a Tubos Asimétrico)
        {
            const float sag = 1.0f / (1.0f + sagEnv[c] * 0.45f * d);
            const float pre = (1.0f + d * 2.6f) * sag;
            const float xIn = x * pre;
            float shaped = 0.0f;
            if (xIn >= 0.0f)
                shaped = std::tanh (xIn * 1.18f) / 1.18f;
            else
                shaped = (2.0f / juce::MathConstants<float>::pi) * std::atan (xIn * 0.92f) * 1.15f;
            y = (shaped + highs * (0.08f * d)) / (1.0f + d * 1.18f);
        }
        else if (circuit == 2) // API 2520 (Op-Amp Discreto + Punch en Medios)
        {
            const float pre = 1.0f + d * 3.1f;
            const float xFed = (x - lows * (0.16f * d) + mids * (0.38f * d)) * pre;
            const float shaped = xFed / std::sqrt (1.0f + xFed * xFed);
            y = (shaped + lows * (0.12f * d)) / (1.0f + d * 1.42f);
        }
        else // SSL CLEAN (4000E Alto Headroom)
        {
            const float pre = 1.0f + d * 1.45f;
            const float xIn = x * pre;
            const float shaped = 0.65f * std::tanh (xIn) + 0.35f * (xIn / std::sqrt (1.0f + 0.5f * xIn * xIn));
            y = (shaped + highs * (0.10f * d)) / (1.0f + d * 0.72f);
        }

        if (speakerBreakup)
            y = 0.76f * std::tanh (y * 1.38f) + 0.24f * y;

        const float dc = y - dcIn[c] + 0.9994f * dcOut[c];
        dcIn[c] = y;
        dcOut[c] = dc;
        return dc;
    }

private:
    double sr = 176400.0;
    float lowCoeff = 0.005f, midCoeff = 0.06f, highCoeff = 0.30f, sagCoeff = 0.0003f;
    std::array<float, 2> lowState  { 0.0f, 0.0f };
    std::array<float, 2> midState  { 0.0f, 0.0f };
    std::array<float, 2> highState { 0.0f, 0.0f };
    std::array<float, 2> sagEnv    { 0.0f, 0.0f };
    std::array<float, 2> dcIn      { 0.0f, 0.0f };
    std::array<float, 2> dcOut     { 0.0f, 0.0f };
};

//==============================================================================
// EQ de Consola Analógica con Frecuencias Seleccionables (4x Oversampled)
//==============================================================================
class ConsoleEqWithTubeStage
{
public:
    void prepare (double oversampledRate)
    {
        sr = oversampledRate > 0.0 ? oversampledRate : 176400.0;
        hpf.prepare (sr); low.prepare (sr); pultecDip.prepare (sr);
        mid.prepare (sr); high.prepare (sr);
        reset();
    }

    void reset()
    {
        hpf.reset(); low.reset(); pultecDip.reset(); mid.reset(); high.reset();
    }

    void updateParameters (int eqModel, float hpfHz,
                           int lowFreqIdx, float lowDb,
                           int midFreqIdx, float midDb,
                           int highFreqIdx, float highDb,
                           bool /*tubeSync*/, int /*manualTubeIdx*/)
    {
        static constexpr std::array<float, 3> lowFreqs  { 60.0f, 110.0f, 220.0f };
        static constexpr std::array<float, 4> midFreqs  { 700.0f, 1600.0f, 3200.0f, 4800.0f };
        static constexpr std::array<float, 3> highFreqs { 10000.0f, 12000.0f, 16000.0f };

        const float fLow  = lowFreqs[static_cast<size_t> (std::clamp (lowFreqIdx, 0, 2))];
        const float fMid  = midFreqs[static_cast<size_t> (std::clamp (midFreqIdx, 0, 3))];
        const float fHigh = highFreqs[static_cast<size_t> (std::clamp (highFreqIdx, 0, 2))];

        if (hpfHz > 22.0f) hpf.setHighPass (hpfHz, 0.7071f);
        else               hpf.setBypass();

        pultecDip.setBypass();

        if (eqModel == 0) // Neve 1073
        {
            low.setLowShelf (fLow, lowDb);
            mid.setPeak (fMid, midDb, 0.85f);
            high.setHighShelf (fHigh, highDb);
        }
        else if (eqModel == 1) // Pultec EQP-1A
        {
            low.setLowShelf (fLow * 0.92f, lowDb * 1.10f);
            if (lowDb > 0.5f)
                pultecDip.setPeak (fLow * 2.45f, -lowDb * 0.36f, 1.15f);
            mid.setPeak (fMid, midDb, 0.65f);
            high.setHighShelf (fHigh, highDb * 1.05f);
        }
        else if (eqModel == 2) // API 550A
        {
            const float propQ = 0.90f + std::abs (midDb) * 0.09f;
            low.setLowShelf (fLow, lowDb);
            mid.setPeak (fMid, midDb, propQ);
            high.setHighShelf (fHigh * 0.90f, highDb);
        }
        else // SSL 4000E
        {
            low.setLowShelf (fLow, lowDb);
            mid.setPeak (fMid, midDb, 1.55f);
            high.setHighShelf (fHigh, highDb);
        }
    }

    inline void processSample (int ch, float& x) noexcept
    {
        x = hpf.processSample (ch, x);
        x = low.processSample (ch, x);
        x = pultecDip.processSample (ch, x);
        x = mid.processSample (ch, x);
        x = high.processSample (ch, x);
    }

private:
    double sr = 176400.0;
    StereoBiquad hpf, low, pultecDip, mid, high;
};

//==============================================================================
// Compresor Clásico (Principal con MIX/SC y Paralelo Crush)
//==============================================================================
class VintageCompressor
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate > 0.0 ? sampleRate : 44100.0;
        scHpf.prepare (sr);
        scHpf.setHighPass (120.0f, 0.7071f);
        reset();
    }

    void reset()
    {
        envFast = { 0.0f, 0.0f };
        envSlow = { 0.0f, 0.0f };
        scHpf.reset();
        gainReductionDb.store (0.0f);
    }

    void configure (int compType, float thresholdDb, bool useScFilter, bool linkStereo)
    {
        thresh = thresholdDb;
        scFilterOn = useScFilter;
        stereoLinked = linkStereo;

        if (compType == 0) // 1176LN (FET)
        {
            ratio = 6.0f; attackMs = 0.45f; relFastMs = 55.0f; relSlowMs = 55.0f;
            kneeDb = 2.0f; makeupDb = 2.0f; isOpto = false;
        }
        else if (compType == 1) // LA-2A (Opto)
        {
            ratio = 3.2f; attackMs = 10.0f; relFastMs = 65.0f; relSlowMs = 520.0f;
            kneeDb = 6.0f; makeupDb = 1.5f; isOpto = true;
        }
        else if (compType == 2) // Mastering (Tube)
        {
            ratio = 2.2f; attackMs = 18.0f; relFastMs = 165.0f; relSlowMs = 165.0f;
            kneeDb = 8.0f; makeupDb = 1.0f; isOpto = false;
        }
        else // Parallel Crush
        {
            ratio = 12.0f; attackMs = 0.25f; relFastMs = 42.0f; relSlowMs = 110.0f;
            kneeDb = 1.5f; makeupDb = 4.5f; isOpto = true;
        }

        atkCoeff     = std::exp (-1.0f / (0.001f * attackMs  * static_cast<float> (sr)));
        relFastCoeff = std::exp (-1.0f / (0.001f * relFastMs * static_cast<float> (sr)));
        relSlowCoeff = std::exp (-1.0f / (0.001f * relSlowMs * static_cast<float> (sr)));
        makeupLin    = juce::Decibels::decibelsToGain (makeupDb);
    }

    inline void processStereoSample (float& inL, float& inR, float wetMix, float& frameMaxGr) noexcept
    {
        float detL = scFilterOn ? scHpf.processSample (0, inL) : inL;
        float detR = scFilterOn ? scHpf.processSample (1, inR) : inR;

        if (stereoLinked)
        {
            const float peak = std::max (std::abs (detL), std::abs (detR));
            const float gr = computeChannelGr (0, peak);
            const float gainLin = juce::Decibels::decibelsToGain (-gr) * makeupLin;
            const float compL = inL * gainLin;
            const float compR = inR * gainLin;
            inL = inL * (1.0f - wetMix) + compL * wetMix;
            inR = inR * (1.0f - wetMix) + compR * wetMix;
            if (gr > frameMaxGr) frameMaxGr = gr;
        }
        else
        {
            const float grL = computeChannelGr (0, std::abs (detL));
            const float grR = computeChannelGr (1, std::abs (detR));
            const float compL = inL * juce::Decibels::decibelsToGain (-grL) * makeupLin;
            const float compR = inR * juce::Decibels::decibelsToGain (-grR) * makeupLin;
            inL = inL * (1.0f - wetMix) + compL * wetMix;
            inR = inR * (1.0f - wetMix) + compR * wetMix;
            const float m = std::max (grL, grR);
            if (m > frameMaxGr) frameMaxGr = m;
        }
    }

    void publishGr (float maxGr) noexcept { gainReductionDb.store (maxGr); }
    float getGainReductionDb() const { return gainReductionDb.load(); }

private:
    inline float computeChannelGr (int ch, float peak) noexcept
    {
        const auto c = static_cast<size_t> (ch & 1);
        const float levelDb = juce::Decibels::gainToDecibels (peak + 1e-6f);

        float overDb = 0.0f;
        const float diff = levelDb - thresh;
        if (2.0f * diff < -kneeDb)
            overDb = 0.0f;
        else if (2.0f * std::abs (diff) <= kneeDb)
            overDb = (diff + kneeDb * 0.5f) * (diff + kneeDb * 0.5f) / (2.0f * kneeDb);
        else
            overDb = diff;

        const float targetGrDb = overDb * (1.0f - 1.0f / ratio);

        if (targetGrDb > envFast[c])
            envFast[c] = atkCoeff * envFast[c] + (1.0f - atkCoeff) * targetGrDb;
        else
            envFast[c] = relFastCoeff * envFast[c] + (1.0f - relFastCoeff) * targetGrDb;

        float finalGrDb = envFast[c];
        if (isOpto)
        {
            if (targetGrDb > envSlow[c])
                envSlow[c] = atkCoeff * envSlow[c] + (1.0f - atkCoeff) * targetGrDb;
            else
                envSlow[c] = relSlowCoeff * envSlow[c] + (1.0f - relSlowCoeff) * targetGrDb;
            finalGrDb = 0.55f * envFast[c] + 0.45f * envSlow[c];
        }
        return finalGrDb;
    }

    double sr = 44100.0;
    float thresh = -16.0f, ratio = 4.0f, attackMs = 0.6f, relFastMs = 55.0f, relSlowMs = 55.0f;
    float kneeDb = 3.0f, makeupDb = 0.0f, makeupLin = 1.0f;
    float atkCoeff = 0.9f, relFastCoeff = 0.99f, relSlowCoeff = 0.999f;
    bool isOpto = false, scFilterOn = false, stereoLinked = true;

    std::array<float, 2> envFast { 0.0f, 0.0f };
    std::array<float, 2> envSlow { 0.0f, 0.0f };
    StereoBiquad scHpf;
    std::atomic<float> gainReductionDb { 0.0f };
};

//==============================================================================
// Cadena Analógica Completa con Oversampling 4x Compartido
//==============================================================================
class OversampledConsoleChain
{
public:
    OversampledConsoleChain()
        : oversampling (2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true)
    {}

    void prepare (double baseSampleRate, int maxBlockSize)
    {
        oversampling.initProcessing (static_cast<size_t> (maxBlockSize));
        const double osRate = baseSampleRate * 4.0;
        preampStage.prepare (osRate);
        eqStage.prepare (osRate);
        mainComp.prepare (osRate);
        parallelComp.prepare (osRate);
        reset();
    }

    void reset()
    {
        oversampling.reset();
        preampStage.reset();
        eqStage.reset();
        mainComp.reset();
        parallelComp.reset();
    }

    int getLatencySamples() const { return static_cast<int> (oversampling.getLatencyInSamples()); }

    AnalogPreampStage preampStage;
    ConsoleEqWithTubeStage eqStage;
    VintageCompressor mainComp;
    VintageCompressor parallelComp;

    struct ChainSettings
    {
        int routingOrder = 0;
        bool eqOn = true;
        bool compOn = true;
        float compMix = 1.0f;
        bool parallelOn = false;
        float parallelAmount = 0.35f;
        int tubeCircuit = 0;
        float tubeDrive = 0.20f;
        bool speakerBreakup = false;
    };

    void processBlock (juce::AudioBuffer<float>& buffer, const ChainSettings& s)
    {
        if (buffer.getNumChannels() < 2) return;

        const bool anyActive = s.eqOn || s.compOn || s.parallelOn || s.tubeDrive > 0.005f || s.speakerBreakup;
        if (! anyActive)
        {
            mainComp.publishGr (0.0f);
            parallelComp.publishGr (0.0f);
            return;
        }

        juce::dsp::AudioBlock<float> block (buffer);
        auto upBlock = oversampling.processSamplesUp (block);
        const auto numSamples = upBlock.getNumSamples();

        auto* ptrL = upBlock.getChannelPointer (0);
        auto* ptrR = upBlock.getChannelPointer (1);

        float maxMainGr = 0.0f;
        float maxParGr  = 0.0f;

        for (size_t i = 0; i < numSamples; ++i)
        {
            float L = ptrL[i];
            float R = ptrR[i];

            if (s.tubeDrive > 0.005f || s.speakerBreakup)
            {
                L = preampStage.processSample (0, L, s.tubeCircuit, s.tubeDrive, s.speakerBreakup);
                R = preampStage.processSample (1, R, s.tubeCircuit, s.tubeDrive, s.speakerBreakup);
            }

            if (s.routingOrder == 1)
            {
                if (s.compOn) mainComp.processStereoSample (L, R, s.compMix, maxMainGr);
                if (s.eqOn)   { eqStage.processSample (0, L); eqStage.processSample (1, R); }
            }
            else
            {
                if (s.eqOn)   { eqStage.processSample (0, L); eqStage.processSample (1, R); }
                if (s.compOn) mainComp.processStereoSample (L, R, s.compMix, maxMainGr);
            }

            if (s.parallelOn && s.parallelAmount > 0.001f)
            {
                float pL = L;
                float pR = R;
                parallelComp.processStereoSample (pL, pR, 1.0f, maxParGr);
                L = L + (pL - L) * s.parallelAmount;
                R = R + (pR - R) * s.parallelAmount;
            }

            ptrL[i] = L;
            ptrR[i] = R;
        }

        oversampling.processSamplesDown (block);
        mainComp.publishGr (maxMainGr);
        parallelComp.publishGr (maxParGr);
    }

private:
    juce::dsp::Oversampling<float> oversampling;
};
