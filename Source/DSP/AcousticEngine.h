#pragma once
#include <juce_dsp/juce_dsp.h>
#include "AnalogModules.h"
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>
#include <atomic>

//==============================================================================
// Configuración de la Sala Multi-Micrófono 3D
//==============================================================================
struct MultiMicRoomConfig
{
    int modeIdx = 1;            // 0 = Reverb, 1 = Re-Mic
    int sourceIdx = 0;          // 0=Vocals, 1=Drums, 2=Ensemble, 3=Monitors, 4=GtrMono, 5=GtrStereo, 6=Bass
    int roomIdx = 0;            // 0=Studio A (12.5m), 1=Studio B (8.5m), 2=Volcano (20m)
    int wallsLive = 1;          // 0=Tight/Damped, 1=Live/Cedar Wood
    bool phaseSafe = true;      // ON = Retardo real + Decorrelación All-Pass; OFF = Crudo + Micro-Mod Doubler
    bool phaseInvert = false;
    bool mainStereo = false;
    bool deMudOn = false;
    int soloStandIdx = -1;      // -1 = Ninguno, 0..3 = Solo atril específico

    float pairSpread = 1.0f;    // Ancho del par 2: ROOM PAIR
    float roomAirTail = 0.35f;  // Nueva perilla AIR / TAIL (0.0 a 1.0)

    bool roomParallelCrush = false;
    float roomParallelAmt = 0.35f;

    std::array<float, 4> micX    { 0.0f, -0.32f, -0.52f,  0.52f };
    std::array<float, 4> micY    { 0.18f, 0.54f,  0.78f,  0.78f };
    std::array<float, 4> micGain { 1.00f, 0.25f,  0.00f,  0.00f };
};

//==============================================================================
// Línea de Retardo con Interpolación Lineal (para retardos físicos y modulación)
//==============================================================================
class SimpleDelayLine
{
public:
    void prepare (int maxSamples)
    {
        buffer.assign (static_cast<size_t> (std::max (64, maxSamples)), 0.0f);
        writePos = 0;
    }

    void reset()
    {
        std::fill (buffer.begin(), buffer.end(), 0.0f);
        writePos = 0;
    }

    inline void push (float x) noexcept
    {
        buffer[static_cast<size_t> (writePos)] = x;
        if (++writePos >= static_cast<int> (buffer.size()))
            writePos = 0;
    }

    inline float read (float delaySamples) const noexcept
    {
        const int size = static_cast<int> (buffer.size());
        const float d = std::clamp (delaySamples, 1.0f, static_cast<float> (size - 2));
        float readPos = static_cast<float> (writePos) - d;
        while (readPos < 0.0f) readPos += static_cast<float> (size);

        const int i0 = static_cast<int> (readPos) % size;
        const int i1 = (i0 + 1) % size;
        const float frac = readPos - std::floor (readPos);
        return buffer[static_cast<size_t> (i0)] * (1.0f - frac) + buffer[static_cast<size_t> (i1)] * frac;
    }

private:
    std::vector<float> buffer;
    int writePos = 0;
};

//==============================================================================
// Filtro All-Pass Corto (para Decorrelación de Fase y Micro-Difusión de Aire)
//==============================================================================
class ShortAllPass
{
public:
    void prepare (int delaySamples, float feedback = 0.5f)
    {
        size = std::max (4, delaySamples);
        buf.assign (static_cast<size_t> (size), 0.0f);
        g = std::clamp (feedback, -0.85f, 0.85f);
        idx = 0;
    }

    void reset()
    {
        std::fill (buf.begin(), buf.end(), 0.0f);
        idx = 0;
    }

    inline float process (float x) noexcept
    {
        const float delayed = buf[static_cast<size_t> (idx)];
        const float y = -g * x + delayed;
        buf[static_cast<size_t> (idx)] = x + g * y;
        if (++idx >= size) idx = 0;
        return y;
    }

private:
    std::vector<float> buf;
    int size = 16;
    int idx = 0;
    float g = 0.5f;
};

