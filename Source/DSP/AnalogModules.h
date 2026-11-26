#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>
#include <algorithm>
#include <atomic>

//==============================================================================
// Filtro Biquad Estéreo (mantenido para compatibilidad con AcousticEngine y Processor)
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
// Estabilidad absoluta bajo modulación/automatización y precisión analógica a 4x OS
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

        // Polo extra para 18 dB/octava (estilo Neve 1073 / SSL 4000E)
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
    void prepare (double oversampledRate)
    {
        sr = oversampledRate > 0.0 ? oversampledRate : 176400.0;
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

        // Separación de bandas para comportamiento dependiente de frecuencia
        lowState[c]  += lowCoeff  * (x - lowState[c]);
        midState[c]  += midCoeff  * (x - midState[c]);
        highState[c] += highCoeff * (x - highState[c]);

        const float lows  = lowState[c];
        const float mids  = midState[c] - lowState[c];
        const float highs = x - highState[c];

        // Envolvente rápida/lenta para Sag de fuente de poder y Dynamic Cathode Bias
        const float absX = std::abs (x);
        const float sCoeff = (absX > sagEnv[c]) ? sagAtkCoeff : sagRelCoeff;
        sagEnv[c] += sCoeff * (absX - sagEnv[c]);

        // Flujo magnético del transformador (satura más en frecuencias bajas: B ~ V/f)
        fluxState[c] += fluxCoeff * (x - fluxState[c]);
        const float coreFlux = std::tanh (fluxState[c] * (1.0f + d * 2.8f)) / (1.0f + d * 1.4f);
        const float hystDiff = (x - fluxState[c]);

        // Slew-rate (derivada de transitorios)
        const float dx = x - prevSample[c];
        prevSample[c] = x;

        float y = x;

        if (circuit == 0) // NEVE CLASS-A (Transformador Marinair + Clase A asimétrica)
        {
            const float pre = 1.0f + d * 3.10f;
            // Empuje de flujo en graves + leve suavizado de agudos por pérdidas de hierro
            const float xFed = (x + lows * (0.36f * d) + coreFlux * (0.32f * d) + hystDiff * (0.12f * d)) * pre;
            const float bias = 0.14f * d + 0.08f * d * sagEnv[c];
            const float shaped = std::tanh (xFed + bias) - std::tanh (bias);
            const float ironLoss = shaped - highs * (0.14f * d);
            y = ironLoss / (1.0f + d * 1.42f);
        }
        else if (circuit == 1) // PULTEC 12AX7 (Tríodo asimétrico + Dynamic Cathode Bias + Sag)
        {
            const float sag = 1.0f / (1.0f + sagEnv[c] * 0.58f * d);
            const float pre = (1.0f + d * 2.85f) * sag;
            const float cathodeShift = 0.16f * d * sagEnv[c];
            const float xIn = (x + coreFlux * (0.18f * d)) * pre - cathodeShift;

            float shaped = 0.0f;
            if (xIn >= 0.0f)
                shaped = std::tanh (xIn * 1.28f) / 1.28f; // Conducción de rejilla suave
            else
                shaped = (2.0f / juce::MathConstants<float>::pi) * std::atan (xIn * 0.88f) * 1.18f; // Corte de placa más redondo

            y = (shaped + cathodeShift * 0.35f + highs * (0.09f * d)) / (1.0f + d * 1.18f);
        }
        else if (circuit == 2) // API 2520 (Op-Amp discreto + Slew-rate limiting + Transformador de salida)
        {
            const float pre = 1.0f + d * 3.45f;
            // Slew-rate limiting sutil en transitorios muy rápidos
            const float maxSlew = juce::jmap (d, 0.0f, 1.0f, 0.95f, 0.24f);
            const float slewLim = std::tanh (dx / maxSlew) * maxSlew;
            const float xSlew = (x - dx) + slewLim;

            const float xFed = (xSlew - lows * (0.10f * d) + mids * (0.44f * d) + coreFlux * (0.22f * d)) * pre;
            // Armónicos impares marcados y con pegada
            const float shaped = xFed / std::sqrt (1.0f + xFed * xFed);
            y = (shaped + lows * (0.12f * d)) / (1.0f + d * 1.48f);
        }
        else // SSL CLEAN (VCA / Consola transparente con micro-armónicos de bus)
        {
            const float pre = 1.0f + d * 1.45f;
            const float xIn = (x + hystDiff * (0.06f * d)) * pre;
            const float shaped = 0.60f * std::tanh (xIn) + 0.40f * (xIn / std::sqrt (1.0f + 0.45f * xIn * xIn));
            y = (shaped + highs * (0.08f * d)) / (1.0f + d * 0.68f);
        }

        if (speakerBreakup)
        {
            const float spkIn = y * (1.30f + d * 0.60f) + coreFlux * 0.35f;
            const float spkSat = std::tanh (spkIn + 0.05f * d);
            y = 0.72f * spkSat + 0.28f * y;
        }

        // DC Blocker limpio
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
// Voicing de Gabinete (Guitar Cab Mono, Guitar Cab Stereo y Bass Amp)
//==============================================================================
class CabinetVoicingStage
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate > 0.0 ? sampleRate : 44100.0;
        for (int ch = 0; ch < 2; ++ch)
        {
            hpf[ch].prepare (sr);
            lpf[ch].prepare (sr);
            cabBox[ch].prepare (sr);
            coneEdge[ch].prepare (sr);
            harshBit[ch].prepare (sr);
        }
        reset();
    }

    void reset()
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            hpf[ch].reset(); lpf[ch].reset();
            cabBox[ch].reset(); coneEdge[ch].reset(); harshBit[ch].reset();
        }
    }

    void configure (int sourceIdx, float driveAmount)
    {
        active = false;
        stereoSpreadAmount = 0.0f;
        const float drv = std::clamp (driveAmount, 0.0f, 1.0f);

        for (int ch = 0; ch < 2; ++ch)
        {
            hpf[ch].setBypass(); lpf[ch].setBypass();
            cabBox[ch].setBypass(); coneEdge[ch].setBypass(); harshBit[ch].setBypass();
        }

        if (sourceIdx == 4) // GUITAR CAB (MONO)
        {
            active = true;
            for (int ch = 0; ch < 2; ++ch)
            {
                hpf[ch].setHighPass (92.0f, 0.78f);
                lpf[ch].setLowPass (5800.0f, 0.85f);
                cabBox[ch].setPeak (520.0f, 2.4f + drv * 1.4f, 1.15f);
                coneEdge[ch].setPeak (2350.0f, 3.8f + drv * 2.2f, 1.65f);
                harshBit[ch].setPeak (3850.0f, 1.8f + drv * 2.8f, 2.2f);
            }
        }
        else if (sourceIdx == 5) // GUITAR CAB (STEREO)
        {
            active = true;
            stereoSpreadAmount = 0.34f;

            hpf[0].setHighPass (84.0f, 0.75f);
            lpf[0].setLowPass (6700.0f, 0.80f);
            cabBox[0].setPeak (580.0f, 2.0f + drv * 1.2f, 1.1f);
            coneEdge[0].setPeak (2650.0f, 4.5f + drv * 2.4f, 1.75f);
            harshBit[0].setPeak (4150.0f, 2.8f + drv * 3.4f, 2.4f);

            hpf[1].setHighPass (96.0f, 0.78f);
            lpf[1].setLowPass (5950.0f, 0.85f);
            cabBox[1].setPeak (440.0f, 3.2f + drv * 1.6f, 1.2f);
            coneEdge[1].setPeak (2150.0f, 3.6f + drv * 2.0f, 1.55f);
            harshBit[1].setPeak (3550.0f, 2.2f + drv * 3.0f, 2.1f);
        }
        else if (sourceIdx == 6) // BASS AMP (LOW END)
        {
            active = true;
            for (int ch = 0; ch < 2; ++ch)
            {
                hpf[ch].setHighPass (36.0f, 0.82f);
                lpf[ch].setLowPass (3650.0f, 0.85f);
                cabBox[ch].setLowShelf (105.0f, 3.4f + drv * 1.8f);
                coneEdge[ch].setPeak (780.0f, 2.2f + drv * 1.6f, 1.25f);
                harshBit[ch].setPeak (2100.0f, 1.2f + drv * 1.4f, 1.6f);
            }
        }
    }

    void processBuffer (juce::AudioBuffer<float>& buffer) noexcept
    {
        if (! active || buffer.getNumChannels() < 2) return;

        const int numSamples = buffer.getNumSamples();
        auto* L = buffer.getWritePointer (0);
        auto* R = buffer.getWritePointer (1);

        for (int i = 0; i < numSamples; ++i)
        {
            const float lSample = L[i];
            const float rSample = R[i];

            if (stereoSpreadAmount <= 0.001f)
            {
                const float monoIn = 0.5f * (lSample + rSample);
                float out = hpf[0].processSample (0, monoIn);
                out = cabBox[0].processSample (0, out);
                out = coneEdge[0].processSample (0, out);
                out = harshBit[0].processSample (0, out);
                out = lpf[0].processSample (0, out);
                L[i] = out;
                R[i] = out;
            }
            else
            {
                float outL = hpf[0].processSample (0, lSample);
                outL = cabBox[0].processSample (0, outL);
                outL = coneEdge[0].processSample (0, outL);
                outL = harshBit[0].processSample (0, outL);
                outL = lpf[0].processSample (0, outL);

                float outR = hpf[1].processSample (1, rSample);
                outR = cabBox[1].processSample (1, outR);
                outR = coneEdge[1].processSample (1, outR);
                outR = harshBit[1].processSample (1, outR);
                outR = lpf[1].processSample (1, outR);

                const float mid = 0.5f * (outL + outR);
                const float side = 0.5f * (outL - outR) * (1.0f + stereoSpreadAmount);
                L[i] = mid + side;
                R[i] = mid - side;
            }
        }
    }

