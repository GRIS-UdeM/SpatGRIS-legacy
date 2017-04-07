
/*
 ==============================================================================
 SpatGRIS: multichannel sound spatialization plug-in.
 
 Copyright (C) 2015  GRIS-UdeM
 
 Developers: Antoine Missout, Vincent Berthiaume
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ==============================================================================
 */


#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SpatGrisAudioProcessor::SpatGrisAudioProcessor()
{
    this->listSources.resize(8);
    this->listSpeakers.resize(16);
    
    for(int iSour = 0; iSour < MaxSources; iSour++){
        Source * newS = new Source(this, iSour+1);
        this->listSources.set(iSour, newS);
    }
    
    for(int iSpeak = 0; iSpeak < MaxSpeakers; iSpeak++){
        Speaker * newS = new Speaker(this, iSpeak+1);
        this->listSpeakers.set(iSpeak, newS);
    }
    
    this->numSourceUsed = 4;
    this->numSpeakerUsed = 2;
}

SpatGrisAudioProcessor::~SpatGrisAudioProcessor()
{
   
}
//==============================================================================
const String SpatGrisAudioProcessor::getName() const {
    String name(JucePlugin_Name);
    return name;
}

//==============================================================================
void SpatGrisAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{


}

void SpatGrisAudioProcessor::releaseResources()
{
   
}
//==============================================================================


//==============================================================================
void SpatGrisAudioProcessor::processBlock(AudioBuffer<float> &pBuffer, MidiBuffer& midiMessages)
{

}
void SpatGrisAudioProcessor::processBlockBypassed (AudioBuffer<float> &buffer, MidiBuffer& midiMessages)
{
    
}
//==============================================================================


//==============================================================================
AudioProcessorEditor* SpatGrisAudioProcessor::createEditor()
{
    return new SpatGrisAudioProcessorEditor(this);
}
bool SpatGrisAudioProcessor::hasEditor() const
{
    return true;
}

//==============================================================================
double SpatGrisAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}
bool SpatGrisAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}
bool SpatGrisAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}
int SpatGrisAudioProcessor::getNumPrograms()
{
    return 1;
}

int SpatGrisAudioProcessor::getCurrentProgram()
{
    return 1;
}

void SpatGrisAudioProcessor::setCurrentProgram (int index)
{
}

const String SpatGrisAudioProcessor::getProgramName (int index)
{
    return String::empty;
}

void SpatGrisAudioProcessor::changeProgramName (int index, const String& newName)
{
}
//==============================================================================
void SpatGrisAudioProcessor::getStateInformation (MemoryBlock& destData)
{

}
void SpatGrisAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    
}
//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SpatGrisAudioProcessor();
}