//==============================================================================
// FDN Estéreo de 8 Líneas con Matriz Hadamard (Sala y Echo Chambers)
//==============================================================================
class HadamardFdn8
{
public:
    void prepare (double sampleRate, const std::array<float, 8>& baseDelaysMs)
    {
        sr = sampleRate > 0.0 ? sampleRate : 44100.0;
        for (size_t i = 0; i < 8; ++i)
        {
            const int maxSamps = static_cast<int> (std::ceil (baseDelaysMs[i] * 0.001 * sr * 2.5)) + 64;
            lines[i].prepare (maxSamps);
            baseDelays[i] = baseDelaysMs[i] * 0.001f * static_cast<float> (sr);
        }
        reset();
    }

    void reset()
    {
        for (auto& l : lines) l.reset();
        lpState.fill (0.0f);
        hpState.fill (0.0f);
    }

    inline void processStereo (float inL, float inR, float& outL, float& outR,
                               float sizeScale, float feedback, float dampCoeff, float hpCoeff) noexcept
    {
        std::array<float, 8> tap {};
        for (size_t i = 0; i < 8; ++i)
        {
            float s = lines[i].read (baseDelays[i] * sizeScale);
            // Damping de agudos (absorción de paredes/aire)
            lpState[i] += dampCoeff * (s - lpState[i]);
            s = lpState[i];
            // HPF suave para evitar acumulación de subgraves
            hpState[i] += hpCoeff * (s - hpState[i]);
            s -= hpState[i];
            tap[i] = s;
        }

        // Matriz Hadamard 8x8 normalizada (1 / sqrt(8) = 0.35355339)
        std::array<float, 8> h = tap;
        for (int step = 1; step < 8; step <<= 1)
        {
            for (int i = 0; i < 8; i += step * 2)
            {
                for (int j = 0; j < step; ++j)
                {
                    const float a = h[static_cast<size_t> (i + j)];
                    const float b = h[static_cast<size_t> (i + j + step)];
                    h[static_cast<size_t> (i + j)]        = a + b;
                    h[static_cast<size_t> (i + j + step)] = a - b;
                }
            }
        }
        constexpr float norm = 0.35355339f;

        const float fb = std::clamp (feedback, 0.0f, 0.965f);
        for (size_t i = 0; i < 8; ++i)
        {
            const float inject = ((i & 1) == 0 ? inL : inR) * 0.35f;
            lines[i].push (inject + h[i] * norm * fb);
        }

        outL = (tap[0] + tap[2] + tap[4] + tap[6]) * 0.35f;
        outR = (tap[1] + tap[3] + tap[5] + tap[7]) * 0.35f;
    }

private:
    double sr = 44100.0;
    std::array<SimpleDelayLine, 8> lines;
    std::array<float, 8> baseDelays {};
    std::array<float, 8> lpState {};
    std::array<float, 8> hpState {};
};

