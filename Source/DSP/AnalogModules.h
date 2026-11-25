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
// EQ de Consola Analógica con Frecuencias Seleccionables + Etapa de Válvula Sincronizada
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
        dcIn = { 0.0f, 0.0f };
        dcOut = { 0.0f, 0.0f };
    }

    void updateParameters (int eqModel, float hpfHz,
                           int lowFreqIdx, float lowDb,
                           int midFreqIdx, float midDb,
                           int highFreqIdx, float highDb,
                           bool tubeSync, int manualTubeIdx)
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

        if (eqModel == 0) // Neve 1073 (British Class-A)
        {
            low.setLowShelf (fLow, lowDb);
            mid.setPeak (fMid, midDb, 0.85f);
            high.setHighShelf (fHigh, highDb);
        }
        else if (eqModel == 1) // Pultec EQP-1A (Tube Passive)
        {
            low.setLowShelf (fLow * 0.92f, lowDb * 1.10f);
            if (lowDb > 0.5f)
                pultecDip.setPeak (fLow * 2.45f, -lowDb * 0.36f, 1.15f);
            mid.setPeak (fMid, midDb, 0.65f);
            high.setHighShelf (fHigh, highDb * 1.05f);
        }
        else if (eqModel == 2) // API 550A (Proportional Q Punch)
        {
            const float propQ = 0.90f + std::abs (midDb) * 0.09f;
            low.setLowShelf (fLow, lowDb);
            mid.setPeak (fMid, midDb, propQ);
            high.setHighShelf (fHigh * 0.90f, highDb);
        }
        else // SSL 4000E (Surgical Console)
        {
            low.setLowShelf (fLow, lowDb);
            mid.setPeak (fMid, midDb, 1.55f);
            high.setHighShelf (fHigh, highDb);
        }

        // Perfil de válvula/transformador sincronizado con el EQ elegido
        activeTubeProfile = tubeSync ? eqModel : std::clamp (manualTubeIdx, 0, 3);
        const float eqPush = (std::abs (lowDb) + std::abs (midDb) + std::abs (highDb)) / 36.0f;

        if (activeTubeProfile == 0)      { satDrive = 0.22f + eqPush * 0.25f; satBias = 0.045f; } // Neve Transformer/Class-A
        else if (activeTubeProfile == 1) { satDrive = 0.28f + eqPush * 0.22f; satBias = 0.065f; } // Pultec 12AX7/12AU7 Tube
        else if (activeTubeProfile == 2) { satDrive = 0.19f + eqPush * 0.28f; satBias = 0.025f; } // API 2520 Op-Amp Punch
        else                             { satDrive = 0.08f + eqPush * 0.12f; satBias = 0.010f; } // SSL Clean VCA
    }

    inline void processSample (int ch, float& x) noexcept
    {
        x = hpf.processSample (ch, x);
        x = low.processSample (ch, x);
        x = pultecDip.processSample (ch, x);
        x = mid.processSample (ch, x);
        x = high.processSample (ch, x);

        // Etapa armónica sincronizada con el EQ dentro del oversampling 4x
        const float pre = 1.0f + satDrive * 1.35f;
        const float post = 1.0f / (1.0f + satDrive * 0.62f);
        const float biasComp = std::tanh (satBias);
        const float y = std::tanh (x * pre + satBias) - biasComp;

        const auto c = static_cast<size_t> (ch & 1);
        const float dc = y - dcIn[c] + 0.9992f * dcOut[c];
        dcIn[c] = y;
        dcOut[c] = dc;
        x = dc * post;
    }

private:
    double sr = 176400.0;
    StereoBiquad hpf, low, pultecDip, mid, high;
    int activeTubeProfile = 0;
    float satDrive = 0.20f, satBias = 0.04f;
    std::array<float, 2> dcIn { 0.0f, 0.0f };
    std::array<float, 2> dcOut { 0.0f, 0.0f };
};

//==============================================================================
// Compresor Clásico (soporta modo Principal con MIX/SC y modo Paralelo Crush)
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
        else // Parallel Crush (All-Buttons-In Style)
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
// Cadena Analógica Completa con Oversampling 4x Compartido (Cero Aliasing / Nyquist)
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
        eqStage.prepare (osRate);
        mainComp.prepare (osRate);
        parallelComp.prepare (osRate);
        reset();
    }

    void reset()
    {
        oversampling.reset();
        eqStage.reset();
        mainComp.reset();
        parallelComp.reset();
        dcIn = { 0.0f, 0.0f };
        dcOut = { 0.0f, 0.0f };
    }

    int getLatencySamples() const { return static_cast<int> (oversampling.getLatencyInSamples()); }

    ConsoleEqWithTubeStage eqStage;
    VintageCompressor mainComp;
    VintageCompressor parallelComp;

    struct ChainSettings
    {
        int routingOrder = 0; // 0: EQ->Comp, 1: Comp->EQ
        bool eqOn = true;
        bool compOn = true;
        float compMix = 1.0f;
        bool parallelOn = false;
        float parallelAmount = 0.35f;
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

        const float preGain = 1.0f + s.tubeDrive * 3.0f + (s.speakerBreakup ? 0.50f : 0.0f);
        const float postGain = 1.0f / (1.0f + s.tubeDrive * 1.40f + (s.speakerBreakup ? 0.22f : 0.0f));
        const float bias = 0.075f * s.tubeDrive + (s.speakerBreakup ? 0.035f : 0.0f);
        const float biasComp = std::tanh (bias);

        float maxMainGr = 0.0f;
        float maxParGr  = 0.0f;

        for (size_t i = 0; i < numSamples; ++i)
        {
            float L = ptrL[i];
            float R = ptrR[i];

            // 1. Saturación de Consola / Cinta / Cono de Parlante en 4x
            if (s.tubeDrive > 0.005f || s.speakerBreakup)
            {
                float yL = std::tanh (L * preGain + bias) - biasComp;
                float yR = std::tanh (R * preGain + bias) - biasComp;
                if (s.speakerBreakup)
                {
                    yL = 0.72f * std::tanh (yL * 1.45f) + 0.28f * yL;
                    yR = 0.72f * std::tanh (yR * 1.45f) + 0.28f * yR;
                }
                const float dL = yL - dcIn[0] + 0.9992f * dcOut[0];
                const float dR = yR - dcIn[1] + 0.9992f * dcOut[1];
                dcIn[0] = yL; dcOut[0] = dL;
                dcIn[1] = yR; dcOut[1] = dR;
                L = dL * postGain;
                R = dR * postGain;
            }

            // 2. Orden entre EQ Analógico y Compresor Principal
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

            // 3. Segundo Compresor Paralelo Dedicado (Crush / Glue)
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
    std::array<float, 2> dcIn { 0.0f, 0.0f };
    std::array<float, 2> dcOut { 0.0f, 0.0f };
};
