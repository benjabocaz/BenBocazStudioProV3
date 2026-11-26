#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>
#include <algorithm>

namespace BenBocazDSP
{

struct MicState
{
    float x = 0.5f;
    float y = 0.25f;
    float gain = 1.0f;
    bool solo = false;
};

class AcousticEngine
{
public:
    enum RoomType { VOLCANO = 0, SHANGRILA = 1, CANYON = 2 };

    void prepare(double sampleRate, int maxBlockSize)
    {
        fs = std::max(8000.0, sampleRate);
        maxBlock = std::max(64, maxBlockSize);
        lastMicDistM.fill(-1.0f);

        const int maxDelaySamples = static_cast<int>(fs * 0.120) + 16;
        for (int m = 0; m < 4; ++m)
        {
            delayL[m].prepare({ fs, static_cast<juce::uint32>(maxBlock), 1 });
            delayR[m].prepare({ fs, static_cast<juce::uint32>(maxBlock), 1 });
            delayL[m].setMaximumDelayInSamples(maxDelaySamples);
            delayR[m].setMaximumDelayInSamples(maxDelaySamples);

            airLpfL[m].reset();
            airLpfR[m].reset();

            distSmooth[m].reset(fs, 0.030);
            gainSmooth[m].reset(fs, 0.025);
            panSmooth[m].reset(fs, 0.025);
        }

        roomRevL.setSampleRate(fs);
        roomRevR.setSampleRate(fs);
        coupledStoneRev.setSampleRate(fs);
        chamberRev.setSampleRate(fs);

        coupledPreDelayL.prepare({ fs, static_cast<juce::uint32>(maxBlock), 1 });
        coupledPreDelayR.prepare({ fs, static_cast<juce::uint32>(maxBlock), 1 });
        coupledPreDelayL.setMaximumDelayInSamples(static_cast<int>(fs * 0.080) + 16);
        coupledPreDelayR.setMaximumDelayInSamples(static_cast<int>(fs * 0.080) + 16);

        chamberPreDelayL.prepare({ fs, static_cast<juce::uint32>(maxBlock), 1 });
        chamberPreDelayR.prepare({ fs, static_cast<juce::uint32>(maxBlock), 1 });
        chamberPreDelayL.setMaximumDelayInSamples(static_cast<int>(fs * 0.150) + 16);
        chamberPreDelayR.setMaximumDelayInSamples(static_cast<int>(fs * 0.150) + 16);

        roomBusBuffer.setSize(2, maxBlock);
        tailBufferL.setSize(2, maxBlock);
        tailBufferR.setSize(2, maxBlock);
        coupledBuffer.setSize(2, maxBlock);
        chamberBuffer.setSize(2, maxBlock);

        reset();
        dirtyRoom = true;
    }

    void reset()
    {
        for (int m = 0; m < 4; ++m)
        {
            delayL[m].reset(); delayR[m].reset();
            airLpfL[m].reset(); airLpfR[m].reset();
        }
        for (int c = 0; c < 2; ++c)
        {
            tailLowFilter[c].reset();
            tailHighFilter[c].reset();
        }
        roomRevL.reset();
        roomRevR.reset();
        coupledStoneRev.reset();
        coupledPreDelayL.reset();
        coupledPreDelayR.reset();
        chamberRev.reset();
        chamberPreDelayL.reset();
        chamberPreDelayR.reset();
    }

    static float computeDistanceMeters(int roomIndex, float x, float y) noexcept
    {
        const float dx = (x - 0.5f);
        const float dy = std::clamp(y, 0.0f, 1.0f);

        if (roomIndex == VOLCANO)
        {
            const float depthM = 1.2f + 10.3f * dy + 10.0f * (dy * dy * dy);
            const float widthM = dx * 12.0f;
            return std::sqrt(depthM * depthM + widthM * widthM);
        }
        if (roomIndex == SHANGRILA)
        {
            const float depthM = 1.0f + 10.5f * dy;
            const float widthM = dx * 7.5f;
            return std::sqrt(depthM * depthM + widthM * widthM);
        }
        const float depthM = 0.9f + 7.6f * dy;
        const float widthM = dx * 5.8f;
        return std::sqrt(depthM * depthM + widthM * widthM);
    }

