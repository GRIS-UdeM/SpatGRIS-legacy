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
    this->listSourceXY.resize(MaxSources);
    this->listSourceRayAng.resize(MaxSources);
    this->listAngSourceSorted.resize(MaxSources);
    
    for(int i = 0; i  < MouvementMode::SIZE_MM; i++){
        this->listMouvement.add(GetMouvementModeName((MouvementMode)i));
    }
    this->mouvementModeAudioParam = new AudioParameterChoice("Mouvement","Mouv Trajectory", this->listMouvement, 0);
    this->filter->addParameter(this->mouvementModeAudioParam);
    
}

SourceMover::~SourceMover()
{
}

void SourceMover::setMouvementMode(MouvementMode m)
{
    (*this->mouvementModeAudioParam) = m;
    switch (*this->mouvementModeAudioParam) {
            
        case CircularFixAng:{
            this->setEqualAngles();
            break;
        }
            
            
        case CircularFullyFix:{
            this->setEqualRadiusAndAngles();
            break;
        }
            
    }
    this->beginMouvement();

}
//============================================================================
void SourceMover::setSourcesPosition(PositionSourceSpeaker pss)
{
    bool alternate = false;
    bool startAtTop = false;
    bool clockwise = false;
    
    switch (pss){
        case LeftAlternate:{
            alternate = true;
            break;
        }
        case LeftClockW:{
            clockwise = true;
            break;
        }
        case LeftCounterClockW:{
            break;
        }
        case TopClockW:{
            startAtTop = true;
            clockwise = true;
            break;
        }
        case TopCounterClockW:{
            startAtTop = true;
            break;
        }
    }
    
    float anglePerSp = ThetaMax / this->filter->getNumSourceUsed();
    if (alternate) {
        float offset = startAtTop ?
        (clockwise ? QuarterCircle : (QuarterCircle - anglePerSp))
        :   (QuarterCircle - anglePerSp/2);
        
        float start = offset;
        
        for (int i = clockwise ? 0 : 1; i < this->filter->getNumSourceUsed(); i += 2){
            FPoint xy = GetXYFromRayAng(1.0f, offset);
            this->filter->setPosXYSource(i, xy.x, xy.y, false);
            offset -= anglePerSp;
        }
        
        offset = start + anglePerSp;
        
        for (int i = clockwise ? 1 : 0; i < this->filter->getNumSourceUsed(); i += 2){
            FPoint xy = GetXYFromRayAng(1.0f, offset);
            this->filter->setPosXYSource(i, xy.x, xy.y, false);
            offset += anglePerSp;
        }
    }
    else {
        float offset = startAtTop ? QuarterCircle : (QuarterCircle + anglePerSp/2);
        float delta = clockwise ? -anglePerSp : anglePerSp;
        
        for (int i = 0; i < this->filter->getNumSourceUsed(); i++){
            FPoint xy = GetXYFromRayAng(1.0f, offset);
            this->filter->setPosXYSource(i, xy.x, xy.y, false);
            offset += delta;
        }
    }
}

void SourceMover::setSpeakersPosition(PositionSourceSpeaker pss)
{
    bool alternate = false;
    bool startAtTop = false;
    bool clockwise = false;
    
    switch (pss){
        case LeftAlternate:{
            alternate = true;
            break;
        }
        case LeftClockW:{
            clockwise = true;
            break;
        }
        case LeftCounterClockW:{
            break;
        }
        case TopClockW:{
            startAtTop = true;
            clockwise = true;
            break;
        }
        case TopCounterClockW:{
            startAtTop = true;
            break;
        }
    }
    
    float anglePerSp = ThetaMax / this->filter->getNumSpeakerUsed();
    if (alternate) {
        float offset = startAtTop ?
        (clockwise ? QuarterCircle : (QuarterCircle - anglePerSp))
        :   (QuarterCircle - anglePerSp/2);
        
        float start = offset;
        
        for (int i = clockwise ? 0 : 1; i < this->filter->getNumSpeakerUsed(); i += 2){
            this->filter->getListSpeaker()[i]->setPosXY(GetXYFromRayAng(1.0f, offset));
            offset -= anglePerSp;
        }
        
        offset = start + anglePerSp;
        
        for (int i = clockwise ? 1 : 0; i < this->filter->getNumSpeakerUsed(); i += 2){
            this->filter->getListSpeaker()[i]->setPosXY(GetXYFromRayAng(1.0f, offset));
            offset += anglePerSp;
        }
    }
    else {
        float offset = startAtTop ? QuarterCircle : (QuarterCircle + anglePerSp/2);
        float delta = clockwise ? -anglePerSp : anglePerSp;
        
        for (int i = 0; i < this->filter->getNumSpeakerUsed(); i++){
            this->filter->getListSpeaker()[i]->setPosXY(GetXYFromRayAng(1.0f, offset));
            offset += delta;
        }
    }
}

