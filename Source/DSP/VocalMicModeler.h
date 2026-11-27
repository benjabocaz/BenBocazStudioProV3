#pragma once
#include "AnalogModules.h"

class VocalMicModeler
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate > 0.0 ? sampleRate : 44100.0;
        const float fs = static_cast<float> (sr);

        invLow.prepare (sr);  invMid1.prepare (sr);  invMid2.prepare (sr);  invHigh.prepare (sr);
        tgtLow.prepare (sr);  tgtMid1.prepare (sr);  tgtMid2.prepare (sr);  tgtHigh.prepare (sr);

        // Detector dinámico de sibilancias (6.5 kHz - 9 kHz) y envolvente de diafragma
        sibDetectHp.prepare (sr);
        sibDetectHp.setHighPass (6200.0f, 0.7071f);

        envAtkCoeff = 1.0f - std::exp (-1.0f / (0.0020f * fs));
        envRelCoeff = 1.0f - std::exp (-1.0f / (0.0450f * fs));
        sibAtkCoeff = 1.0f - std::exp (-1.0f / (0.0012f * fs));
        sibRelCoeff = 1.0f - std::exp (-1.0f / (0.0280f * fs));

        smInGain.reset (sr, 0.015);
        smOutGain.reset (sr, 0.015);

        reset();
    }

    void reset()
    {
        invLow.reset(); invMid1.reset(); invMid2.reset(); invHigh.reset();
        tgtLow.reset(); tgtMid1.reset(); tgtMid2.reset(); tgtHigh.reset();
        sibDetectHp.reset();

        capEnv.fill (0.0f);
        sibEnv.fill (0.0f);
        dcIn.fill (0.0f);
        dcOut.fill (0.0f);
    }

    void updateCurves (int sourceMicIdx, int targetMicIdx)
    {
        // 1. DE-VOICING PRECISO DEL MICRÓFONO DE ENTRADA (4 bandas TPT-SVF)
        switch (sourceMicIdx)
        {
            case 0: // Shure SM27
                invLow.setLowShelf   (165.0f,  0.9f, 0.72f);
                invMid1.setBell      (420.0f, -0.7f, 1.20f);
                invMid2.setBell      (7400.0f,-1.8f, 1.45f);
                invHigh.setHighShelf (12500.0f,-0.6f, 0.70f);
                break;

            case 1: // Shure SM57
                invLow.setLowShelf   (140.0f,  3.4f, 0.70f);
                invMid1.setBell      (260.0f, -1.4f, 1.15f);
                invMid2.setBell      (4400.0f,-4.4f, 1.15f);
                invHigh.setHighShelf (10800.0f, 3.8f, 0.68f);
                break;

            case 2: // Shure SM7B
                invLow.setBell       (215.0f, -2.4f, 0.92f);
                invMid1.setBell      (1150.0f,-0.8f, 1.10f);
                invMid2.setBell      (3100.0f, 2.9f, 1.05f);
                invHigh.setHighShelf (10200.0f, 3.6f, 0.65f);
                break;

            default: // Generic Large Diaphragm Condenser
                invLow.setBell       (300.0f, -1.6f, 1.25f);
                invMid1.setBell      (2400.0f,-0.7f, 1.30f);
                invMid2.setBell      (8600.0f,-3.0f, 1.85f);
                invHigh.setHighShelf (13500.0f,-0.8f, 0.72f);
                break;
        }

        // 2. CURVA DE CÁPSULA DEL MICRÓFONO DESTINO
        targetIdx = std::clamp (targetMicIdx, 0, 2);
        switch (targetIdx)
        {
            case 0: // Sony C-800G (Brillo hi-fi sedoso + presencia limpia + sub firme)
                tgtLow.setLowShelf   (90.0f,  -1.1f, 0.68f);
                tgtMid1.setBell      (3200.0f, 1.5f, 1.10f);
                tgtMid2.setBell      (6800.0f,-0.8f, 1.80f); // Evita dureza en "S" al subir el aire
                tgtHigh.setHighShelf (10800.0f, 4.2f, 0.62f);
                break;

            case 1: // Neumann TLM 103 (Cuerpo K87/K103 + lift clásico sobre 5 kHz)
                tgtLow.setLowShelf   (135.0f,  1.5f, 0.70f);
                tgtMid1.setBell      (5600.0f, 2.8f, 0.92f);
                tgtMid2.setBell      (2800.0f, 0.6f, 1.20f);
                tgtHigh.setHighShelf (11800.0f, 2.1f, 0.68f);
                break;

            default: // Aston Origin (Cálido británico en medios-bajos, agudos controlados)
                tgtLow.setBell       (235.0f,  2.0f, 0.88f);
                tgtMid1.setBell      (2150.0f, 1.5f, 1.08f);
                tgtMid2.setBell      (7800.0f,-1.4f, 1.65f);
                tgtHigh.setHighShelf (12800.0f,-0.9f, 0.70f);
                break;
        }
    }

    void process (juce::AudioBuffer<float>& buffer, bool enabled, float inputDb, float outputDb)
    {
        if (! enabled) return;

        smInGain.setTargetValue  (juce::Decibels::decibelsToGain (inputDb));
        smOutGain.setTargetValue (juce::Decibels::decibelsToGain (outputDb));

        const int numCh = std::min (2, buffer.getNumChannels());
        const int numSamples = buffer.getNumSamples();

        for (int i = 0; i < numSamples; ++i)
        {
            const float inG  = smInGain.getNextValue();
            const float outG = smOutGain.getNextValue();

            for (int ch = 0; ch < numCh; ++ch)
            {
                const auto c = static_cast<size_t> (ch);
                float x = buffer.getSample (ch, i) * inG;

                // 1. De-voicing del micrófono real + cápsula destino
                x = invLow.processSample  (ch, x);
                x = invMid1.processSample (ch, x);
                x = invMid2.processSample (ch, x);
                x = invHigh.processSample (ch, x);

                x = tgtLow.processSample  (ch, x);
                x = tgtMid1.processSample (ch, x);
                x = tgtMid2.processSample (ch, x);
                x = tgtHigh.processSample (ch, x);

                // 2. Micro-dinámica natural de diafragma (controla picos vocales sin sonar comprimido)
                const float absX = std::abs (x);
                const float cCoeff = (absX > capEnv[c]) ? envAtkCoeff : envRelCoeff;
                capEnv[c] += cCoeff * (absX - capEnv[c]);
                const float capComp = 1.0f / (1.0f + capEnv[c] * (targetIdx == 1 ? 0.14f : 0.24f));
                x *= capComp;

                // 3. Control dinámico suave de sibilancias (solo actúa cuando la zona de "S" excede umbral)
                const float sibBand = sibDetectHp.processSample (ch, x);
                const float absSib = std::abs (sibBand);
                const float sCoeff = (absSib > sibEnv[c]) ? sibAtkCoeff : sibRelCoeff;
                sibEnv[c] += sCoeff * (absSib - sibEnv[c]);
                const float sibOver = std::max (0.0f, sibEnv[c] - 0.16f);
                const float sibDuck = std::clamp (sibOver * 1.45f, 0.0f, 0.38f);
                x -= sibBand * sibDuck;

                // 4. Etapa electrónica real del micrófono destino
                float y = x;
                if (targetIdx == 0) // Sony C-800G: Válvula 6AU6 con Dynamic Cathode Bias y Sag
                {
                    const float sag = 1.0f / (1.0f + capEnv[c] * 0.28f);
                    const float bias = 0.055f + 0.045f * capEnv[c];
                    const float driven = x * (1.38f * sag) + bias;
                    y = (std::tanh (driven) - std::tanh (bias)) * 0.80f;
                }
                else if (targetIdx == 1) // Neumann TLM 103: FET sin transformador (ultralimpio y rápido)
                {
                    const float driven = x * 1.06f;
                    y = (driven / std::sqrt (1.0f + 0.12f * driven * driven)) * 0.95f;
                }
                else // Aston Origin: Clase A discreto con calidez sutil
                {
                    const float bias = 0.030f;
                    const float driven = x * 1.24f + bias;
                    y = (std::tanh (driven) - std::tanh (bias)) * 0.86f;
                }

                // DC Blocker transparente
                const float dc = y - dcIn[c] + 0.9985f * dcOut[c];
                dcIn[c] = y;
                dcOut[c] = dc;

                buffer.setSample (ch, i, dc * outG);
            }
        }
    }

private:
    double sr = 44100.0;
    int targetIdx = 0;

    TptSvfStereo invLow, invMid1, invMid2, invHigh;
    TptSvfStereo tgtLow, tgtMid1, tgtMid2, tgtHigh;
    StereoBiquad sibDetectHp;

    juce::SmoothedValue<float> smInGain { 1.0f }, smOutGain { 1.0f };
    float envAtkCoeff = 0.01f, envRelCoeff = 0.0005f;
    float sibAtkCoeff = 0.02f, sibRelCoeff = 0.001f;

    std::array<float, 2> capEnv { 0.0f, 0.0f };
    std::array<float, 2> sibEnv { 0.0f, 0.0f };
    std::array<float, 2> dcIn   { 0.0f, 0.0f };
    std::array<float, 2> dcOut  { 0.0f, 0.0f };
};
