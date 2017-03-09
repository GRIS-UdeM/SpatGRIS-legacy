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

#ifndef TRAJECTORIES_H_INCLUDED
#define TRAJECTORIES_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

#include <memory>

#define N_DECIMAL_POINTS_PRECISION (1000)

typedef Point<float> FPoint;

class SpatGrisAudioProcessor;
class SourceMover;

enum AllTrajectoryDirections {
    CW = 1,
    CCW,
    In,
    Out,
    InCW,
    InCCW,
    OutCW,
    OutCCW,
    Slow,
    Mid,
    Fast,
    None
};

struct TrajectoryProperties {
    int                     type;
    SpatGrisAudioProcessor* filter;
    SourceMover*            mover;
    float                   duration;
    bool                    beats;
    std::unique_ptr<AllTrajectoryDirections> direction;
    bool                    bReturn;
    float                   repeats;
    float                   dampening;
    float                   deviation;
    float                   turns;
    float                   width;
    FPoint                  endPoint;
};

class Trajectory : public ReferenceCountedObject
{
public:
	typedef ReferenceCountedObjectPtr<Trajectory> Ptr;

	static int NumberOfTrajectories();
	static String GetTrajectoryName(int i);
	static Trajectory::Ptr CreateTrajectory(const TrajectoryProperties& properties);
    
    static std::unique_ptr<std::vector<String>> getAllPossibleDirections(int p_iTrajectory);
    static std::unique_ptr<AllTrajectoryDirections> getCurDirection(int p_iSelectedTrajectory, int p_iSelectedDirection);
    static std::unique_ptr<std::vector<String>> getAllPossibleReturns(int p_iTrajectory);

public:
	virtual ~Trajectory() {}
	
    
	bool process(float seconds, float beats, float speed);
    bool useBeats();
    bool isInfinite();
    float getTotalDuration();
    float getCurrentTime();
	float progress();
    int progressCycle();
	void stop(bool clearTrajectory = true);

protected:
	virtual void childProcess(float duration, float seconds) = 0;
    virtual void childInit() {}
    Array<Point<float>> mSourcesInitialPositionRT;
	
private:
	void start();
	
protected:
	Trajectory(const TrajectoryProperties& properties);
	SpatGrisAudioProcessor *mFilter;
    SourceMover *m_pMover;
	bool  m_bStarted, m_bStopped;
	float m_fTimeDone;
	float m_fDurationSingleTraj;
	float m_fTotalDuration;
	bool  m_bUseBeats;
    float m_fSpeed;
    bool m_bInfLoopRepeats;
};

#endif  // TRAJECTORIES_H_INCLUDED
