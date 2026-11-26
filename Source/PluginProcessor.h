#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/AnalogModules.h"
#include "DSP/VocalMicModeler.h"
#include "DSP/AcousticEngine.h"
#include <atomic>

class BenBocazStudioProAudioProcessor : public juce::AudioProcessor
{
public:
    BenBocazStudioProAudioProcessor();
    ~BenBocazStudioProAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Los Andes Capital"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.5; }

    int getNumPrograms() override { return 6; }
    int getCurrentProgram() override { return currentPresetIndex; }
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void applyFactoryPreset(int presetIndex);

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts;

    // Medidores atomicos para la UI
    std::atomic<float> vuOutDb { -60.0f };
    std::atomic<float> vuGrDb  { 0.0f };

private:
    BenBocazDSP::VocalMicModeler vocalModeler;
    BenBocazDSP::OversampledConsoleChain consoleChain;
    BenBocazDSP::AcousticEngine acousticEngine;
    BenBocazDSP::VintageCompressor parallelComp;

    juce::AudioBuffer<float> roomOnlyBuffer;
    juce::AudioBuffer<float> parCrushBuffer;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> outGainSmooth;
    int currentPresetIndex = 0;
    int lastReportedLatency = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BenBocazStudioProAudioProcessor)
};
