/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
enum Channel
{
    Right,
    Left 
};
enum Slope
{
    Slope_12,
    Slope_24,
    Slope_36,
    Slope_48
};

struct propstring
{
    float pf { 0 }, pGain{ 0 }, pQ {1.f};
    float lCut { 0 }, hCut { 0 };
    
    Slope lslop { Slope::Slope_12 }, hslop { Slope::Slope_12 };
    
   
};




//==============================================================================
/**
*/
class VonicAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    VonicAudioProcessor();
    ~VonicAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    static juce::AudioProcessorValueTreeState::ParameterLayout
        createParameterLayout();

    juce::AudioProcessorValueTreeState bleh{
        *this,nullptr,"Parameters",createParameterLayout()
    };

private:
    using alias1 = juce::dsp::IIR::Filter<float>;
    using alias2 = juce::dsp::ProcessorChain<alias1,alias1,alias1,alias1>;
    using alias3 = juce::dsp::ProcessorChain<alias2,alias1,alias2>;
    alias3 L,R;
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VonicAudioProcessor)
};
