/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class EchoDelayInterpAudioProcessorEditor  : public juce::AudioProcessorEditor, public juce::Slider::Listener, public juce::ComboBox::Listener
{
public:
    EchoDelayInterpAudioProcessorEditor (EchoDelayInterpAudioProcessor&);
    ~EchoDelayInterpAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    EchoDelayInterpAudioProcessor& audioProcessor;
    
    
    juce::Slider mFeedbackSlider;
    juce::Slider mLfoFreqSlider;
    juce::Slider mLfoDepthSlider;
    
    juce::ComboBox mLfoTypeComboBox;
    juce::ComboBox mLforMotionComboBox;
    
    void sliderValueChanged(juce::Slider* slider) override;
    void comboBoxChanged(juce::ComboBox* comboBox) override;
    
    
    enum LfoType
    {
        sin = 1,
        saw = 2
        
    };
    
    enum LfoMotion
    {
        sync = 1,
        contrary
        
    };
    
    
    
    

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EchoDelayInterpAudioProcessorEditor)
};