    void processRoom(juce::AudioBuffer<float>& ioBuffer,
                     juce::AudioBuffer<float>& outRoomOnlyBuffer,
                     bool roomEnabled,
                     int roomIndex,
                     int wallsIndex,
                     bool mainStereo,
                     bool phaseSafe,
                     float pairWide,
                     float farWide,
                     const std::array<MicState, 4>& mics)
    {
        const int numSamples = ioBuffer.getNumSamples();
        if (numSamples == 0) return;

        if (!roomEnabled)
        {
            outRoomOnlyBuffer.setSize(2, numSamples, false, false, true);
            outRoomOnlyBuffer.clear();
            return;
        }

        if (roomIndex != lastRoom || wallsIndex != lastWalls)
        {
            lastRoom = roomIndex;
            lastWalls = wallsIndex;
            dirtyRoom = true;
        }

        if (dirtyRoom)
            updateRoomProfile();

        const bool anySolo = mics[0].solo || mics[1].solo || mics[2].solo || mics[3].solo;
        const bool volcanoGatesOpen = (roomIndex == VOLCANO && wallsIndex == 0);

        tailBufferL.setSize(2, numSamples, false, false, true);
        tailBufferR.setSize(2, numSamples, false, false, true);
        tailBufferL.copyFrom(0, 0, ioBuffer, 0, 0, numSamples);
        tailBufferL.copyFrom(1, 0, ioBuffer, ioBuffer.getNumChannels() > 1 ? 1 : 0, 0, numSamples);
        tailBufferR.copyFrom(0, 0, tailBufferL, 1, 0, numSamples);
        tailBufferR.copyFrom(1, 0, tailBufferL, 0, 0, numSamples);

        roomRevL.processStereo(tailBufferL.getWritePointer(0), tailBufferL.getWritePointer(1), numSamples);
        roomRevR.processStereo(tailBufferR.getWritePointer(0), tailBufferR.getWritePointer(1), numSamples);

        // Perfil de materiales por bandas (Canyon seco, Shangri-La asimetrico vidrio L / madera R, Volcano piedra)
        auto* tLL = tailBufferL.getWritePointer(0);
        auto* tLR = tailBufferL.getWritePointer(1);
        auto* tRL = tailBufferR.getWritePointer(0);
        auto* tRR = tailBufferR.getWritePointer(1);
        for (int i = 0; i < numSamples; ++i)
        {
            tLL[i] = tailHighFilter[0].processSample(tailLowFilter[0].processSample(tLL[i]));
            tLR[i] = tailHighFilter[1].processSample(tailLowFilter[1].processSample(tLR[i]));
            tRL[i] = tailHighFilter[0].processSample(tailLowFilter[0].processSample(tRL[i]));
            tRR[i] = tailHighFilter[1].processSample(tailLowFilter[1].processSample(tRR[i]));
        }

        // Camara trasera de piedra volcanica acoplada (solo Volcano Gates Open)
        const float* cL = nullptr;
        const float* cR = nullptr;
        if (volcanoGatesOpen)
        {
            coupledBuffer.setSize(2, numSamples, false, false, true);
            const float preSmp = static_cast<float>(fs) * 0.028f; // ~28ms de distancia a la camara trasera
            const float* srcL = ioBuffer.getReadPointer(0);
            const float* srcR = ioBuffer.getNumChannels() > 1 ? ioBuffer.getReadPointer(1) : srcL;
            auto* dstL = coupledBuffer.getWritePointer(0);
            auto* dstR = coupledBuffer.getWritePointer(1);

            for (int i = 0; i < numSamples; ++i)
            {
                coupledPreDelayL.pushSample(0, srcL[i]);
                coupledPreDelayR.pushSample(0, srcR[i]);
                dstL[i] = coupledPreDelayL.popSample(0, preSmp);
                dstR[i] = coupledPreDelayR.popSample(0, preSmp);
            }
            coupledStoneRev.processStereo(dstL, dstR, numSamples);
            cL = dstL;
            cR = dstR;
        }

        const float maxRoomM = (roomIndex == VOLCANO) ? 21.5f : (roomIndex == SHANGRILA ? 11.5f : 8.5f);
        for (int m = 0; m < 4; ++m)
        {
            const float distM = computeDistanceMeters(roomIndex, mics[m].x, mics[m].y);
            const float effGain = (anySolo && !mics[m].solo) ? 0.0f : mics[m].gain;

            distSmooth[m].setTargetValue(distM);
            gainSmooth[m].setTargetValue(effGain);

            float pan = (mics[m].x - 0.5f) * 2.0f;
            if (m == 1) pan *= (0.4f + 0.6f * pairWide);
            if (m == 2 || m == 3) pan = std::clamp(pan * (0.6f + 0.6f * farWide), -1.0f, 1.0f);
            panSmooth[m].setTargetValue(pan);

            if (std::abs(distM - lastMicDistM[m]) > 0.25f)
            {
                lastMicDistM[m] = distM;
                const float normD = std::clamp(distM / maxRoomM, 0.0f, 1.0f);
                const float lpfHz = 18500.0f - normD * (roomIndex == VOLCANO ? 11000.0f : (roomIndex == SHANGRILA ? 7500.0f : 9500.0f));
                auto c = juce::dsp::IIR::Coefficients<float>::makeLowPass(fs, std::clamp(lpfHz, 3500.0f, 19000.0f), 0.707f);
                airLpfL[m].coefficients = c;
                airLpfR[m].coefficients = c;
            }
        }

        roomBusBuffer.setSize(2, numSamples, false, false, true);
        roomBusBuffer.clear();
        outRoomOnlyBuffer.setSize(2, numSamples, false, false, true);
        outRoomOnlyBuffer.clear();

        const float* inL = ioBuffer.getReadPointer(0);
        const float* inR = ioBuffer.getNumChannels() > 1 ? ioBuffer.getReadPointer(1) : inL;
        auto* outL = roomBusBuffer.getWritePointer(0);
        auto* outR = roomBusBuffer.getWritePointer(1);
        auto* roomOnlyL = outRoomOnlyBuffer.getWritePointer(0);
        auto* roomOnlyR = outRoomOnlyBuffer.getWritePointer(1);

        const float samplesPerMeter = static_cast<float>(fs) / 343.0f;
        const float gateTailBoost = volcanoGatesOpen ? 1.25f : 1.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            const float dryL = inL[i];
            const float dryR = inR[i];
            const float dryMono = 0.5f * (dryL + dryR);

            float sumL = 0.0f, sumR = 0.0f;
            float ambOnlyL = 0.0f, ambOnlyR = 0.0f;

            for (int m = 0; m < 4; ++m)
            {
                const float g = gainSmooth[m].getNextValue();
                const float distM = distSmooth[m].getNextValue();
                const float pan = panSmooth[m].getNextValue();

                float delaySamples = distM * samplesPerMeter;
                if (phaseSafe)
                    delaySamples = (m == 0) ? 0.0f : std::min(delaySamples * 0.25f, static_cast<float>(fs) * 0.012f);

                delayL[m].pushSample(0, (m == 0 && !mainStereo) ? dryMono : dryL);
                delayR[m].pushSample(0, (m == 0 && !mainStereo) ? dryMono : dryR);

                if (g < 0.0005f) continue;

                float dL = airLpfL[m].processSample(delayL[m].popSample(0, delaySamples));
                float dR = airLpfR[m].processSample(delayR[m].popSample(0, delaySamples));

                const float normD = std::clamp(distM / maxRoomM, 0.0f, 1.0f);
                const float directRatio = std::clamp(1.0f - normD * (m >= 2 ? 0.78f : 0.55f), 0.15f, 1.0f);
                const float wetRatio = (0.18f + normD * (m >= 2 ? 0.95f : 0.65f)) * gateTailBoost;

                const float stoneL = volcanoGatesOpen ? (cL[i] * (m >= 2 ? 0.42f : 0.16f)) : 0.0f;
                const float stoneR = volcanoGatesOpen ? (cR[i] * (m >= 2 ? 0.42f : 0.16f)) : 0.0f;

                float sigL = 0.0f, sigR = 0.0f;
                if (m == 0)
                {
                    sigL = dL * directRatio + tLL[i] * (wetRatio * 0.30f) + stoneL * 0.35f;
                    sigR = dR * directRatio + tLR[i] * (wetRatio * 0.30f) + stoneR * 0.35f;
                }
                else if (m == 1)
                {
                    sigL = dL * (directRatio * 0.65f) + tLL[i] * (wetRatio * 0.70f) + stoneL * 0.75f;
                    sigR = dR * (directRatio * 0.65f) + tLR[i] * (wetRatio * 0.70f) + stoneR * 0.75f;
                }
                else if (m == 2)
                {
                    const float monoFarL = 0.5f * (dL + dR) * (directRatio * 0.45f) + (0.65f * tLL[i] + 0.35f * tLR[i]) * wetRatio + stoneL;
                    sigL = sigR = monoFarL;
                }
                else
                {
                    const float monoFarR = 0.5f * (dL + dR) * (directRatio * 0.45f) + (0.65f * tRR[i] + 0.35f * tRL[i]) * wetRatio + stoneR;
                    sigL = sigR = monoFarR;
                }

                const float angle = (pan * 0.5f + 0.5f) * juce::MathConstants<float>::halfPi;
                const float outMicL = sigL * std::cos(angle) * g * 1.15f;
                const float outMicR = sigR * std::sin(angle) * g * 1.15f;

                sumL += outMicL;
                sumR += outMicR;
                if (m >= 1) { ambOnlyL += outMicL; ambOnlyR += outMicR; }
            }

            outL[i] = sumL; outR[i] = sumR;
            roomOnlyL[i] = ambOnlyL; roomOnlyR[i] = ambOnlyR;
        }

