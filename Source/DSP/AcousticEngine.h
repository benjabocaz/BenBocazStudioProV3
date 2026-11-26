#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>
#include <algorithm>

namespace BenBocazDSP
{

struct MicState
{
    float x = 0.5f;      // 0.0 (izq) .. 1.0 (der)
    float y = 0.25f;     // 0.0 (cerca fuente) .. 1.0 (fondo sala)
    float gain = 1.0f;   // 0.0 .. 1.0
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

        // Buffer de retardo de hasta 120 ms (cubre > 35 metros a cualquier fs)
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
        chamberRev.setSampleRate(fs);

        chamberPreDelayL.prepare({ fs, static_cast<juce::uint32>(maxBlock), 1 });
        chamberPreDelayR.prepare({ fs, static_cast<juce::uint32>(maxBlock), 1 });
        chamberPreDelayL.setMaximumDelayInSamples(static_cast<int>(fs * 0.150) + 16);
        chamberPreDelayR.setMaximumDelayInSamples(static_cast<int>(fs * 0.150) + 16);

        roomBusBuffer.setSize(2, maxBlock);
        tailBufferL.setSize(2, maxBlock);
        tailBufferR.setSize(2, maxBlock);
        chamberBuffer.setSize(2, maxBlock);

        reset();
        dirtyRoom = true;
    }

    void reset()
    {
        for (int m = 0; m < 4; ++m)
        {
            delayL[m].reset();
            delayR[m].reset();
            airLpfL[m].reset();
            airLpfR[m].reset();
        }
        roomRevL.reset();
        roomRevR.reset();
        chamberRev.reset();
        chamberPreDelayL.reset();
        chamberPreDelayR.reset();
    }

