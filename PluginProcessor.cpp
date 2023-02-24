/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
EchoDelayInterpAudioProcessor::EchoDelayInterpAudioProcessor()
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
    mSampleRate = 48000.0;
    mBlockSize = 1024.0;
    mFeedbackGain = FEEDBACKGAININIT;

    mRingBuf.debug(true);
    
    mLfoDepthSliderValue =1.0;
    mLfoMotion = true;
    
}

EchoDelayInterpAudioProcessor::~EchoDelayInterpAudioProcessor()
{
}

//==============================================================================
void EchoDelayInterpAudioProcessor::setLfoFreq(double freq)
{
    for (int channel=0; channel < mNumInputChannels; channel++)
    {
        if (mLfoMotion)
        {
            mLfoArray[0].setFreq(freq);
            mLfoArray[1].setFreq(freq * -1.0);
            
        }
        else
        {
            for (int channel=0; channel < mNumInputChannels; channel++)
                mLfoArray[channel].setFreq(freq);
        }
        
        
        // setting the phase to 0 so thqt the LFO is synced
        for (int channel=0; channel < mNumInputChannels; channel++)
            mLfoArray[channel].setPhase(0.0);
    }
}

void EchoDelayInterpAudioProcessor::setLfoSampleRate(double sr)
{
    for(int channel=0; channel < mNumInputChannels; channel++)
        mLfoArray[channel].setSampleRate(sr);
}

void EchoDelayInterpAudioProcessor::setLfoType(int lfotypeId)
{
    for(int channel = 0; channel < mNumInputChannels; channel++)
    {
        switch (lfotypeId)
        {
            case 1:
                mLfoArray[channel].setType(atec::LFO::sin);
                break;
                
            case 2:
                mLfoArray[channel].setType(atec::LFO::saw);
                break;
                
            default:
                break;
        }
    }
    
}


const juce::String EchoDelayInterpAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool EchoDelayInterpAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool EchoDelayInterpAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool EchoDelayInterpAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double EchoDelayInterpAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int EchoDelayInterpAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int EchoDelayInterpAudioProcessor::getCurrentProgram()
{
    return 0;
}

void EchoDelayInterpAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String EchoDelayInterpAudioProcessor::getProgramName (int index)
{
    return {};
}

void EchoDelayInterpAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void EchoDelayInterpAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    mNumInputChannels  = getTotalNumInputChannels();

    mSampleRate = sampleRate;
    mBlockSize = samplesPerBlock;

    mMaxDelaySamps = atec::Utilities::sec2samp(25.0/ 1000.0, mSampleRate);
    
    // 3-second ring buffer
    mRingBuf.setSize (mNumInputChannels, 3.0 * mSampleRate, mBlockSize);
    mRingBuf.init();
    
    setLfoType(1);
    setLfoSampleRate(mSampleRate);
    setLfoFreq(LFOFREQINIT);
    

    // initialize the delay times for the left and right channels
}

void EchoDelayInterpAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool EchoDelayInterpAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void EchoDelayInterpAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    auto bufSize = buffer.getNumSamples();
    juce::AudioBuffer<float> delayBlock;

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, bufSize);

    // buffer to store per-block processing. should be same size as incoming buffer from host
    delayBlock.setSize(totalNumInputChannels, bufSize);
    // clear the delay block buffer for good measure (all channels)
    delayBlock.clear();

    // pull the buffered audio out of the ring buffer and into the delay block (all channels)
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* delayBlockPtr = delayBlock.getWritePointer(channel);

        for (int sample = 0; sample < bufSize; ++sample)
        {
            double thisDelayTime;

            // get the current delay time in samples for this channel
            thisDelayTime = mMaxDelaySamps * mLfoArray[channel].getNextSample();
            
            
            delayBlockPtr[sample] = mRingBuf.readInterpSample(channel, sample, thisDelayTime);
        }

        // reduce amplitude of the delayed block (all channels)
        delayBlock.applyGain(channel, 0, bufSize, mFeedbackGain);

        // add the amp-reduced delayed audio block to the current input block, per channel
        // this way, whatever's in the buffer on input stays there, and we just mix in our delayed audio
        buffer.addFrom(channel, 0, delayBlock, channel, 0, bufSize);
    }
    
    // throw the mix back into the ring buffer (all channels)
    mRingBuf.write(buffer);

    // reduce output gain (all channels)
    buffer.applyGain(0, bufSize, 0.25);
}

//==============================================================================
bool EchoDelayInterpAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* EchoDelayInterpAudioProcessor::createEditor()
{
    return new EchoDelayInterpAudioProcessorEditor (*this);
}

//==============================================================================
void EchoDelayInterpAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void EchoDelayInterpAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EchoDelayInterpAudioProcessor();
}
