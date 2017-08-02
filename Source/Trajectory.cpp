/*
 ==============================================================================
 SpatGRIS: multichannel sound spatialization plug-in.
 
 Copyright (C) 2015  GRIS-UdeM
 
 Trajectories.cpp
 Created: 3 Aug 2014 11:42:38am
 
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

#include "Trajectory.h"
#include "PluginProcessor.h"

Trajectory::Trajectory(SpatGrisAudioProcessor * filter)
{
    this->filter = filter;
    this->listSourceRayAng = vector<FPoint>();
    for(int i = 0; i < MaxSources; ++i){
        this->listSourceRayAng.push_back(FPoint());
    }
}

Trajectory::~Trajectory()
{
}


void Trajectory::start()
{
    this->turnCount = 0;
    this->timeDone = 0.0f;
    this->timeTotalDuration = (this->timeDuration * this->cycle);
    
    for (int i = 0; i < this->filter->getNumSourceUsed(); i++){
        this->listSourceRayAng[i] = this->filter->getRayAngleSource(i);
    }
    this->filter->getSourceMover()->beginMouvement();
    this->processTrafEnd = true;
}

//return true if the trajectory is finished, false otherwise
bool Trajectory::process(float seconds, float beats)
{
    if(!processTrajectory){ return false; }
    
    if (this->speed * this->timeDone >= this->timeTotalDuration && ! this->infCycleLoop) {
        stop();
        return false;
    }
    
    float duration = this->inSeconds ? seconds : beats;
    switch (this->typeSelect) {
        case Circle:{
            circleProcess();
            break;
        }
        case Ellipse:{
            ellipseProcess();
            break;
        }
        case Spiral:{
            spiralProcess();
            break;
        }
            /*case Pendulum:{
             circleProcess();
             break;
             }
             case RandomTraj:{
             circleProcess();
             break;
             }
             case RandomTarget:{
             circleProcess();
             break;
             }
             case SymXTarget:{
             circleProcess();
             break;
             }
             case SymYTarget:{
             circleProcess();
             break;
             }
             case FreeDrawing:{
             circleProcess();
             break;
             }*/
            
    }
    
    
    this->timeDone += (duration * this->speed);
    return true;
}

float Trajectory::getProgressBar()
{
    //progress bar
    if(this->infCycleLoop){ return 1.0f; }
    return   (this->speed * this->timeDone) / this->timeTotalDuration;
}

int Trajectory::progressCycle()
{
    return (int) (this->timeDone / this->timeDuration);
}

void Trajectory::stop(bool clearTrajectory)
{
    this->processTrafEnd = false;   
}

void Trajectory::restorePosSources()
{
    for (int i = 0; i < this->filter->getNumSourceUsed(); i++){
        this->filter->setPosRayAngRadSource(i, this->listSourceRayAng[i].x, this->listSourceRayAng[i].y);
    }
}
// ===========================================================================================================
void Trajectory::circleProcess()
{
    float deltaTheta = ((float)(this->timeDone / this->timeDuration) * 2 * M_PI);
    
    double maxDelta = (this->turnCount * 2 * M_PI) + (2 * M_PI * (this->cyclePercent/100.0f));
    
    if(this->speed < 0.0f){maxDelta = -maxDelta;}
    if(this->turnCount>0){
        deltaTheta += this->turnCount *((2 * M_PI) - (2 * M_PI * (this->cyclePercent/100.0f))) ;
    }
    cout << maxDelta << " - "<< deltaTheta << newLine;

    if(deltaTheta > maxDelta){
        this->turnCount+=1;
         cout << " >> "<< this->turnCount << newLine;
    }
    
    const int idMaster = 0;//this->filter->getSelectItem()->selectID;
    FPoint startPointRT  = this->listSourceRayAng[idMaster];
    
    this->filter->setPosRayAngRadSource(idMaster, startPointRT.x, startPointRT.y+deltaTheta);
    
}

void Trajectory::ellipseProcess()
{
    float deltaTheta = (float)(this->timeDone / this->timeDuration);
    deltaTheta = deltaTheta * 2 * M_PI ;
    
    // calculate fCurR and fCurT , using http://www.edmath.org/MATtours/ellipses/ellipses1.07.3.html first part at the top, with a = 1
    float cosDa = cosf(deltaTheta);
    float b2 = this->ellipseWidth*this->ellipseWidth;
    float r2 = b2 / ((b2-1) * cosDa * cosDa + 1);
    
    const int idMaster = 0;///this->filter->getSelectItem()->selectID;
    FPoint startPointRT  = this->listSourceRayAng[idMaster];
    
    this->filter->setPosRayAngRadSource(idMaster, startPointRT.x * sqrt(r2), startPointRT.y + deltaTheta);
    
}

void Trajectory::spiralProcess()
{
    float deltaTheta = (float)(this->timeDone / this->timeDuration);
    float deltaThetaFactor = deltaTheta;
    
    int multiDetlTheta = (this->inOneWay ? 1 : 2);
    deltaTheta = deltaTheta * multiDetlTheta * M_PI;
    
    if(!this->inOneWay && (deltaTheta > M_PI || deltaTheta < 0.0f)){
        deltaTheta = multiDetlTheta * fmodf(this->timeDone / this->timeDuration * M_PI, M_PI);
        if(this->timeDone<=0){ this->timeDone = this->timeDuration; }
    }
    
    const int idMaster = 0;//this->filter->getSelectItem()->selectID;
    FPoint startPointRT  = this->listSourceRayAng[idMaster];
    
    float fCurT   = startPointRT.y + deltaTheta * 2;
    float fStartR = startPointRT.x;
    float fDeltaR = (cosf(deltaTheta)+1) * 0.5;   //l here oscillates between 1 @ start and 0 when fDeltaTheta == M_PI), following a cosine. linear is : float fDeltaR = (M_PI - fDeltaTheta) / M_PI;
    float fCurR;
    if (startPointRT.x > radiusEnd) {
        fCurR = fStartR * fDeltaR;
    } else {
        fCurR = fStartR + (1 - fDeltaR) * (radiusEnd- fStartR);
    }
    
    //CARTESIAN TRANSLATION
    
    FPoint curPointXY01 = GetXYFromRayAng(fCurR, fCurT);
    
    if (this->inOneWay && fabs(deltaTheta) >= M_PI){
        deltaThetaFactor = 1-deltaThetaFactor;
    }
    if (startPointRT.x > radiusEnd){
        curPointXY01.x += deltaThetaFactor * (1 - radiusEnd);
        curPointXY01.y -= deltaThetaFactor * (1 - angleEnd);
    } else {
        FPoint untranslatedEndOutPointXY01 = GetXYFromRayAng(radiusEnd, angleEnd);
        curPointXY01.x += deltaThetaFactor * (radiusEnd - untranslatedEndOutPointXY01.x);
        curPointXY01.y -= deltaThetaFactor * (angleEnd - untranslatedEndOutPointXY01.y);
    }
    
    this->filter->setPosXYSource(idMaster, curPointXY01.x, curPointXY01.y);
    
    
}
