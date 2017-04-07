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
    Source(SpatGrisAudioProcessor * filt, int idS = 0);
    ~Source();
    
    AudioParameterFloat * getX(){ return this->audPX; }
    AudioParameterFloat * getY(){ return this->audPY; }
    AudioParameterFloat * getSurf(){ return this->audPSurf; }
    AudioParameterFloat * getAzim(){ return this->audPAzim; }
    AudioParameterFloat * getElev(){ return this->audPElev; }
    
private:
    SpatGrisAudioProcessor * filter;
    int id;
    
    AudioParameterFloat     * audPX;
    AudioParameterFloat     * audPY;
    AudioParameterFloat     * audPSurf;
    AudioParameterFloat     * audPAzim;
    AudioParameterFloat     * audPElev;
    
    
};

#endif /* Source_h */
