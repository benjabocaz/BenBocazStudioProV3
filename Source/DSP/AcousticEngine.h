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
    int roomIdx = 0;       // 0: Studio A (Shangri-La), 1: Studio B (Canyon), 2: Volcano (~20m)
    int wallsLive = 1;     // 0: Tight / Closed, 1: Live / Open
    bool phaseSafe = true;
    bool phaseInvert = false;
    bool mainStereo = false;
    bool deMudOn = false;
    int soloStandIdx = -1; // -1: Ninguno, 0..3: Solo atril específico (0: MAIN, 1: ROOM PAIR, 2: FAR L, 3: FAR R)
    float pairSpread = 1.0f;
    float deccaWidth = 1.0f;

    bool roomParallelCrush = false;
    float roomParallelAmt  = 0.45f;

    std::array<float, 4> micX    { 0.0f, -0.42f, -0.52f, 0.52f };
    std::array<float, 4> micY    { 0.18f, 0.54f,  0.78f, 0.78f };
    std::array<float, 4> micGain { 1.0f,  0.25f,  0.0f,  0.0f };
};

class AcousticEngine
{
public:
    void prepare (double sampleRate, int maxBlockSize)
    {
        sr = sampleRate > 0.0 ? sampleRate : 44100.0;
        smoothCoeff = 1.0f - std::exp (-1.0f / (0.030f * static_cast<float> (sr)));
        smoothInitialized = false;

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
        chamberAmbComp.prepare (sr);

        midPhaseHpf.prepare (sr);   midPhaseHpf.setHighPass (115.0f, 0.7071f);
        airPhaseHpf.prepare (sr);   airPhaseHpf.setHighPass (155.0f, 0.7071f);
        deccaPhaseHpf.prepare (sr); deccaPhaseHpf.setHighPass (155.0f, 0.7071f);

        roomLowShelf.prepare (sr);
        roomMidPeak.prepare (sr);
        roomHighShelf.prepare (sr);
        roomBoomDip.prepare (sr);   roomBoomDip.setPeak (240.0f, -2.4f, 1.05f);
        roomSubHpf.prepare (sr);    roomSubHpf.setHighPass (48.0f, 0.7071f);
        deMudHpf.prepare (sr);      deMudHpf.setHighPass (165.0f, 0.7071f);
        deMudDip.prepare (sr);      deMudDip.setPeak (275.0f, -4.5f, 1.1f);

        chamberBoomDip.prepare (sr);  chamberBoomDip.setPeak (240.0f, -2.4f, 1.05f);
        chamberSubHpf.prepare (sr);   chamberSubHpf.setHighPass (48.0f, 0.7071f);
        chamberDeMudHpf.prepare (sr); chamberDeMudHpf.setHighPass (165.0f, 0.7071f);
        chamberDeMudDip.prepare (sr); chamberDeMudDip.setPeak (275.0f, -4.5f, 1.1f);

        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sr;
        spec.maximumBlockSize = static_cast<juce::uint32> (maxBlockSize);
        spec.numChannels = 2;

        preDelay.prepare (spec);
        customConvolver.prepare (spec);
        mainAirLpL = mainAirLpR = 0.0f;
        airLpL = airLpR = 0.0f;
        smLowDb = smMidDb = smHighDb = 0.0f;
    }

