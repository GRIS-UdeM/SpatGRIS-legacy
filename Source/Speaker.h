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

#include "DefaultParam.h"

class SpatGrisAudioProcessor;
class Speaker{

public:
    Speaker(SpatGrisAudioProcessor * filt, unsigned int idS = 0);
    ~Speaker();

    unsigned int getId(){ return this->id; };
    float getX(){ return this->px; }
    float getY(){ return this->py; }
    FPoint getPosXY(){ return FPoint(this->px, this->py); }
    bool isMuted(){ return this->muted; }
    float getLevel(){ return this->level; }
    
    void setX(float x){ this->px = x; }
    void setY(float y){ this->py = y; }
    void setPosXY(FPoint xy){ this->px = xy.x; this->py = xy.y; }
    void setMuted(bool m){ this->muted = m; }
    void setLevel(float lev){ this->level = lev; }
    
private:
    SpatGrisAudioProcessor * filter;
    unsigned int id;
    
    float px;
    float py;
    bool muted;
    float level;
    
    
};

#endif /* Speaker_h */