//==============================================================================
// Motor Acústico Principal: Sala 3D Multi-Mic + Echo Chambers + Custom WAV IR
//==============================================================================
class AcousticEngine
{
public:
    void prepare (double sampleRate, int samplesPerBlock)
    {
        sr = sampleRate > 0.0 ? sampleRate : 44100.0;
        const int maxRoomDelay = static_cast<int> (sr * 0.25) + 64; // hasta 250 ms
        const int maxPreDelay  = static_cast<int> (sr * 0.20) + 64;

        srcDelayL.prepare (maxRoomDelay);
        srcDelayR.prepare (maxRoomDelay);
        erDelayL.prepare (maxRoomDelay);
        erDelayR.prepare (maxRoomDelay);
        chPreDelayL.prepare (maxPreDelay);
        chPreDelayR.prepare (maxPreDelay);

        // Decorreladores All-Pass para PHASE SAFE (primos cortos en muestras)
        const float srScale = static_cast<float> (sr / 44100.0);
        apRoomL[0].prepare (static_cast<int> (19 * srScale), 0.52f);
        apRoomL[1].prepare (static_cast<int> (43 * srScale), 0.48f);
        apRoomR[0].prepare (static_cast<int> (23 * srScale), 0.52f);
        apRoomR[1].prepare (static_cast<int> (53 * srScale), 0.48f);

        apFarL[0].prepare  (static_cast<int> (31 * srScale), 0.55f);
        apFarL[1].prepare  (static_cast<int> (67 * srScale), 0.50f);
        apFarR[0].prepare  (static_cast<int> (37 * srScale), 0.55f);
        apFarR[1].prepare  (static_cast<int> (79 * srScale), 0.50f);

        // FDN Sala (tiempos primos en ms)
        roomFdn.prepare (sr, { 11.3f, 15.7f, 21.1f, 27.7f, 34.9f, 43.3f, 52.1f, 61.9f });

        // FDN Echo Chambers (tiempos más densos y brillantes)
        chamberFdn.prepare (sr, { 13.1f, 19.3f, 26.9f, 35.3f, 46.1f, 58.7f, 71.9f, 86.3f });

        roomAirShelf.prepare (sr);
        roomHpfProtect.prepare (sr);
        farHpfProtect.prepare (sr);
        roomCrushComp.prepare (sr);
        roomCrushComp.configure (3, -24.0f, true, true, true);

        chamberHpf.prepare (sr);
        chamberTone.prepare (sr);
        chamberCrushComp.prepare (sr);
        chamberCrushComp.configure (3, -24.0f, true, true, true);

        juce::dsp::ProcessSpec spec { sr, static_cast<juce::uint32> (std::max (1, samplesPerBlock)), 2 };
        customConvolution.prepare (spec);

        lfoPhaseL = 0.0f;
        lfoPhaseR = 1.5707963f; // 90 grados de desfase

        reset();
    }

    void reset()
    {
        srcDelayL.reset(); srcDelayR.reset();
        erDelayL.reset(); erDelayR.reset();
        chPreDelayL.reset(); chPreDelayR.reset();
        for (int i = 0; i < 2; ++i)
        {
            apRoomL[i].reset(); apRoomR[i].reset();
            apFarL[i].reset();  apFarR[i].reset();
        }
        roomFdn.reset();
        chamberFdn.reset();
        roomAirShelf.reset();
        roomHpfProtect.reset();
        farHpfProtect.reset();
        roomCrushComp.reset();
        chamberHpf.reset();
        chamberTone.reset();
        chamberCrushComp.reset();
        customConvolution.reset();
        airLpState.fill (0.0f);
        lastAirTail = -1.0f;
        lastChamberType = -1;
    }

    void loadCustomImpulseResponse (const juce::File& file)
    {
        if (! file.existsAsFile()) return;
        customConvolution.loadImpulseResponse (file,
                                               juce::dsp::Convolution::Stereo::yes,
                                               juce::dsp::Convolution::Trim::yes,
                                               0);
        loadedIrName = file.getFileNameWithoutExtension();
        hasCustomIr.store (true);
    }

    juce::String getLoadedIRName() const { return loadedIrName; }

