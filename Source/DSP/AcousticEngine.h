#pragma once
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "AnalogModules.h"
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>
#include <atomic>

class HadamardFDN
{
public:
    static constexpr int NumLines = 8;

    void prepare (double sampleRate)
    {
        sr = sampleRate > 0.0 ? sampleRate : 44100.0;
        size_t needed = static_cast<size_t> (sr * 0.25);
        bufSize = 1;
        while (bufSize < needed) bufSize <<= 1;
        mask = bufSize - 1;

        for (auto& line : lines) line.assign (bufSize, 0.0f);
        reset();
    }

    void reset()
    {
        for (auto& line : lines) std::fill (line.begin(), line.end(), 0.0f);
        lpState.fill (0.0f); dcIn.fill (0.0f); dcOut.fill (0.0f);
        writePos = 0;
        for (int i = 0; i < NumLines; ++i)
            lfoPhase[static_cast<size_t> (i)] = static_cast<float> (i) * 0.785f;
    }

    void setProfile (const std::array<float, NumLines>& delayMs, float rt60Seconds,
                     float dampingAmount, float modDepthMs)
    {
        rt60 = std::clamp (rt60Seconds, 0.18f, 7.5f);
        damp = std::clamp (dampingAmount, 0.05f, 0.88f);
        modDepthSamples = modDepthMs * 0.001f * static_cast<float> (sr);

        for (size_t i = 0; i < NumLines; ++i)
        {
            baseDelaySamples[i] = std::clamp (delayMs[i] * 0.001f * static_cast<float> (sr),
                                              32.0f, static_cast<float> (bufSize - 64));
            const float delaySec = baseDelaySamples[i] / static_cast<float> (sr);
            lineGain[i] = std::min (0.988f, std::pow (10.0f, -3.0f * delaySec / rt60));
        }
    }

    inline void processStereo (float inL, float inR, float& outL, float& outR) noexcept
    {
        static constexpr std::array<float, NumLines> lfoRates {
            0.17f, 0.23f, 0.29f, 0.37f, 0.41f, 0.47f, 0.53f, 0.61f
        };

        std::array<float, NumLines> delayed {};

        for (size_t i = 0; i < NumLines; ++i)
        {
            lfoPhase[i] += juce::MathConstants<float>::twoPi * lfoRates[i] / static_cast<float> (sr);
            if (lfoPhase[i] > juce::MathConstants<float>::twoPi)
                lfoPhase[i] -= juce::MathConstants<float>::twoPi;

            const float d = baseDelaySamples[i] + std::sin (lfoPhase[i]) * modDepthSamples;
            const auto dInt = static_cast<size_t> (d);
            const float frac = d - static_cast<float> (dInt);

            const size_t idx0 = (writePos - dInt) & mask;
            const size_t idx1 = (idx0 - 1) & mask;
            const float sample = lines[i][idx0] * (1.0f - frac) + lines[i][idx1] * frac;

            lpState[i] = sample * (1.0f - damp) + lpState[i] * damp;
            const float dc = lpState[i] - dcIn[i] + 0.996f * dcOut[i];
            dcIn[i] = lpState[i];
            dcOut[i] = dc;
            delayed[i] = dc * lineGain[i];
        }

        const float a0 = delayed[0] + delayed[1], a1 = delayed[0] - delayed[1];
        const float a2 = delayed[2] + delayed[3], a3 = delayed[2] - delayed[3];
        const float a4 = delayed[4] + delayed[5], a5 = delayed[4] - delayed[5];
        const float a6 = delayed[6] + delayed[7], a7 = delayed[6] - delayed[7];

        const float b0 = a0 + a2, b1 = a1 + a3, b2 = a0 - a2, b3 = a1 - a3;
        const float b4 = a4 + a6, b5 = a5 + a7, b6 = a4 - a6, b7 = a5 - a7;

        constexpr float norm = 0.35355339059f;
        std::array<float, NumLines> mixed {
            (b0 + b4) * norm, (b1 + b5) * norm, (b2 + b6) * norm, (b3 + b7) * norm,
            (b0 - b4) * norm, (b1 - b5) * norm, (b2 - b6) * norm, (b3 - b7) * norm
        };

        const float sL = inL * 0.35f, sR = inR * 0.35f;
        lines[0][writePos] = mixed[0] + sL; lines[1][writePos] = mixed[1] + sR;
        lines[2][writePos] = mixed[2] + sR; lines[3][writePos] = mixed[3] + sL;
        lines[4][writePos] = mixed[4] - sL; lines[5][writePos] = mixed[5] - sR;
        lines[6][writePos] = mixed[6] - sR; lines[7][writePos] = mixed[7] - sL;
        writePos = (writePos + 1) & mask;

        outL = 0.32f * (delayed[0] - delayed[3] + delayed[5] - delayed[6]);
        outR = 0.32f * (delayed[1] - delayed[2] + delayed[4] - delayed[7]);
    }

private:
    double sr = 44100.0;
    size_t bufSize = 16384, mask = 16383, writePos = 0;
    float rt60 = 1.5f, damp = 0.35f, modDepthSamples = 8.0f;
    std::array<std::vector<float>, NumLines> lines;
    std::array<float, NumLines> baseDelaySamples {}, lineGain {}, lpState {}, dcIn {}, dcOut {}, lfoPhase {};
};