private:
    double sr = 44100.0;
    bool active = false;
    float stereoSpreadAmount = 0.0f;
    std::array<StereoBiquad, 2> hpf, lpf, cabBox, coneEdge, harshBit;
};

//==============================================================================
// EQ de Consola Analógica TPT-SVF (4x Oversampled) con Inductor Saturation
//==============================================================================
class ConsoleEqWithTubeStage
{
public:
    void prepare (double oversampledRate)
    {
        sr = oversampledRate > 0.0 ? oversampledRate : 176400.0;
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

        // HPF de 18 dB/oct para Neve 1073 y SSL 4000E; 12 dB/oct para Pultec y API
        if (hpfHz > 22.0f)
            hpf.setHighPass (hpfHz, (eqModel == 0 || eqModel == 3) ? 0.78f : 0.7071f, (eqModel == 0 || eqModel == 3));
        else
            hpf.setBypass();

        pultecDip.setBypass();

        if (eqModel == 0) // NEVE 1073 (Inductor British: shelves musicales + medios anchos)
        {
            low.setLowShelf (fLow, lowDb, 0.64f);
            mid.setBell (fMid, midDb, 0.78f + std::abs (midDb) * 0.02f);
            high.setHighShelf (fHigh, highDb, 0.62f);
        }
        else if (eqModel == 1) // PULTEC EQP-1A (Curva pasiva + Pultec Low-End Trick)
        {
            low.setLowShelf (fLow * 0.90f, lowDb * 1.12f, 0.56f);
            if (lowDb > 0.4f)
                pultecDip.setBell (fLow * 2.55f, -lowDb * 0.38f, 1.10f);
            mid.setBell (fMid, midDb, 0.62f);
            high.setHighShelf (fHigh * 0.95f, highDb * 1.06f, 0.55f);
        }
        else if (eqModel == 2) // API 550A (Proportional-Q real en todas las bandas)
        {
            const float lowQ  = 0.62f + std::abs (lowDb)  * 0.045f;
            const float midQ  = 0.75f + std::abs (midDb)  * 0.115f;
            const float highQ = 0.62f + std::abs (highDb) * 0.045f;
            low.setLowShelf (fLow, lowDb, lowQ);
            mid.setBell (fMid, midDb, midQ);
            high.setHighShelf (fHigh * 0.92f, highDb, highQ);
        }
        else // SSL 4000E (Black Knob: quirúrgico, pegada controlada)
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
            // Micro-saturación magnética del inductor solo sobre la banda media impulsada
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
        optoEmph.setHighShelf (2800.0f, 3.2f); // Énfasis clásico R37 del LA-2A
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

    void configure (int compType, float thresholdDb, bool useScFilter, bool linkStereo)
    {
        model = compType;
        thresh = thresholdDb;
        scFilterOn = useScFilter;
        stereoLinked = linkStereo;

        if (compType == 0) // 1176LN (FET Feedback)
        {
            ratio = 6.0f; attackMs = 0.35f; relFastMs = 48.0f; relSlowMs = 140.0f;
            kneeDb = 2.5f; isFeedback = true; isOpto = false; isVariMu = false;
            // Auto-makeup musical calibrado al umbral
            makeupDb = std::clamp ((-thresholdDb - 10.0f) * 0.24f, 0.0f, 7.5f);
        }
        else if (compType == 1) // LA-2A (Opto T4B Feedback con memoria)
        {
            ratio = 3.5f; attackMs = 9.5f; relFastMs = 62.0f; relSlowMs = 680.0f;
            kneeDb = 7.0f; isFeedback = true; isOpto = true; isVariMu = false;
            makeupDb = std::clamp ((-thresholdDb - 10.0f) * 0.20f, 0.0f, 6.0f);
        }
        else if (compType == 2) // Mastering (Vari-Mu Tube Feedforward/RMS)
        {
            ratio = 2.0f; attackMs = 16.0f; relFastMs = 145.0f; relSlowMs = 420.0f;
            kneeDb = 9.5f; isFeedback = false; isOpto = true; isVariMu = true;
            makeupDb = std::clamp ((-thresholdDb - 12.0f) * 0.16f, 0.0f, 5.0f);
        }
        else // Parallel Crush (1176 All-Buttons-In style)
        {
            ratio = 14.0f; attackMs = 0.20f; relFastMs = 38.0f; relSlowMs = 115.0f;
            kneeDb = 1.5f; isFeedback = true; isOpto = true; isVariMu = false;
            makeupDb = 5.0f;
        }

        const float fs = static_cast<float> (sr);
        atkCoeff     = std::exp (-1.0f / (0.001f * attackMs  * fs));
        relFastCoeff = std::exp (-1.0f / (0.001f * relFastMs * fs));
        relSlowCoeff = std::exp (-1.0f / (0.001f * relSlowMs * fs));
        makeupLin    = juce::Decibels::decibelsToGain (makeupDb);
    }

    inline void processStereoSample (float& inL, float& inR, float wetMix, float& frameMaxGr) noexcept
    {
        // En topología Feedback (1176 / LA-2A) detectamos desde la salida previa (1 muestra a 4x OS)
        float srcL = isFeedback ? fbOut[0] : inL;
        float srcR = isFeedback ? fbOut[1] : inR;

        float detL = scFilterOn ? scHpf.processSample (0, srcL) : srcL;
        float detR = scFilterOn ? scHpf.processSample (1, srcR) : srcR;

        if (model == 1) // Énfasis de frecuencias medias-altas en el sidechain del LA-2A
        {
            detL = optoEmph.processSample (0, detL);
            detR = optoEmph.processSample (1, detR);
        }

        float grL = 0.0f, grR = 0.0f;

        if (stereoLinked)
        {
            const float detVal = (model == 2)
                ? std::sqrt (0.5f * (detL * detL + detR * detR) + 1e-12f) // Suma RMS estéreo en Mastering
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

        // Guardamos la salida pre-makeup para el lazo Feedback real
        fbOut[0] = compL;
        fbOut[1] = compR;

        // Color armónico sutil ligado a la reducción de ganancia (GR)
        const float avgGr = 0.5f * (grL + grR);
        if (avgGr > 0.25f)
        {
            const float grit = std::clamp (avgGr * 0.025f, 0.0f, 0.35f);
            if (model == 0 || model == 3) // FET grit (armónicos impares)
            {
                compL = std::tanh (compL * (1.0f + grit * 1.35f)) / (1.0f + grit * 0.95f);
                compR = std::tanh (compR * (1.0f + grit * 1.35f)) / (1.0f + grit * 0.95f);
            }
            else // Tube warmth (armónicos pares suaves)
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

        // Detección híbrida Peak/RMS para Mastering y Opto
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

        // Ratio dinámico en Vari-Mu (crece progresivamente con la intensidad del pico)
        const float effRatio = isVariMu ? (1.45f + std::clamp (overDb * 0.22f, 0.0f, 3.2f)) : ratio;
        const float targetGrDb = overDb * (1.0f - 1.0f / effRatio);

        // Etapa rápida
        if (targetGrDb > envFast[c])
            envFast[c] = atkCoeff * envFast[c] + (1.0f - atkCoeff) * targetGrDb;
        else
            envFast[c] = relFastCoeff * envFast[c] + (1.0f - relFastCoeff) * targetGrDb;

        // Etapa lenta con memoria dependiente del programa (LA-2A / Mastering / Crush)
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
    bool isFeedback = false, isOpto = false, isVariMu = false, scFilterOn = false, stereoLinked = true;

    std::array<float, 2> envFast { 0.0f, 0.0f };
    std::array<float, 2> envSlow { 0.0f, 0.0f };
    std::array<float, 2> rmsSq   { 0.0f, 0.0f };
    std::array<float, 2> fbOut   { 0.0f, 0.0f };
    StereoBiquad scHpf, optoEmph;
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
