
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
    this->sourceMover = new SourceMover(this);
    this->trajectory = new Trajectory();
    
    this->selectItem = new SelectItem();
    this->selectItem->selectID = 1;
    this->selectItem->selecType = SelectedSource;
    
	this->listSources = vector<Source *>();
	this->listSpeakers = vector<Speaker *>();

    
    for(int iSour = 0; iSour < MaxSources; iSour++){
        Source * newS = new Source(this, iSour+1);
        this->listSources.push_back(newS);
    }
    
    for(int iSpeak = 0; iSpeak < MaxSpeakers; iSpeak++){
        Speaker * newS = new Speaker(this, iSpeak+1);
        this->listSpeakers.push_back(newS);
    }
	
    this->numSourceUsed = MaxSources;
    this->numSpeakerUsed = MaxSpeakers;
    
    this->sourceMover->setSourcesPosition();
}

SpatGrisAudioProcessor::~SpatGrisAudioProcessor()
{
    delete this->selectItem;
    delete this->sourceMover;
    delete this->trajectory;
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


//==============================================================================

void SpatGrisAudioProcessor::setPosXYSource(int idS, float x, float y, bool updateAll)
{
    *(this->listSources[idS]->getX()) = x;
    *(this->listSources[idS]->getY()) = y;
    
    if(updateAll){
        this->sourceMover->updateSourcesPosition(idS, x, y);
    }
}
FPoint SpatGrisAudioProcessor::getRayAngleSource(int idS)
{
    float x = *(this->listSources.at(idS)->getX());
    float y = *(this->listSources.at(idS)->getY());
    return FPoint(GetRaySpat(x, y), GetAngleSpat(x, y));
}

void SpatGrisAudioProcessor::setSurfaceValue(float surf)
{
    if(this->linkSurface){
        for(int iSour = 0; iSour < this->numSourceUsed; iSour++){
            *(this->listSources.at(iSour)->getSurf()) = surf;
        }
    }else{
        *(this->listSources.at(this->selectItem->selectID)->getSurf()) = surf;
    }
}

void SpatGrisAudioProcessor::setAzimuthValue(float azim)
{
    if(this->linkAzimuth){
        for(int iSour = 0; iSour < this->numSourceUsed; iSour++){
            *(this->listSources.at(iSour)->getAzim()) = azim;
        }
    }else{
        *(this->listSources.at(this->selectItem->selectID)->getAzim()) = azim;
    }
}

void SpatGrisAudioProcessor::setElevationValue(float elev)
{
    if(this->linkElevation){
        for(int iSour = 0; iSour < this->numSourceUsed; iSour++){
            *(this->listSources.at(iSour)->getElev()) = elev;
        }
    }else{
        *(this->listSources.at(this->selectItem->selectID)->getElev()) = elev;
    }
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
