#include "PluginProcessor.h"
#include "PluginEditor.h"

KeyTempoAudioProcessor::KeyTempoAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

KeyTempoAudioProcessor::~KeyTempoAudioProcessor()
{
}

const juce::String KeyTempoAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool KeyTempoAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool KeyTempoAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool KeyTempoAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double KeyTempoAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int KeyTempoAudioProcessor::getNumPrograms()
{
    return 1;
}

int KeyTempoAudioProcessor::getCurrentProgram()
{
    return 0;
}

void KeyTempoAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String KeyTempoAudioProcessor::getProgramName (int index)
{
    return {};
}

void KeyTempoAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

void KeyTempoAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
}

void KeyTempoAudioProcessor::releaseResources()
{
}

bool KeyTempoAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    juce::ignoreUnused (layouts);
    return true;
}

void KeyTempoAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    if (!initialBpmCaptured)
    {
        if (auto* playHead = getPlayHead())
        {
            if (auto position = playHead->getPosition())
            {
                if (auto bpm = position->getBpm())
                {
                    baseTempo = *bpm;
                    lockedTempo = *bpm;
                    initialBpmCaptured = true;
                }
            }
        }
    }
    else if (useHostTempo && !isTempoLocked)
    {
        if (auto* playHead = getPlayHead())
        {
            if (auto position = playHead->getPosition())
            {
                if (auto bpm = position->getBpm())
                {
                    baseTempo = *bpm;
                }
            }
        }
    }
}

bool KeyTempoAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* KeyTempoAudioProcessor::createEditor()
{
    return new KeyTempoAudioProcessorEditor (*this);
}

void KeyTempoAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Create an XML element to store our settings
    juce::XmlElement xml ("KeyTempoSettings");
    xml.setAttribute ("baseTempo", baseTempo);
    xml.setAttribute ("baseKeyIndex", baseKeyIndex);
    xml.setAttribute ("baseScaleIndex", baseScaleIndex);
    xml.setAttribute ("useHostTempo", useHostTempo);
    xml.setAttribute ("isTempoLocked", isTempoLocked);
    xml.setAttribute ("lockedTempo", lockedTempo);
    xml.setAttribute ("initialBpmCaptured", initialBpmCaptured);
    copyXmlToBinary (xml, destData);
}

void KeyTempoAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr)
    {
        if (xmlState->hasTagName ("KeyTempoSettings"))
        {
            baseTempo = xmlState->getDoubleAttribute ("baseTempo", 140.0);
            baseKeyIndex = xmlState->getIntAttribute ("baseKeyIndex", 0);
            baseScaleIndex = xmlState->getIntAttribute ("baseScaleIndex", 0);
            useHostTempo = xmlState->getBoolAttribute ("useHostTempo", false);
            isTempoLocked = xmlState->getBoolAttribute ("isTempoLocked", false);
            lockedTempo = xmlState->getDoubleAttribute ("lockedTempo", 140.0);
            initialBpmCaptured = xmlState->getBoolAttribute ("initialBpmCaptured", true);
        }
    }
}

// This creates the new editor instance
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KeyTempoAudioProcessor();
}
