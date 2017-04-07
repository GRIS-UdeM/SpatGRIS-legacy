//
//  Speaker.hpp
//  SpatGRIS
//
//  Created by GRIS on 2017-04-06.
//
//

#ifndef Speaker_h
#define Speaker_h

#include "../JuceLibraryCode/JuceHeader.h"

class SpatGrisAudioProcessor;
class Speaker{

public:
    Speaker(SpatGrisAudioProcessor * filt, int idS = 0);
    ~Speaker();

    
private:
    SpatGrisAudioProcessor * filter;
    int id;
    
    
    
};

#endif /* Speaker_h */