    // Distancia real en metros segun sala y coordenada Y (curva no lineal en Volcano)
    static float computeDistanceMeters(int roomIndex, float x, float y) noexcept
    {
        const float dx = (x - 0.5f);
        const float dy = std::clamp(y, 0.0f, 1.0f);

        if (roomIndex == VOLCANO)
        {
            // Curva suave: 0..0.65 -> 1.2m..11.5m (precision), 0.65..1.0 -> hasta ~21.5m
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
        // CANYON
        const float depthM = 0.9f + 7.6f * dy;
        const float widthM = dx * 5.8f;
        return std::sqrt(depthM * depthM + widthM * widthM);
    }

    void processRoom(juce::AudioBuffer<float>& ioBuffer,
                     juce::AudioBuffer<float>& outRoomOnlyBuffer,
                     bool roomEnabled,
                     int roomIndex,
                     int wallsIndex, // 0 = Open / Live, 1 = Closed / Tight
                     bool mainStereo,
                     bool phaseSafe,
                     float pairWide,
                     float farWide,
                     const std::array<MicState, 4>& mics)
    {
        const int numSamples = ioBuffer.getNumSamples();
        if (numSamples == 0)
            return;

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
            updateRoomReverbParams();

        const bool anySolo = mics[0].solo || mics[1].solo || mics[2].solo || mics[3].solo;

        // Prepara colas difusas descorrelacionadas (L y R distintos para FAR L / FAR R)
        tailBufferL.setSize(2, numSamples, false, false, true);
        tailBufferR.setSize(2, numSamples, false, false, true);
        tailBufferL.copyFrom(0, 0, ioBuffer, 0, 0, numSamples);
        tailBufferL.copyFrom(1, 0, ioBuffer, ioBuffer.getNumChannels() > 1 ? 1 : 0, 0, numSamples);
        tailBufferR.copyFrom(0, 0, tailBufferL, 1, 0, numSamples);
        tailBufferR.copyFrom(1, 0, tailBufferL, 0, 0, numSamples);

        roomRevL.processStereo(tailBufferL.getWritePointer(0), tailBufferL.getWritePointer(1), numSamples);
        roomRevR.processStereo(tailBufferR.getWritePointer(0), tailBufferR.getWritePointer(1), numSamples);

        // Actualiza targets de los 4 atriles
        const float maxRoomM = (roomIndex == VOLCANO) ? 21.5f : (roomIndex == SHANGRILA ? 11.5f : 8.5f);
        for (int m = 0; m < 4; ++m)
        {
            const float distM = computeDistanceMeters(roomIndex, mics[m].x, mics[m].y);
            const float effGain = (anySolo && !mics[m].solo) ? 0.0f : mics[m].gain;

            distSmooth[m].setTargetValue(distM);
            gainSmooth[m].setTargetValue(effGain);

            float pan = (mics[m].x - 0.5f) * 2.0f; // -1..+1
            if (m == 1) pan *= (0.4f + 0.6f * pairWide);
            if (m == 2 || m == 3) pan = std::clamp(pan * (0.6f + 0.6f * farWide), -1.0f, 1.0f);
            panSmooth[m].setTargetValue(pan);

            // Filtro de absorcion de aire por distancia (actualizado solo si cambia > 0.25m)
            if (std::abs(distM - lastMicDistM[m]) > 0.25f)
            {
                lastMicDistM[m] = distM;
                const float normD = std::clamp(distM / maxRoomM, 0.0f, 1.0f);
                const float lpfHz = 18500.0f - normD * (roomIndex == VOLCANO ? 11500.0f : 9000.0f);
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
        const float* revLL = tailBufferL.getReadPointer(0);
        const float* revLR = tailBufferL.getReadPointer(1);
        const float* revRL = tailBufferR.getReadPointer(0);
        const float* revRR = tailBufferR.getReadPointer(1);

        auto* outL = roomBusBuffer.getWritePointer(0);
        auto* outR = roomBusBuffer.getWritePointer(1);
        auto* roomOnlyL = outRoomOnlyBuffer.getWritePointer(0);
        auto* roomOnlyR = outRoomOnlyBuffer.getWritePointer(1);

        const float samplesPerMeter = static_cast<float>(fs) / 343.0f;
        const float gateTailBoost = (roomIndex == VOLCANO && wallsIndex == 0) ? 1.32f : 1.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            const float dryL = inL[i];
            const float dryR = inR[i];
            const float dryMono = 0.5f * (dryL + dryR);

            float sumL = 0.0f;
            float sumR = 0.0f;
            float ambOnlyL = 0.0f;
            float ambOnlyR = 0.0f;

            for (int m = 0; m < 4; ++m)
            {
                const float g = gainSmooth[m].getNextValue();
                const float distM = distSmooth[m].getNextValue();
                const float pan = panSmooth[m].getNextValue();

                // Tiempo de vuelo real (reducido si Phase Safe esta activo para evitar comb filtering)
                float delaySamples = distM * samplesPerMeter;
                if (phaseSafe)
                    delaySamples = (m == 0) ? 0.0f : std::min(delaySamples * 0.25f, static_cast<float>(fs) * 0.012f);

                delayL[m].pushSample(0, (m == 0 && !mainStereo) ? dryMono : dryL);
                delayR[m].pushSample(0, (m == 0 && !mainStereo) ? dryMono : dryR);

                if (g < 0.0005f)
                    continue;

                float dL = delayL[m].popSample(0, delaySamples);
                float dR = delayR[m].popSample(0, delaySamples);

                dL = airLpfL[m].processSample(dL);
                dR = airLpfR[m].processSample(dR);

                const float normD = std::clamp(distM / maxRoomM, 0.0f, 1.0f);
                const float directRatio = std::clamp(1.0f - normD * (m >= 2 ? 0.78f : 0.55f), 0.15f, 1.0f);
                const float wetRatio = (0.18f + normD * (m >= 2 ? 0.95f : 0.65f)) * gateTailBoost;

                float sigL = 0.0f;
                float sigR = 0.0f;

                if (m == 0) // 1: MAIN
                {
                    sigL = dL * directRatio + revLL[i] * (wetRatio * 0.30f);
                    sigR = dR * directRatio + revLR[i] * (wetRatio * 0.30f);
                }
                else if (m == 1) // 2: ROOM / AIR
                {
                    sigL = dL * (directRatio * 0.65f) + revLL[i] * (wetRatio * 0.70f);
                    sigR = dR * (directRatio * 0.65f) + revLR[i] * (wetRatio * 0.70f);
                }
                else if (m == 2) // 3: FAR L (Mono descorrelacionado hacia izquierda)
                {
                    const float monoFarL = 0.5f * (dL + dR) * (directRatio * 0.45f) + (0.65f * revLL[i] + 0.35f * revLR[i]) * wetRatio;
                    sigL = monoFarL;
                    sigR = monoFarL;
                }
                else // 4: FAR R (Mono descorrelacionado hacia derecha)
                {
                    const float monoFarR = 0.5f * (dL + dR) * (directRatio * 0.45f) + (0.65f * revRR[i] + 0.35f * revRL[i]) * wetRatio;
                    sigL = monoFarR;
                    sigR = monoFarR;
                }

                // Paneo constant-power
                const float angle = (pan * 0.5f + 0.5f) * juce::MathConstants<float>::halfPi;
                const float pL = std::cos(angle);
                const float pR = std::sin(angle);

                const float outMicL = sigL * pL * g * 1.15f;
                const float outMicR = sigR * pR * g * 1.15f;

                sumL += outMicL;
                sumR += outMicR;

                if (m >= 1)
                {
                    ambOnlyL += outMicL;
                    ambOnlyR += outMicR;
                }
            }

            outL[i] = sumL;
            outR[i] = sumR;
            roomOnlyL[i] = ambOnlyL;
            roomOnlyR[i] = ambOnlyR;
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
    void updateRoomReverbParams()
    {
        juce::Reverb::Parameters pL, pR;
        const bool gatesOpen = (lastWalls == 0);

        if (lastRoom == VOLCANO)
        {
            pL.roomSize = gatesOpen ? 0.90f : 0.68f;
            pL.damping  = gatesOpen ? 0.28f : 0.52f;
            pL.width    = 1.0f;
        }
        else if (lastRoom == SHANGRILA)
        {
            pL.roomSize = gatesOpen ? 0.66f : 0.48f;
            pL.damping  = gatesOpen ? 0.42f : 0.62f;
            pL.width    = 0.92f;
        }
        else // CANYON
        {
            pL.roomSize = gatesOpen ? 0.46f : 0.30f;
            pL.damping  = gatesOpen ? 0.55f : 0.74f;
            pL.width    = 0.85f;
        }

        pL.wetLevel = 1.0f;
        pL.dryLevel = 0.0f;
        pL.freezeMode = 0.0f;

        // Ligera diferencia en pR para descorrelar FAR L vs FAR R
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

    juce::Reverb roomRevL, roomRevR, chamberRev;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> chamberPreDelayL, chamberPreDelayR;

    juce::AudioBuffer<float> roomBusBuffer, tailBufferL, tailBufferR, chamberBuffer;
};

} // namespace BenBocazDSP
