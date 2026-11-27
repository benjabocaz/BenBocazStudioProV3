#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>
#include <algorithm>
#include <atomic>

//==============================================================================
// Filtro Biquad Estéreo (compatibilidad con AcousticEngine y Processor)
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
// Filtro TPT State Variable Filter (Zero-Delay Feedback - estilo Cytomic)
//==============================================================================
class TptSvfStereo
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate > 0.0 ? sampleRate : 176400.0;
        reset();
    }

    void reset()
    {
        ic1eq = { 0.0f, 0.0f };
        ic2eq = { 0.0f, 0.0f };
        hp1State = { 0.0f, 0.0f };
    }

    void setBypass()
    {
        active = false;
        thirdOrderHp = false;
    }

    void setHighPass (float freq, float q = 0.7071f, bool is18dB = false)
    {
        active = true;
        thirdOrderHp = is18dB;
        freq = std::clamp (freq, 18.0f, static_cast<float> (sr * 0.44));
        const float g = std::tan (juce::MathConstants<float>::pi * freq / static_cast<float> (sr));
        const float k = 1.0f / std::max (0.1f, q);
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
        m0 = 1.0f; m1 = -k; m2 = -1.0f;
        hp1Coeff = g / (1.0f + g);
    }

    void setBell (float freq, float gainDb, float q = 1.0f)
    {
        if (std::abs (gainDb) < 0.01f) { setBypass(); return; }
        active = true;
        thirdOrderHp = false;
        freq = std::clamp (freq, 20.0f, static_cast<float> (sr * 0.44));
        const float A = std::pow (10.0f, gainDb / 40.0f);
        const float g = std::tan (juce::MathConstants<float>::pi * freq / static_cast<float> (sr));
        const float k = 1.0f / (std::max (0.1f, q) * A);
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
        m0 = 1.0f; m1 = k * (A * A - 1.0f); m2 = 0.0f;
    }

    void setLowShelf (float freq, float gainDb, float q = 0.7071f)
    {
        if (std::abs (gainDb) < 0.01f) { setBypass(); return; }
        active = true;
        thirdOrderHp = false;
        freq = std::clamp (freq, 20.0f, static_cast<float> (sr * 0.44));
        const float A = std::pow (10.0f, gainDb / 40.0f);
        const float g = std::tan (juce::MathConstants<float>::pi * freq / static_cast<float> (sr)) / std::sqrt (A);
        const float k = 1.0f / std::max (0.25f, q);
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
        m0 = 1.0f; m1 = k * (A - 1.0f); m2 = (A * A - 1.0f);
    }

    void setHighShelf (float freq, float gainDb, float q = 0.7071f)
    {
        if (std::abs (gainDb) < 0.01f) { setBypass(); return; }
        active = true;
        thirdOrderHp = false;
        freq = std::clamp (freq, 20.0f, static_cast<float> (sr * 0.44));
        const float A = std::pow (10.0f, gainDb / 40.0f);
        const float g = std::tan (juce::MathConstants<float>::pi * freq / static_cast<float> (sr)) * std::sqrt (A);
        const float k = 1.0f / std::max (0.25f, q);
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
        m0 = A * A; m1 = k * (1.0f - A) * A; m2 = (1.0f - A * A);
    }

    inline float processSample (int ch, float v0) noexcept
    {
        if (! active) return v0;
        const auto c = static_cast<size_t> (ch & 1);

        const float v3 = v0 - ic2eq[c];
        const float v1 = a1 * ic1eq[c] + a2 * v3;
        const float v2 = ic2eq[c] + a2 * ic1eq[c] + a3 * v3;
        ic1eq[c] = 2.0f * v1 - ic1eq[c];
        ic2eq[c] = 2.0f * v2 - ic2eq[c];

        float out = m0 * v0 + m1 * v1 + m2 * v2;

        if (thirdOrderHp)
        {
            const float lp = hp1State[c] + hp1Coeff * (out - hp1State[c]);
            hp1State[c] = 2.0f * lp - hp1State[c];
            out -= lp;
        }
        return out;
    }

private:
    double sr = 176400.0;
    bool active = false;
    bool thirdOrderHp = false;
    float a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;
    float m0 = 1.0f, m1 = 0.0f, m2 = 0.0f;
    float hp1Coeff = 0.0f;
    std::array<float, 2> ic1eq { 0.0f, 0.0f };
    std::array<float, 2> ic2eq { 0.0f, 0.0f };
    std::array<float, 2> hp1State { 0.0f, 0.0f };
};

