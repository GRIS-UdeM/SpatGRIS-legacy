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

#include "Trajectories.h"
#include "PluginProcessor.h"
#include "SourceMover.h"

Trajectory::Trajectory(const TrajectoryProperties& properties)
:mFilter(properties.filter)
,m_pMover(properties.mover)
,m_bStarted(false)
,m_bStopped(false)
,m_fTimeDone(0)
,m_fDurationSingleTraj(properties.duration)
,m_bUseBeats(properties.beats)
,m_fSpeed(1.f)
{
    if (m_fDurationSingleTraj < 0.0001) {
        m_fDurationSingleTraj = 0.0001;
    }
    
    m_bInfLoopRepeats  = (properties.repeats == 0.0);
   
    float fRepeats = (properties.repeats < 0.0001) ? 1.0 : properties.repeats;
    m_fTotalDuration = m_fDurationSingleTraj * fRepeats;
}

void Trajectory::start() {
    //tell mover to start the automation, etc
    m_pMover->begin(mFilter->getSelectedSrc(), kTrajectory);
    //store initial position of sources
    for (int i = 0; i < mFilter->getNumberOfSources(); i++){
        mSourcesInitialPositionRT.add(mFilter->getSourceRT(i));
    }
    childInit();
	m_bStarted = true;
}

//return true if the trajectory is finished, false otherwise
bool Trajectory::process(float seconds, float beats) {
	if (m_bStopped) return true;
    if (!m_bStarted) {
        start();
    }
	if (m_fSpeed*m_fTimeDone >= m_fTotalDuration && ! m_bInfLoopRepeats) {
        stop();
		return true;
	}

	float duration = m_bUseBeats ? beats : seconds;
	childProcess(duration, seconds);
	
	m_fTimeDone += (duration*m_fSpeed);
    
	return false;
}

float Trajectory::progress() {
    if(m_bInfLoopRepeats){ return 1.0f; }
	return  m_fSpeed * m_fTimeDone / m_fTotalDuration;
}

//the returned value here will change integers when we're done with one trajectory cycle. E.g., .1,.2,.3,.4,.5,.6,.7,.8,.9, 1.0 (new cycle), 1.1, 1.2 ... 2.0 (new cycle), etc
int Trajectory::progressCycle(){
    return (int) (m_fTimeDone / m_fDurationSingleTraj);
}

void Trajectory::stop(bool clearTrajectory) {
	if (!m_bStarted || m_bStopped) return;
    m_pMover->end(kTrajectory, clearTrajectory);
	m_bStopped = true;
}

//using a unique_ptr here is correct. see http://stackoverflow.com/questions/6876751/differences-between-unique-ptr-and-shared-ptr
std::unique_ptr<vector<String>> Trajectory::getAllPossibleDirections(int p_iTrajectory){
    unique_ptr<vector<String>> vDirections (new vector<String>);
    
    switch(p_iTrajectory) {
        case Circle:
        case EllipseTr:
        case Pendulum:
        case Spiral:
            vDirections->push_back("Clockwise");
            vDirections->push_back("Counter Clockwise");
            break;
        case RandomTrajectory:
            vDirections->push_back("Slow");
            vDirections->push_back("Mid");
            vDirections->push_back("Fast");
            break;
        case RandomTarget:
        case SymXTarget:
        case SymYTarget:
        case ClosestSpeakerTarget:
            return nullptr;
            
        default:
            jassert(0);
    }
    
    return vDirections;
}

unique_ptr<AllTrajectoryDirections> Trajectory::getCurDirection(int p_iSelectedTrajectory, int p_iSelectedDirection){
    
    unique_ptr<AllTrajectoryDirections> pDirection (new AllTrajectoryDirections);
    
    switch (p_iSelectedTrajectory) {
        case Circle:
        case EllipseTr:
            *pDirection = static_cast<AllTrajectoryDirections>(p_iSelectedDirection);
            break;
        case Spiral:
            *pDirection = static_cast<AllTrajectoryDirections>(p_iSelectedDirection+5);
            break;
        case Pendulum:
            *pDirection = static_cast<AllTrajectoryDirections>(p_iSelectedDirection);
            break;
        case RandomTrajectory:
            *pDirection = static_cast<AllTrajectoryDirections>(p_iSelectedDirection+8);
            break;
        case RandomTarget:
        case SymXTarget:
        case SymYTarget:
        case ClosestSpeakerTarget:
            *pDirection = None;
            break;
        default:
            jassert(0);
    }
    
    return pDirection;
}

