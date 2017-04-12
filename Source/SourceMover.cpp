/*
 ==============================================================================
 SpatGRIS: multichannel sound spatialization plug-in.
 
 Copyright (C) 2015  GRIS-UdeM
 
 SourceMover.cpp
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

#include "SourceMover.h"
#include "PluginProcessor.h"


SourceMover::SourceMover(SpatGrisAudioProcessor *filt):
filter(filt)
{
    this->listSourceRayAng.resize(MaxSources);
    for(int i = 0; i  < MouvementMode::SIZE_MM; i++){
        listMouvement.add(getMouvementModeName((MouvementMode)i));
    }
    //this->mouvementChoiceAuto = new AudioParameterChoice("Mouvement","Mouv Trajectory", listMouvement,1);
    //this->filter->addParameter(this->mouvementChoiceAuto);
}
SourceMover::~SourceMover()
{
}

String SourceMover::getMouvementModeName(MouvementMode i)
{
    switch(i) {
        case Independent:       return "Independent";
        case Circular:          return "Circular";
        case CircularFixRad:    return "Circular Fixed Radius";
        case CircularFixAng:    return "Circular Fixed Angle";
        case CircularFullyFix:  return "Circular Fully Fixed";
        case DeltaLock:         return "Delta Lock";
        case SymmetricX:        return "Symmetric X";
        case SymmetricY:        return "Symmetric Y";

        default:
            jassertfalse;
            return "";
    }
}

void SourceMover::setMouvementMode(MouvementMode m)
{
    this->mouvementModeSelect = m;
    //*this->mouvementChoiceAuto = (int)m;
}
//============================================================================
void SourceMover::setSourcesPosition()
{
    for (int i = 0; i < this->filter->getNumSourceUsed(); ++i) {
        
        FPoint sourceP = FPoint(*(this->filter->getListSource().at(i)->getX()), *(this->filter->getListSource().at(i)->getY()));
        float ray = GetRaySpat(sourceP.x, sourceP.y);
        float ang = GetAngleSpat(sourceP.x, sourceP.y);
            
        ray = 1;
        ang = (i*0.3f);
            
        FPoint xy = GetXYFromRayAng(ray, ang);
        this->filter->setPosXYSource(i, xy.x, xy.y, false);

    }
}

void SourceMover::beginMouvement()
{
    for (int i = 0; i < this->filter->getNumSourceUsed(); i++) {
        this->listSourceRayAng.setUnchecked(i, this->filter->getRayAngleSource(i));
    }
}

//============================================================================
void SourceMover::updateSourcesPosition(int iSource, float x, float y)
{

    //deltaMasterPos = NewRayAnl - OldRayAng
    FPoint deltaMasterPos = FPoint(GetRaySpat(x, y), GetAngleSpat(x, y)) - this->listSourceRayAng[iSource];
    
    if (deltaMasterPos.isOrigin() || isnan(deltaMasterPos.x)|| isnan(deltaMasterPos.y)) {
        return;     //return if delta is null
    }
    

    for (int i = 0; i < this->filter->getNumSourceUsed(); ++i) {
        
        //newCurSrcPosRT = Old + delta
        FPoint newCurSrcPosRT = this->listSourceRayAng[i] + deltaMasterPos;
        
        if (newCurSrcPosRT.x < 0.0f){
            newCurSrcPosRT.x = 0.0f;
        }
        if (newCurSrcPosRT.x > RadiusMax){
            newCurSrcPosRT.x = RadiusMax;
        }
        if (newCurSrcPosRT.y < 0.0f){
            newCurSrcPosRT.y += ThetaMax;
        }
        if (newCurSrcPosRT.y > ThetaMax){
            newCurSrcPosRT.y -= ThetaMax;
        }
        
        FPoint xy = GetXYFromRayAng(newCurSrcPosRT.x, newCurSrcPosRT.y);
        this->filter->setPosXYSource(i, xy.x, xy.y, false);
        
    }
}
