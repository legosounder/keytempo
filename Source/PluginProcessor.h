#pragma once

#include <JuceHeader.h>

class KeyTempoAudioProcessor  : public juce::AudioProcessor
{
public:
    KeyTempoAudioProcessor();
    ~KeyTempoAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Calculator State
    double baseTempo = 140.0;
    int baseKeyIndex = 0;      // 0: C, 1: C#, etc.
    int baseScaleIndex = 0;    // 0: Major, 1: Minor
    bool useHostTempo = false;
    bool isTempoLocked = false;
    double lockedTempo = 140.0;
    bool initialBpmCaptured = false;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KeyTempoAudioProcessor)
};