        ioBuffer.copyFrom(0, 0, roomBusBuffer, 0, 0, numSamples);
        if (ioBuffer.getNumChannels() > 1)
            ioBuffer.copyFrom(1, 0, roomBusBuffer, 1, 0, numSamples);
    }

    void processChamber(juce::AudioBuffer<float>& ioBuffer,
                        bool chamberEnabled,
                        int chamberModel,
                        float send01,
                        float decay01,
                        float preDelayMs,
                        float width01)
    {
        if (!chamberEnabled || send01 < 0.001f || ioBuffer.getNumSamples() == 0)
            return;

        const int numSamples = ioBuffer.getNumSamples();
        chamberBuffer.setSize(2, numSamples, false, false, true);

        const float preSamples = std::clamp(preDelayMs * 0.001f * static_cast<float>(fs), 0.0f, static_cast<float>(fs) * 0.140f);
        const float* inL = ioBuffer.getReadPointer(0);
        const float* inR = ioBuffer.getNumChannels() > 1 ? ioBuffer.getReadPointer(1) : inL;
        auto* cL = chamberBuffer.getWritePointer(0);
        auto* cR = chamberBuffer.getWritePointer(1);

        for (int i = 0; i < numSamples; ++i)
        {
            chamberPreDelayL.pushSample(0, inL[i]);
            chamberPreDelayR.pushSample(0, inR[i]);
            cL[i] = chamberPreDelayL.popSample(0, preSamples);
            cR[i] = chamberPreDelayR.popSample(0, preSamples);
        }

        juce::Reverb::Parameters p;
        p.roomSize   = std::clamp(decay01 * (chamberModel == 2 ? 0.92f : (chamberModel == 1 ? 0.80f : 0.68f)), 0.15f, 0.98f);
        p.damping    = (chamberModel == 0) ? 0.45f : (chamberModel == 1 ? 0.30f : 0.38f);
        p.wetLevel   = 1.0f;
        p.dryLevel   = 0.0f;
        p.width      = std::clamp(width01, 0.0f, 1.0f);
        p.freezeMode = 0.0f;
        chamberRev.setParameters(p);

        chamberRev.processStereo(cL, cR, numSamples);

        const float wetGain = send01 * 0.65f;
        ioBuffer.addFrom(0, 0, chamberBuffer, 0, 0, numSamples, wetGain);
        if (ioBuffer.getNumChannels() > 1)
            ioBuffer.addFrom(1, 0, chamberBuffer, 1, 0, numSamples, wetGain);
    }

private:
    void updateRoomProfile()
    {
        juce::Reverb::Parameters pL, pR;
        const bool isOpen = (lastWalls == 0);

        if (lastRoom == VOLCANO)
        {
            // Piedra volcanica + madera ranurada: sosten profundo en graves y difusion alta
            pL.roomSize = isOpen ? 0.88f : 0.68f;
            pL.damping  = isOpen ? 0.26f : 0.50f;
            pL.width    = 1.0f;

            auto lo = juce::dsp::IIR::Coefficients<float>::makeLowShelf(fs, 210.0f, 0.707f, juce::Decibels::decibelsToGain(isOpen ? 2.2f : 1.0f));
            auto hi = juce::dsp::IIR::Coefficients<float>::makeHighShelf(fs, 5500.0f, 0.707f, juce::Decibels::decibelsToGain(isOpen ? 1.0f : -1.2f));
            tailLowFilter[0].coefficients  = tailLowFilter[1].coefficients  = lo;
            tailHighFilter[0].coefficients = tailHighFilter[1].coefficients = hi;

            juce::Reverb::Parameters stoneP;
            stoneP.roomSize = 0.94f;
            stoneP.damping  = 0.22f;
            stoneP.wetLevel = 1.0f;
            stoneP.dryLevel = 0.0f;
            stoneP.width    = 1.0f;
            stoneP.freezeMode = 0.0f;
            coupledStoneRev.setParameters(stoneP);
        }
        else if (lastRoom == SHANGRILA)
        {
            // Asimetria fisica: Ventanal vidrio a la izquierda (+brillo) vs Estanterias/madera a la derecha (+calidez)
            pL.roomSize = isOpen ? 0.66f : 0.48f;
            pL.damping  = isOpen ? 0.38f : 0.58f;
            pL.width    = 0.94f;

            auto lo = juce::dsp::IIR::Coefficients<float>::makeLowShelf(fs, 180.0f, 0.707f, juce::Decibels::decibelsToGain(-0.8f));
            tailLowFilter[0].coefficients = tailLowFilter[1].coefficients = lo;

            // Canal 0 (Izquierda - Vidrio vivo) vs Canal 1 (Derecha - Estanterias/difusion suave)
            tailHighFilter[0].coefficients = juce::dsp::IIR::Coefficients<float>::makeHighShelf(fs, 4500.0f, 0.707f, juce::Decibels::decibelsToGain(isOpen ? 2.6f : 1.2f));
            tailHighFilter[1].coefficients = juce::dsp::IIR::Coefficients<float>::makeHighShelf(fs, 4200.0f, 0.707f, juce::Decibels::decibelsToGain(isOpen ? -1.4f : -2.4f));
        }
        else // CANYON
        {
            // Techo artesonado + trampas de graves de esquina: graves secos/amarrados y agudos sedosos
            pL.roomSize = isOpen ? 0.42f : 0.26f;
            pL.damping  = isOpen ? 0.58f : 0.76f;
            pL.width    = 0.84f;

            auto lo = juce::dsp::IIR::Coefficients<float>::makeHighPass(fs, isOpen ? 140.0f : 180.0f, 0.707f);
            auto hi = juce::dsp::IIR::Coefficients<float>::makeLowPass(fs, isOpen ? 6800.0f : 5200.0f, 0.707f);
            tailLowFilter[0].coefficients  = tailLowFilter[1].coefficients  = lo;
            tailHighFilter[0].coefficients = tailHighFilter[1].coefficients = hi;
        }

        pL.wetLevel = 1.0f;
        pL.dryLevel = 0.0f;
        pL.freezeMode = 0.0f;

        pR = pL;
        pR.roomSize = std::clamp(pL.roomSize * 0.97f, 0.20f, 0.96f);
        pR.damping  = std::clamp(pL.damping + 0.04f, 0.10f, 0.90f);

        roomRevL.setParameters(pL);
        roomRevR.setParameters(pR);
        dirtyRoom = false;
    }

    double fs = 48000.0;
    int maxBlock = 512;
    int lastRoom = -1;
    int lastWalls = -1;
    bool dirtyRoom = true;

    std::array<juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>, 4> delayL, delayR;
    std::array<juce::dsp::IIR::Filter<float>, 4> airLpfL, airLpfR;
    std::array<float, 4> lastMicDistM { -1.0f, -1.0f, -1.0f, -1.0f };

    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, 4> distSmooth, gainSmooth, panSmooth;

    juce::dsp::IIR::Filter<float> tailLowFilter[2], tailHighFilter[2];
    juce::Reverb roomRevL, roomRevR, coupledStoneRev, chamberRev;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> coupledPreDelayL, coupledPreDelayR;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> chamberPreDelayL, chamberPreDelayR;

    juce::AudioBuffer<float> roomBusBuffer, tailBufferL, tailBufferR, coupledBuffer, chamberBuffer;
};

} // namespace BenBocazDSP