std::unique_ptr<vector<String>> Trajectory::getAllPossibleReturns(int p_iTrajectory){
    unique_ptr<vector<String>> vReturns (new vector<String>);
    switch(p_iTrajectory) {
        case Spiral:
        case Pendulum:
            vReturns->push_back("One Way");
            vReturns->push_back("Return");
            break;
        case RandomTarget:
            vReturns->push_back("Continuous");
            vReturns->push_back("Discontinuous");
            break;
        case Circle:
        case EllipseTr:
        case RandomTrajectory:
        case SymXTarget:
        case SymYTarget:
        case ClosestSpeakerTarget:
            return nullptr;
        default:
            jassert(0);
    }
    return vReturns;
}

// ==============================================================================
class CircleTrajectory : public Trajectory {
public:
//    CircleTrajectory(SpatGrisAudioProcessor *filter, SourceMover *p_pMover, float duration, bool beats, float times, bool ccw, float p_fTurns)
    CircleTrajectory(const TrajectoryProperties& properties)
    : Trajectory(properties)
    , m_bCCW(*properties.direction == CCW)
    , m_fTurns(properties.turns)
    {
    }
    
protected:
    void childProcess(float duration, float seconds) {
        float fDeltaTheta = (float)(m_fTurns * m_fTimeDone / ((m_fDurationSingleTraj))) ;
        
        //DeltaTheta = modf(fDeltaTheta/m_fTurns, & integralPart) * m_fTurns;      //the modf makes da cycle back to 0 when it reaches m_fTurn, then we multiply it back by m_fTurn to undo the modification
        
        if (!m_bCCW) fDeltaTheta = -fDeltaTheta;
        fDeltaTheta = fDeltaTheta * 2 * M_PI ;
        
        //move to initial position + delta theta
        FPoint startPointRT  = mSourcesInitialPositionRT.getUnchecked(mFilter->getSelectedSrc());
        FPoint newPointXY01  = mFilter->convertRt2Xy01(startPointRT.x, startPointRT.y+fDeltaTheta);
        m_pMover->move(newPointXY01, kTrajectory);
        

    }
	
private:
	bool m_bCCW;
    float m_fTurns;
    float currSpeed = 1;
    float startPChange = 0;
    float accel = 1;
};



