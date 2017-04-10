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
class SpatGrisAudioProcessor : public AudioProcessor
{
public:
    //==============================================================================
    SpatGrisAudioProcessor();
    ~SpatGrisAudioProcessor();

    const String getName() const;
    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock);
    void releaseResources();

    void processBlock (AudioBuffer<float> &buffer, MidiBuffer& midiMessages);
    void processBlockBypassed (AudioBuffer<float> &buffer, MidiBuffer& midiMessages);
    //==============================================================================
    AudioProcessorEditor* createEditor();
    bool hasEditor() const;

    //==============================================================================
    double getTailLengthSeconds() const;
    bool acceptsMidi() const;
    bool producesMidi() const;
    int getNumPrograms();
    int getCurrentProgram();
    void setCurrentProgram (int index);
    const String getProgramName (int index);
    void changeProgramName (int index, const String& newName);
    //==============================================================================
    
    
    //==============================================================================
    SourceMover * getSourceMover()  { return this->sourceMover; }
    Trajectory * getTrajectory()    { return this->trajectory; }
    
    unsigned int getNumSourceUsed() { return this->numSourceUsed;   }
    unsigned int getNumSpeakerUsed(){ return this->numSpeakerUsed;  }
    
	vector<Source *>  getListSource() { return this->listSources;  }
	vector<Speaker *> getListSpeaker() { return this->listSpeakers; }
    void  setPosXYSource(int idS, float x, float y){ *(this->listSources[idS]->getX()) = x; *(this->listSources[idS]->getY()) = y; }
    
    SelectItem * getSelectItem() { return this->selectItem; }
    
    bool getLinkSurface()   { return this->linkSurface; }
    bool getLinkAzimuth()   { return this->linkAzimuth; }
    bool getLinkElevation() { return this->linkElevation; }
    
    void setLinkSurface(bool v)     { this->linkSurface = v; }
    void setLinkAzimuth(bool v)     { this->linkAzimuth = v; }
    void setLinkElevation(bool v)   { this->linkElevation = v; }
    //==============================================================================
    
    
    //==============================================================================
    void setSurfaceValue(float surf);
    void setAzimuthValue(float azim);
    void setElevationValue(float elev);
    
    //==============================================================================
    
    
    //==============================================================================
    void getStateInformation (MemoryBlock& destData);
    void setStateInformation (const void* data, int sizeInBytes);
    //==============================================================================

private:

    vector<Source *>   listSources;
    vector<Speaker *>  listSpeakers;
    unsigned int numSourceUsed;
    unsigned int numSpeakerUsed;
    
    SelectItem * selectItem;
    
    bool linkSurface;
    bool linkAzimuth;
    bool linkElevation;
    
    //Trajectory param========================
    SourceMover     * sourceMover;
    Trajectory      * trajectory;
    
    
    //========================================
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpatGrisAudioProcessor)
};



#endif  // PLUGINPROCESSOR_H_INCLUDED