//==============================================================================
// Etapa de Preamp Analógico de Alta Gama (Transformador, Válvula, Op-Amp Discreto)
//==============================================================================
class AnalogPreampStage
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate > 0.0 ? sampleRate : 176400.0;
        const float fs = static_cast<float> (sr);
        lowCoeff    = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 115.0f   / fs);
        midCoeff    = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 1750.0f  / fs);
        highCoeff   = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 12500.0f / fs);
        fluxCoeff   = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 85.0f    / fs);
        sagAtkCoeff = 1.0f - std::exp (-1.0f / (0.0025f * fs));
        sagRelCoeff = 1.0f - std::exp (-1.0f / (0.0450f * fs));
        reset();
    }

    void reset()
    {
        lowState.fill (0.0f);
        midState.fill (0.0f);
        highState.fill (0.0f);
        fluxState.fill (0.0f);
        prevSample.fill (0.0f);
        sagEnv.fill (0.0f);
        dcIn.fill (0.0f);
        dcOut.fill (0.0f);
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

        const float absX = std::abs (x);
        const float sCoeff = (absX > sagEnv[c]) ? sagAtkCoeff : sagRelCoeff;
        sagEnv[c] += sCoeff * (absX - sagEnv[c]);

        fluxState[c] += fluxCoeff * (x - fluxState[c]);
        const float coreFlux = std::tanh (fluxState[c] * (1.0f + d * 2.8f)) / (1.0f + d * 1.4f);
        const float hystDiff = (x - fluxState[c]);

        const float dx = x - prevSample[c];
        prevSample[c] = x;

        float y = x;

        if (circuit == 0) // NEVE CLASS-A
        {
            const float pre = 1.0f + d * 3.10f;
            const float xFed = (x + lows * (0.36f * d) + coreFlux * (0.32f * d) + hystDiff * (0.12f * d)) * pre;
            const float bias = 0.14f * d + 0.08f * d * sagEnv[c];
            const float shaped = std::tanh (xFed + bias) - std::tanh (bias);
            const float ironLoss = shaped - highs * (0.14f * d);
            y = ironLoss / (1.0f + d * 1.42f);
        }
        else if (circuit == 1) // PULTEC 12AX7
        {
            const float sag = 1.0f / (1.0f + sagEnv[c] * 0.58f * d);
            const float pre = (1.0f + d * 2.85f) * sag;
            const float cathodeShift = 0.16f * d * sagEnv[c];
            const float xIn = (x + coreFlux * (0.18f * d)) * pre - cathodeShift;

            float shaped = 0.0f;
            if (xIn >= 0.0f)
                shaped = std::tanh (xIn * 1.28f) / 1.28f;
            else
                shaped = (2.0f / juce::MathConstants<float>::pi) * std::atan (xIn * 0.88f) * 1.18f;

            y = (shaped + cathodeShift * 0.35f + highs * (0.09f * d)) / (1.0f + d * 1.18f);
        }
        else if (circuit == 2) // API 2520
        {
            const float pre = 1.0f + d * 3.45f;
            const float maxSlew = juce::jmap (d, 0.0f, 1.0f, 0.95f, 0.24f);
            const float slewLim = std::tanh (dx / maxSlew) * maxSlew;
            const float xSlew = (x - dx) + slewLim;

            const float xFed = (xSlew - lows * (0.10f * d) + mids * (0.44f * d) + coreFlux * (0.22f * d)) * pre;
            const float shaped = xFed / std::sqrt (1.0f + xFed * xFed);
            y = (shaped + lows * (0.12f * d)) / (1.0f + d * 1.48f);
        }
        else // SSL CLEAN
        {
            const float pre = 1.0f + d * 1.45f;
            const float xIn = (x + hystDiff * (0.06f * d)) * pre;
            const float shaped = 0.60f * std::tanh (xIn) + 0.40f * (xIn / std::sqrt (1.0f + 0.45f * xIn * xIn));
            y = (shaped + highs * (0.08f * d)) / (1.0f + d * 0.68f);
        }

        if (speakerBreakup)
        {
            const float spkIn = y * (1.25f + d * 0.55f) + coreFlux * 0.30f;
            const float spkSat = std::tanh (spkIn + 0.04f * d);
            y = 0.75f * spkSat + 0.25f * y;
        }

        const float dc = y - dcIn[c] + 0.9994f * dcOut[c];
        dcIn[c] = y;
        dcOut[c] = dc;
        return dc;
    }

