#pragma once
#include "AnalogModules.h"

class VocalMicModeler
{
public:
    void prepare (double sampleRate)
    {
        invLow.prepare (sampleRate);  invMid.prepare (sampleRate);  invHigh.prepare (sampleRate);
        tgtLow.prepare (sampleRate);  tgtMid.prepare (sampleRate);  tgtHigh.prepare (sampleRate);
        reset();
    }

    void reset()
    {
        invLow.reset(); invMid.reset(); invHigh.reset();
        tgtLow.reset(); tgtMid.reset(); tgtHigh.reset();
        dcIn = { 0.0f, 0.0f };
        dcOut = { 0.0f, 0.0f };
    }

    void updateCurves (int sourceMicIdx, int targetMicIdx)
    {
        switch (sourceMicIdx)
        {
            case 0: // Shure SM27
                invLow.setLowShelf (180.0f, 0.8f);
                invMid.setPeak (7500.0f, -1.6f, 1.4f);
                invHigh.setHighShelf (12500.0f, -0.6f);
                break;

            case 1: // Shure SM57
                invLow.setLowShelf (135.0f, 3.2f);
                invMid.setPeak (4500.0f, -4.2f, 1.1f);
                invHigh.setHighShelf (11000.0f, 3.8f);
                break;

            case 2: // Shure SM7B
                invLow.setPeak (220.0f, -2.2f, 0.9f);
                invMid.setPeak (3000.0f, 2.8f, 1.0f);
                invHigh.setHighShelf (10500.0f, 3.5f);
                break;

            default: // Generic Large Diaphragm Condenser
                invLow.setPeak (310.0f, -1.5f, 1.2f);
                invMid.setPeak (8800.0f, -2.8f, 1.8f);
                invHigh.setBypass();
                break;
        }

        targetIdx = targetMicIdx;
        switch (targetMicIdx)
        {
            case 0: // Sony C-800G
                tgtLow.setLowShelf (85.0f, -1.2f);
                tgtMid.setPeak (3200.0f, 1.4f, 1.1f);
                tgtHigh.setHighShelf (10500.0f, 4.0f);
                break;

            case 1: // Neumann TLM 103
                tgtLow.setLowShelf (140.0f, 1.4f);
                tgtMid.setPeak (5800.0f, 2.6f, 0.95f);
                tgtHigh.setHighShelf (11500.0f, 2.0f);
                break;

            default: // Aston Origin
                tgtLow.setPeak (240.0f, 1.8f, 0.9f);
                tgtMid.setPeak (2100.0f, 1.4f, 1.1f);
                tgtHigh.setHighShelf (12500.0f, -1.2f);
                break;
        }
    }

    void process (juce::AudioBuffer<float>& buffer, bool enabled, float inputDb, float outputDb)
    {
        if (! enabled) return;

        const float inGain = juce::Decibels::decibelsToGain (inputDb);
        buffer.applyGain (inGain);

        invLow.processBuffer (buffer);
        invMid.processBuffer (buffer);
        invHigh.processBuffer (buffer);

        tgtLow.processBuffer (buffer);
        tgtMid.processBuffer (buffer);
        tgtHigh.processBuffer (buffer);

        float drive = 0.0f, asym = 0.0f;
        if (targetIdx == 0)      { drive = 0.24f; asym = 0.055f; } // C-800G Tube 6AU6
        else if (targetIdx == 1) { drive = 0.04f; asym = 0.005f; } // TLM 103 Transformerless
        else                     { drive = 0.16f; asym = 0.030f; } // Aston Origin Class-A

        const float pre = 1.0f + drive * 1.6f;
        const float post = (1.0f / (1.0f + drive * 0.72f)) * juce::Decibels::decibelsToGain (outputDb);
        const float biasComp = std::tanh (asym);

        const int numCh = std::min (2, buffer.getNumChannels());
        const int numSamples = buffer.getNumSamples();

        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            for (int i = 0; i < numSamples; ++i)
            {
                const float y = std::tanh (data[i] * pre + asym) - biasComp;
                const float dc = y - dcIn[static_cast<size_t> (ch)] + 0.997f * dcOut[static_cast<size_t> (ch)];
                dcIn[static_cast<size_t> (ch)] = y;
                dcOut[static_cast<size_t> (ch)] = dc;
                data[i] = dc * post;
            }
        }
    }

private:
    StereoBiquad invLow, invMid, invHigh;
    StereoBiquad tgtLow, tgtMid, tgtHigh;
    int targetIdx = 0;
    std::array<float, 2> dcIn { 0.0f, 0.0f };
    std::array<float, 2> dcOut { 0.0f, 0.0f };
};