class SpiralTrajectory : public Trajectory {
public:
    SpiralTrajectory(const TrajectoryProperties& properties)
    : Trajectory(properties)
    , m_bReturn(properties.bReturn)
    , m_fTurns(properties.turns)
    , m_fEndPairXY01(properties.endPoint)
    {
        if (*properties.direction == InCCW || *properties.direction == OutCCW){
            m_bCCW = true;
        } else {
            m_bCCW = false;
        }
    }
    
protected:
    void childInit() {
        mStartPointRt = mSourcesInitialPositionRT.getUnchecked(mFilter->getSelectedSrc());
        mEndPointRt   = mFilter->convertXy012Rt(FPoint(m_fEndPairXY01.x, m_fEndPairXY01.y));
        //if start ray is bigger than end ray, we are going in
        m_bGoingIn = (mStartPointRt.x > mEndPointRt.x) ? true : false;
    }
    void childProcess(float duration, float seconds) {
        
        //figure out delta theta, which will go [0, m_fTurns*2*pi] or [0, m_fTurns*4*pi] for return spiral
        //float fDeltaTheta, integralPart;
        
        int iMultiple = (m_bReturn ? 2 : 1);    //in return spiral, delta angle goes twice as fast
        //float fDeltaTheta = iMultiple * fmodf(m_fSpeed * m_fTimeDone / m_fDurationSingleTraj * M_PI, M_PI);
        float fDeltaTheta =  (1 * m_fTimeDone / m_fDurationSingleTraj);
        
        fDeltaTheta = fDeltaTheta * iMultiple * M_PI;
        if(!m_bReturn && (fDeltaTheta> M_PI || fDeltaTheta < 0.0f)){
            fDeltaTheta =iMultiple * fmodf(m_fTimeDone / m_fDurationSingleTraj * M_PI, M_PI);
            if(m_fTimeDone<=0){ m_fTimeDone = m_fDurationSingleTraj;}
        }
        
        if (m_bCCW){
            fDeltaTheta = -fDeltaTheta;
        }

        
        //figure fCurR and fCurT. in this part of the algo, it is assumed that the end point is either the middle (if going in) or the outside (if going out) of the circle
        float fCurT   = mStartPointRt.y + fDeltaTheta * 2 * m_fTurns;
        float fStartR = mStartPointRt.x;
        float fDeltaR = (cosf(fDeltaTheta)+1) * 0.5;   //l here oscillates between 1 @ start and 0 when fDeltaTheta == M_PI), following a cosine. linear is : float fDeltaR = (M_PI - fDeltaTheta) / M_PI;
        float fCurR;
        if (m_bGoingIn) {
            //fCurR simply goes [fStartR, 0] following a cosine curve
            fCurR = fStartR * fDeltaR;
        } else {
            //fCurR goes from fStartR to kRadiusMax following a cosine curve
            //            fCurR = fStartR + (1 - fDeltaR) * (kRadiusMax - fStartR);
            fCurR = fStartR + (1 - fDeltaR) * (mEndPointRt.x - fStartR);
        }
        
        //CARTESIAN TRANSLATION
        FPoint curPointXY01 = mFilter->convertRt2Xy01(fCurR, fCurT);
        //do linear XY translation to end point
        
        float fTranslationFactor = (float)( m_fTimeDone / ((m_fDurationSingleTraj))) ;//* (float)m_fSpeed;
        
        //fDeltaTheta = fDeltaTheta*2*M_PI;//modf(m_fSpeed * m_fTimeDone / m_fDurationSingleTraj, &integralPart) * m_fTurns*2*M_PI;      //the modf makes da cycle back to 0 when it reaches m_fTurn, then we multiply it back by m_fTurn to undo the modification

        //float fTranslationFactor = modf(m_fTimeDone / m_fDurationSingleTraj, &integralPart);    //fTranslationFactor goes [0,1] for every cycle
        if (m_bReturn && fabs(fDeltaTheta) >= M_PI){
            //reverse direction when we reach halfway in return spiral
            fTranslationFactor = 1-fTranslationFactor;
        }
        if (m_bGoingIn){
            curPointXY01.x += fTranslationFactor * (m_fEndPairXY01.x -.5);
            curPointXY01.y -= fTranslationFactor * (m_fEndPairXY01.y-.5);
        } else {
            //here, fTranslationFactor grows linearly, the rest is constants
//            FPoint untranslatedEndOutPointXY01 = mFilter->convertRt2Xy01(kRadiusMax, mStartPointRt.y);
                        FPoint untranslatedEndOutPointXY01 = mFilter->convertRt2Xy01(mEndPointRt.x, mStartPointRt.y);
            curPointXY01.x += fTranslationFactor * (m_fEndPairXY01.x  - untranslatedEndOutPointXY01.x);
            curPointXY01.y -= fTranslationFactor * (m_fEndPairXY01.y - untranslatedEndOutPointXY01.y);
        }
        m_pMover->move(curPointXY01, kTrajectory);
    }
    
private:
    bool m_bCCW, m_bGoingIn, m_bReturn;
    float m_fTurns;
    FPoint m_fEndPairXY01;
    FPoint mStartPointRt, mEndPointRt;
};
// ================================================================================================
class PendulumTrajectory : public Trajectory
{
public:
    PendulumTrajectory(const TrajectoryProperties& properties)
    :Trajectory(properties)
    , m_bCCW(*properties.direction == CCW)
    , m_bRT(properties.bReturn)
    , m_fDeviation(properties.deviation)
    , m_fTotalDampening(properties.dampening)
    {
        mEndPointXy01 = FPoint(properties.endPoint.x, 1-properties.endPoint.y);
    }

protected:
    void childInit() {
        //get XY01 start point
        int src = mFilter->getSelectedSrc();
        mStartPointXy01 = mFilter->convertRt2Xy01(mSourcesInitialPositionRT[src].x, mSourcesInitialPositionRT[src].y);
        //calculate slope, offset and initial length, which represents only the length over the dependent variable
        if (!areSame(mEndPointXy01.x, mStartPointXy01.x)){
            m_bYisDependent = true;
            m_fM = (mEndPointXy01.y - mStartPointXy01.y) / (mEndPointXy01.x - mStartPointXy01.x);
            m_fB = mStartPointXy01.y - m_fM * mStartPointXy01.x;
            m_fInitialLength = mEndPointXy01.x - mStartPointXy01.x;
        } else {
            m_bYisDependent = false;
            m_fM = 0;
            m_fB = mStartPointXy01.x;
            m_fInitialLength = mEndPointXy01.y - mStartPointXy01.y;
        }
    }
    void childProcess(float duration, float seconds) {
        float fCurX01, fCurY01, integralPart;
        int iReturn = m_bRT ? 2:1;
        //calculate current progress and dampening
        
        //float fCurrentProgress  = modf(m_fSpeed * m_fTimeDone / m_fDurationSingleTraj, &integralPart);    //currentProgress goes 0->1 for every cycle
        //float fCurDampening     = m_fTotalDampening * m_fTimeDone / m_fTotalDuration;    //fCurDampening goes 0->m_fTotalDampening during the whole duration of the trajectory
        
        float fCurrentProgress  = m_fTimeDone / m_fDurationSingleTraj;                   //modf(m_fSpeed * m_fTimeDone / m_fDurationSingleTraj, &integralPart);    //currentProgress goes 0->1 for every cycle
        float fCurDampening     = m_fTotalDampening * m_fTimeDone / m_fTotalDuration;    //fCurDampening goes 0->m_fTotalDampening during the whole duration of the trajectory
       
        if(!m_bRT && (fCurrentProgress> 1.0f || fCurrentProgress < 0.0f)){
            fCurrentProgress = modf(m_fTimeDone / m_fDurationSingleTraj, &integralPart) ;
            if(m_fTimeDone<=0){ m_fTimeDone = m_fDurationSingleTraj; }
        }
        
        //if y is dependent, use slope equation, otherwise just go vertically. All calculations in cartesian coordinates.
        if (m_bYisDependent){
            //as we dampen, we move the start point and shorten the length
            float fCurStartX01  = mStartPointXy01.x + fCurDampening * m_fInitialLength /2;
            float fCurLength    = m_fInitialLength    - fCurDampening * m_fInitialLength;
            fCurrentProgress    = fCurLength * (1-cos(fCurrentProgress * iReturn * M_PI)) / 2;
            //use new start point and lenght to calculate current coordinates
            fCurX01 = fCurStartX01 + fCurrentProgress;
            fCurY01 = m_fM * fCurX01 + m_fB;
        } else {
            //as we dampen, we move the start point and shorten the length
            float fCurStartY01  = mStartPointXy01.y + fCurDampening * m_fInitialLength /2;
            float fCurLength    = m_fInitialLength    - fCurDampening * m_fInitialLength;
            fCurrentProgress    = fCurLength * (1-cos(fCurrentProgress * iReturn * M_PI)) / 2;
            //use new start point and lenght to calculate current coordinates
            fCurX01 = mStartPointXy01.x;
            fCurY01 = fCurStartY01 + fCurrentProgress;
        }
        //convert to RT to implement angular deviation
        FPoint pointRT = mFilter->convertXy012Rt(FPoint(fCurX01, fCurY01), false);
        //float deviationAngle = modf(m_fTimeDone / m_fTotalDuration, &integralPart) * 2 * M_PI * m_fDeviation;
        
       // -float deviationAngle = (float)(m_fTimeDone / ((m_fDurationSingleTraj))) ;//* (float)m_fSpeed;
       //- deviationAngle = m_fDeviation*2*M_PI;// m_fDeviation    //modf(m_fSpeed * m_fTimeDone / m_fDurationSingleTraj, &integralPart) * m_fTurns*2*M_PI;      //the modf makes da cycle back to 0 when it reaches m_fTurn, then we multiply it back by m_fTurn to undo the modification
        float deviationAngle = (m_fTimeDone / m_fTotalDuration) * 2 * M_PI * m_fDeviation;
        
        
        //float deviationAngle = modf(m_fTimeDone / m_fTotalDuration, &integralPart) * 2 * M_PI * m_fDeviation;
        if (!m_bCCW) {
            deviationAngle = - deviationAngle;
        }
        
        //convert back to XY01 and move
        FPoint pointXY01 = mFilter->convertRt2Xy01(pointRT.x, pointRT.y + deviationAngle);
        m_pMover->move(pointXY01, kTrajectory);
    }
private:
    bool m_bCCW, m_bRT, m_bYisDependent;
    FPoint mStartPointXy01, mEndPointXy01;
    float m_fM;
    float m_fB;
    float m_fDeviation;
    float m_fTotalDampening;
    float m_fInitialLength;
};

