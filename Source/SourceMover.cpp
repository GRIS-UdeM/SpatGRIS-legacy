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