private:
    double sr = 176400.0;
    float lowCoeff = 0.004f, midCoeff = 0.06f, highCoeff = 0.35f, fluxCoeff = 0.003f;
    float sagAtkCoeff = 0.002f, sagRelCoeff = 0.00012f;
    std::array<float, 2> lowState {}, midState {}, highState {}, fluxState {}, prevSample {};
    std::array<float, 2> sagEnv {}, dcIn {}, dcOut {};
};

//==============================================================================
// Modelado Físico de Gabinetes (Resonancia de Caja, Notches de Cono, Beaming
// Off-Axis, Excursión Mecánica y Breakup Pre-Sala)
//==============================================================================
class CabinetVoicingStage
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate > 0.0 ? sampleRate : 44100.0;
        const float fs = static_cast<float> (sr);
        exAtkCoeff = 1.0f - std::exp (-1.0f / (0.003f * fs));
        exRelCoeff = 1.0f - std::exp (-1.0f / (0.065f * fs));
        exLowCoeff = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 160.0f / fs);

        for (int ch = 0; ch < 2; ++ch)
        {
            hpf[ch].prepare (sr);
            lpf1[ch].prepare (sr);
            lpf2[ch].prepare (sr);
            boxRes[ch].prepare (sr);
            cabBody[ch].prepare (sr);
            coneNotch1[ch].prepare (sr);
            coneNotch2[ch].prepare (sr);
            coneEdge[ch].prepare (sr);
        }
        reset();
    }

    void reset()
    {
        exLowState.fill (0.0f);
        exEnv.fill (0.0f);
        for (int ch = 0; ch < 2; ++ch)
        {
            hpf[ch].reset(); lpf1[ch].reset(); lpf2[ch].reset();
            boxRes[ch].reset(); cabBody[ch].reset();
            coneNotch1[ch].reset(); coneNotch2[ch].reset(); coneEdge[ch].reset();
        }
    }

    // offAxisNorm: 0.0 = justo frente al centro del cono (máximo filo), 1.0 = fuera de eje (cálido/redondo)
    void configure (int sourceIdx, float driveAmount, float offAxisNorm = 0.0f)
    {
        active = false;
        stereoSpreadAmount = 0.0f;
        isBassCab = false;
        cabDrive = std::clamp (driveAmount, 0.0f, 1.0f);
        const float offAx = std::clamp (offAxisNorm, 0.0f, 1.0f);

        for (int ch = 0; ch < 2; ++ch)
        {
            hpf[ch].setBypass(); lpf1[ch].setBypass(); lpf2[ch].setBypass();
            boxRes[ch].setBypass(); cabBody[ch].setBypass();
            coneNotch1[ch].setBypass(); coneNotch2[ch].setBypass(); coneEdge[ch].setBypass();
        }

        if (sourceIdx == 4) // GUITAR CAB (MONO — 4x12 / 1x12 Celestion style)
        {
            active = true;
            const float beamCut = juce::jmap (offAx, 0.0f, 1.0f, 5600.0f, 3450.0f);
            const float edgeDb  = juce::jmap (offAx, 0.0f, 1.0f, 3.6f, 0.6f) + cabDrive * 1.4f;

            for (int ch = 0; ch < 2; ++ch)
            {
                hpf[ch].setHighPass (86.0f, 0.85f);
                boxRes[ch].setPeak (118.0f, 3.4f, 2.2f);             // Resonancia física de caja cerrada
                cabBody[ch].setPeak (480.0f, 2.0f + cabDrive * 1.0f, 1.15f);
                coneNotch1[ch].setPeak (1150.0f, -4.2f, 3.6f);       // Imperfección de cono de papel
                coneNotch2[ch].setPeak (3150.0f, -3.5f, 4.2f);       // Cancelación de dust-cap
                coneEdge[ch].setPeak (2250.0f, edgeDb, 1.75f);       // Mordida sensible al eje
                lpf1[ch].setLowPass (beamCut, 0.82f);                // Corte abrupto 24 dB/oct
                lpf2[ch].setLowPass (beamCut * 1.08f, 0.72f);
            }
        }
        else if (sourceIdx == 5) // GUITAR CAB (STEREO — L: British V30 / R: American Tweed)
        {
            active = true;
            stereoSpreadAmount = 0.36f;
            const float beamCutL = juce::jmap (offAx, 0.0f, 1.0f, 6100.0f, 3750.0f);
            const float beamCutR = juce::jmap (offAx, 0.0f, 1.0f, 5200.0f, 3300.0f);

            // Izquierdo: British mordedor
            hpf[0].setHighPass (82.0f, 0.84f);
            boxRes[0].setPeak (124.0f, 3.2f, 2.1f);
            cabBody[0].setPeak (540.0f, 1.8f + cabDrive * 1.0f, 1.1f);
            coneNotch1[0].setPeak (1280.0f, -4.0f, 3.8f);
            coneNotch2[0].setPeak (3400.0f, -3.2f, 4.4f);
            coneEdge[0].setPeak (2550.0f, juce::jmap (offAx, 0.0f, 1.0f, 4.0f, 1.0f) + cabDrive * 1.5f, 1.8f);
            lpf1[0].setLowPass (beamCutL, 0.80f);
            lpf2[0].setLowPass (beamCutL * 1.08f, 0.72f);

            // Derecho: American cálido y abierto
            hpf[1].setHighPass (92.0f, 0.80f);
            boxRes[1].setPeak (108.0f, 3.6f, 1.9f);
            cabBody[1].setPeak (410.0f, 2.6f + cabDrive * 1.2f, 1.2f);
            coneNotch1[1].setPeak (980.0f,  -3.6f, 3.4f);
            coneNotch2[1].setPeak (2850.0f, -3.8f, 4.0f);
            coneEdge[1].setPeak (1950.0f, juce::jmap (offAx, 0.0f, 1.0f, 3.0f, 0.6f) + cabDrive * 1.3f, 1.55f);
            lpf1[1].setLowPass (beamCutR, 0.85f);
            lpf2[1].setLowPass (beamCutR * 1.10f, 0.74f);
        }
        else if (sourceIdx == 6) // BASS AMP (LOW END — Ampeg 8x10 / 4x10 style)
        {
            active = true;
            isBassCab = true;
            const float tweeterCut = juce::jmap (offAx, 0.0f, 1.0f, 4200.0f, 2600.0f);

            for (int ch = 0; ch < 2; ++ch)
            {
                hpf[ch].setHighPass (34.0f, 0.85f);
                boxRes[ch].setPeak (68.0f, 3.8f + cabDrive * 1.2f, 1.35f);   // Thump de gabinete grande
                cabBody[ch].setPeak (760.0f, 2.8f + cabDrive * 1.6f, 1.35f); // Gruñido clásico de bajo
                coneNotch1[ch].setPeak (360.0f, -2.6f, 2.1f);                // Limpieza de caja hueca
                coneNotch2[ch].setPeak (1850.0f, -2.4f, 2.8f);
                coneEdge[ch].setPeak (2450.0f, juce::jmap (offAx, 0.0f, 1.0f, 1.8f, -0.5f), 1.6f); // Tweeter sutil
                lpf1[ch].setLowPass (tweeterCut, 0.82f);
                lpf2[ch].setLowPass (tweeterCut * 1.12f, 0.72f);
            }
        }
    }

    void processBuffer (juce::AudioBuffer<float>& buffer) noexcept
    {
        if (! active || buffer.getNumChannels() < 2) return;

        const int numSamples = buffer.getNumSamples();
        auto* L = buffer.getWritePointer (0);
        auto* R = buffer.getWritePointer (1);

        auto processChannel = [&] (int ch, float x) noexcept -> float
        {
            const auto c = static_cast<size_t> (ch & 1);

            // Excursión mecánica del cono (compresión suave ligada a la energía en graves)
            exLowState[c] += exLowCoeff * (x - exLowState[c]);
            const float absLow = std::abs (exLowState[c]);
            const float coeff = (absLow > exEnv[c]) ? exAtkCoeff : exRelCoeff;
            exEnv[c] += coeff * (absLow - exEnv[c]);
            const float excursionComp = 1.0f / (1.0f + exEnv[c] * (isBassCab ? 0.85f : 0.60f));

            // Saturación de bobina/cono PRE-SALA (antes de que el sonido viaje por el aire)
            const float satDrive = 1.08f + cabDrive * (isBassCab ? 0.95f : 1.35f);
            const float bias = (isBassCab ? 0.035f : 0.055f) * cabDrive;
            float y = (std::tanh (x * satDrive + bias) - std::tanh (bias)) / (1.0f + cabDrive * 0.52f);
            y *= excursionComp;

            // Respuesta física del gabinete y cono de papel
            y = hpf[c].processSample (ch, y);
            y = boxRes[c].processSample (ch, y);
            y = cabBody[c].processSample (ch, y);
            y = coneNotch1[c].processSample (ch, y);
            y = coneNotch2[c].processSample (ch, y);
            y = coneEdge[c].processSample (ch, y);
            y = lpf1[c].processSample (ch, y);
            y = lpf2[c].processSample (ch, y);
            return y;
        };

        for (int i = 0; i < numSamples; ++i)
        {
            if (stereoSpreadAmount <= 0.001f)
            {
                const float monoIn = 0.5f * (L[i] + R[i]);
                const float out = processChannel (0, monoIn);
                L[i] = out;
                R[i] = out;
            }
            else
            {
                const float outL = processChannel (0, L[i]);
                const float outR = processChannel (1, R[i]);
                const float mid  = 0.5f * (outL + outR);
                const float side = 0.5f * (outL - outR) * (1.0f + stereoSpreadAmount);
                L[i] = mid + side;
                R[i] = mid - side;
            }
        }
    }