struct MultiMicRoomConfig
{
    int modeIdx = 1;       // 0: Reverb, 1: Re-Mic
    int sourceIdx = 0;     // 0: Vocals, 1: Drums, 2: Ensemble, 3: Monitors, 4: Guitar Mono, 5: Guitar Stereo, 6: Bass Amp
    int roomIdx = 0;       // 0: Studio A (Shangri-La), 1: Studio B (Canyon)
    int wallsLive = 1;     // 0: Tight, 1: Live
    bool phaseSafe = true;
    bool phaseInvert = false;
    bool mainStereo = false;
    bool deMudOn = false;
    int soloStandIdx = -1; // -1: Ninguno, 0..3: Solo atril específico
    float pairSpread = 1.0f;
    float deccaWidth = 1.0f;

    bool roomParallelCrush = false;
    float roomParallelAmt  = 0.45f;

    std::array<float, 4> micX    { 0.0f, -0.42f, 0.42f, 0.05f };
    std::array<float, 4> micY    { 0.18f, 0.54f, 0.76f, 0.86f };
    std::array<float, 4> micGain { 1.0f,  0.25f, 0.0f,  0.0f };
};

class AcousticEngine
{
public:
    void prepare (double sampleRate, int maxBlockSize)
    {
        sr = sampleRate > 0.0 ? sampleRate : 44100.0;

        size_t needed = static_cast<size_t> (sr * 0.25);
        erBufSize = 1;
        while (erBufSize < needed) erBufSize <<= 1;
        erMask = erBufSize - 1;

        erBufL.assign (erBufSize, 0.0f);
        erBufR.assign (erBufSize, 0.0f);
        erWritePos = 0;

        roomFdn.prepare (sr);
        chamberFdn.prepare (sr);
        roomAmbComp.prepare (sr);

        midPhaseHpf.prepare (sr);   midPhaseHpf.setHighPass (105.0f, 0.7071f);
        airPhaseHpf.prepare (sr);   airPhaseHpf.setHighPass (165.0f, 0.7071f);
        deccaPhaseHpf.prepare (sr); deccaPhaseHpf.setHighPass (90.0f, 0.7071f);

        deMudHpf.prepare (sr);      deMudHpf.setHighPass (165.0f, 0.7071f);
        deMudDip.prepare (sr);      deMudDip.setPeak (275.0f, -4.5f, 1.1f);

        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sr;
        spec.maximumBlockSize = static_cast<juce::uint32> (maxBlockSize);
        spec.numChannels = 2;

        preDelay.prepare (spec);
        customConvolver.prepare (spec);
        airLpL = airLpR = 0.0f;
    }

    void reset()
    {
        std::fill (erBufL.begin(), erBufL.end(), 0.0f);
        std::fill (erBufR.begin(), erBufR.end(), 0.0f);
        erWritePos = 0;
        roomFdn.reset(); chamberFdn.reset(); roomAmbComp.reset();
        midPhaseHpf.reset(); airPhaseHpf.reset(); deccaPhaseHpf.reset();
        deMudHpf.reset(); deMudDip.reset();
        preDelay.reset(); customConvolver.reset();
    }

