//
//  Source.hpp
//  SpatGRIS
//
//  Created by GRIS on 2017-04-07.
//
//

#ifndef Source_h
#define Source_h


#include "../JuceLibraryCode/JuceHeader.h"

class SpatGrisAudioProcessor;
class Source{
    
public:
    Source(SpatGrisAudioProcessor * filt, unsigned int idS = 0);
    ~Source();
    
    unsigned int getId(){ return this->id; }
    //unsigned int getIdOSC(){ return this->idOSC; }
    //void setIdOSC(unsigned int idO){ this->idOSC = idO; }
    
    AudioParameterFloat * getX(){ return this->audPX; }
    AudioParameterFloat * getY(){ return this->audPY; }
    AudioParameterFloat * getHeigt(){ return this->audPHeight; }
    AudioParameterFloat * getAzim(){ return this->audPAzim; }
    AudioParameterFloat * getElev(){ return this->audPElev; }
    
private:
    SpatGrisAudioProcessor * filter;
    unsigned int id;
    //unsigned int idOSC;
    
    AudioParameterFloat     * audPX;
    AudioParameterFloat     * audPY;
    AudioParameterFloat     * audPHeight;
    AudioParameterFloat     * audPAzim;
    AudioParameterFloat     * audPElev;
    
    
};

#endif /* Source_h */