private:
    double sr = 44100.0;
    bool active = false;
    bool isBassCab = false;
    float cabDrive = 0.2f;
    float stereoSpreadAmount = 0.0f;
    float exAtkCoeff = 0.005f, exRelCoeff = 0.0003f, exLowCoeff = 0.02f;
    std::array<float, 2> exLowState { 0.0f, 0.0f };
    std::array<float, 2> exEnv      { 0.0f, 0.0f };
    std::array<StereoBiquad, 2> hpf, lpf1, lpf2, boxRes, cabBody, coneNotch1, coneNotch2, coneEdge;
};

//==============================================================================
// EQ de Consola Analógica TPT-SVF con Inductor Saturation
//==============================================================================
class ConsoleEqWithTubeStage
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate > 0.0 ? sampleRate : 176400.0;
        hpf.prepare (sr);
        low.prepare (sr);
        pultecDip.prepare (sr);
        mid.prepare (sr);
        high.prepare (sr);
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

        currentModel = eqModel;
        midInductorDrive = (eqModel == 0 && std::abs (midDb) > 1.0f) ? std::clamp (std::abs (midDb) * 0.025f, 0.0f, 0.28f) : 0.0f;

        if (hpfHz > 22.0f)
            hpf.setHighPass (hpfHz, (eqModel == 0 || eqModel == 3) ? 0.78f : 0.7071f, (eqModel == 0 || eqModel == 3));
        else
            hpf.setBypass();

        pultecDip.setBypass();

        if (eqModel == 0) // NEVE 1073
        {
            low.setLowShelf (fLow, lowDb, 0.64f);
            mid.setBell (fMid, midDb, 0.78f + std::abs (midDb) * 0.02f);
            high.setHighShelf (fHigh, highDb, 0.62f);
        }
        else if (eqModel == 1) // PULTEC EQP-1A
        {
            low.setLowShelf (fLow * 0.90f, lowDb * 1.12f, 0.56f);
            if (lowDb > 0.4f)
                pultecDip.setBell (fLow * 2.55f, -lowDb * 0.38f, 1.10f);
            mid.setBell (fMid, midDb, 0.62f);
            high.setHighShelf (fHigh * 0.95f, highDb * 1.06f, 0.55f);
        }
        else if (eqModel == 2) // API 550A (Proportional-Q real)
        {
            const float lowQ  = 0.62f + std::abs (lowDb)  * 0.045f;
            const float midQ  = 0.75f + std::abs (midDb)  * 0.115f;
            const float highQ = 0.62f + std::abs (highDb) * 0.045f;
            low.setLowShelf (fLow, lowDb, lowQ);
            mid.setBell (fMid, midDb, midQ);
            high.setHighShelf (fHigh * 0.92f, highDb, highQ);
        }
        else // SSL 4000E
        {
            low.setLowShelf (fLow, lowDb, 0.85f);
            mid.setBell (fMid, midDb, 1.65f);
            high.setHighShelf (fHigh, highDb, 0.82f);
        }
    }

    inline void processSample (int ch, float& x) noexcept
    {
        x = hpf.processSample (ch, x);
        x = low.processSample (ch, x);
        x = pultecDip.processSample (ch, x);

        if (midInductorDrive > 0.001f)
        {
            const float preMid = x;
            const float postMid = mid.processSample (ch, x);
            const float midDelta = postMid - preMid;
            const float satDelta = std::tanh (midDelta * (1.0f + midInductorDrive * 2.2f)) / (1.0f + midInductorDrive * 2.2f);
            x = preMid + satDelta;
        }
        else
        {
            x = mid.processSample (ch, x);
        }

        x = high.processSample (ch, x);
    }

