#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/AnalogModules.h"
#include "DSP/VocalMicModeler.h"
#include "DSP/AcousticEngine.h"

class BenBocazStudioProProcessor : public juce::AudioProcessor
{
public:
    BenBocazStudioProProcessor();
    ~BenBocazStudioProProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 5.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void loadCustomIR (const juce::File& file) { acousticEngine.loadCustomImpulseResponse (file); }
    juce::String getLoadedIRName() const { return acousticEngine.getLoadedIRName(); }

    float getOutputVuDb() const { return outputVuDb.load(); }
    float getCompGrDb() const { return consoleChain.getMainCompGrDb(); }

    void setSoloStand (int idx) { soloStandIndex.store (idx); }
    int getSoloStand() const { return soloStandIndex.load(); }

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void updateFiltersAndChain (bool force = false);
    void processConsoleStage (juce::AudioBuffer<float>& buffer, bool speakerBreakup, bool hqMode);
    void processChamberStage (juce::AudioBuffer<float>& buffer);

    double currentSampleRate = 44100.0;

    StereoBiquad srcHpf, srcLpf, srcMid;
    CabinetVoicingStage cabVoicing;

    VocalMicModeler vocalMicModeler;
    OversampledConsoleChain consoleChain;
    AcousticEngine acousticEngine;

    juce::AudioBuffer<float> chamberScratch;
    juce::SmoothedValue<float> smoothedOutGain { 1.0f };

    std::atomic<float> outputVuDb { -60.0f };
    std::atomic<int> soloStandIndex { -1 };

    // Caché para evitar recalcular coeficientes cuando no se mueven perillas (ahorro de CPU)
    int lastVSrc = -1, lastVTgt = -1;
    int lastSrcIdx = -1, lastMonIdx = -1;
    float lastDrvAmt = -1.0f;
    float lastOffAxis = -1.0f;
    int lastEqType = -1, lastTubeModel = -1, lastLowF = -1, lastMidF = -1, lastHighF = -1;
    float lastHpf = -1.0f, lastLow = -99.0f, lastMid = -99.0f, lastHigh = -99.0f;
    int lastCompType = -1;
    float lastThresh = -99.0f;
    bool lastScLink = false, lastScFilter = false;
    bool lastHqMode = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BenBocazStudioProProcessor)
};
