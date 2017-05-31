/*
 ==============================================================================
 SpatGRIS: multichannel sound spatialization plug-in.
 
 Copyright (C) 2015  GRIS-UdeM
 
 SourceMover.h
 Created: 8 Aug 2014 1:04:53pm
 
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


#ifndef SOURCEMOVER_H_INCLUDED
#define SOURCEMOVER_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"


#include "DefaultParam.h"

using namespace std;


class SpatGrisAudioProcessor;

class SourceMover
{
public:
    
    SourceMover(SpatGrisAudioProcessor *filt);
    ~SourceMover();
    
    MouvementMode getMouvementMode(){ return (MouvementMode)this->mouvementModeAudioParam->getIndex(); }
    void setMouvementMode(MouvementMode m);
    
    //==================================================
    void setSourcesPosition(PositionSourceSpeaker pss = LeftAlternate);
    void setSpeakersPosition(PositionSourceSpeaker pss = LeftAlternate);
    
    void beginMouvement();
    
    //==================================================
    void updateSourcesPosition(int iSource, float x, float y);
    
    
    /*void begin(int s, MoverType mt);
     void sortAngles();
     void setEqualRadius();
     void setEqualAngles();
     void setEqualRadiusAndAngles();
     void setSymmetricX();
     void setSymmetricY();
     void move(FPoint p, MoverType mt);
     void storeAllDownPositions();
     void storeDownPosition(int id, FPoint pointRT);
     void end(MoverType mt, bool clearTrajectory = true);
     void setFieldExists(bool v){ mFieldExists = v; }
     
     void setFieldComponent(FieldComponent* field){
     mFieldExists = true;
     mField = field;
     }*/
    
private:
    void setEqualAngles();
    void setEqualRadiusAndAngles();
    void sortAngles();
    
    
    SpatGrisAudioProcessor * filter;
    //MouvementMode   mouvementModeSelect;
    AudioParameterChoice * mouvementModeAudioParam;
    
    StringArray     listMouvement;
    
    Array<FPoint>   listSourceXY;
    Array<FPoint>   listSourceRayAng;
    Array<float>    listAngSourceSorted;

    //AudioParameterChoice * mouvementChoiceAuto;
    /*MoverType mMoverType;
     int mSelectedSrc;
     bool mFieldExists;
     
     
     Array<FPoint> mSourcesDownRT;
     Array<float> mSourcesAngularOrder;
     FieldComponent* mField;*/
};

#endif  // SOURCEMOVER_H_INCLUDED