void SourceMover::beginMouvement()
{
    for (int i = 0; i < this->filter->getNumSourceUsed(); i++) {
        this->listSourceXY.setUnchecked(i, this->filter->getXYSource(i));
        this->listSourceRayAng.setUnchecked(i, this->filter->getRayAngleSource(i));
    }
}

//============================================================================
void SourceMover::updateSourcesPosition(int iSource, float x, float y)
{
    
    //deltaMasterPos = NewRayAnl - OldRayAng
    FPoint currSelectSXY        = FPoint(x,y);
    FPoint currSelectSRayAng    = FPoint(GetRaySpat(x, y), GetAngleSpat(x, y));
    FPoint deltaMasterPos;
    
    
    switch (*this->mouvementModeAudioParam) {
            
        case Independent:{
            deltaMasterPos = currSelectSRayAng - this->listSourceRayAng[iSource];
            if (deltaMasterPos.isOrigin() || isnan(deltaMasterPos.x)|| isnan(deltaMasterPos.y)) {
                return;     //return if delta is null
            }
            FPoint newCurSrcPosRT = this->listSourceRayAng[iSource] + deltaMasterPos;
            NormalizeSourceMoverRayAng(newCurSrcPosRT);
            
            FPoint xy = GetXYFromRayAng(newCurSrcPosRT.x, newCurSrcPosRT.y);
            this->filter->setPosXYSource(iSource, xy.x, xy.y, false);
            break;
        }
            
            
        case Circular:{
            deltaMasterPos = currSelectSRayAng - this->listSourceRayAng[iSource];
            if (deltaMasterPos.isOrigin() || isnan(deltaMasterPos.x)|| isnan(deltaMasterPos.y)) {
                return;     //return if delta is null
            }
            for (int i = 0; i < this->filter->getNumSourceUsed(); ++i) {
                //newCurSrcPosRT = Old + delta
                FPoint newCurSrcPosRT = this->listSourceRayAng[i] + deltaMasterPos;
                NormalizeSourceMoverRayAng(newCurSrcPosRT);
                
                FPoint xy = GetXYFromRayAng(newCurSrcPosRT.x, newCurSrcPosRT.y);
                this->filter->setPosXYSource(i, xy.x, xy.y, false);
            }
            break;
        }
            
            
        case CircularFixRad:{
            deltaMasterPos = currSelectSRayAng - this->listSourceRayAng[iSource];
            if (deltaMasterPos.isOrigin() || isnan(deltaMasterPos.x)|| isnan(deltaMasterPos.y)) {
                return;     //return if delta is null
            }
            for (int i = 0; i < this->filter->getNumSourceUsed(); ++i) {
                //newCurSrcPosRT = Old + delta
                FPoint newCurSrcPosRT = this->listSourceRayAng[i] + deltaMasterPos;
                NormalizeSourceMoverRayAng(newCurSrcPosRT);
                
                FPoint xy = GetXYFromRayAng(currSelectSRayAng.x, newCurSrcPosRT.y);
                this->filter->setPosXYSource(i, xy.x, xy.y, false);
            }
            break;
        }
            
            
        case CircularFixAng:{
            deltaMasterPos = currSelectSRayAng - this->listSourceRayAng[iSource];
            if (deltaMasterPos.isOrigin() || isnan(deltaMasterPos.x)|| isnan(deltaMasterPos.y)) {
                return;     //return if delta is null
            }
            for (int i = 0; i < this->filter->getNumSourceUsed(); ++i) {
                //newCurSrcPosRT = Old + delta
                FPoint newCurSrcPosRT = this->listSourceRayAng[i] + deltaMasterPos;
                NormalizeSourceMoverRayAng(newCurSrcPosRT);
                
                FPoint xy = GetXYFromRayAng(newCurSrcPosRT.x, newCurSrcPosRT.y);
                this->filter->setPosXYSource(i, xy.x, xy.y, false);
            }
            break;
            
        }
            
            
        case CircularFullyFix:{
            deltaMasterPos = currSelectSRayAng - this->listSourceRayAng[iSource];
            if (deltaMasterPos.isOrigin() || isnan(deltaMasterPos.x)|| isnan(deltaMasterPos.y)) {
                return;     //return if delta is null
            }
            for (int i = 0; i < this->filter->getNumSourceUsed(); ++i) {
                //newCurSrcPosRT = Old + delta
                FPoint newCurSrcPosRT = this->listSourceRayAng[i] + deltaMasterPos;
                NormalizeSourceMoverRayAng(newCurSrcPosRT);
                
                FPoint xy = GetXYFromRayAng(newCurSrcPosRT.x, newCurSrcPosRT.y);
                this->filter->setPosXYSource(i, xy.x, xy.y, false);
            }
            break;
        }
            
            
        case DeltaLock:{
            deltaMasterPos =  currSelectSXY - this->listSourceXY[iSource];
            if (deltaMasterPos.isOrigin() || isnan(deltaMasterPos.x)|| isnan(deltaMasterPos.y)) {
                return;     //return if delta is null
            }
            for (int i = 0; i < this->filter->getNumSourceUsed(); ++i) {
                FPoint newCurSrcPosXY = this->listSourceXY[i] + deltaMasterPos;
                NormalizeSourceMoverXY(newCurSrcPosXY);
                this->filter->setPosXYSource(i, newCurSrcPosXY.x, newCurSrcPosXY.y, false);
            }
            break;
        }
            
            
        case SymmetricX:{
            deltaMasterPos = currSelectSXY - this->listSourceXY[iSource];
            if (deltaMasterPos.isOrigin() || isnan(deltaMasterPos.x)|| isnan(deltaMasterPos.y)) {
                return;     //return if delta is null
            }
            for (int i = 0; i < this->filter->getNumSourceUsed(); ++i) {
                
                if(this->listSourceXY[i].y<0 && i != iSource){
                    deltaMasterPos.y = -deltaMasterPos.y;
                }
                
                
                FPoint newCurSrcPosXY =this->listSourceXY[i] + deltaMasterPos;
                NormalizeSourceMoverXY(newCurSrcPosXY);
                this->filter->setPosXYSource(i, newCurSrcPosXY.x, newCurSrcPosXY.y, false);
            }
            break;
        }
            
            
        case SymmetricY:{
            deltaMasterPos =  currSelectSXY - this->listSourceXY[iSource];
            if (deltaMasterPos.isOrigin() || isnan(deltaMasterPos.x)|| isnan(deltaMasterPos.y)) {
                return;     //return if delta is null
            }
            for (int i = 0; i < this->filter->getNumSourceUsed(); ++i) {
                if(this->listSourceXY[i].x<0 && i != iSource){
                    deltaMasterPos.x = -deltaMasterPos.x;
                    
                }
                FPoint newCurSrcPosXY =this->listSourceXY[i] + deltaMasterPos;
                NormalizeSourceMoverXY(newCurSrcPosXY);
                this->filter->setPosXYSource(i, newCurSrcPosXY.x, newCurSrcPosXY.y, false);
            }
            break;
        }
        default:
            jassertfalse;
            break;
            
    }
    
}

