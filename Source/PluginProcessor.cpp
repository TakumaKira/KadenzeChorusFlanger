/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
KadenzeChorusFlangerAudioProcessor::KadenzeChorusFlangerAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    addParameter(mDryWetParameter = new AudioParameterFloat("drywet",
                                                            "Dry Wet",
                                                            0.0,
                                                            1.0,
                                                            0.5));
    
    addParameter(mDepthParameter = new AudioParameterFloat("depth",
                                                           "Depth",
                                                           0.0,
                                                           1.0,
                                                           0.5));
    
    addParameter(mRateParameter = new AudioParameterFloat("rate",
                                                          "Rate",
                                                          0.1f,
                                                          20.f,
                                                          10.f));
    
    addParameter(mPhaseOffsetParameter = new AudioParameterFloat("phaseoffset",
                                                                 "Phase Offset",
                                                                 0.0f,
                                                                 1.f,
                                                                 0.f));
    
    addParameter(mFeedbackParameter = new AudioParameterFloat("feedback",
                                                              "Feedback",
                                                              0,
                                                              0.98,
                                                              0.5));
    
    addParameter(mTypeParameter = new AudioParameterInt("type",
                                                        "Type",
                                                        0,
                                                        1,
                                                        0));
    
    mCircularBufferLeft = nullptr;
    mCircularBufferRight = nullptr;
    mCircularBufferWriteHead = 0;
    mCircularBufferLength = 0;
    
    mFeedbackLeft = 0;
    mFeedbackRight = 0;
    
    mLFOPhase = 0;
}

KadenzeChorusFlangerAudioProcessor::~KadenzeChorusFlangerAudioProcessor()
{
}

//==============================================================================
const String KadenzeChorusFlangerAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool KadenzeChorusFlangerAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool KadenzeChorusFlangerAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool KadenzeChorusFlangerAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double KadenzeChorusFlangerAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int KadenzeChorusFlangerAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int KadenzeChorusFlangerAudioProcessor::getCurrentProgram()
{
    return 0;
}

void KadenzeChorusFlangerAudioProcessor::setCurrentProgram (int index)
{
}

const String KadenzeChorusFlangerAudioProcessor::getProgramName (int index)
{
    return {};
}

void KadenzeChorusFlangerAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void KadenzeChorusFlangerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    mLFOPhase = 0;
    
    mCircularBufferLength = sampleRate * MAX_DELAY_TIME;
    
    if (mCircularBufferLeft != nullptr) {
        delete [] mCircularBufferLeft;
        mCircularBufferLeft = nullptr;
    }
    
    if (mCircularBufferLeft == nullptr) {
        mCircularBufferLeft = new float[mCircularBufferLength];
    }
    
    zeromem(mCircularBufferLeft, mCircularBufferLength * sizeof(float));
    
    if (mCircularBufferRight != nullptr) {
        delete [] mCircularBufferRight;
        mCircularBufferRight = nullptr;
    }
    
    if (mCircularBufferRight == nullptr) {
        mCircularBufferRight = new float[mCircularBufferLength];
    }
    
    zeromem(mCircularBufferRight, mCircularBufferLength * sizeof(float));
    
    mCircularBufferWriteHead = 0;    
}

void KadenzeChorusFlangerAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool KadenzeChorusFlangerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
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

void KadenzeChorusFlangerAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
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

    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel = buffer.getWritePointer(1);
    
    for (int i = 0; i < buffer.getNumSamples(); i++) {
        
        float lfoOut = sin(2*M_PI * mLFOPhase);
        
        mLFOPhase += *mRateParameter / getSampleRate();
        
        if (mLFOPhase > 1) {
            mLFOPhase -= 1;
        }
        
        lfoOut *= *mDepthParameter;
        
        float lfoOutMapped = jmap(lfoOut, -1.f, 1.f, 0.005f, 0.03f);
        
        float delayTimeSamples = getSampleRate() * lfoOutMapped;
        
        mCircularBufferLeft[mCircularBufferWriteHead] = leftChannel[i] + mFeedbackLeft;
        mCircularBufferRight[mCircularBufferWriteHead] = rightChannel[i] + mFeedbackRight;
        
        float delayReadHead = mCircularBufferWriteHead - delayTimeSamples;
        
        int readHead_x = (int)delayReadHead;
        int readHead_x1 = readHead_x + 1;
        float readHeadFloat = delayReadHead - readHead_x;
        
        if (readHead_x1 >= mCircularBufferLength) {
            readHead_x1 -= mCircularBufferLength;
        }
        
        float delay_sample_left = lin_interp(mCircularBufferLeft[readHead_x], mCircularBufferLeft[readHead_x1], readHeadFloat);
        float delay_sample_right = lin_interp(mCircularBufferRight[readHead_x], mCircularBufferRight[readHead_x1], readHeadFloat);
        
        mFeedbackLeft = delay_sample_left * *mFeedbackParameter;
        mFeedbackRight = delay_sample_right * *mFeedbackParameter;
        
        mCircularBufferWriteHead++;
        
        buffer.setSample(0, i, buffer.getSample(0, i) * (1 - *mDryWetParameter) + delay_sample_left * *mDryWetParameter);
        buffer.setSample(1, i, buffer.getSample(1, i) * (1 - *mDryWetParameter) + delay_sample_right * *mDryWetParameter);
        
        if (mCircularBufferWriteHead >= mCircularBufferLength) {
            mCircularBufferWriteHead = 0;
        }
        
    }
}

//==============================================================================
bool KadenzeChorusFlangerAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* KadenzeChorusFlangerAudioProcessor::createEditor()
{
    return new KadenzeChorusFlangerAudioProcessorEditor (*this);
}

//==============================================================================
void KadenzeChorusFlangerAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void KadenzeChorusFlangerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// Added

float KadenzeChorusFlangerAudioProcessor::lin_interp(float sample_x, float sample_x1, float inPhase)
{
    return (1 - inPhase) * sample_x + inPhase * sample_x1;
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KadenzeChorusFlangerAudioProcessor();
}
