/*
 ==============================================================================
 SpatGRIS: multichannel sound spatialization plug-in.
 
 Copyright (C) 2015  GRIS-UdeM
 
 PluginProcessor.h
 
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


#ifndef PLUGINPROCESSOR_H_INCLUDED
#define PLUGINPROCESSOR_H_INCLUDED


#if JUCE_MSVC

/*#include <sstream>
 #include <string>
 #include <windows.h>*/

/*
 template<class T>
 string toString(const T &value) {
 ostringstream os;
 os << value;
 return os.str();
 }
 
 // from https://github.com/objectx/strlcpy/blob/master/strlcpy/strlcpy.c
 size_t strlcpy(char * dst, const char * src, size_t dstsize)
 {
 if (dst == 0 || dstsize == 0) {
 return 0;
 }
 if (src == 0) {
 dst [0] = 0;
 return 0;
 } else {
 size_t	src_len = strlen (src);
 size_t	dstlimit = dstsize - 1;
 
 if (dstlimit < src_len) {
 src_len = dstlimit;
 }
 memcpy (dst, src, src_len);
 dst [src_len] = 0;
 return src_len;
 }
 }
 */
#endif

#if WIN32
#define M_PI 3.14159265358979323846264338327950288
#else
#ifndef USE_JOYSTICK
#define USE_JOYSTICK 1
#endif

#ifndef USE_LEAP
#define USE_LEAP 1
#endif
#endif



#include "../JuceLibraryCode/JuceHeader.h"

#include "Speaker.h"
#include "Source.h"
#include "DefaultParam.h"
#include "SourceMover.h"
#include "Trajectory.h"

using namespace std;

typedef Point<float> FPoint;


//==============================================================================
class SpatGrisAudioProcessor :  public AudioProcessor,
private Timer
{
public:
    //==============================================================================
    SpatGrisAudioProcessor();
    ~SpatGrisAudioProcessor();
    
    const String getName() const override;
    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    
    void processBlock (AudioBuffer<float> &buffer, MidiBuffer& midiMessages) override;
    void processBlockBypassed (AudioBuffer<float> &buffer, MidiBuffer& midiMessages) override;
    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    
    //==============================================================================
    double getTailLengthSeconds() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;
    //==============================================================================
    void timerCallback() override;
    
    //==============================================================================
    SourceMover * getSourceMover()  { return this->sourceMover; }
    Trajectory * getTrajectory()    { return this->trajectory; }
    
    unsigned int getNumSourceUsed() { return this->numSourceUsed;   }
    unsigned int getNumSpeakerUsed(){ return this->numSpeakerUsed;  }
    
    Source **  getListSource() { return this->listSources;  }
    Speaker ** getListSpeaker() { return this->listSpeakers; }
    
    SelectItem * getSelectItem() { return this->selectItem; }
    
    bool    isPlaying(){ return this->cpi.isPlaying; }
    //----
    ProcessType getTypeProcess() { return this->typeProcess; }
    void setTypeProcess(ProcessType v);
    
    bool getLinkHeight()    { return this->linkHeight; }
    bool getLinkAzimuth()   { return this->linkAzimuth; }
    bool getLinkElevation() { return this->linkElevation; }

    
    void setLinkAzimuth(bool v)     { this->linkAzimuth = v; }
    void setLinkElevation(bool v)   { this->linkElevation = v; }
    bool setLinkHeight(bool v)      { this->linkHeight= v; }
    
    void setIdInOutMode(int idMode);
    InOutMode getInIoutMode(){return (InOutMode)((int)this->inOutModeUsed+1); }
    //----
    bool            getOscOn()              { return this->oscOn; }
    unsigned int    getOscFirstIdSource()   { return this->oscFirstIdSource; }
    unsigned int    getOscPort()            { return this->oscPort; }
    bool            getOscRun()             { return this->oscRun; }
    
    void setOscOn(bool v)                   { this->oscOn = v; }
    void setOscFirstIdSource(unsigned int v){ this->oscFirstIdSource = v; }
    void setOscPort(unsigned int v)         {   this->oscPort = v;
        this->oscRun = this->oscSender.connect(this->oscIpAddress, this->oscPort);
    }
    //==============================================================================
    
    
    //==============================================================================
    void    setPosXYSource(int idS, float x, float y, bool updateAll = true);
    void    setPosRayAngSource(int idS, float ray, float ang, bool updateAll = true);
    void    setPosRayAngRadSource(int idS, float ray, float ang, bool updateAll = true);
    
    FPoint  getXYSource(int idS);
    FPoint  getRayAngleSource(int idS);
    
    void setHeightSValue(float elev);
    void setAzimuthValue(float azim);
    void setElevationValue(float elev);

    float getLevel(int index){ return (15.0f * log10f(sqrtf(this->listSpeakers[index]->getLevel()))); }
    //==============================================================================
    
    
    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    //==============================================================================
    
private:
    void processTrajectory();
    void sendOscMessageValues();
    
    //Audio Param =================================
    double  sampleRate;
    int     bufferSize;
    AudioBuffer<float> pBufferIn;
    
    //=============================================
    Source *   listSources[MaxSources];
    Speaker *  listSpeakers[MaxSpeakers];
    unsigned int numSourceUsed;
    unsigned int numSpeakerUsed;
    InOutMode    inOutModeUsed;
    
    ProcessType typeProcess;
    
    SelectItem * selectItem;
    
    bool linkAzimuth    = false;
    bool linkElevation  = false;
    bool linkHeight     = false;
    
    AudioPlayHead::CurrentPositionInfo cpi;
    //Trajectory param========================
    SourceMover     * sourceMover;
    Trajectory      * trajectory;
    //========================================
    
    //OSC param========================
    OSCSender       oscSender;
    bool            oscOn               = true;
    bool            oscRun              = false;
    unsigned int    oscFirstIdSource    = 1;
    unsigned int    oscPort             = OscDefPort;
    String          oscIpAddress        = "127.0.0.1";
    //========================================
    
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpatGrisAudioProcessor)
};



#endif  // PLUGINPROCESSOR_H_INCLUDED
