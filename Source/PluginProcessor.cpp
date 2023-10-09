/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/
 
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <bits/fs_fwd.h>
#include <memory>

//==============================================================================
VonicAudioProcessor::VonicAudioProcessor()
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

VonicAudioProcessor::~VonicAudioProcessor()
{
}

//==============================================================================
const juce::String VonicAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool VonicAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool VonicAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool VonicAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double VonicAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int VonicAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int VonicAudioProcessor::getCurrentProgram()
{
    return 0;
}

void VonicAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String VonicAudioProcessor::getProgramName (int index)
{
    return {};
}

void VonicAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void VonicAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    using props = juce::dsp::ProcessSpec;
    props prop;
    prop.maximumBlockSize = samplesPerBlock;
    prop.numChannels = 1;
    prop.sampleRate = sampleRate;
    L.prepare(prop);    
    R.prepare(prop);
}

void VonicAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool VonicAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void VonicAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    juce::dsp::AudioBlock<float> seg(buffer);

    auto lSeg = seg.getSingleChannelBlock(0);  // extracting single channels
    auto rSeg = seg.getSingleChannelBlock(1);
    juce::dsp::ProcessContextReplacing<float> lContext(lSeg);
    juce::dsp::ProcessContextReplacing<float> rContext(rSeg);
    L.process(lContext);
    R.process(rContext);
    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);

        // ..do something to the data...
    }
}

//==============================================================================
bool VonicAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* VonicAudioProcessor::createEditor()
{
    // return new VonicAudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void VonicAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void VonicAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}
propstring getChainSettings(juce::AudioProcessorValueTreeState& apvts)
{
    propstring lemao;
    
    lemao.lCut = apvts.getRawParameterValue("LF")->load();
    lemao.hCut = apvts.getRawParameterValue("HF")->load();
    lemao.pf = apvts.getRawParameterValue("PF")->load();
    lemao.pGain = apvts.getRawParameterValue("PG")->load();
    lemao.pQ = apvts.getRawParameterValue("PQ")->load();
    lemao.lslop = static_cast<Slope>(apvts.getRawParameterValue("LF GRADIENT")->load());
    lemao.hslop = static_cast<Slope>(apvts.getRawParameterValue("HF GRADIENT")->load());
    return lemao;

}
juce::AudioProcessorValueTreeState::ParameterLayout
    VonicAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout first;
    first.add(std::make_unique<juce::AudioParameterFloat>("LF","LF",juce::NormalisableRange<float>(20.f,20000.f,2.f,2.f),100));
    first.add(std::make_unique<juce::AudioParameterFloat>("HF","HF",juce::NormalisableRange<float>(20.f,20000.f,2.f,2.f),100));
    first.add(std::make_unique<juce::AudioParameterFloat>("PF","PF",juce::NormalisableRange<float>(20.f,20000.f,2.f,2.f),690));
    first.add(std::make_unique<juce::AudioParameterFloat>("PG","PG",juce::NormalisableRange<float>(-24.f,24.f,0.6f,1.f),0));
    first.add(std::make_unique<juce::AudioParameterFloat>("LQ","LQ",juce::NormalisableRange<float>(0.1f,10.f,0.05f,2.f),1));
    juce::StringArray strarr;
    for (int i = 0; i < 7; ++i)
    {
        juce::String str;
        str << (7 + i*10);
        str << "db per OCTAVE";
        strarr.add(str);
    }

    
    first.add(std::make_unique<juce::AudioParameterChoice>("LF GRADIENT","LF GRADIENT",strarr,0));
    first.add(std::make_unique<juce::AudioParameterChoice>("HF GRADIENT","HF GRADIENT",strarr,0));

    return first;
}
//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VonicAudioProcessor();
}