    void loadCustomImpulseResponse (const juce::File& file)
    {
        if (! file.existsAsFile()) return;
        customConvolver.loadImpulseResponse (file,
                                             juce::dsp::Convolution::Stereo::yes,
                                             juce::dsp::Convolution::Trim::yes,
                                             0);
        customIRLoaded.store (true);
        loadedIRName = file.getFileNameWithoutExtension();
    }

    juce::String getLoadedIRName() const { return loadedIRName; }

    void processMultiMicRoom (juce::AudioBuffer<float>& buffer, const MultiMicRoomConfig& cfg)
    {
        if (buffer.getNumChannels() < 2) return;

        static constexpr std::array<float, 8> studioADelays { 29.3f, 37.1f, 43.7f, 53.3f, 61.9f, 71.1f, 83.3f, 97.7f };
        static constexpr std::array<float, 8> studioBDelays { 17.3f, 21.1f, 26.9f, 31.7f, 37.9f, 43.1f, 51.3f, 59.7f };

        const float baseRt60 = (cfg.roomIdx == 0) ? (cfg.wallsLive ? 1.85f : 1.15f)
                                                  : (cfg.wallsLive ? 1.05f : 0.62f);
        const float damp = cfg.wallsLive ? 0.26f : 0.56f;
        roomFdn.setProfile (cfg.roomIdx == 0 ? studioADelays : studioBDelays, baseRt60, damp, 0.35f);

        if (cfg.roomParallelCrush)
            roomAmbComp.configure (3, -24.0f, true, true);

        const float roomW = (cfg.roomIdx == 0) ? 16.0f : 10.5f;
        const float roomD = (cfg.roomIdx == 0) ? 12.0f : 8.5f;
        const float speed = 343.0f;
        const float wallRef = cfg.wallsLive ? 0.68f : 0.42f;

        std::array<float, 4> distMeters {};
        std::array<float, 4> panL {}, panR {};
        std::array<float, 4> delaySamples {};
        std::array<float, 4> activeGain {};

        for (size_t s = 0; s < 4; ++s)
        {
            const bool mutedBySolo = (cfg.soloStandIdx >= 0 && cfg.soloStandIdx != static_cast<int> (s));
            activeGain[s] = mutedBySolo ? 0.0f : cfg.micGain[s];

            const float mx = cfg.micX[s] * (roomW * 0.38f);
            const float my = juce::jmap (cfg.micY[s], 0.0f, 1.0f, 1.2f, roomD - 1.2f);
            distMeters[s] = std::hypot (mx, my);

            const float pan = std::clamp (cfg.micX[s] * 0.65f, -0.85f, 0.85f);
            const float ang = (pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
            panL[s] = std::cos (ang) * 1.414f;
            panR[s] = std::sin (ang) * 1.414f;

            // Retardo físico real por distancia entre atriles:
            // - Con Phase Safe OFF: retardo directo según distancia respecto al atril 1.
            // - Con Phase Safe ON: mantiene retardo real de distancia pero más contenido para evitar comb agresivo.
            const float relDist = std::max (0.0f, distMeters[s] - distMeters[0]);
            const float rawDelaySec = relDist / speed;
            const float delaySec = cfg.phaseSafe ? (rawDelaySec * 0.65f) : rawDelaySec;
            delaySamples[s] = delaySec * static_cast<float> (sr);
        }

        const size_t erNear = static_cast<size_t> (0.0145f * sr);
        const size_t erFar  = static_cast<size_t> (0.0195f * sr);
        const size_t erCeil = static_cast<size_t> (0.0245f * sr);
        const size_t erBack = static_cast<size_t> (0.0320f * sr);

        const float deccaSideOffset = juce::jmap (cfg.deccaWidth, 0.2f, 1.5f, 0.0008f, 0.0085f) * static_cast<float> (sr);
        const float pairW = std::clamp (cfg.pairSpread, 0.0f, 1.5f);
        const float deccaW = std::clamp (cfg.deccaWidth, 0.2f, 1.5f);
        const float polaritySign = cfg.phaseInvert ? -1.0f : 1.0f;

        auto* left  = buffer.getWritePointer (0);
        auto* right = buffer.getWritePointer (1);
        const int numSamples = buffer.getNumSamples();
        float dummyGr = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            const float dryL = left[i];
            const float dryR = right[i];

            float srcL = dryL, srcR = dryR;
            if (cfg.sourceIdx == 0 || cfg.sourceIdx == 4 || cfg.sourceIdx == 6)
            {
                srcL = srcR = 0.5f * (dryL + dryR);
            }
            else if (cfg.sourceIdx == 3)
            {
                srcL = dryL * 0.82f + dryR * 0.18f;
                srcR = dryR * 0.82f + dryL * 0.18f;
            }

            erBufL[erWritePos] = srcL;
            erBufR[erWritePos] = srcR;

            const float erL = (erBufL[(erWritePos - erNear) & erMask] * 0.42f
                             + erBufR[(erWritePos - erFar)  & erMask] * 0.32f
                             + erBufL[(erWritePos - erCeil) & erMask] * 0.30f
                             + erBufR[(erWritePos - erBack) & erMask] * 0.24f) * wallRef;

            const float erR = (erBufR[(erWritePos - erNear) & erMask] * 0.42f
                             + erBufL[(erWritePos - erFar)  & erMask] * 0.32f
                             + erBufR[(erWritePos - erCeil) & erMask] * 0.30f
                             + erBufL[(erWritePos - erBack) & erMask] * 0.24f) * wallRef;

            float fdnL = 0.0f, fdnR = 0.0f;
            roomFdn.processStereo (srcL + erL * 0.35f, srcR + erR * 0.35f, fdnL, fdnR);

            float mainL = 0.0f, mainR = 0.0f;
            float ambL  = 0.0f, ambR  = 0.0f;

            // ATRIL 1: MAIN (refleja distancia y retardo real al alejarse de la fuente)
            if (activeGain[0] > 0.001f)
            {
                const float g = activeGain[0];
                const float yDist = cfg.micY[0];
                const float dirW = juce::jmap (yDist, 0.0f, 1.0f, 0.94f, 0.35f);
                const float ambW = juce::jmap (yDist, 0.0f, 1.0f, 0.14f, 0.65f);

                const float mainDelay = (distMeters[0] / speed) * static_cast<float> (sr);
                float dirL = readFrac (erBufL, mainDelay);
                float dirR = readFrac (erBufR, mainDelay);

                float sL = dirL * dirW + (erL * 0.7f + fdnL * 0.5f) * ambW;
                float sR = dirR * dirW + (erR * 0.7f + fdnR * 0.5f) * ambW;

                if (! cfg.mainStereo)
                    sL = sR = 0.5f * (sL + sR);

                mainL += sL * panL[0] * g;
                mainR += sR * panR[0] * g;
            }

            // ATRIL 2: MID
            if (activeGain[1] > 0.001f)
            {
                const float g = activeGain[1] * 0.78f;
                const float yDist = cfg.micY[1];
                float mL = readFrac (erBufL, delaySamples[1]) * (1.0f - yDist * 0.55f) * 0.45f + erL * 0.85f + fdnL * 0.65f;
                float mR = readFrac (erBufR, delaySamples[1]) * (1.0f - yDist * 0.55f) * 0.45f + erR * 0.85f + fdnR * 0.65f;

                if (cfg.phaseSafe)
                {
                    mL = midPhaseHpf.processSample (0, mL);
                    mR = midPhaseHpf.processSample (1, mR);
                }

                const float mid  = 0.5f * (mL + mR);
                const float side = 0.5f * (mL - mR) * pairW;
                ambL += (mid + side) * panL[1] * g;
                ambR += (mid - side) * panR[1] * g;
            }

            // ATRIL 3: AIR
            if (activeGain[2] > 0.001f)
            {
                const float g = activeGain[2] * 0.82f;
                float aL = readFrac (erBufL, delaySamples[2]) * 0.25f + erL * 0.55f + fdnL * 1.15f;
                float aR = readFrac (erBufR, delaySamples[2]) * 0.25f + erR * 0.55f + fdnR * 1.15f;

                const float airCoeff = juce::jmap (cfg.micY[2], 0.0f, 1.0f, 0.12f, 0.45f);
                airLpL = aL * (1.0f - airCoeff) + airLpL * airCoeff;
                airLpR = aR * (1.0f - airCoeff) + airLpR * airCoeff;
                aL = airLpL; aR = airLpR;

                if (cfg.phaseSafe)
                {
                    aL = airPhaseHpf.processSample (0, aL);
                    aR = airPhaseHpf.processSample (1, aR);
                }

                const float mid  = 0.5f * (aL + aR);
                const float side = 0.5f * (aL - aR) * pairW;
                ambL += (mid + side) * panL[2] * g;
                ambR += (mid - side) * panR[2] * g;
            }

            // ATRIL 4: DECCA TREE
            if (activeGain[3] > 0.001f)
            {
                const float g = activeGain[3] * 0.72f;
                const float dBase = delaySamples[3];
                const float capC = 0.5f * (readFrac (erBufL, dBase) + readFrac (erBufR, dBase));
                const float capL = readFrac (erBufL, dBase + deccaSideOffset);
                const float capR = readFrac (erBufR, dBase + deccaSideOffset);

                float dL = (capL * 0.58f + capC * 0.34f) * 0.42f + erL * 0.65f + fdnL * 0.85f;
                float dR = (capR * 0.58f + capC * 0.34f) * 0.42f + erR * 0.65f + fdnR * 0.85f;

                if (cfg.phaseSafe)
                {
                    dL = deccaPhaseHpf.processSample (0, dL);
                    dR = deccaPhaseHpf.processSample (1, dR);
                }

                const float mid  = 0.5f * (dL + dR);
                const float side = 0.5f * (dL - dR) * deccaW;
                ambL += (mid + side) * panL[3] * g;
                ambR += (mid - side) * panR[3] * g;
            }

            if (cfg.deMudOn)
            {
                ambL = deMudDip.processSample (0, deMudHpf.processSample (0, ambL));
                ambR = deMudDip.processSample (1, deMudHpf.processSample (1, ambR));
            }

            if (cfg.roomParallelCrush && cfg.roomParallelAmt > 0.001f)
                roomAmbComp.processStereoSample (ambL, ambR, cfg.roomParallelAmt, dummyGr);

            const float mixL = mainL + ambL;
            const float mixR = mainR + ambR;

            if (cfg.modeIdx == 0)
            {
                left[i]  = dryL + mixL * 0.55f * polaritySign;
                right[i] = dryR + mixR * 0.55f * polaritySign;
            }
            else
            {
                left[i]  = mixL * polaritySign;
                right[i] = mixR * polaritySign;
            }

            erWritePos = (erWritePos + 1) & erMask;
        }
    }