// ==============================================================================
class EllipseTrajectory : public Trajectory
{
public:
	EllipseTrajectory(const TrajectoryProperties& properties)
	: Trajectory(properties)
    , m_bCCW(*properties.direction == CCW)
    , m_fTurns(properties.turns)
    , m_fWidth(properties.width)
    { }
	
protected:
    void childInit(){
        mStartPointRT = mSourcesInitialPositionRT.getUnchecked(mFilter->getSelectedSrc());
    }
    
	void childProcess(float duration, float seconds) {
        //calculate delta theta
        float fDeltaTheta = (float)(m_fTurns * m_fTimeDone / ((m_fDurationSingleTraj))) ;//* (float)m_fSpeed;
        
        fDeltaTheta = fDeltaTheta*2*M_PI;//modf(m_fSpeed * m_fTimeDone / m_fDurationSingleTraj, &integralPart) * m_fTurns*2*M_PI;      //the modf makes da cycle back to 0 when it reaches m_fTurn, then we multiply it back by m_fTurn to undo the modification
        if (!m_bCCW){
            fDeltaTheta = -fDeltaTheta;
        }
        // calculate fCurR and fCurT , using http://www.edmath.org/MATtours/ellipses/ellipses1.07.3.html first part at the top, with a = 1
        float b = m_fWidth;
        float cosDa = cos(fDeltaTheta);
        float b2 = b*b;
        float r2 = b2 / ((b2-1) * cosDa * cosDa + 1);
        float fCurR = mStartPointRT.x * sqrt(r2);
        float fCurT = mStartPointRT.y + fDeltaTheta;
        //move using XY01
        FPoint curPointXy01 = mFilter->convertRt2Xy01(fCurR, fCurT);
        m_pMover->move(curPointXy01, kTrajectory);
    }
	
private:
	bool m_bCCW;
    float m_fTurns;
    float m_fWidth;
    FPoint mStartPointRT;
};