private:
    double sr = 176400.0;
    int currentModel = 0;
    float midInductorDrive = 0.0f;
    TptSvfStereo hpf, low, pultecDip, mid, high;
};

//==============================================================================
// Compresor Vintage de Alta Gama (Feedback 1176/LA-2A, Vari-Mu, Auto-Makeup)
//==============================================================================
class VintageCompressor
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate > 0.0 ? sampleRate : 44100.0;
        scHpf.prepare (sr);
        scHpf.setHighPass (120.0f, 0.7071f);
        optoEmph.prepare (sr);
        optoEmph.setHighShelf (2800.0f, 3.2f);
        rmsCoeff = 1.0f - std::exp (-1.0f / (0.012f * static_cast<float> (sr)));
        reset();
    }

    void reset()
    {
        envFast.fill (0.0f);
        envSlow.fill (0.0f);
        rmsSq.fill (0.0f);
        fbOut.fill (0.0f);
        scHpf.reset();
        optoEmph.reset();
        gainReductionDb.store (0.0f);
    }

    void configure (int compType, float thresholdDb, bool useScFilter, bool linkStereo, bool enableHarmonicGrit = true)
    {
        model = compType;
        thresh = thresholdDb;
        scFilterOn = useScFilter;
        stereoLinked = linkStereo;
        allowGrit = enableHarmonicGrit;

        if (compType == 0) // 1176LN (FET Feedback)
        {
            ratio = 6.0f; attackMs = 0.35f; relFastMs = 48.0f; relSlowMs = 140.0f;
            kneeDb = 2.5f; isFeedback = true; isOpto = false; isVariMu = false;
            makeupDb = std::clamp ((-thresholdDb - 10.0f) * 0.24f, 0.0f, 7.5f);
        }
        else if (compType == 1) // LA-2A (Opto T4B Feedback)
        {
            ratio = 3.5f; attackMs = 9.5f; relFastMs = 62.0f; relSlowMs = 680.0f;
            kneeDb = 7.0f; isFeedback = true; isOpto = true; isVariMu = false;
            makeupDb = std::clamp ((-thresholdDb - 10.0f) * 0.20f, 0.0f, 6.0f);
        }
        else if (compType == 2) // Mastering (Vari-Mu Tube)
        {
            ratio = 2.0f; attackMs = 16.0f; relFastMs = 145.0f; relSlowMs = 420.0f;
            kneeDb = 9.5f; isFeedback = false; isOpto = true; isVariMu = true;
            makeupDb = std::clamp ((-thresholdDb - 12.0f) * 0.16f, 0.0f, 5.0f);
        }
        else // Parallel Crush (1176 All-Buttons-In)
        {
            ratio = 14.0f; attackMs = 0.22f; relFastMs = 40.0f; relSlowMs = 120.0f;
            kneeDb = 2.0f; isFeedback = true; isOpto = true; isVariMu = false;
            makeupDb = 5.5f;
        }

        const float fs = static_cast<float> (sr);
        atkCoeff     = std::exp (-1.0f / (0.001f * attackMs  * fs));
        relFastCoeff = std::exp (-1.0f / (0.001f * relFastMs * fs));
        relSlowCoeff = std::exp (-1.0f / (0.001f * relSlowMs * fs));
        makeupLin    = juce::Decibels::decibelsToGain (makeupDb);
    }

    inline void processStereoSample (float& inL, float& inR, float wetMix, float& frameMaxGr) noexcept
    {
        float srcL = isFeedback ? fbOut[0] : inL;
        float srcR = isFeedback ? fbOut[1] : inR;

        float detL = scFilterOn ? scHpf.processSample (0, srcL) : srcL;
        float detR = scFilterOn ? scHpf.processSample (1, srcR) : srcR;

        if (model == 1)
        {
            detL = optoEmph.processSample (0, detL);
            detR = optoEmph.processSample (1, detR);
        }

        float grL = 0.0f, grR = 0.0f;

        if (stereoLinked)
        {
            const float detVal = (model == 2)
                ? std::sqrt (0.5f * (detL * detL + detR * detR) + 1e-12f)
                : std::max (std::abs (detL), std::abs (detR));
            grL = grR = computeChannelGr (0, detVal);
        }
        else
        {
            grL = computeChannelGr (0, std::abs (detL));
            grR = computeChannelGr (1, std::abs (detR));
        }

        const float gainL = juce::Decibels::decibelsToGain (-grL);
        const float gainR = juce::Decibels::decibelsToGain (-grR);

        float compL = inL * gainL;
        float compR = inR * gainR;

        fbOut[0] = compL;
        fbOut[1] = compR;

        const float avgGr = 0.5f * (grL + grR);
        if (allowGrit && avgGr > 0.25f)
        {
            const float grit = std::clamp (avgGr * 0.025f, 0.0f, 0.35f);
            if (model == 0 || model == 3)
            {
                compL = std::tanh (compL * (1.0f + grit * 1.35f)) / (1.0f + grit * 0.95f);
                compR = std::tanh (compR * (1.0f + grit * 1.35f)) / (1.0f + grit * 0.95f);
            }
            else
            {
                const float bias = 0.04f * grit;
                compL = std::tanh (compL + bias) - std::tanh (bias);
                compR = std::tanh (compR + bias) - std::tanh (bias);
            }
        }

        compL *= makeupLin;
        compR *= makeupLin;

        inL = inL * (1.0f - wetMix) + compL * wetMix;
        inR = inR * (1.0f - wetMix) + compR * wetMix;

        const float m = std::max (grL, grR);
        if (m > frameMaxGr) frameMaxGr = m;
    }

    void publishGr (float maxGr) noexcept { gainReductionDb.store (maxGr); }
    float getGainReductionDb() const { return gainReductionDb.load(); }

