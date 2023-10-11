
/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
VonicRewriteAudioProcessor::VonicRewriteAudioProcessor()
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

VonicRewriteAudioProcessor::~VonicRewriteAudioProcessor()
{
}

//==============================================================================
const juce::String VonicRewriteAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool VonicRewriteAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool VonicRewriteAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool VonicRewriteAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double VonicRewriteAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int VonicRewriteAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int VonicRewriteAudioProcessor::getCurrentProgram()
{
    return 0;
}

void VonicRewriteAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String VonicRewriteAudioProcessor::getProgramName (int index)
{
    return {};
}

void VonicRewriteAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void VonicRewriteAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    juce::dsp::ProcessSpec set;
    set.maximumBlockSize = samplesPerBlock;
    set.numChannels = 1;
    set.sampleRate = sampleRate;
    left.prepare(set);
    right.prepare(set);
    auto chainSettings = getChainSettings(bleh);
    auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,chainSettings.peakFreq,chainSettings.peakQual,juce::Decibels::decibelsToGain(chainSettings.peakGain));
    *left.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
    *right.get<ChainPositions::Peak>().coefficients = *peakCoefficients;

}

void VonicRewriteAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool VonicRewriteAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void VonicRewriteAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
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

    auto chainSettings = getChainSettings(bleh);
    auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),chainSettings.peakFreq,chainSettings.peakQual,juce::Decibels::decibelsToGain(chainSettings.peakGain));


    juce::dsp::AudioBlock<float> block(buffer);
    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);

    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    left.process(leftContext);
    right.process(rightContext);

}

//==============================================================================
bool VonicRewriteAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* VonicRewriteAudioProcessor::createEditor()
{
 //   return new VonicRewriteAudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void VonicRewriteAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void VonicRewriteAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    
}
ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& bleh){
    ChainSettings props;

    props.lowCutFreq = bleh.getRawParameterValue("HighPass")->load();
    props.highCutFreq = bleh.getRawParameterValue("LowPass")->load();
    props.peakFreq = bleh.getRawParameterValue("Peak")->load();
    props.peakGain = bleh.getRawParameterValue("Gain")->load();
    props.peakQual = bleh.getRawParameterValue("Quality")->load();
    props.lowCutSlope = bleh.getRawParameterValue("HighPassGrad")->load();
    props.highCutSlope = bleh.getRawParameterValue("LowPassGrad")->load();
    return props;
}
juce::AudioProcessorValueTreeState::ParameterLayout VonicRewriteAudioProcessor::createParams(){
        juce::AudioProcessorValueTreeState::ParameterLayout map;
        map.add(std::make_unique<juce::AudioParameterFloat>("HighPass","HighPass",juce::NormalisableRange<float>(20.f,20000.f,1.f,1.f),20.f));
        map.add(std::make_unique<juce::AudioParameterFloat>("LowPass","LowPass",juce::NormalisableRange<float>(20.f,20000.f,1.f,1.f),20.f));
        map.add(std::make_unique<juce::AudioParameterFloat>("Peak","Peak",juce::NormalisableRange<float>(20.f,20000.f,1.f,1.f),750.f));
        map.add(std::make_unique<juce::AudioParameterFloat>("Gain","Gain",juce::NormalisableRange<float>(-24.f,24.f,0.5f,1.f),0.f));
        map.add(std::make_unique<juce::AudioParameterFloat>("Quality","Quality",juce::NormalisableRange<float>(0.1f,10.f,0.05f,1.f),1.f));

    juce::StringArray choices;
    for (int i = 0; i < 4; ++i)
    {
        juce::String brr;
        brr << (12 + 12*i);
        brr << "decibelsPerOct";
        choices.add(brr);
        /* code */
    }
    map.add(std::make_unique<juce::AudioParameterChoice>("HighPassGrad","HighPassGrad",choices,0));
    map.add(std::make_unique<juce::AudioParameterChoice>("LowPassGrad","LowPassGrad",choices,0));

    
    
    return map;
    }
//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VonicRewriteAudioProcessor();
}