// ==============================================================================
// Mersenne Twister random number generator, this is now included in c++11, see here: http://en.cppreference.com/w/cpp/numeric/random
class MTRand_int32
{
public:
	MTRand_int32() {
		seed(rand());
	}
	uint32_t rand_uint32() {
		if (p == n) gen_state();
		unsigned long x = state[p++];
		x ^= (x >> 11);
		x ^= (x << 7) & 0x9D2C5680;
		x ^= (x << 15) & 0xEFC60000;
		return x ^ (x >> 18);
	}
	void seed(uint32_t s) {
		state[0] = s;
		for (int i = 1; i < n; ++i)
			state[i] = 1812433253 * (state[i - 1] ^ (state[i - 1] >> 30)) + i;

		p = n; // force gen_state() to be called for next random number
	}

private:
	static const int n = 624, m = 397;
	int p;
	unsigned long state[n];
	unsigned long twiddle(uint32_t u, uint32_t v) {
		return (((u & 0x80000000) | (v & 0x7FFFFFFF)) >> 1) ^ ((v & 1) * 0x9908B0DF);
	}
	void gen_state() {
		for (int i = 0; i < (n - m); ++i)
			state[i] = state[i + m] ^ twiddle(state[i], state[i + 1]);
		for (int i = n - m; i < (n - 1); ++i)
			state[i] = state[i + m - n] ^ twiddle(state[i], state[i + 1]);
		state[n - 1] = state[m - 1] ^ twiddle(state[n - 1], state[0]);
		
		p = 0; // reset position
	}
	// make copy constructor and assignment operator unavailable, they don't make sense
	MTRand_int32(const MTRand_int32&);
	void operator=(const MTRand_int32&);
};