    void reset()
    {
        std::fill (erBufL.begin(), erBufL.end(), 0.0f);
        std::fill (erBufR.begin(), erBufR.end(), 0.0f);
        erWritePos = 0;
        smoothInitialized = false;
        roomFdn.reset(); chamberFdn.reset();
        roomAmbComp.reset(); chamberAmbComp.reset();
        midPhaseHpf.reset(); airPhaseHpf.reset(); deccaPhaseHpf.reset();
        roomLowShelf.reset(); roomMidPeak.reset(); roomHighShelf.reset();
        roomBoomDip.reset(); roomSubHpf.reset();
        deMudHpf.reset(); deMudDip.reset();
        chamberBoomDip.reset(); chamberSubHpf.reset();
        chamberDeMudHpf.reset(); chamberDeMudDip.reset();
        preDelay.reset(); customConvolver.reset();
        mainAirLpL = mainAirLpR = 0.0f;
        airLpL = airLpR = 0.0f;
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

        // 0: Studio A (Shangri-La) — ~12.5m
        // 1: Studio B (Canyon)     — ~8.5m (compacta y con pegada)
        // 2: Volcano               — ~20.0m (salón masivo y expansivo)
        static constexpr std::array<float, 8> studioADelays { 29.3f, 37.1f, 43.7f, 53.3f, 61.9f, 71.1f, 83.3f, 97.7f };
        static constexpr std::array<float, 8> studioBDelays { 17.3f, 21.1f, 26.9f, 31.7f, 37.9f, 43.1f, 51.3f, 59.7f };
        static constexpr std::array<float, 8> volcanoDelays { 38.3f, 47.9f, 58.7f, 70.1f, 82.9f, 96.7f, 111.7f, 128.3f };

        const int rIdx = std::clamp (cfg.roomIdx, 0, 2);
        const int wLive = (cfg.wallsLive != 0) ? 1 : 0;

        float targetLowDb = 0.0f, targetMidDb = 0.0f, targetHighDb = 0.0f;
        float lowFreq = 125.0f, midFreq = 1350.0f, midQ = 0.95f, highFreq = 8500.0f;

        if (rIdx == 0) // Studio A (Shangri-La)
        {
            lowFreq = 125.0f;  midFreq = 1350.0f; midQ = 0.95f; highFreq = 8500.0f;
            targetLowDb  = wLive ?  1.8f :  0.4f;
            targetMidDb  = wLive ?  0.9f : -0.6f;
            targetHighDb = wLive ?  1.4f : -1.2f;
            roomSubHpf.setHighPass (wLive ? 45.0f : 55.0f, 0.7071f);
            roomBoomDip.setPeak (235.0f, wLive ? -1.8f : -2.8f, 1.05f);
        }
        else if (rIdx == 1) // Studio B (Canyon)
        {
            lowFreq = 145.0f;  midFreq = 2150.0f; midQ = 1.05f; highFreq = 7200.0f;
            targetLowDb  = wLive ?  1.0f : -0.5f;
            targetMidDb  = wLive ?  2.1f :  0.8f;
            targetHighDb = wLive ?  0.4f : -2.2f;
            roomSubHpf.setHighPass (wLive ? 54.0f : 65.0f, 0.72f);
            roomBoomDip.setPeak (260.0f, wLive ? -2.2f : -3.4f, 1.15f);
        }
        else // Volcano (~20.0m expansivo)
        {
            if (wLive)
            {
                lowFreq = 110.0f;  midFreq = 880.0f;  midQ = 0.85f; highFreq = 9500.0f;
                targetLowDb  =  2.5f;
                targetMidDb  =  1.3f;
                targetHighDb =  1.8f;
                roomSubHpf.setHighPass (40.0f, 0.7071f);
                roomBoomDip.setPeak (220.0f, -2.0f, 1.0f);
            }
            else
            {
                lowFreq = 130.0f;  midFreq = 1650.0f; midQ = 1.00f; highFreq = 7500.0f;
                targetLowDb  =  0.7f;
                targetMidDb  =  0.5f;
                targetHighDb = -2.0f;
                roomSubHpf.setHighPass (58.0f, 0.75f);
                roomBoomDip.setPeak (250.0f, -3.2f, 1.15f);
            }
        }

        float baseRt60 = 1.85f;
        float damp = 0.35f;
        if (rIdx == 0)
        {
            baseRt60 = wLive ? 1.85f : 1.15f;
            damp     = wLive ? 0.26f : 0.56f;
        }
        else if (rIdx == 1)
        {
            baseRt60 = wLive ? 1.05f : 0.62f;
            damp     = wLive ? 0.34f : 0.64f;
        }
        else // Volcano 20m: gran profundidad en Live Wood, controlado en Tight
        {
            baseRt60 = wLive ? 2.65f : 1.05f;
            damp     = wLive ? 0.20f : 0.58f;
        }

        const auto& activeDelays = (rIdx == 0) ? studioADelays : ((rIdx == 1) ? studioBDelays : volcanoDelays);
        roomFdn.setProfile (activeDelays, baseRt60, damp, wLive ? 0.48f : 0.24f);

        // Sin grit agresivo a 1x en sala para mantener agudos limpios sin aliasing
        if (cfg.roomParallelCrush)
            roomAmbComp.configure (3, -24.0f, true, true, false);

        const float roomW = (rIdx == 0) ? 15.0f : ((rIdx == 1) ? 10.5f : 14.0f);
        const float roomD = (rIdx == 0) ? 12.5f : ((rIdx == 1) ? 8.5f  : 20.0f);
        const float speed = 343.0f;
        const float wallRef = (rIdx == 2) ? (wLive ? 0.80f : 0.42f)
                                          : (wLive ? 0.72f : 0.44f);

        const float farW = std::clamp (cfg.deccaWidth, 0.2f, 1.5f);

        std::array<float, 4> distMeters {};
        std::array<float, 4> targetPanL {}, targetPanR {};
        std::array<float, 4> targetDelay {};
        std::array<float, 4> targetGain {};
        std::array<float, 4> targetMicY {};
        std::array<float, 4> targetDirWeight {};
        std::array<float, 4> targetAmbWeight {};

        for (size_t s = 0; s < 4; ++s)
        {
            const bool mutedBySolo = (cfg.soloStandIdx >= 0 && cfg.soloStandIdx != static_cast<int> (s));
            targetGain[s] = mutedBySolo ? 0.0f : cfg.micGain[s];
            targetMicY[s] = cfg.micY[s];

            const float depthFactor = juce::jmap (cfg.micY[s], 0.0f, 1.0f, 0.55f, 1.0f);
            const float mx = cfg.micX[s] * (roomW * 0.42f) * depthFactor;
            const float my = juce::jmap (cfg.micY[s], 0.0f, 1.0f, 0.35f, roomD - 1.5f);
            distMeters[s] = std::hypot (mx, my);

            const float atten = 1.0f / (1.0f + 0.12f * std::max (0.0f, distMeters[s] - 0.5f));
            targetDirWeight[s] = atten;
            targetAmbWeight[s] = std::clamp (0.18f + (distMeters[s] / std::max (1.0f, roomD)) * 0.92f, 0.15f, 1.35f);

            // Para FAR L (2) y FAR R (3), FAR WIDE escala la apertura del paneo estéreo real
            const float panScale = (s >= 2) ? (0.78f * farW) : 0.72f;
            const float pan = std::clamp (cfg.micX[s] * panScale, -0.96f, 0.96f);
            const float ang = (pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
            targetPanL[s] = std::cos (ang) * 1.414f;
            targetPanR[s] = std::sin (ang) * 1.414f;
        }

        const float mainDelaySamples = (distMeters[0] / speed) * static_cast<float> (sr);
        targetDelay[0] = mainDelaySamples;

        for (size_t s = 1; s < 4; ++s)
        {
            const float relDist = std::max (0.0f, distMeters[s] - distMeters[0]);
            const float rawRelSec = relDist / speed;
            const float relSec = cfg.phaseSafe ? (rawRelSec * 0.65f) : rawRelSec;
            targetDelay[s] = mainDelaySamples + relSec * static_cast<float> (sr);
        }

        const float targetFarOffset = juce::jmap (farW, 0.2f, 1.5f, 0.0005f, 0.0065f) * static_cast<float> (sr);

        if (! smoothInitialized)
        {
            smDelay = targetDelay;
            smPanL  = targetPanL;
            smPanR  = targetPanR;
            smGain  = targetGain;
            smMicY  = targetMicY;
            smDirWeight = targetDirWeight;
            smAmbWeight = targetAmbWeight;
            smDeccaOffset = targetFarOffset;
            smLowDb = targetLowDb;
            smMidDb = targetMidDb;
            smHighDb = targetHighDb;
            roomLowShelf.setLowShelf (lowFreq, smLowDb);
            roomMidPeak.setPeak (midFreq, smMidDb, midQ);
            roomHighShelf.setHighShelf (highFreq, smHighDb);
            smoothInitialized = true;
        }

        const float roomScaleEr = (rIdx == 0) ? 1.12f : ((rIdx == 1) ? 0.82f : 1.55f);
        const float openErScale = (rIdx == 2 && ! wLive) ? 0.80f : 1.0f;
        const size_t erSideL = static_cast<size_t> (0.0112f * roomScaleEr * openErScale * sr);
        const size_t erSideR = static_cast<size_t> (0.0158f * roomScaleEr * openErScale * sr);
        const size_t erFloor = static_cast<size_t> (0.0194f * roomScaleEr * openErScale * sr);
        const size_t erCeil  = static_cast<size_t> (0.0242f * roomScaleEr * openErScale * sr);
        const size_t erBackL = static_cast<size_t> (0.0315f * roomScaleEr * openErScale * sr);
        const size_t erBackR = static_cast<size_t> (0.0368f * roomScaleEr * openErScale * sr);

        const float pairW = std::clamp (cfg.pairSpread, 0.0f, 1.5f);
        const float polaritySign = cfg.phaseInvert ? -1.0f : 1.0f;

        const float airMaxCoeff = (rIdx == 2) ? (wLive ? 0.42f : 0.62f)
                                              : ((rIdx == 1) ? 0.56f : 0.52f);
        const float airMinCoeff = (rIdx == 2) ? (wLive ? 0.12f : 0.24f) : 0.16f;

        auto* left  = buffer.getWritePointer (0);
        auto* right = buffer.getWritePointer (1);
        const int numSamples = buffer.getNumSamples();
        float dummyGr = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            for (size_t s = 0; s < 4; ++s)
            {
                smDelay[s]     += smoothCoeff * (targetDelay[s]     - smDelay[s]);
                smPanL[s]      += smoothCoeff * (targetPanL[s]      - smPanL[s]);
                smPanR[s]      += smoothCoeff * (targetPanR[s]      - smPanR[s]);
                smGain[s]      += smoothCoeff * (targetGain[s]      - smGain[s]);
                smMicY[s]      += smoothCoeff * (targetMicY[s]      - smMicY[s]);
                smDirWeight[s] += smoothCoeff * (targetDirWeight[s] - smDirWeight[s]);
                smAmbWeight[s] += smoothCoeff * (targetAmbWeight[s] - smAmbWeight[s]);
            }
            smDeccaOffset += smoothCoeff * (targetFarOffset - smDeccaOffset);

            const float nextLow  = smLowDb  + smoothCoeff * (targetLowDb  - smLowDb);
            const float nextMid  = smMidDb  + smoothCoeff * (targetMidDb  - smMidDb);
            const float nextHigh = smHighDb + smoothCoeff * (targetHighDb - smHighDb);
            if (std::abs (nextLow - smLowDb) > 0.002f || std::abs (nextMid - smMidDb) > 0.002f || std::abs (nextHigh - smHighDb) > 0.002f)
            {
                smLowDb = nextLow;
                smMidDb = nextMid;
                smHighDb = nextHigh;
                roomLowShelf.setLowShelf (lowFreq, smLowDb);
                roomMidPeak.setPeak (midFreq, smMidDb, midQ);
                roomHighShelf.setHighShelf (highFreq, smHighDb);
            }

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

            const float erL = (erBufL[(erWritePos - erSideL) & erMask] * 0.44f
                             + erBufR[(erWritePos - erSideR) & erMask] * 0.34f
                             + erBufL[(erWritePos - erFloor) & erMask] * 0.28f
                             + erBufR[(erWritePos - erCeil)  & erMask] * 0.24f
                             + erBufL[(erWritePos - erBackL) & erMask] * 0.22f
                             + erBufR[(erWritePos - erBackR) & erMask] * 0.16f) * wallRef;

            const float erR = (erBufR[(erWritePos - erSideR) & erMask] * 0.44f
                             + erBufL[(erWritePos - erSideL) & erMask] * 0.34f
                             + erBufR[(erWritePos - erFloor) & erMask] * 0.28f
                             + erBufL[(erWritePos - erCeil)  & erMask] * 0.24f
                             + erBufR[(erWritePos - erBackR) & erMask] * 0.22f
                             + erBufL[(erWritePos - erBackL) & erMask] * 0.16f) * wallRef;

            float fdnL = 0.0f, fdnR = 0.0f;
            roomFdn.processStereo (srcL + erL * 0.38f, srcR + erR * 0.38f, fdnL, fdnR);

            float mainL = 0.0f, mainR = 0.0f;
            float ambL  = 0.0f, ambR  = 0.0f;

            // ATRIL 1: MAIN
            if (smGain[0] > 0.0005f)
            {
                const float g = smGain[0] * smDirWeight[0];
                const float yDist = smMicY[0];
                const float dirW = juce::jmap (yDist, 0.0f, 1.0f, 0.98f, 0.30f);
                const float ambW = juce::jmap (yDist, 0.0f, 1.0f, 0.06f, 0.72f) * smAmbWeight[0];

                float dirL = readFrac (erBufL, smDelay[0]);
                float dirR = readFrac (erBufR, smDelay[0]);

                const float mainAirCoeff = juce::jmap (yDist, 0.0f, 1.0f, 0.0f, 0.34f);
                mainAirLpL = dirL * (1.0f - mainAirCoeff) + mainAirLpL * mainAirCoeff;
                mainAirLpR = dirR * (1.0f - mainAirCoeff) + mainAirLpR * mainAirCoeff;
                dirL = mainAirLpL;
                dirR = mainAirLpR;

                float sL = dirL * dirW + (erL * 0.72f + fdnL * 0.52f) * ambW;
                float sR = dirR * dirW + (erR * 0.72f + fdnR * 0.52f) * ambW;

                if (! cfg.mainStereo)
                    sL = sR = 0.5f * (sL + sR);

                mainL += sL * smPanL[0] * g;
                mainR += sR * smPanR[0] * g;
            }

            // ATRIL 2: ROOM PAIR (par estéreo de sala con PAIR WIDE)
            if (smGain[1] > 0.0005f)
            {
                const float g = smGain[1] * 0.80f * smDirWeight[1];
                const float yDist = smMicY[1];
                const float ambW = smAmbWeight[1];
                float mL = readFrac (erBufL, smDelay[1]) * (1.0f - yDist * 0.55f) * 0.35f + (erL * 0.75f + fdnL * 0.95f) * ambW;
                float mR = readFrac (erBufR, smDelay[1]) * (1.0f - yDist * 0.55f) * 0.35f + (erR * 0.75f + fdnR * 0.95f) * ambW;

                const float pairAirCoeff = juce::jmap (yDist, 0.0f, 1.0f, airMinCoeff * 0.85f, airMaxCoeff * 0.9f);
                airLpL = mL * (1.0f - pairAirCoeff) + airLpL * pairAirCoeff;
                airLpR = mR * (1.0f - pairAirCoeff) + airLpR * pairAirCoeff;
                mL = airLpL;
                mR = airLpR;

                if (cfg.phaseSafe)
                {
                    mL = midPhaseHpf.processSample (0, mL);
                    mR = midPhaseHpf.processSample (1, mR);
                }

                const float mid  = 0.5f * (mL + mR);
                const float side = 0.5f * (mL - mR) * pairW;
                ambL += (mid + side) * smPanL[1] * g;
                ambR += (mid - side) * smPanR[1] * g;
            }

            // ATRIL 3: FAR L — Cápsula MONO real ubicada en el costado izquierdo
            if (smGain[2] > 0.0005f)
            {
                const float g = smGain[2] * 0.78f * smDirWeight[2];
                const float ambW = smAmbWeight[2];
                const float dFarL = smDelay[2] + smDeccaOffset * 0.35f;
                const float dirMonoL = 0.65f * readFrac (erBufL, dFarL) + 0.35f * readFrac (erBufR, dFarL);
                float capsuleL = dirMonoL * 0.22f + (erL * 0.76f + fdnL * 0.96f) * ambW;

                if (cfg.phaseSafe)
                    capsuleL = airPhaseHpf.processSample (0, capsuleL);

                // Señal mono real paneada en el campo estéreo según posición X + FAR WIDE
                ambL += capsuleL * smPanL[2] * g;
                ambR += capsuleL * smPanR[2] * g;
            }

            // ATRIL 4: FAR R — Cápsula MONO real ubicada en el costado derecho
            if (smGain[3] > 0.0005f)
            {
                const float g = smGain[3] * 0.78f * smDirWeight[3];
                const float ambW = smAmbWeight[3];
                const float dFarR = smDelay[3] + smDeccaOffset * 0.65f;
                const float dirMonoR = 0.35f * readFrac (erBufL, dFarR) + 0.65f * readFrac (erBufR, dFarR);
                float capsuleR = dirMonoR * 0.22f + (erR * 0.76f + fdnR * 0.96f) * ambW;

                if (cfg.phaseSafe)
                    capsuleR = deccaPhaseHpf.processSample (1, capsuleR);

                ambL += capsuleR * smPanL[3] * g;
                ambR += capsuleR * smPanR[3] * g;
            }

            ambL = roomHighShelf.processSample (0, roomMidPeak.processSample (0, roomLowShelf.processSample (0,
                   roomBoomDip.processSample (0, roomSubHpf.processSample (0, ambL)))));
            ambR = roomHighShelf.processSample (1, roomMidPeak.processSample (1, roomLowShelf.processSample (1,
                   roomBoomDip.processSample (1, roomSubHpf.processSample (1, ambR)))));

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
                left[i]  = (dryL + mixL * 0.55f * polaritySign) * 0.92f;
                right[i] = (dryR + mixR * 0.55f * polaritySign) * 0.92f;
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
        if (scratchBuffer.getNumChannels() < 2 || scratchBuffer.getNumSamples() < numSamples)
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
            chamberAmbComp.configure (3, -24.0f, true, true, false);

        auto* sL = scratchBuffer.getWritePointer (0);
        auto* sR = scratchBuffer.getWritePointer (1);
        auto* mL = mainBuffer.getWritePointer (0);
        auto* mR = mainBuffer.getWritePointer (1);
        float dummyGr = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            float cL = chamberBoomDip.processSample (0, chamberSubHpf.processSample (0, sL[i]));
            float cR = chamberBoomDip.processSample (1, chamberSubHpf.processSample (1, sR[i]));

            if (deMudOn)
            {
                cL = chamberDeMudDip.processSample (0, chamberDeMudHpf.processSample (0, cL));
                cR = chamberDeMudDip.processSample (1, chamberDeMudHpf.processSample (1, cR));
            }
            if (chamberParallelCrush && parallelAmt > 0.001f)
                chamberAmbComp.processStereoSample (cL, cR, parallelAmt, dummyGr);

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
    float mainAirLpL = 0.0f, mainAirLpR = 0.0f;
    float airLpL = 0.0f, airLpR = 0.0f;

    float smLowDb = 0.0f, smMidDb = 0.0f, smHighDb = 0.0f;

    float smoothCoeff = 0.001f;
    bool smoothInitialized = false;
    std::array<float, 4> smDelay {}, smPanL {}, smPanR {}, smGain {}, smMicY {};
    std::array<float, 4> smDirWeight {}, smAmbWeight {};
    float smDeccaOffset = 0.0f;

    StereoBiquad midPhaseHpf, airPhaseHpf, deccaPhaseHpf;
    StereoBiquad roomLowShelf, roomMidPeak, roomHighShelf;
    StereoBiquad roomBoomDip, roomSubHpf, deMudHpf, deMudDip;
    StereoBiquad chamberBoomDip, chamberSubHpf, chamberDeMudHpf, chamberDeMudDip;
    HadamardFDN roomFdn, chamberFdn;
    VintageCompressor roomAmbComp, chamberAmbComp;

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> preDelay { 48000 };
    juce::dsp::Convolution customConvolver;
    std::atomic<bool> customIRLoaded { false };
    juce::String loadedIRName { "No IR Loaded" };
};