private:
    inline float computeChannelGr (int ch, float detLevel) noexcept
    {
        const auto c = static_cast<size_t> (ch & 1);

        rmsSq[c] += rmsCoeff * (detLevel * detLevel - rmsSq[c]);
        const float rmsVal = std::sqrt (std::max (0.0f, rmsSq[c]));
        const float effectiveLevel = isVariMu ? (0.65f * rmsVal + 0.35f * detLevel)
                                              : (isOpto ? (0.45f * rmsVal + 0.55f * detLevel) : detLevel);

        const float levelDb = juce::Decibels::gainToDecibels (effectiveLevel + 1e-6f);

        float overDb = 0.0f;
        const float diff = levelDb - thresh;
        if (2.0f * diff < -kneeDb)
            overDb = 0.0f;
        else if (2.0f * std::abs (diff) <= kneeDb)
            overDb = (diff + kneeDb * 0.5f) * (diff + kneeDb * 0.5f) / (2.0f * kneeDb);
        else
            overDb = diff;

        const float effRatio = isVariMu ? (1.45f + std::clamp (overDb * 0.22f, 0.0f, 3.2f)) : ratio;
        const float targetGrDb = overDb * (1.0f - 1.0f / effRatio);

        if (targetGrDb > envFast[c])
            envFast[c] = atkCoeff * envFast[c] + (1.0f - atkCoeff) * targetGrDb;
        else
            envFast[c] = relFastCoeff * envFast[c] + (1.0f - relFastCoeff) * targetGrDb;

        if (isOpto)
        {
            if (targetGrDb > envSlow[c])
                envSlow[c] = atkCoeff * envSlow[c] + (1.0f - atkCoeff) * targetGrDb;
            else
                envSlow[c] = relSlowCoeff * envSlow[c] + (1.0f - relSlowCoeff) * targetGrDb;

            const float slowWeight = (model == 1) ? 0.52f : 0.35f;
            return (1.0f - slowWeight) * envFast[c] + slowWeight * envSlow[c];
        }

        return envFast[c];
    }

    double sr = 44100.0;
    int model = 0;
    float thresh = -16.0f, ratio = 4.0f, attackMs = 0.6f, relFastMs = 55.0f, relSlowMs = 150.0f;
    float kneeDb = 3.0f, makeupDb = 0.0f, makeupLin = 1.0f;
    float atkCoeff = 0.9f, relFastCoeff = 0.99f, relSlowCoeff = 0.999f, rmsCoeff = 0.001f;
    bool isFeedback = false, isOpto = false, isVariMu = false, scFilterOn = false, stereoLinked = true, allowGrit = true;

    std::array<float, 2> envFast { 0.0f, 0.0f };
    std::array<float, 2> envSlow { 0.0f, 0.0f };
    std::array<float, 2> rmsSq   { 0.0f, 0.0f };
    std::array<float, 2> fbOut   { 0.0f, 0.0f };
    StereoBiquad scHpf, optoEmph;
    std::atomic<float> gainReductionDb { 0.0f };
};