class RandomTrajectoryClass : public Trajectory
{
public:
	RandomTrajectoryClass(const TrajectoryProperties& properties)
	: Trajectory(properties)
    , mClock(0)
    {
        switch(*properties.direction){
                case Slow:
                    mSpeed = .02;
                    break;
                case Mid:
                    mSpeed = .06;
                    break;
                case Fast:
                    mSpeed = .1;
                    break;
            default:
                jassert(0);
                return;
        }
    }
	
protected:
    void childProcess(float duration, float seconds){
        if (mFilter->getIndependentMode()){
            for (int iCurSrc = 0; iCurSrc < mFilter->getNumberOfSources(); ++iCurSrc){
                if (fmodf(m_fTimeDone, m_fDurationSingleTraj) < 0.01){
                    mFilter->restoreCurrentLocations(iCurSrc);
                }
                mClock += seconds;
                while(mClock > 0.01){
                    mClock -= 0.01;
                    
                    float rand1 = mRNG.rand_uint32() / (float)0xFFFFFFFF;
                    float rand2 = mRNG.rand_uint32() / (float)0xFFFFFFFF;
                    
                    FPoint p = mFilter->getSourceXY(iCurSrc);
                    
                    p.x +=   (rand1 - 0.5) * mSpeed*2.0f;
                    p.y +=  (rand2 - 0.5) * mSpeed*2.0f;
                    //convert ±radius range to 01 range
                    p.x = (p.x + kRadiusMax) / (2*kRadiusMax);
                    p.y = (p.y + kRadiusMax) / (2*kRadiusMax);
                    
                    mFilter->setSourceXY01(iCurSrc, p);
                }
            }
        } else {
            //reset position at every new cycle
            if (fmodf(m_fTimeDone, m_fDurationSingleTraj) < 0.01){
                mFilter->restoreCurrentLocations(mFilter->getSelectedSrc());
            }
            mClock += seconds;
            while(mClock > 0.01){
                mClock -= 0.01;
                
                float rand1 = mRNG.rand_uint32() / (float)0xFFFFFFFF;
                float rand2 = mRNG.rand_uint32() / (float)0xFFFFFFFF;
                
                FPoint p = mFilter->getSourceXY(mFilter->getSelectedSrc());
                
                p.x += (rand1 - 0.5) * mSpeed*2.0f;
                p.y += (rand2 - 0.5) * mSpeed*2.0f;
                //convert ±radius range to 01 range
                p.x = (p.x + kRadiusMax) / (2*kRadiusMax);
                p.y = (p.y + kRadiusMax) / (2*kRadiusMax);
                m_pMover->move(p, kTrajectory);
            }
        }
    }
    
private:
    MTRand_int32 mRNG;
    float mClock;
    float mSpeed;
};


// ==============================================================================
class TargetTrajectory : public Trajectory
{
public:
	TargetTrajectory(const TrajectoryProperties& properties)
	: Trajectory(properties)
    , mCycle(-1)
    , m_bReturn(properties.bReturn)
    {}
	
protected:
	virtual FPoint destinationForSource(int s, FPoint o) = 0;
    virtual void resetIfRandomTarget(){};

	void childProcess(float duration, float seconds) {

        bool bWriteAutomationForAllSources = mFilter->getIndependentMode();
        
        float p =  m_fTimeDone / m_fDurationSingleTraj;
        int iSelectedSrc = mFilter->getSelectedSrc();
		int cycle = (int)p;

        //reset stuff when we start a new cycle
		if (mCycle != cycle) {
            if (m_bReturn){
                resetIfRandomTarget();
            }
			mCycle = cycle;
			mSourcesOrigins.clearQuick();
			mSourcesDestinations.clearQuick();
            //get destinations for all sources
            for (int i = 0; i < mFilter->getNumberOfSources(); ++i){
                FPoint o = mFilter->getSourceXY(i);
                mSourcesOrigins.add(o);
                mSourcesDestinations.add(destinationForSource(i, o));
            }
		}

        //do the trajectory
		float d = fmodf(p, 1);
        for (int i = 0; i < mFilter->getNumberOfSources(); ++i){
            if (bWriteAutomationForAllSources || iSelectedSrc == i) {
                FPoint a = mSourcesOrigins.getUnchecked(i);
                FPoint b = mSourcesDestinations.getUnchecked(i);
                FPoint p = a + (b - a) * d;
                bool bWriteAutomation = (bWriteAutomationForAllSources || iSelectedSrc == i);
                if (bWriteAutomationForAllSources){
                    mFilter->setSourceXY(i, p, bWriteAutomation);
                } else {
                    //convert ±radius range to 01 range
                    p.x = (p.x + kRadiusMax) / (2*kRadiusMax);
                    p.y = (p.y + kRadiusMax) / (2*kRadiusMax);
                    m_pMover->move(p, kTrajectory);
                }
            }
        }
	}
private:
	Array<FPoint> mSourcesOrigins;
	Array<FPoint> mSourcesDestinations;
	int mCycle;
    bool m_bReturn;
};


