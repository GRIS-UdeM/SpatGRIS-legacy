
/*
 ==============================================================================
 SpatGRIS: multichannel sound spatialization plug-in.
 
 Copyright (C) 2015  GRIS-UdeM
 
 Developers: Antoine Missout, Vincent Berthiaume, Nicolas Masson
 
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
    for(int iSour = 0; iSour < MaxSources; iSour++){
        this->listSources[iSour] = new Source(this, iSour+1);
    }
    
    for(int iSpeak = 0; iSpeak < MaxSpeakers; iSpeak++){
        this->listSpeakers[iSpeak] = new Speaker(this, iSpeak+1);
    }
    
    
    this->sourceMover = new SourceMover(this);
    this->trajectory = new Trajectory(this);
    
    this->selectItem = new SelectItem();
    this->selectItem->selectIdSource = 0;
    this->selectItem->selectIdSpeaker = 0;
    this->selectItem->mouseOverSpeaker = false;
    this->selectItem->mouseOverSource = false;

    
    this->typeProcess = FreeVolum;
    this->numSourceUsed = MaxSources;
    this->numSpeakerUsed = MaxSpeakers;
    this->inOutModeUsed = i1o1;
    
    this->sourceMover->setSourcesPosition();
    this->sourceMover->setSpeakersPosition();
    
    this->oscRun = this->oscSender.connect(this->oscIpAddress, this->oscPort);
    this->startTimerHz(OscTimerHz);
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
    this->sampleRate = sampleRate;
    this->bufferSize = samplesPerBlock;
}

void SpatGrisAudioProcessor::releaseResources()
{
    
}
//==============================================================================


//==============================================================================
void SpatGrisAudioProcessor::processBlock(AudioBuffer<float> &pBuffer, MidiBuffer& midiMessages)
{
    if (this->bufferSize != pBuffer.getNumSamples()){
        this->bufferSize = pBuffer.getNumSamples();
    }
    this->getPlayHead()->getCurrentPosition(this->cpi);
    
    //==================================== PROCESS TRAJECTORIES ===========================================
    this->processTrajectory();
    
    
    if(this->typeProcess == OSCZirkonium ||  this->typeProcess == OSCSpatServer)
    {
        return;
    }
    
    //==================================== PROCESS BUFFERS ================================================
    //Copy Buffer to pBufferIn and Clear pBuffer
    pBufferIn.makeCopyOf(pBuffer);
    for (int i = 0; i < pBuffer.getNumChannels(); ++i) {
        memset (pBuffer.getWritePointer(i), 0, sizeof (float) * this->bufferSize);
    }

    
    //==================================== PROCESS MODE ===================================================
    switch (this->typeProcess) {
        case FreeVolum:
            this->processFreeVolume(pBuffer);
            break;
            
        case PanSpan:
            this->processPanSpan(pBuffer);
            break;
        
        default:
            break;
    }
    
   
    //==================================== PROCESS VUMETER & MUTE ==========================================
    for (int i = 0; i < pBuffer.getNumChannels(); ++i) {
        //Mute ----------------
        if(this->listSpeakers[i]->isMuted()){
            memset (pBuffer.getWritePointer(i), 0, sizeof (float) * this->bufferSize);
        }
        
        float sumOut = 0.0f;
        //Vu Meter ----------------
        for(int nF = 0; nF < this->bufferSize; ++nF) {
            sumOut += (*pBuffer.getWritePointer(i, nF)) * (*pBuffer.getWritePointer(i, nF));
        }
        this->listSpeakers[i]->setLevel(sumOut/this->bufferSize);
    }


}
void SpatGrisAudioProcessor::processBlockBypassed (AudioBuffer<float> &buffer, MidiBuffer& midiMessages)
{
    
}
//==============================================================================
void SpatGrisAudioProcessor::processFreeVolume(AudioBuffer<float> &pBuffer)
{
    for (int o = 0; o < this->numSpeakerUsed; ++o){
        
        float outputX = this->listSpeakers[o]->getX();
        float outputY = this->listSpeakers[o]->getY();
        
        for(int i = 0; i < this->numSourceUsed; ++i){
            
            float dx = (*this->listSources[i]->getX()) - outputX;
            float dy = (*this->listSources[i]->getY()) - outputY;
            float di = MaxHeigSource - (*this->listSources[i]->getHeigt());
            float da = sqrtf(dx*dx + dy*dy) * di;
            
            if (da > 1.0f) da = 1.0f;
            if (da < 0.01f) da = 0.01f;
            
            da = -log10f(da);
            for (int f = 0; f < this->bufferSize; ++f){
                if(i == 0){
                    (*pBuffer.getWritePointer(o, f)) = (da *  (*this->pBufferIn.getWritePointer(i, f)));
                }
                else{
                    (*pBuffer.getWritePointer(o, f))  += (da *  (*this->pBufferIn.getWritePointer(i, f)));
                }
            }
        }
    }
}

void SpatGrisAudioProcessor::processPanSpan(AudioBuffer<float> &pBuffer)
{

}

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


void SpatGrisAudioProcessor::setTypeProcess(ProcessType v)
{
    this->typeProcess = v;
    this->stopTimer();
    
    switch (this->typeProcess) {
        case FreeVolum:
            break;
        case PanSpan:
            break;

        case OSCSpatServer:
            this->startTimerHz(OscTimerHz);
            break;
            
        case OSCZirkonium:
            this->startTimerHz(OscTimerHz);
            break;
            
        default:
            this->typeProcess = FreeVolum;
            break;
    }

}


void SpatGrisAudioProcessor::setIdInOutMode(int idMode)
{
    this->inOutModeUsed = (InOutMode)(idMode-1);
    switch (this->inOutModeUsed){
        case i1o1:
            this->numSourceUsed = 1;
            this->numSpeakerUsed = 1;
            break;
        case i1o2:
            this->numSourceUsed = 1;
            this->numSpeakerUsed = 2;
            break;
        case i1o4:
            this->numSourceUsed = 1;
            this->numSpeakerUsed = 4;
            break;
        case i1o6:
            this->numSourceUsed = 1;
            this->numSpeakerUsed = 6;
            break;
        case i1o8:
            this->numSourceUsed = 1;
            this->numSpeakerUsed = 8;
            break;
        case i1o12:
            this->numSourceUsed = 1;
            this->numSpeakerUsed = 12;
            break;
        case i1o16:
            this->numSourceUsed = 1;
            this->numSpeakerUsed = 16;
            break;
        case i2o2:
            this->numSourceUsed = 2;
            this->numSpeakerUsed = 2;
            break;
        case i2o4:
            this->numSourceUsed = 2;
            this->numSpeakerUsed = 4;
            break;
        case i2o6:
            this->numSourceUsed = 2;
            this->numSpeakerUsed = 6;
            break;
        case i2o8:
            this->numSourceUsed = 2;
            this->numSpeakerUsed = 8;
            break;
        case i2o12:
            this->numSourceUsed = 2;
            this->numSpeakerUsed = 12;
            break;
        case i2o16:
            this->numSourceUsed = 2;
            this->numSpeakerUsed = 16;
            break;
        case i4o4:
            this->numSourceUsed = 4;
            this->numSpeakerUsed = 4;
            break;
        case i4o6:
            this->numSourceUsed = 4;
            this->numSpeakerUsed = 6;
            break;
        case i4o8:
            this->numSourceUsed = 4;
            this->numSpeakerUsed = 8;
            break;
        case i4o12:
            this->numSourceUsed = 4;
            this->numSpeakerUsed = 12;
            break;
        case i4o16:
            this->numSourceUsed = 4;
            this->numSpeakerUsed = 16;
            break;
        case i6o6:
            this->numSourceUsed = 6;
            this->numSpeakerUsed = 6;
            break;
        case i6o8:
            this->numSourceUsed = 6;
            this->numSpeakerUsed = 8;
            break;
        case i6o12:
            this->numSourceUsed = 6;
            this->numSpeakerUsed = 12;
            break;
        case i6o16:
            this->numSourceUsed = 6;
            this->numSpeakerUsed = 16;
            break;
        case i8o8:
            this->numSourceUsed = 8;
            this->numSpeakerUsed = 8;
            break;
        case i8o12:
            this->numSourceUsed = 8;
            this->numSpeakerUsed = 12;
            break;
        case i8o16:
            this->numSourceUsed = 8;
            this->numSpeakerUsed = 16;
            break;
        default:
            this->numSourceUsed = this->getTotalNumInputChannels();
            this->numSpeakerUsed = this->getTotalNumOutputChannels();
            break;
    }
}

//==============================================================================

void SpatGrisAudioProcessor::setPosXYSource(int idS, float x, float y, bool updateAll)
{
    *(this->listSources[idS]->getX()) = x;
    *(this->listSources[idS]->getY()) = y;
    
    if(updateAll){
        this->sourceMover->updateSourcesPosition(idS, x, y);
    }
}
void SpatGrisAudioProcessor::setPosRayAngSource(int idS, float ray, float ang, bool updateAll)
{
    FPoint xyS = GetXYFromRayAng(ray, DegreeToRadian(ang));
    
    *(this->listSources[idS]->getX()) = xyS.x;
    *(this->listSources[idS]->getY()) = xyS.y;
    
    if(updateAll){
        this->sourceMover->updateSourcesPosition(idS, xyS.x, xyS.y);
    }
}

void SpatGrisAudioProcessor::setPosRayAngRadSource(int idS, float ray, float ang, bool updateAll)
{
    FPoint xyS = GetXYFromRayAng(ray, ang);
    
    *(this->listSources[idS]->getX()) = xyS.x;
    *(this->listSources[idS]->getY()) = xyS.y;
    
    if(updateAll){
        this->sourceMover->updateSourcesPosition(idS, xyS.x, xyS.y);
    }
}

FPoint SpatGrisAudioProcessor::getXYSource(int idS)
{
    float x = *(this->listSources[idS]->getX());
    float y = *(this->listSources[idS]->getY());
    return FPoint(x, y);
}
FPoint SpatGrisAudioProcessor::getRayAngleSource(int idS)
{
    float x = *(this->listSources[idS]->getX());
    float y = *(this->listSources[idS]->getY());
    return FPoint(GetRaySpat(x, y), GetAngleSpat(x, y));
}


void SpatGrisAudioProcessor::setHeightSValue(float hei)
{
    if(this->linkHeight){
        for(int iSour = 0; iSour < this->numSourceUsed; iSour++){
            *(this->listSources[iSour]->getHeigt()) = hei;
        }
    }else{
        *(this->listSources[this->selectItem->selectIdSource]->getHeigt()) = hei;
    }
}

void SpatGrisAudioProcessor::setAzimuthValue(float azim)
{
    if(this->linkAzimuth){
        for(int iSour = 0; iSour < this->numSourceUsed; iSour++){
            *(this->listSources[iSour]->getAzim()) = azim;
        }
    }else{
        *(this->listSources[this->selectItem->selectIdSource]->getAzim()) = azim;
    }
}

void SpatGrisAudioProcessor::setElevationValue(float elev)
{
    if(this->linkElevation){
        for(int iSour = 0; iSour < this->numSourceUsed; iSour++){
            *(this->listSources[iSour]->getElev()) = elev;
        }
    }else{
        *(this->listSources[this->selectItem->selectIdSource]->getElev()) = elev;
    }
}



//==============================================================================
void SpatGrisAudioProcessor::processTrajectory()
{
    if(this->trajectory->getProcessTrajectory() && this->isPlaying()){
        double bps = cpi.bpm / 60;
        float seconds = this->bufferSize / this->sampleRate;
        float beats = seconds * bps;
        
        bool done = this->trajectory->process(seconds, beats);
    }
    
}


void SpatGrisAudioProcessor::sendOscMessageValues()
{
    if(this->oscOn ){
        switch (this->typeProcess) {
                
            case OSCSpatServer:{
                for(int iCurSrc = 0; iCurSrc < this->numSourceUsed; ++iCurSrc){
                    int   channel_osc   = this->oscFirstIdSource+iCurSrc-1;
                    FPoint rayAng       = getRayAngleSource(iCurSrc);
                    float azim_osc      = ThetaMax - (rayAng.y - QuarterCircle);
                    float elev_osc      = (rayAng.x / RadiusMax) * QuarterCircle;
                    float azimspan_osc  = *this->getListSource()[iCurSrc]->getAzim();
                    float elevspan_osc  = *this->getListSource()[iCurSrc]->getElev();
                    float height_osc    = *this->getListSource()[iCurSrc]->getHeigt();
                    float gain_osc      = 1;
                
                    OSCAddressPattern oscPattern("/spat/serv");
                    OSCMessage message(oscPattern);
                
                    message.addInt32(channel_osc);
                    message.addFloat32(azim_osc);
                    message.addFloat32(elev_osc);
                    message.addFloat32(azimspan_osc);
                    message.addFloat32(elevspan_osc);
                    message.addFloat32(height_osc);
                    message.addFloat32(gain_osc);
                    if (!this->oscSender.send(message)) {
                        DBG("Error: could not send OSC message.");
                        return;
                    }
                }
                break;
            }
                
            case OSCZirkonium:{
                for(int iCurSrc = 0; iCurSrc < this->numSourceUsed; ++iCurSrc){
                    int   channel_osc   = this->oscFirstIdSource+iCurSrc-1;             //in gui the range is 1-99, for zirkonium it actually starts at 0 (or potentially lower, but Zirkosc uses 0 as starting channel)
                    FPoint rayAng       = getRayAngleSource(iCurSrc);
                    float azim_osc      = ((rayAng.y+(M_PI/2.0f))/M_PI) - 1.0f;                     //For Zirkonium, -1 is in the back right and +1 in the back left. 0 is forward
                    float elev_osc      = 0.5f -  ((rayAng.x/2.0f)*0.5f);                     //For Zirkonium, 0 is the edge of the dome, .5 is the top
                    float azimspan_osc  = *this->getListSource()[iCurSrc]->getAzim();     //min azim span is 0, max is 2. I figure this is radians.
                    float elevspan_osc  = *this->getListSource()[iCurSrc]->getElev();     //min elev span is 0, max is .5
                    float gain_osc      = 1;                                //gain is just locked to max value
                    
                    OSCAddressPattern oscPattern("/pan/az");
                    OSCMessage message(oscPattern);
                    
                    message.addInt32(channel_osc);
                    message.addFloat32(azim_osc);
                    message.addFloat32(elev_osc);
                    message.addFloat32(azimspan_osc);
                    message.addFloat32(elevspan_osc);
                    message.addFloat32(gain_osc);
                    if (!this->oscSender.send(message)) {
                        DBG("Error: could not send OSC message.");
                        return;
                    }
                }
                break;
            }
                
        }
    } else {
        switch (this->typeProcess) {
            case OSCSpatServer:{
                for(int iCurSrc = 0; iCurSrc < this->numSourceUsed; ++iCurSrc){
                    int channel_osc = this->oscFirstIdSource+iCurSrc-1;
                    OSCAddressPattern oscPattern("/spat/serv");
                    OSCMessage message(oscPattern);
                    message.addString("reset");
                    message.addInt32(channel_osc);
                    if (!this->oscSender.send(message)) {
                        DBG("Error: could not send OSC message.");
                        return;
                    }
                }
                break;
            }
        }
    }
}

void SpatGrisAudioProcessor::resetOSC() {
    switch (this->typeProcess) {
        case OSCSpatServer:{
            for(int iCurSrc = 0; iCurSrc < this->numSourceUsed; ++iCurSrc){
                int channel_osc = this->oscFirstIdSource+iCurSrc-1;
                OSCAddressPattern oscPattern("/spat/serv");
                OSCMessage message(oscPattern);
                message.addString("reset");
                message.addInt32(channel_osc);
                if (!this->oscSender.send(message)) {
                    DBG("Error: could not send OSC message.");
                    return;
                }
            }
            break;
        }
    }
}

void SpatGrisAudioProcessor::timerCallback()
{
    this->sendOscMessageValues();
}

//=====================================================================================
void SpatGrisAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    XmlElement xml ("SPATGRIS_SETTINGS");
    
    for(auto&& it : this->listSources){
        String is = String(it->getId());
        xml.setAttribute ("src" + is + "x", *it->getX());
        xml.setAttribute ("src" + is + "y", *it->getY());
        xml.setAttribute ("src" + is + "H", *it->getHeigt());
        xml.setAttribute ("src" + is + "A", *it->getAzim());
        xml.setAttribute ("src" + is + "E", *it->getElev());
    }
    
    for(auto&& it : this->listSpeakers){
        String is = String(it->getId());
        xml.setAttribute ("spk" + is + "x", it->getX());
        xml.setAttribute ("spk" + is + "y", it->getY());
        xml.setAttribute ("spk" + is + "M", it->isMuted());
    }
    
    copyXmlToBinary (xml, destData);
}
//----------------------------------------------------------------------------------
void SpatGrisAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    ScopedPointer<XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName("SPATGRIS_SETTINGS"))
    {
        for(auto&& it : this->listSources){
            String is = String(it->getId());
            *it->getX() = static_cast<float>(xmlState->getDoubleAttribute("src" + is + "x", 0.0f));
            *it->getY() = static_cast<float>(xmlState->getDoubleAttribute("src" + is + "y", 0.0f));
            *it->getHeigt() = static_cast<float>(xmlState->getDoubleAttribute("src" + is + "H", 0.0f));
            *it->getAzim()  = static_cast<float>(xmlState->getDoubleAttribute("src" + is + "A", 0.0f));
            *it->getElev()  = static_cast<float>(xmlState->getDoubleAttribute("src" + is + "E", 0.0f));
        }
        
        for(auto&& it : this->listSpeakers){
            String is = String(it->getId());
            it->setX(static_cast<float>(xmlState->getDoubleAttribute("spk" + is + "x", 0.0f)));
            it->setY(static_cast<float>(xmlState->getDoubleAttribute("spk" + is + "y", 0.0f)));
            it->setMuted(static_cast<bool>(xmlState->getBoolAttribute("spk" + is + "M", false)));
        }
        
    }
}
//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SpatGrisAudioProcessor();
}
