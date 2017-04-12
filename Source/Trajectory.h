/*
 ==============================================================================
 SpatGRIS: multichannel sound spatialization plug-in.
 
 Copyright (C) 2015  GRIS-UdeM
 
 Trajectories.h
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

#ifndef TRAJECTORY_H_INCLUDED
#define TRAJECTORY_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

#include "DefaultParam.h"

using namespace std;

typedef Point<float> FPoint;




/*struct TrajectoryProperties {
    int                     type;
    //SpatGrisAudioProcessor* filter;
    //SourceMover*            mover;
    float                   duration;
    bool                    beats;
    //unique_ptr<AllTrajectoryDirections> direction;
    bool                    bReturn;
    float                   repeats;
    float                   dampening;
    float                   deviation;
    float                   turns;
    float                   width;
    FPoint                  endPoint;
    vector<FPoint>          listPoints;
};*/

class SpatGrisAudioProcessor;
class Trajectory
{
public:
    Trajectory();
    ~Trajectory();
    
    //Getter
    bool   getProcessTrajectory(){ return this->processTrajectory; };
    TrajectoryType getTrajectoryType(){ return this->typeSelect; }
  
    float getTimeDuration(){ return this->timeDuration; }
    bool  getInSeconds(){ return this->inSeconds; }
    float getCycle(){ return this->cycle; }
    float getSpeed(){ return this->speed; }
    
    float getEllipseWidth(){ return this->ellipseWidth; }
    bool  getInOneWay(){ return this->inOneWay; }
    float getRadiusEnd(){ return this->radiusEnd; }
    float getAngleEnd(){ return this->angleEnd; }
    
    float getPendDampening(){ return this->pendDampening; }
    float getPendDeviation(){ return this->pendDeviation; }
    
    float getRandSpeed(){ return this->randSpeed; }
    bool  getRandSeparate(){ return this->randSeparate; }
    vector<FPoint>  getListPointsFreeDraw(){ return this->listPointsFreeDraw; }
    
    //Setter
    void setProcessTrajectory(bool b){ this->processTrajectory = b; };
    void setTrajectoryType(TrajectoryType i){ this->typeSelect = i; }
    void setTimeDuration(float t){ this->timeDuration = t; }
    void setInSeconds(bool b){ this->inSeconds = b; }
    void setCycle(float c){ this->cycle = c; }
    void setSpeed(float s){ this->speed = s; }
    
    void setEllipseWidth(float w){ this->ellipseWidth = w; }
    void setInOneWay(bool o){ this->inOneWay = o; }
    void setRadiusEnd(float r){ this->radiusEnd = r; }
    void setAngleEnd(float a){ this->angleEnd = a; }
    
    void setPendDampening(float d){ this->pendDampening = d; }
    void setPendDeviation(float d){ this->pendDeviation = d; }
    
    void setRandSpeed(float s){ this->randSpeed = s; }
    void setRandSeparate(bool s){ this->randSeparate = s; }
    void setListPointsFreeDraw(vector<FPoint> vfp){ this->listPointsFreeDraw = vfp; }

private:
    bool    processTrajectory = false;
    
    TrajectoryType  typeSelect;
    float           timeDuration = 1.f;
    bool            inSeconds = true;      //Second(true) or beat(false)
    float           cycle = 1.f;
    float           speed = 1.f;
    
    float           ellipseWidth = 0.f;
    bool            inOneWay = true;       //OneWay(true) or Return(false)
    float           radiusEnd = 0.f;
    float           angleEnd = 0.f;
    
    float           pendDampening = 0.f;
    float           pendDeviation = 0.f;
    
    float           randSpeed = 1.f;
    bool            randSeparate = false;
    vector<FPoint>  listPointsFreeDraw;
};

/*
class Trajectory : public ReferenceCountedObject
{
public:
    typedef ReferenceCountedObjectPtr<Trajectory> Ptr;

    static String GetTrajectoryName(TrajectoryType i);
    static Trajectory::Ptr CreateTrajectory(const TrajectoryProperties& properties);
    
    static unique_ptr<vector<String>> getAllPossibleDirections(int p_iTrajectory);
    //static unique_ptr<AllTrajectoryDirections> getCurDirection(int p_iSelectedTrajectory, int p_iSelectedDirection);
    static unique_ptr<std::vector<String>> getAllPossibleReturns(int p_iTrajectory);
    
public:
    virtual ~Trajectory() {}
    
    
    bool process(float seconds, float beats, float speed, float speedRand);
    bool useBeats();
    bool isInfinite();
    float getTotalDuration();
    float getCurrentTime();
    float progress();
    int progressCycle();
    void stop(bool clearTrajectory = true);
    
protected:
    virtual void childProcess(float duration, float seconds,float speedRand = 0) = 0;
    virtual void childInit() {}
    Array<Point<float>> mSourcesInitialPositionRT;
    
private:
    void start();
    
protected:
    Trajectory(const TrajectoryProperties& properties);
    SpatGrisAudioProcessor *mFilter;
    //SourceMover *m_pMover;
    bool  m_bStarted, m_bStopped;
    float m_fTimeDone;
    float m_fDurationSingleTraj;
    float m_fTotalDuration;
    bool  m_bUseBeats;
    float m_fSpeed;
    bool m_bInfLoopRepeats;
};*/
#endif  // TRAJECTORY_H_INCLUDED