//==================================================================================================
void SourceMover::setEqualAngles()
{
    this->sortAngles();
    
    //then set them
    FPoint selSrcRT = this->filter->getRayAngleSource(this->filter->getSelectItem()->selectIdSource);
    for (int iCurSrc = 0; iCurSrc < this->filter->getNumSourceUsed(); iCurSrc++) {
        if (iCurSrc == this->filter->getSelectItem()->selectIdSource){
            continue;
        }
        FPoint curSrcRT = this->filter->getRayAngleSource(iCurSrc);
        curSrcRT.y = selSrcRT.y + this->listAngSourceSorted[iCurSrc];
        NormalizeSourceMoverRayAng(curSrcRT);
        
        FPoint xy = GetXYFromRayAng(curSrcRT.x, curSrcRT.y);
        this->filter->setPosXYSource(iCurSrc, xy.x, xy.y, false);
        
        /*mFilter->setPreventSourceLocationUpdate(true);
         mFilter->setSourceRT(iCurSrc, curSrcRT, false);
         storeDownPosition(iCurSrc, curSrcRT);
         mFilter->setPreventSourceLocationUpdate(false);*/
        
    }
}

void SourceMover::setEqualRadiusAndAngles()
{
    this->sortAngles();
    
    FPoint selSrcRT = this->filter->getRayAngleSource(this->filter->getSelectItem()->selectIdSource);
    for (int iCurSrc = 0; iCurSrc < this->filter->getNumSourceUsed(); iCurSrc++) {
        if (iCurSrc == this->filter->getSelectItem()->selectIdSource){
            continue;
        }
        FPoint curSrcRT = this->filter->getRayAngleSource(iCurSrc);
        curSrcRT.x = selSrcRT.x;
        curSrcRT.y = selSrcRT.y + this->listAngSourceSorted[iCurSrc];
        NormalizeSourceMoverRayAng(curSrcRT);
        
        FPoint xy = GetXYFromRayAng(curSrcRT.x, curSrcRT.y);
        this->filter->setPosXYSource(iCurSrc, xy.x, xy.y, false);
    }
}

void SourceMover::sortAngles()
{
    int iNbrSrc = this->filter->getNumSourceUsed();
    
    IndexedAngle * ia = new IndexedAngle[iNbrSrc];
    
    for (int j = 0; j < iNbrSrc; j++) {
        ia[j].i = j;
        ia[j].a = this->filter->getRayAngleSource(j).y;
    }
    
    qsort(ia, iNbrSrc, sizeof(IndexedAngle), IndexedAngleCompare);
    
    int b;
    for (b = 0; b < iNbrSrc && ia[b].i != this->filter->getSelectItem()->selectIdSource; b++) ;
    
    if (b == iNbrSrc) {
        printf("sort angle error!\n");
        b = 0;
    }
    
    for (int j = 1; j < iNbrSrc; j++) {
        int o = (b + j) % iNbrSrc;
        o = ia[o].i;
        this->listAngSourceSorted.set(o, (M_PI * 2. * j) / iNbrSrc);
    }
    
    delete[] ia;
}



