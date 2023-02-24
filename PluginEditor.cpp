/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
EchoDelayInterpAudioProcessorEditor::EchoDelayInterpAudioProcessorEditor (EchoDelayInterpAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (400, 300);
    
    mLfoDepthSlider.setRange(0.0, 100.0);
    mLfoDepthSlider.setValue(audioProcessor.mLfoDepthSliderValue * 100.0f);
    addAndMakeVisible(&mLfoDepthSlider);
    mLfoDepthSlider.addListener(this);
    
    mFeedbackSlider.setRange(0.0,99.0);
    mFeedbackSlider.setValue(audioProcessor.mFeedbackGain * 100.0f);
    addAndMakeVisible(&mFeedbackSlider);
    mFeedbackSlider.addListener(this);
    
    mLfoFreqSlider.setRange(0.1,3.0);
    mLfoFreqSlider.setValue(audioProcessor.mLfoArray[0].getFreq());
    addAndMakeVisible(&mLfoFreqSlider);
    mLfoFreqSlider.addListener(this);
    
    mLfoTypeComboBox.addItem("Sin", LfoType::sin);
    mLfoTypeComboBox.addItem("Saw", LfoType::saw);
    mLfoTypeComboBox.setSelectedId(LfoType::sin);
    addAndMakeVisible(&mLfoTypeComboBox);
    mLfoTypeComboBox.addListener(this);
    
    mLforMotionComboBox.addItem("true", LfoMotion::contrary);
    mLforMotionComboBox.addItem("false", LfoMotion::sync);
    mLforMotionComboBox.setSelectedId(LfoMotion::sync);
    addAndMakeVisible(&mLforMotionComboBox);
    mLforMotionComboBox.addListener(this);
    
    
    
}
EchoDelayInterpAudioProcessorEditor::~EchoDelayInterpAudioProcessorEditor()
{
    mFeedbackSlider.removeListener(this);
    mLfoDepthSlider.removeListener(this);
    mLfoTypeComboBox.removeListener(this);
    mLforMotionComboBox.removeListener(this);
    mLfoFreqSlider.removeListener(this);
    
}

//==============================================================================
void EchoDelayInterpAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &mLfoDepthSlider)
    {
        audioProcessor.mLfoDepthSliderValue = mLfoDepthSlider.getValue() /100.0;
        
        juce::Range<double> newRange(0.0, audioProcessor.mLfoDepthSliderValue);
        
        
        audioProcessor.mLfoArray[0].setRange(newRange);
        audioProcessor.mLfoArray[1].setRange(newRange);
    }
    else if (slider == &mFeedbackSlider)
    {
        // convert to percent
        audioProcessor.mFeedbackGain = mFeedbackSlider.getValue() / 100.0;
    }
    
    else if (slider == &mLfoFreqSlider)
    {
        audioProcessor.setLfoFreq(mLfoFreqSlider.getValue());
        
    }
}

void EchoDelayInterpAudioProcessorEditor::comboBoxChanged(juce::ComboBox *comboBox)
{
    if (comboBox == &mLforMotionComboBox)
    {
        audioProcessor.mLfoMotion = mLforMotionComboBox.getSelectedId()-1;
        audioProcessor.setLfoFreq(audioProcessor.mLfoArray[0].getFreq());
    }
                                  
    else
    {
        audioProcessor.setLfoType(mLfoTypeComboBox.getSelectedId());
    }
}
    
void EchoDelayInterpAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (juce::Colours::pink);
}

void EchoDelayInterpAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    mFeedbackSlider.setBounds(50, 100, 300, 50);
    mLfoFreqSlider.setBounds(50, 150, 300, 50);
    mLfoDepthSlider.setBounds(50, 50, 300, 50);
    
    
    mLforMotionComboBox.setBounds(50,200,300,50);
    mLfoTypeComboBox.setBounds(50,250,300,50);
}