//==============================================================================
// Cadena Analógica Dual: TRACKING (1x, 0ms) y MIXING (4x Oversampling HQ)
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

        preamp4x.prepare (osRate);
        eq4x.prepare (osRate);
        mainComp4x.prepare (osRate);
        parallelComp4x.prepare (osRate);

        preamp1x.prepare (baseSampleRate);
        eq1x.prepare (baseSampleRate);
        mainComp1x.prepare (baseSampleRate);
        parallelComp1x.prepare (baseSampleRate);

        reset();
    }

    void reset()
    {
        oversampling.reset();
        preamp4x.reset(); eq4x.reset(); mainComp4x.reset(); parallelComp4x.reset();
        preamp1x.reset(); eq1x.reset(); mainComp1x.reset(); parallelComp1x.reset();
        activeGrDb.store (0.0f);
    }

    int getOversampledLatencySamples() const { return static_cast<int> (oversampling.getLatencyInSamples()); }
    float getMainCompGrDb() const { return activeGrDb.load(); }

    void updateEq (int eqModel, float hpfHz,
                   int lowFreqIdx, float lowDb,
                   int midFreqIdx, float midDb,
                   int highFreqIdx, float highDb,
                   bool tubeSync, int manualTubeIdx)
    {
        eq4x.updateParameters (eqModel, hpfHz, lowFreqIdx, lowDb, midFreqIdx, midDb, highFreqIdx, highDb, tubeSync, manualTubeIdx);
        eq1x.updateParameters (eqModel, hpfHz, lowFreqIdx, lowDb, midFreqIdx, midDb, highFreqIdx, highDb, tubeSync, manualTubeIdx);
    }

    void configureComps (int compType, float thresholdDb, bool useScFilter, bool linkStereo)
    {
        mainComp4x.configure (compType, thresholdDb, useScFilter, linkStereo, true);
        parallelComp4x.configure (3, -24.0f, true, true, true);

        mainComp1x.configure (compType, thresholdDb, useScFilter, linkStereo, false);
        parallelComp1x.configure (3, -24.0f, true, true, false);
    }

    struct ChainSettings
    {
        bool hqOversampling = true;
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
            activeGrDb.store (0.0f);
            return;
        }

        auto runLoop = [&] (float* ptrL, float* ptrR, size_t numSamples,
                            AnalogPreampStage& pre, ConsoleEqWithTubeStage& eq,
                            VintageCompressor& mc, VintageCompressor& pc)
        {
            float maxMainGr = 0.0f;
            float maxParGr  = 0.0f;

            for (size_t i = 0; i < numSamples; ++i)
            {
                float L = ptrL[i];
                float R = ptrR[i];

                if (s.tubeDrive > 0.005f || s.speakerBreakup)
                {
                    L = pre.processSample (0, L, s.tubeCircuit, s.tubeDrive, s.speakerBreakup);
                    R = pre.processSample (1, R, s.tubeCircuit, s.tubeDrive, s.speakerBreakup);
                }

                if (s.routingOrder == 1)
                {
                    if (s.compOn) mc.processStereoSample (L, R, s.compMix, maxMainGr);
                    if (s.eqOn)   { eq.processSample (0, L); eq.processSample (1, R); }
                }
                else
                {
                    if (s.eqOn)   { eq.processSample (0, L); eq.processSample (1, R); }
                    if (s.compOn) mc.processStereoSample (L, R, s.compMix, maxMainGr);
                }

                if (s.parallelOn && s.parallelAmount > 0.001f)
                {
                    float pL = L;
                    float pR = R;
                    pc.processStereoSample (pL, pR, 1.0f, maxParGr);
                    L = L + (pL - L) * s.parallelAmount;
                    R = R + (pR - R) * s.parallelAmount;
                }

                ptrL[i] = L;
                ptrR[i] = R;
            }

            mc.publishGr (maxMainGr);
            pc.publishGr (maxParGr);
            activeGrDb.store (maxMainGr);
        };

        if (s.hqOversampling)
        {
            juce::dsp::AudioBlock<float> block (buffer);
            auto upBlock = oversampling.processSamplesUp (block);
            runLoop (upBlock.getChannelPointer (0), upBlock.getChannelPointer (1), upBlock.getNumSamples(),
                     preamp4x, eq4x, mainComp4x, parallelComp4x);
            oversampling.processSamplesDown (block);
        }
        else
        {
            runLoop (buffer.getWritePointer (0), buffer.getWritePointer (1), static_cast<size_t> (buffer.getNumSamples()),
                     preamp1x, eq1x, mainComp1x, parallelComp1x);
        }
    }

private:
    juce::dsp::Oversampling<float> oversampling;
    AnalogPreampStage preamp4x, preamp1x;
    ConsoleEqWithTubeStage eq4x, eq1x;
    VintageCompressor mainComp4x, mainComp1x;
    VintageCompressor parallelComp4x, parallelComp1x;
    std::atomic<float> activeGrDb { 0.0f };
};