    void processChamber (juce::AudioBuffer<float>& mainBuffer,
                         juce::AudioBuffer<float>& scratchBuffer,
                         bool enabled, int chamberIdx, float sendMix,
                         float decayNorm, float preDelayMs, float stereoWidth,
                         bool deMudOn, bool chamberParallelCrush, float parallelAmt)
    {
        if (! enabled || sendMix <= 0.001f || mainBuffer.getNumChannels() < 2) return;

        const int numSamples = mainBuffer.getNumSamples();
        scratchBuffer.setSize (2, numSamples, false, false, true);
        scratchBuffer.copyFrom (0, 0, mainBuffer, 0, 0, numSamples);
        scratchBuffer.copyFrom (1, 0, mainBuffer, 1, 0, numSamples);

        const float delaySamples = std::clamp (preDelayMs * 0.001f * static_cast<float> (sr), 0.0f, 47999.0f);
        preDelay.setDelay (delaySamples);

        for (int ch = 0; ch < 2; ++ch)
        {
            auto* data = scratchBuffer.getWritePointer (ch);
            for (int i = 0; i < numSamples; ++i)
            {
                preDelay.pushSample (ch, data[i]);
                data[i] = preDelay.popSample (ch);
            }
        }

        if (chamberIdx == 4 && customIRLoaded.load())
        {
            juce::dsp::AudioBlock<float> block (scratchBuffer);
            juce::dsp::ProcessContextReplacing<float> ctx (block);
            customConvolver.process (ctx);
        }
        else
        {
            static constexpr std::array<std::array<float, 8>, 4> chamberDelays {{
                { 23.1f, 29.9f, 38.3f, 46.7f, 56.3f, 67.1f, 76.9f, 89.3f },
                { 31.7f, 39.3f, 48.1f, 58.7f, 68.3f, 79.7f, 92.9f, 107.3f },
                { 35.9f, 44.3f, 54.7f, 66.1f, 78.7f, 91.9f, 106.3f, 121.7f },
                { 26.3f, 33.7f, 41.9f, 49.9f, 60.7f, 72.7f, 85.3f, 99.1f }
            }};

            static constexpr std::array<float, 4> naturalRt60 { 2.9f, 4.2f, 5.1f, 3.6f };
            static constexpr std::array<float, 4> chamberDamp { 0.24f, 0.34f, 0.44f, 0.30f };

            const size_t idx = static_cast<size_t> (chamberIdx == 4 ? 1 : std::clamp (chamberIdx, 0, 3));
            chamberFdn.setProfile (chamberDelays[idx], naturalRt60[idx] * decayNorm, chamberDamp[idx], 0.55f);

            auto* chL = scratchBuffer.getWritePointer (0);
            auto* chR = scratchBuffer.getWritePointer (1);
            for (int i = 0; i < numSamples; ++i)
            {
                float outL = 0.0f, outR = 0.0f;
                chamberFdn.processStereo (chL[i], chR[i], outL, outR);
                chL[i] = outL; chR[i] = outR;
            }
        }

        if (chamberParallelCrush && parallelAmt > 0.001f)
            roomAmbComp.configure (3, -24.0f, true, true);

        auto* sL = scratchBuffer.getWritePointer (0);
        auto* sR = scratchBuffer.getWritePointer (1);
        auto* mL = mainBuffer.getWritePointer (0);
        auto* mR = mainBuffer.getWritePointer (1);
        float dummyGr = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            float cL = sL[i];
            float cR = sR[i];
            if (deMudOn)
            {
                cL = deMudDip.processSample (0, deMudHpf.processSample (0, cL));
                cR = deMudDip.processSample (1, deMudHpf.processSample (1, cR));
            }
            if (chamberParallelCrush && parallelAmt > 0.001f)
                roomAmbComp.processStereoSample (cL, cR, parallelAmt, dummyGr);

            const float mid  = 0.5f * (cL + cR);
            const float side = 0.5f * (cL - cR) * stereoWidth;
            mL[i] += (mid + side) * sendMix;
            mR[i] += (mid - side) * sendMix;
        }
    }

private:
    inline float readFrac (const std::vector<float>& buf, float delaySamples) const noexcept
    {
        const float d = std::clamp (delaySamples, 0.0f, static_cast<float> (erBufSize - 4));
        const auto dInt = static_cast<size_t> (d);
        const float frac = d - static_cast<float> (dInt);
        const size_t idx0 = (erWritePos - dInt) & erMask;
        const size_t idx1 = (idx0 - 1) & erMask;
        return buf[idx0] * (1.0f - frac) + buf[idx1] * frac;
    }

    double sr = 44100.0;
    size_t erBufSize = 16384, erMask = 16383, erWritePos = 0;
    std::vector<float> erBufL, erBufR;
    float airLpL = 0.0f, airLpR = 0.0f;

    StereoBiquad midPhaseHpf, airPhaseHpf, deccaPhaseHpf;
    StereoBiquad deMudHpf, deMudDip;
    HadamardFDN roomFdn, chamberFdn;
    VintageCompressor roomAmbComp;

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> preDelay { 48000 };
    juce::dsp::Convolution customConvolver;
    std::atomic<bool> customIRLoaded { false };
    juce::String loadedIRName { "No IR Loaded" };
};