// ==============================================================================
class RandomTargetTrajectory : public TargetTrajectory
{
public:
	RandomTargetTrajectory(const TrajectoryProperties& properties)
	: TargetTrajectory(properties) {}
	
protected:
	FPoint destinationForSource(int s, FPoint o) {
		float r1 = mRNG.rand_uint32() / (float)0xFFFFFFFF;
		float r2 = mRNG.rand_uint32() / (float)0xFFFFFFFF;
		float x = r1 * (kRadiusMax*2) - kRadiusMax;
		float y = r2 * (kRadiusMax*2) - kRadiusMax;
		float r = hypotf(x, y);
		if (r > kRadiusMax) {
			float c = kRadiusMax/r;
			x *= c;
			y *= c;
		}
		return FPoint(x,y);
	}
    
    void resetIfRandomTarget(){
        bool bUsingSourceUnique = false;
        int iSrc = bUsingSourceUnique ? mFilter->getSelectedSrc() : -1;
        mFilter->restoreCurrentLocations(iSrc);
    }
	
private:
	MTRand_int32 mRNG;
};

// ==============================================================================
class SymXTargetTrajectory : public TargetTrajectory
{
public:
	SymXTargetTrajectory(const TrajectoryProperties& properties)
	: TargetTrajectory(properties) {}
	
protected:
	FPoint destinationForSource(int s, FPoint o) {
		return FPoint(o.x,-o.y);
	}
};

// ==============================================================================
class SymYTargetTrajectory : public TargetTrajectory
{
public:
	SymYTargetTrajectory(const TrajectoryProperties& properties)
	: TargetTrajectory(properties) {}
	
protected:
	FPoint destinationForSource(int s, FPoint o) {
		return FPoint(-o.x,o.y);
	}
};

// ==============================================================================
class ClosestSpeakerTargetTrajectory : public TargetTrajectory
{
public:
	ClosestSpeakerTargetTrajectory(const TrajectoryProperties& properties)
	: TargetTrajectory(properties) {}
	
protected:
	FPoint destinationForSource(int s, FPoint o) {
		int bestSpeaker = 0;
		float bestDistance = o.getDistanceFrom(mFilter->getSpeakerXY(0));
		
		for (int i = 1; i < mFilter->getNumberOfSpeakers(); i++) {
			float d = o.getDistanceFrom(mFilter->getSpeakerXY(i));
			if (d < bestDistance) {
				bestSpeaker = i;
				bestDistance = d;
			}
		}
		
		return mFilter->getSpeakerXY(bestSpeaker);
	}
};


int Trajectory::NumberOfTrajectories() { return TotalNumberTrajectories-1; }

String Trajectory::GetTrajectoryName(int i) {
    switch(i) {
        case Circle: return "Circle";
        case EllipseTr: return "Ellipse";
        case Spiral: return "Spiral";
        case Pendulum: return "Pendulum";
        case RandomTrajectory: return "Random";
        case RandomTarget: return "Random Target";
        case SymXTarget: return "Sym X Target";
        case SymYTarget: return "Sym Y Target";
        case ClosestSpeakerTarget: return "Closest Speaker Target";
        default:
            jassertfalse;
            return "";
    }
}

Trajectory::Ptr Trajectory::CreateTrajectory(const TrajectoryProperties& properties) {
    
    switch(properties.type) {
        case Circle:                     return new CircleTrajectory                (properties);
        case EllipseTr:                  return new EllipseTrajectory               (properties);
        case Spiral:                     return new SpiralTrajectory                (properties);
        case Pendulum:                   return new PendulumTrajectory              (properties);
        case RandomTrajectory:           return new RandomTrajectoryClass           (properties);
        case RandomTarget:               return new RandomTargetTrajectory          (properties);
        case SymXTarget:                 return new SymXTargetTrajectory            (properties);
        case SymYTarget:                 return new SymYTargetTrajectory            (properties);
        case ClosestSpeakerTarget:       return new ClosestSpeakerTargetTrajectory  (properties);
    }
    jassert(0);
    return nullptr;
}