    //==========================================================================
    // PROCESAMIENTO DE SALA 3D MULTI-MICRÓFONO
    //==========================================================================
    void processMultiMicRoom (juce::AudioBuffer<float>& buffer, const MultiMicRoomConfig& cfg)
    {
        if (buffer.getNumChannels() < 2) return;
        const int numSamples = buffer.getNumSamples();
        const float fs = static_cast<float> (sr);

        // 1. Parámetros físicos de las 3 salas
        // Studio A (Shangri-La): 12.5m | Studio B (Canyon): 8.5m | Volcano: 20.0m
        const float roomLenM   = (cfg.roomIdx == 0) ? 12.5f : (cfg.roomIdx == 1 ? 8.5f : 20.0f);
        const float roomWidthM = roomLenM * 0.78f;
        const bool isLive      = (cfg.wallsLive == 1);

        // Distancia crítica física (Dc): en Tight es más larga (domina el directo), en Live más corta (domina lo difuso)
        const float critDistM = roomLenM * (isLive ? 0.24f : 0.42f);

        // Tamaño y decaimiento del FDN de sala
        const float fdnSize = (cfg.roomIdx == 0) ? 1.00f : (cfg.roomIdx == 1 ? 0.72f : 1.48f);
        const float baseFb  = (cfg.roomIdx == 0) ? (isLive ? 0.76f : 0.56f)
                            : (cfg.roomIdx == 1) ? (isLive ? 0.68f : 0.48f)
                                                 : (isLive ? 0.85f : 0.66f);
        const float airTail = std::clamp (cfg.roomAirTail, 0.0f, 1.0f);
        const float fdnFb   = std::clamp (baseFb + airTail * 0.07f, 0.35f, 0.91f);

        // Damping de agudos en la cola (AIR / TAIL abre sedosamente sin volverse chillón)
        const float dampHz = juce::jmap (airTail, 0.0f, 1.0f,
                                         isLive ? 4200.0f : 2600.0f,
                                         isLive ? 9800.0f : 6200.0f);
        const float dampCoeff = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * dampHz / fs);
        const float fdnHpCoeff = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 115.0f / fs);

        // Actualizar filtros solo si cambia Air/Tail o PhaseSafe
        if (std::abs (airTail - lastAirTail) > 0.005f || cfg.phaseSafe != lastPhaseSafe)
        {
            const float shelfDb = juce::jmap (airTail, 0.0f, 1.0f, -1.5f, 3.8f);
            roomAirShelf.setHighShelf (6500.0f, shelfDb);
            roomHpfProtect.setHighPass (cfg.phaseSafe ? 165.0f : 110.0f, 0.7071f);
            farHpfProtect.setHighPass  (cfg.phaseSafe ? 190.0f : 120.0f, 0.7071f);
            lastAirTail = airTail;
            lastPhaseSafe = cfg.phaseSafe;
        }

        // 2. Geometría de los 4 atriles (Distancia física, Relación Directo/Difuso, Muros Laterales)
        constexpr float speedOfSound = 343.0f;
        std::array<float, 4> distM {}, delaySamps {}, directRatio {}, diffuseRatio {}, airLpCoeff {};
        std::array<float, 4> wallDelayL {}, wallDelayR {}, wallGainL {}, wallGainR {};
        std::array<float, 4> panL {}, panR {}, effectiveGain {};

        float totalGainSum = 0.0f;

        for (int m = 0; m < 4; ++m)
        {
            const bool isSoloed = (cfg.soloStandIdx >= 0);
            const float g = (isSoloed && cfg.soloStandIdx != m) ? 0.0f : cfg.micGain[static_cast<size_t> (m)];
            effectiveGain[static_cast<size_t> (m)] = g;
            totalGainSum += g;

            const float mx = std::clamp (cfg.micX[static_cast<size_t> (m)], -1.0f, 1.0f);
            const float my = std::clamp (cfg.micY[static_cast<size_t> (m)],  0.0f, 1.0f);

            const float posXM = mx * (roomWidthM * 0.48f);
            const float posYM = 0.35f + my * (roomLenM * 0.85f);
            const float dM = std::sqrt (posXM * posXM + posYM * posYM);

            distM[static_cast<size_t> (m)] = dM;
            delaySamps[static_cast<size_t> (m)] = (dM / speedOfSound) * fs;

            // Ley de Distancia Crítica: Directo cae como 1/(1 + d/Dc), Difuso crece con d
            const float dir = 1.0f / (1.0f + (dM / critDistM) * (m == 0 ? 0.55f : 1.05f));
            directRatio[static_cast<size_t> (m)]  = dir;
            diffuseRatio[static_cast<size_t> (m)] = std::clamp (1.0f - dir * 0.72f, 0.15f, 1.0f);

            // Absorción natural del aire por metros (suaviza agudos a mayor distancia)
            const float airCutHz = std::clamp (18500.0f / (1.0f + dM * 0.18f), 2800.0f, 18500.0f);
            airLpCoeff[static_cast<size_t> (m)] = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * airCutHz / fs);

            // Interacción con muros laterales (X): distancia al muro izquierdo y derecho
            const float distWallL = std::max (0.4f, (roomWidthM * 0.5f) + posXM);
            const float distWallR = std::max (0.4f, (roomWidthM * 0.5f) - posXM);
            const float wallBaseGain = isLive ? 0.32f : 0.14f;

            wallDelayL[static_cast<size_t> (m)] = ((dM + distWallL * 1.35f) / speedOfSound) * fs;
            wallDelayR[static_cast<size_t> (m)] = ((dM + distWallR * 1.35f) / speedOfSound) * fs;
            wallGainL[static_cast<size_t> (m)]  = wallBaseGain / (1.0f + distWallL * 0.35f);
            wallGainR[static_cast<size_t> (m)]  = wallBaseGain / (1.0f + distWallR * 0.35f);

            // Panorama natural según X (sutil en MAIN, completo en FAR L/R)
            const float panScale = (m == 0) ? 0.45f : (m == 1 ? 0.65f : 0.92f);
            const float p = std::clamp (mx * panScale, -1.0f, 1.0f);
            const float angle = (p + 1.0f) * 0.25f * juce::MathConstants<float>::pi;
            panL[static_cast<size_t> (m)] = std::cos (angle);
            panR[static_cast<size_t> (m)] = std::sin (angle);
        }

        // Compensación suave de Headroom al sumar múltiples atriles (evita saturar fácil)
        const float sumNorm = (totalGainSum > 1.15f) ? (1.0f / std::sqrt (totalGainSum * 0.85f)) : 1.0f;

        // LFO lento para micro-modulación orgánica en FAR L/R cuando PHASE SAFE = OFF
        const float lfoIncL = juce::MathConstants<float>::twoPi * 0.37f / fs;
        const float lfoIncR = juce::MathConstants<float>::twoPi * 0.43f / fs;
        const float modDepthSamps = 0.00024f * fs; // +/- 0.24 ms (chorus/doubler orgánico)

        // Carácter por fuente: en Drums/Bass el ambiente es un poco más cálido
        const float roomTailGain = (cfg.modeIdx == 0 ? 0.42f : 0.34f) * (isLive ? 1.12f : 0.68f);
        const float polarity = cfg.phaseInvert ? -1.0f : 1.0f;

        auto* ptrL = buffer.getWritePointer (0);
        auto* ptrR = buffer.getWritePointer (1);
        float maxRoomCrushGr = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            const float inL = ptrL[i];
            const float inR = ptrR[i];
            const float inMono = 0.5f * (inL + inR);

            srcDelayL.push (inL);
            srcDelayR.push (inR);
            erDelayL.push (inL);
            erDelayR.push (inR);

            //------------------------------------------------------------------
            // 1: MAIN (Ancla directa + X-Y estéreo opcional + rebote lateral X)
            //------------------------------------------------------------------
            float mainOutL = 0.0f, mainOutR = 0.0f;
            if (effectiveGain[0] > 0.0005f)
            {
                const float d0 = delaySamps[0];
                float mL = cfg.mainStereo ? srcDelayL.read (d0) : 0.5f * (srcDelayL.read (d0) + srcDelayR.read (d0));
                float mR = cfg.mainStereo ? srcDelayR.read (d0) : mL;

                // Absorción de aire leve según distancia de MAIN
                airLpState[0] += airLpCoeff[0] * (mL - airLpState[0]);
                airLpState[1] += airLpCoeff[0] * (mR - airLpState[1]);
                mL = mL * 0.65f + airLpState[0] * 0.35f;
                mR = mR * 0.65f + airLpState[1] * 0.35f;

                // Reflexión temprana de muros laterales según X de MAIN
                const float erML = erDelayL.read (wallDelayL[0]) * wallGainL[0];
                const float erMR = erDelayR.read (wallDelayR[0]) * wallGainR[0];

                const float dir0 = directRatio[0];
                mainOutL = (mL * dir0 * panL[0] * 1.414f + erML * 0.55f) * effectiveGain[0];
                mainOutR = (mR * dir0 * panR[0] * 1.414f + erMR * 0.55f) * effectiveGain[0];
            }

            //------------------------------------------------------------------
            // 2: ROOM PAIR (Par estéreo medio + All-Pass en Phase Safe)
            //------------------------------------------------------------------
            float roomPairL = 0.0f, roomPairR = 0.0f;
            if (effectiveGain[1] > 0.0005f)
            {
                const float spreadSamps = cfg.pairSpread * 0.00085f * fs;
                float rL = srcDelayL.read (delaySamps[1] + spreadSamps * 0.35f);
                float rR = srcDelayR.read (delaySamps[1] + spreadSamps * 0.85f);

                // Absorción de aire + suavizado de transitorios
                airLpState[2] += airLpCoeff[1] * (rL - airLpState[2]);
                airLpState[3] += airLpCoeff[1] * (rR - airLpState[3]);
                rL = airLpState[2];
                rR = airLpState[3];

                // Protección de graves para no adelgazar la voz al sumar con MAIN
                rL = roomHpfProtect.processSample (0, rL);
                rR = roomHpfProtect.processSample (1, rR);

                if (cfg.phaseSafe)
                {
                    rL = apRoomL[1].process (apRoomL[0].process (rL));
                    rR = apRoomR[1].process (apRoomR[0].process (rR));
                }

                // Muros laterales según X del par
                const float erRL = erDelayL.read (wallDelayL[1]) * wallGainL[1];
                const float erRR = erDelayR.read (wallDelayR[1]) * wallGainR[1];

                // Ancho M/S controlado por pairSpread
                const float mid  = 0.5f * (rL + rR);
                const float side = 0.5f * (rL - rR) * std::clamp (cfg.pairSpread, 0.0f, 1.5f);

                roomPairL = ((mid + side) * directRatio[1] * panL[1] * 1.25f + erRL) * effectiveGain[1];
                roomPairR = ((mid - side) * directRatio[1] * panR[1] * 1.25f + erRR) * effectiveGain[1];
            }

            //------------------------------------------------------------------
            // 3: FAR L y 4: FAR R (Profundidad 3D, Muros Laterales y Doubler)
            //------------------------------------------------------------------
            float farOutL = 0.0f, farOutR = 0.0f;

            lfoPhaseL += lfoIncL;
            if (lfoPhaseL > juce::MathConstants<float>::twoPi) lfoPhaseL -= juce::MathConstants<float>::twoPi;
            lfoPhaseR += lfoIncR;
            if (lfoPhaseR > juce::MathConstants<float>::twoPi) lfoPhaseR -= juce::MathConstants<float>::twoPi;

            if (effectiveGain[2] > 0.0005f)
            {
                const float modL = cfg.phaseSafe ? 0.0f : (std::sin (lfoPhaseL) * modDepthSamps);
                float fL = srcDelayL.read (delaySamps[2] + modL);

                airLpState[4] += airLpCoeff[2] * (fL - airLpState[4]);
                fL = farHpfProtect.processSample (0, airLpState[4]);

                if (cfg.phaseSafe)
                    fL = apFarL[1].process (apFarL[0].process (fL));

                const float erFL = erDelayL.read (wallDelayL[2]) * wallGainL[2];
                const float erFR = erDelayR.read (wallDelayR[2]) * wallGainR[2];

                const float sig = (fL * directRatio[2] + (erFL + erFR) * 0.65f) * effectiveGain[2];
                farOutL += sig * panL[2] * 1.35f;
                farOutR += sig * panR[2] * 1.35f;
            }

            if (effectiveGain[3] > 0.0005f)
            {
                const float modR = cfg.phaseSafe ? 0.0f : (std::sin (lfoPhaseR) * modDepthSamps);
                float fR = srcDelayR.read (delaySamps[3] + modR);

                airLpState[5] += airLpCoeff[3] * (fR - airLpState[5]);
                fR = farHpfProtect.processSample (1, airLpState[5]);

                if (cfg.phaseSafe)
                    fR = apFarR[1].process (apFarR[0].process (fR));

                const float erFL = erDelayL.read (wallDelayL[3]) * wallGainL[3];
                const float erFR = erDelayR.read (wallDelayR[3]) * wallGainR[3];

                const float sig = (fR * directRatio[3] + (erFL + erFR) * 0.65f) * effectiveGain[3];
                farOutL += sig * panL[3] * 1.35f;
                farOutR += sig * panR[3] * 1.35f;
            }

            //------------------------------------------------------------------
            // Campo Difuso de la Sala (FDN 8x8 alimentado según distancia crítica)
            //------------------------------------------------------------------
            const float fdnFeedScale = (effectiveGain[0] * diffuseRatio[0] * 0.28f
                                      + effectiveGain[1] * diffuseRatio[1] * 0.55f
                                      + effectiveGain[2] * diffuseRatio[2] * 0.65f
                                      + effectiveGain[3] * diffuseRatio[3] * 0.65f);

            float tailL = 0.0f, tailR = 0.0f;
            roomFdn.processStereo (inL * fdnFeedScale, inR * fdnFeedScale, tailL, tailR,
                                   fdnSize, fdnFb, dampCoeff, fdnHpCoeff);

            // Brillo sedoso AIR / TAIL aplicado a las reflexiones + cola difusa
            float ambL = roomPairL + farOutL + tailL * roomTailGain;
            float ambR = roomPairR + farOutR + tailR * roomTailGain;

            ambL = roomAirShelf.processSample (0, ambL);
            ambR = roomAirShelf.processSample (1, ambR);

            // Parallel Crush dedicado a Room Mics si está activo
            if (cfg.roomParallelCrush && cfg.roomParallelAmt > 0.001f)
            {
                float cL = ambL, cR = ambR;
                roomCrushComp.processStereoSample (cL, cR, 1.0f, maxRoomCrushGr);
                ambL = ambL + (cL - ambL) * cfg.roomParallelAmt;
                ambR = ambR + (cR - ambR) * cfg.roomParallelAmt;
            }

            // Suma final calibrada (Modo Reverb mezcla Dry + Sala; Modo Re-Mic es 100% microfoneo)
            float outL = (mainOutL + ambL) * sumNorm * polarity;
            float outR = (mainOutR + ambR) * sumNorm * polarity;

            if (cfg.modeIdx == 0) // Reverb Mode
            {
                outL = inL * 0.72f + outL * 0.58f;
                outR = inR * 0.72f + outR * 0.58f;
            }

            ptrL[i] = outL;
            ptrR[i] = outR;
        }
    }

    //==========================================================================
    // ECHO CHAMBERS SUBTERRÁNEAS + CUSTOM WAV IR
    //==========================================================================
    void processChamber (juce::AudioBuffer<float>& buffer,
                         juce::AudioBuffer<float>& scratch,
                         bool enabled, int chamberType, float mix,
                         float decay, float preDelayMs, float width,
                         bool /*unused*/, bool parallelCrush, float parallelAmt)
    {
        if (! enabled || mix <= 0.001f || buffer.getNumChannels() < 2)
            return;

        const int numSamples = buffer.getNumSamples();
        const float fs = static_cast<float> (sr);

        if (scratch.getNumSamples() < numSamples)
            scratch.setSize (2, numSamples, false, false, true);

        // 1. Custom WAV IR
        if (chamberType == 4 && hasCustomIr.load())
        {
            scratch.copyFrom (0, 0, buffer, 0, 0, numSamples);
            scratch.copyFrom (1, 0, buffer, 1, 0, numSamples);

            juce::dsp::AudioBlock<float> irBlock (scratch.getArrayOfWritePointers(), 2, static_cast<size_t> (numSamples));
            juce::dsp::ProcessContextReplacing<float> ctx (irBlock);
            customConvolution.process (ctx);

            auto* dstL = buffer.getWritePointer (0);
            auto* dstR = buffer.getWritePointer (1);
            auto* wetL = scratch.getWritePointer (0);
            auto* wetR = scratch.getWritePointer (1);

            for (int i = 0; i < numSamples; ++i)
            {
                const float mid  = 0.5f * (wetL[i] + wetR[i]);
                const float side = 0.5f * (wetL[i] - wetR[i]) * width;
                dstL[i] += (mid + side) * mix;
                dstR[i] += (mid - side) * mix;
            }
            return;
        }

        // 2. Echo Chambers Algorítmicas (1: Bright, 3: Classic, 5: Deep, 8: Dense)
        if (chamberType != lastChamberType)
        {
            if (chamberType == 0)      { chamberHpf.setHighPass (260.0f, 0.7f); chamberTone.setHighShelf (4200.0f,  2.8f); }
            else if (chamberType == 1) { chamberHpf.setHighPass (200.0f, 0.7f); chamberTone.setPeak      (2400.0f,  1.6f, 0.9f); }
            else if (chamberType == 2) { chamberHpf.setHighPass (150.0f, 0.7f); chamberTone.setHighShelf (3800.0f, -1.8f); }
            else                       { chamberHpf.setHighPass (180.0f, 0.7f); chamberTone.setHighShelf (5200.0f,  1.2f); }
            lastChamberType = chamberType;
        }

        const float sizeScale = (chamberType == 0) ? 0.88f : (chamberType == 1 ? 1.02f : (chamberType == 2 ? 1.28f : 1.14f));
        const float fb        = juce::jmap (std::clamp (decay, 0.15f, 1.0f), 0.15f, 1.0f, 0.58f, 0.935f);
        const float dampHz    = (chamberType == 0) ? 10500.0f : (chamberType == 1 ? 7600.0f : (chamberType == 2 ? 4800.0f : 6800.0f));
        const float dampCoeff = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * dampHz / fs);
        const float hpCoeff   = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 140.0f / fs);
        const float preSamps  = std::clamp (preDelayMs, 0.0f, 150.0f) * 0.001f * fs;

        auto* ptrL = buffer.getWritePointer (0);
        auto* ptrR = buffer.getWritePointer (1);
        float maxChGr = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            const float inL = ptrL[i];
            const float inR = ptrR[i];

            chPreDelayL.push (inL);
            chPreDelayR.push (inR);

            const float sendL = (preSamps > 1.0f) ? chPreDelayL.read (preSamps) : inL;
            const float sendR = (preSamps > 1.0f) ? chPreDelayR.read (preSamps) : inR;

            float wL = 0.0f, wR = 0.0f;
            chamberFdn.processStereo (sendL, sendR, wL, wR, sizeScale, fb, dampCoeff, hpCoeff);

            wL = chamberTone.processSample (0, chamberHpf.processSample (0, wL));
            wR = chamberTone.processSample (1, chamberHpf.processSample (1, wR));

            if (parallelCrush && parallelAmt > 0.001f)
            {
                float cL = wL, cR = wR;
                chamberCrushComp.processStereoSample (cL, cR, 1.0f, maxChGr);
                wL = wL + (cL - wL) * parallelAmt;
                wR = wR + (cR - wR) * parallelAmt;
            }

            const float mid  = 0.5f * (wL + wR);
            const float side = 0.5f * (wL - wR) * std::clamp (width, 0.0f, 1.5f);

            ptrL[i] = inL + (mid + side) * mix;
            ptrR[i] = inR + (mid - side) * mix;
        }
    }

private:
    double sr = 44100.0;

    SimpleDelayLine srcDelayL, srcDelayR;
    SimpleDelayLine erDelayL, erDelayR;
    SimpleDelayLine chPreDelayL, chPreDelayR;

    std::array<ShortAllPass, 2> apRoomL, apRoomR, apFarL, apFarR;
    HadamardFdn8 roomFdn, chamberFdn;

    StereoBiquad roomAirShelf, roomHpfProtect, farHpfProtect;
    StereoBiquad chamberHpf, chamberTone;
    VintageCompressor roomCrushComp, chamberCrushComp;

    juce::dsp::Convolution customConvolution;
    juce::String loadedIrName { "None" };
    std::atomic<bool> hasCustomIr { false };

    std::array<float, 6> airLpState {};
    float lfoPhaseL = 0.0f, lfoPhaseR = 1.5707963f;
    float lastAirTail = -1.0f;
    bool lastPhaseSafe = true;
    int lastChamberType = -1;
};
