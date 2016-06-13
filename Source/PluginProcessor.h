/*
 ==============================================================================
 SpatGRIS: multichannel sound spatialization plug-in.
 
 Copyright (C) 2015  GRIS-UdeM
 
 PluginProcessor.h
 
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

#ifndef PLUGINPROCESSOR_H_INCLUDED
#define PLUGINPROCESSOR_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"
#include <stdint.h>
#include "FirFilter.h"
#include "Trajectories.h"
#include "Routing.h"
#include <memory>
using namespace std;


#ifndef USE_DB_METERS
#define USE_DB_METERS 1
#endif

#ifndef USE_TOUCH_OSC
    #define USE_TOUCH_OSC 1
#endif

#if WIN32
    #define M_PI 3.14159265358979323846264338327950288
#else
    #ifndef USE_LEAP
        #define USE_LEAP 1
    #endif
    #ifndef USE_JOYSTICK
        #define USE_JOYSTICK 1
    #endif
#endif

#if JUCE_MSVC
    size_t strlcpy(char * dst, const char * src, size_t dstsize);
#endif

//==============================================================================

static const int s_iMaxAreas = 3; //this number is used as a multiplicator of mNumberOfSpeakers
static const bool s_bUseNewGui = true;

// x, y, distance
enum sourceParameters{
    kSourceX = 0,
    kSourceY,
    kSourceD,
    kSourceAzimSpan,
    kSourceElevSpan,
    kParamsPerSource
};

// x, y, attenuation, mute
enum speakerParameters{
    kSpeakerX = 0,
    kSpeakerY,
    kSpeakerUnused1,
    kSpeakerM,
    kSpeakerUnused2,
    kParamsPerSpeakers
};

//the JucePlugin values are from the introjucer:   8           * 5                + 16                              * 5 = 120
#define kNonConstantParameters (JucePlugin_MaxNumInputChannels * kParamsPerSource + JucePlugin_MaxNumOutputChannels * kParamsPerSpeakers)

enum constantParameters{
	kSmooth =				0 + kNonConstantParameters,
	kVolumeNear =			1 + kNonConstantParameters,
	kVolumeMid =			2 + kNonConstantParameters,
	kVolumeFar =			3 + kNonConstantParameters,
	kFilterNear =			4 + kNonConstantParameters,
	kFilterMid =			5 + kNonConstantParameters,
	kFilterFar =			6 + kNonConstantParameters,
	kMaxSpanVolume =		7 + kNonConstantParameters,
	kRoutingVolume =		8 + kNonConstantParameters,
	kConstantParameters =	9
};

#define kNumberOfParameters (kConstantParameters + kNonConstantParameters)

//==============================================================================
static const float kSourceRadius = 10;
static const float kSourceDiameter = kSourceRadius * 2;
static const float kSpeakerRadius = 10;
static const float kSpeakerDiameter = kSpeakerRadius * 2;
//==============================================================================
enum
{
    kTrReady,
    kTrWriting
};

enum AllTrajectoryTypes {
    Circle = 1,
    EllipseTr, //Ellipse was clashing with some random windows class...
    Spiral,
    Pendulum,
    RandomTrajectory,
    RandomTarget,
    SymXTarget,
    SymYTarget,
    ClosestSpeakerTarget,
    TotalNumberTrajectories
};


//these have to start at 0 because of backwards-compatibility
enum InputOutputModes {
    i1o2 = 0, i1o4, i1o6, i1o8, i1o16, i2o2, i2o4, i2o6, i2o8, i2o16, i4o4, i4o6, i4o8, i4o16, i6o6, i6o8, i6o16, i8o8, i8o16
};

enum ProcessModes{ kFreeVolumeMode = 0, kPanVolumeMode, kPanSpanMode, kOscSpatMode, kNumberOfModes };

enum RoutingModes{
    kNormalRouting = 0
    ,kInternalWrite
    ,kInternalRead12
    ,kInternalRead34
    ,kInternalRead56
    ,kInternalRead78
    ,kInternalRead910
    ,kInternalRead1112
    ,kInternalRead1213
    ,kInternalRead1314
    ,kInternalRead1516
};

//==============================================================================
// these must be normalized/denormalized for processing
static const float kSourceMinDistance = 2.5 * 0.5;
static const float kSourceMaxDistance = 20 * 0.5;
static const float kSourceDefaultDistance = 5 * 0.5;

static const float kSpeakerDefaultAttenuation = 0;

static const float kSmoothMin = 1;
static const float kSmoothMax = 200;
static const float kSmoothDefault = 50;

static const float kVolumeNearMin = -10;
static const float kVolumeNearMax = 30;
static const float kVolumeNearDefault = 6;

static const float kVolumeMidMin = -30;
static const float kVolumeMidMax = 10;
static const float kVolumeMidDefault = 0;

static const float kVolumeFarMin = -120;
static const float kVolumeFarMax = 0;
static const float kVolumeFarDefault = -36;

static const float kMaxDistance = 2000;

static const float kFilterNearMin = kMaxDistance;
static const float kFilterNearMax = 0;
static const float kFilterNearDefault = 0;

static const float kFilterMidMin = kMaxDistance;
static const float kFilterMidMax = 0;
static const float kFilterMidDefault = kMaxDistance / 10;

static const float kFilterFarMin = kMaxDistance;
static const float kFilterFarMax = 0;
static const float kFilterFarDefault = kMaxDistance / 2;

static const float kMaxSpanVolumeMin = 0;
static const float kMaxSpanVolumeMax = 20;
static const float kMaxSpanVolumeDefault = 0;

static const float kRoutingVolumeMin = -120;
static const float kRoutingVolumeMax = 6;
static const float kRoutingVolumeDefault = 0;

static const float kSpanMin = 0;
static const float kSpanMax = 2;
static const float kSpanDefault = 0;

static const float kRadiusMax = 2;
static const float kHalfCircle = M_PI;
static const float kQuarterCircle = M_PI / 2;
static const float kThetaMax = M_PI * 2;
static const float kThetaLockRadius = 0.05;
static const float kThetaLockRampRadius = 0.025;
static const float kSourceDefaultRadius = 1.f;

static const int    kMargin = 10;
static const int    kCenterColumnWidth = 180;
static const int    kDefaultFieldSize = 500;
static const int    kRightColumnWidth = 340;
static const int    kDefaultWidth  = kMargin + kDefaultFieldSize + kMargin + kCenterColumnWidth + kMargin + kRightColumnWidth + kMargin;
static const int    kDefaultHeight = kMargin + kDefaultFieldSize + kMargin;

//==============================================================================
static inline float normalize(float min, float max, float value)
{
	return (value - min) / (max - min);
}
static inline float denormalize(float min, float max, float value)
{
	return min + value * (max - min);
}
static inline float dbToLinear(float db)
{
	return powf(10.f, (db) * 0.05f);
}
static inline float linearToDb(float linear)
{
	return log10f(linear) * 20.f;
}

static bool areSame(double a, double b)
{
    return fabs(a - b) < .0001;
}

typedef Point<float> FPoint;

typedef struct
{
	int i;
	float a;
} IndexedAngle;
int IndexedAngleCompare(const void *a, const void *b);


class OscSpatThread;
class SourceUpdateThread;
class SourceMover;

//==============================================================================
class SpatGrisAudioProcessor : public AudioProcessor
{
public:
    //==============================================================================
    SpatGrisAudioProcessor();
    ~SpatGrisAudioProcessor();

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock);
    void releaseResources();

    void processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages);
	void processBlockBypassed (AudioSampleBuffer& buffer, MidiBuffer& midiMessages);

    //==============================================================================
    AudioProcessorEditor* createEditor();
    bool hasEditor() const;

    //==============================================================================
    const String getName() const;

    int getNumParameters();

    float getParameter (int index);
    void setParameter (int index, float newValue);
	void setParameterNotifyingHost (int index, float newValue);

    const String getParameterName (int index);
    const String getParameterText (int index);

    const String getInputChannelName (int channelIndex) const;
    const String getOutputChannelName (int channelIndex) const;
    bool isInputChannelStereoPair (int index) const;
    bool isOutputChannelStereoPair (int index) const;

    bool acceptsMidi() const;
    bool producesMidi() const;
    bool silenceInProducesSilenceOut() const;
    double getTailLengthSeconds() const;

    //==============================================================================
    int getNumPrograms();
    int getCurrentProgram();
    void setCurrentProgram (int index);
    const String getProgramName (int index);
    void changeProgramName (int index, const String& newName);

    //==============================================================================
    void getStateInformation (MemoryBlock& destData);
    void setStateInformation (const void* data, int sizeInBytes);

    //==============================================================================

	bool getApplyFilter() const { return mApplyFilter; }
	void setApplyFilter(bool s) { mApplyFilter = s; }
	
	bool getShowGridLines() const { return mShowGridLines; }
	void setShowGridLines(bool s) { mShowGridLines = s; }
    
    bool getIndependentMode() const { return mTrSeparateAutomationMode; }
    void setIndependentMode(bool b) { mTrSeparateAutomationMode = b; }
    
	int getMovementMode() const { return m_iMovementMode; }

    void setMovementMode(int s) {
        m_iMovementMode = s;
        startOrStopSourceUpdateThread();
    }
	
	bool getLinkDistance() const { return mLinkSurfaceOrPan; }
	void setLinkDistance(bool s) { mLinkSurfaceOrPan = s; }

    bool getLinkAzimSpan() const { return mLinkAzimSpan; }
    void setLinkAzimSpan(bool s) { mLinkAzimSpan = s; }

    bool getLinkElevSpan() const { return mLinkElevSpan; }
    void setLinkElevSpan(bool s) { mLinkElevSpan = s; }
    
	int getProcessMode() const { return mProcessMode; }
    void setProcessMode(int s) ;
    
    void setJustSelectedEndPoint(bool selected){ m_bJustSelectedEndPoint = selected;}
    bool justSelectedEndPoint(){ return m_bJustSelectedEndPoint;}
    
	int getRoutingMode() const { return mRoutingMode; }
	void setRoutingMode(int s) {
        mRoutingMode = s;
        if (mRoutingMode == kInternalWrite){
            updateRoutingTempAudioBuffer();
        }
    }
	void updateRoutingTempAudioBuffer();
    
    int getGuiWidth() const{return mGuiWidth;}
    int getGuiHeight() const{return mGuiHeight;}
    
    void setGuiWidth(int w) {mGuiWidth = w;}
    void setGuiHeight(int h) {mGuiHeight = h;}

    int getInputOutputMode() const {return mInputOutputMode+1;}
    void setInputOutputMode(int i);
    void updateInputOutputMode();
    
    int getSrcPlacementMode() const {return mSrcPlacementMode;}
    void setSrcPlacementMode(int i);

    int getSpPlacementMode() const {return mSpPlacementMode;}
    void setSpPlacementMode(int i);

    int getTrType() const {return m_iTrType+1;}
    void setTrType(int i){m_iTrType = i-1;}
    
    int getTrDirection() const {return m_iTrDirection;}
    void setTrDirection(int i){m_iTrDirection = i;}
    
    int getTrReturn() const {return m_iTrReturn;}
    void setTrReturn(int i){m_iTrReturn = i;}

    int getTrSrcSelect() const {return m_iTrSrcSelect;}
    void setTrSrcSelect(int i){m_iTrSrcSelect = i;}

    float getTrDuration() const {return m_fTrDuration;}
    void setTrDuration(float i){m_fTrDuration = i;}
    
    int getTrUnits() const {return m_iTrUnits + 1;}
    void setTrUnits(int i){m_iTrUnits = i - 1;}

    float getTrRepeats() const {return m_fTrRepeats;}
    void setTrRepeats(float f){m_fTrRepeats = f;}

    float getTrDampening() const {return m_fTrDampening;}
    void setTrDampening(float f){m_fTrDampening = f;}
    
    int getOscSpat1stSrcId() const{return m_iOscSpat1stSrcId;}
    void setOscSpat1stSrcId(int i){m_iOscSpat1stSrcId = i;}
    
    int getOscSpatPort() const{return m_iOscSpatPort;}
    void setOscSpatPort(int i){m_iOscSpatPort = i;}

    float getTrDeviation() const {return m_fTrDeviation;}
    void setTrDeviation(float i){m_fTrDeviation = i;}

    float getTrEllipseWidth() const {return m_fTrEllipseWidth;}
    void setTrEllipseWidth(float i){m_fTrEllipseWidth = i;}
    
    float getTrTurns() const {return m_fTrTurns;}
    void setTrTurns(float i){m_fTrTurns = i;}
    
    int getTrState() const {return mTrState;}
    void setTrState(int tr) {mTrState = tr;}
    
	int getGuiTab() const { return mGuiTab; }
	void setGuiTab(int s) {
        mGuiTab = s;
        ++mHostChangedParameter;
    }

    void sendOscSpatValues();
    void connectOscSpat();
    void disconnectOscSpat();
    
    int getIsJoystickEnabled() const { return mJoystickEnabled; }
    void setIsJoystickEnabled(int s) { mJoystickEnabled = s; }
    
    int getOscJoystickSource() const { return mOscJoystickSource; }
    void setOscJoystickSource(int s) { mOscJoystickSource = s; }
    
    int getIsLeapEnabled() const { return mLeapEnabled; }
    void setIsLeapEnabled(int s) { mLeapEnabled = s; }
	
	int getOscLeapSource() const { return mOscLeapSource; }
	void setOscLeapSource(int s) { mOscLeapSource = s; }
	
	int getOscReceiveEnabled() const { return mOscReceiveEnabled; }
	void setOscReceiveEnabled(int s) { mOscReceiveEnabled = s; }
	
	int getOscReceivePort() const { return mOscReceivePort; }
	void setOscReceivePort(int s) { mOscReceivePort = s; }
	
	int getOscSendEnabled() const { return mOscSendEnabled; }
	void setOscSendEnabled(int s) { mOscSendEnabled = s; }
	
	int getOscSendPort() const { return mOscSendPort; }
	void setOscSendPort(int s) { mOscSendPort = s; }
    
	const String getOscSendIp() const { return mOscSendIp; }
	void setOscSendIp(String s) { mOscSendIp = s;}
	
	float getLevel(int index) const {
#if USE_DB_METERS
        return mLevels.getUnchecked(index);
#else
        return -1.f;
#endif
    }
	void setCalculateLevels(bool c);
	
	bool getIsAllowInputOutputModeSelection(){
		return m_bAllowInputOutputModeSelection;
	}

	uint64_t getHostChangedParameter() { return mHostChangedParameter; }
	uint64_t getHostChangedProperty() { return mHostChangedProperty; }
	uint64_t getProcessCounter() { return mProcessCounter; }
	
    int getNumberOfSources() const { return mNumberOfSources; }
    int getParamForSourceX(int index) const { return kSourceX + index * kParamsPerSource; }
    int getParamForSourceY(int index) const { return kSourceY + index * kParamsPerSource; }
    int getParamForSourceD(int index) const { return kSourceD + index * kParamsPerSource; }
    int getParamForSourceAzimSpan(int index) const { return kSourceAzimSpan + index * kParamsPerSource; }
    int getParamForSourceElevSpan(int index) const { return kSourceElevSpan + index * kParamsPerSource; }
    
    float getSourceX(int index) const { return mParameters.getUnchecked(kSourceX + index * kParamsPerSource); }
    float getSourceY(int index) const { return mParameters.getUnchecked(kSourceY + index * kParamsPerSource); }
    float getSourceD(int index) const { return mParameters.getUnchecked(kSourceD + index * kParamsPerSource); }
    float getDenormedSourceD(int index) const { return denormalize(kSourceMinDistance, kSourceMaxDistance, getSourceD(index)); }
    float getSourceAzimSpan01(int index) const { return mParameters.getUnchecked(kSourceAzimSpan + index * kParamsPerSource); }
    float getSourceElevSpan01(int index) const { return mParameters.getUnchecked(kSourceElevSpan + index * kParamsPerSource); }
    
    int getNumberOfSpeakers() const { return mNumberOfSpeakers; }
    
    inline int getParamForSpeakerX(int index) const { return kSpeakerX + JucePlugin_MaxNumInputChannels * kParamsPerSource + index * kParamsPerSpeakers; }
    inline int getParamForSpeakerY(int index) const { return kSpeakerY + JucePlugin_MaxNumInputChannels * kParamsPerSource + index * kParamsPerSpeakers; }
    inline int getParamForSpeakerM(int index) const { return kSpeakerM + JucePlugin_MaxNumInputChannels * kParamsPerSource + index * kParamsPerSpeakers; }
    
    float getSpeakerX(int index) const { return mParameters.getUnchecked(getParamForSpeakerX(index)); }
    float getSpeakerY(int index) const { return mParameters.getUnchecked(getParamForSpeakerY(index)); }
    float getSpeakerM(int index) const { return mParameters.getUnchecked(getParamForSpeakerM(index)); }
    
	// convenience functions for gui:
	//01 here means that the output is normalized to [0,1]
	FPoint getSourceXY01(int i)	{
		float x = getSourceX(i);
		float y = getSourceY(i);
		return FPoint(x, y);
	}

	// these return in the interval [-kRadiusMax .. kRadiusMax]
	FPoint getSourceXY(int i) {
		float x = getSourceX(i) * (2*kRadiusMax) - kRadiusMax;
		float y = getSourceY(i) * (2*kRadiusMax) - kRadiusMax;
		return FPoint(x, y);
	}

	FPoint getSpeakerXY(int i) {
		float x = getSpeakerX(i) * (2*kRadiusMax) - kRadiusMax;
		float y = getSpeakerY(i) * (2*kRadiusMax) - kRadiusMax;
		if (mProcessMode != kFreeVolumeMode) {
			// force radius to 1
			float r = hypotf(x, y);
			if (r == 0) return FPoint(1, 0);
			x /= r;
			y /= r;
		}
		return FPoint(x, y);
	}

	FPoint getSpeakerRT(int i) {
		FPoint p = getSpeakerXY(i);
		float r = hypotf(p.x, p.y);
		float t = atan2f(p.y, p.x);
		if (t < 0) t += kThetaMax;
		return FPoint(r, t);
	}

	FPoint getSourceRT(int i) {
		FPoint p = getSourceXY(i);
		float r = hypotf(p.x, p.y);
		float t = atan2f(p.y, p.x);
		if (t < 0) t += kThetaMax;
		return FPoint(r, t);
	}
    
    void setFieldWidth(float fieldWidth){ m_fFieldWidth = fieldWidth;}
    
    FPoint getSourceAzimElev(int i) {
        //get source position in [-kRadiusMax, kRadiusMax]
        FPoint pXY = getSourceXY(i);

        //calculate azim in range [0,1], and negate it because zirkonium wants -1 on right side
        float fAzim = -atan2f(pXY.x, pXY.y)/M_PI;
        
        //calculate xy distance from origin, and clamp it to 2 (ie ignore outside of circle)
        float hypo = hypotf(pXY.x, pXY.y);
        if (hypo > 2){
            hypo = 2;
        }
        float fElev = acosf(hypo/kRadiusMax);   //fElev is elevation in radian, [0,pi/2)
        fElev /= (M_PI/2);                        //making range [0,1]
        fElev /= 2.;                            //making range [0,.5] because that's what the zirkonium wants

        return FPoint(fAzim, fElev);
    }
    
    FPoint convertRt2Xy(FPoint p) {
        float x = p.x * cosf(p.y);
        float y = p.x * sinf(p.y);
        return FPoint(x, y);
    }
    
    //01 here means that the output is normalized to [0,1]
    FPoint convertRt2Xy01(float r, float t) {
        float x = r * cosf(t);
        float y = r * sinf(t);
        return FPoint((x + kRadiusMax)/(kRadiusMax*2), (y + kRadiusMax)/(kRadiusMax*2));
    }
	
	FPoint convertXy2Rt01(FPoint p) {
		float vx = p.x;
		float vy = p.y;
		float r = sqrtf(vx*vx + vy*vy) / kRadiusMax;
		if (r > 1) r = 1;
		float t = atan2f(vy, vx);
		if (t < 0) t += kThetaMax;
		t /= kThetaMax;
		return FPoint(r, t);
	}

    FPoint convertXy012Rt01(FPoint p) {
        return convertXy2Rt01(FPoint(p.x * (kRadiusMax*2) - kRadiusMax, p.y * (kRadiusMax*2) - kRadiusMax));
    }
    
    FPoint convertXy2Rt(FPoint p, bool p_bLimitR = true) {
        float vx = p.x;
        float vy = p.y;
        float r = sqrtf(vx*vx + vy*vy);
        if (p_bLimitR && r > 1) r = 1;
        float t = atan2f(vy, vx);
        if (t < 0) t += kThetaMax;
        return FPoint(r, t);
    }
    
    FPoint convertXy012Rt(FPoint p, bool p_bLimitR = true) {
        return convertXy2Rt(FPoint(p.x * (kRadiusMax*2) - kRadiusMax, p.y * (kRadiusMax*2) - kRadiusMax), p_bLimitR);
    }

	FPoint clampRadius01(FPoint p) {
		float dx = p.x - 0.5f;
		float dy = p.y - 0.5f;
		float r = hypotf(dx, dy);
		if (r > 0.5f)
		{
			float c = 0.5f / r;
			dx *= c;
			dy *= c;
			p.x = dx + 0.5f;
			p.y = dy + 0.5f;
		}
		return p;
	}

	void setSourceXY01(int i, FPoint p, bool p_bNotifyHost = true) {
		p = clampRadius01(p);
        if (p_bNotifyHost){
            setParameterNotifyingHost(getParamForSourceX(i), p.x);
            setParameterNotifyingHost(getParamForSourceY(i), p.y);
        } else {
            setParameter(getParamForSourceX(i), p.x);
            setParameter(getParamForSourceY(i), p.y);
        }
	}

	void setSourceXY(int i, FPoint p, bool p_bNotifyHost = true) {
		float r = hypotf(p.x, p.y);
		if (r > kRadiusMax)
		{
			float c = kRadiusMax / r;
			p.x *= c;
			p.y *= c;
		}
		p.x = (p.x + kRadiusMax) / (kRadiusMax*2);
		p.y = (p.y + kRadiusMax) / (kRadiusMax*2);
        if (p_bNotifyHost){
            setParameterNotifyingHost(getParamForSourceX(i), p.x);
            setParameterNotifyingHost(getParamForSourceY(i), p.y);
        } else {
            setParameter(getParamForSourceX(i), p.x);
            setParameter(getParamForSourceY(i), p.y);
        }
	}

	void setSourceRT(int i, FPoint p, bool p_bNotifyHost = true) {
		float x = p.x * cosf(p.y);
		float y = p.x * sinf(p.y);
		setSourceXY(i, FPoint(x, y), p_bNotifyHost);
	}
 
	void setSpeakerXY01(int i, FPoint p) {
		p = clampRadius01(p);
        setParameter(getParamForSpeakerX(i), p.x);
        setParameter(getParamForSpeakerY(i), p.y);
	}

	void setSpeakerRT(int i, FPoint p) {
		float x = p.x * cosf(p.y);
		float y = p.x * sinf(p.y);
        setParameter(getParamForSpeakerX(i), (x + kRadiusMax) / (kRadiusMax*2));
        setParameter(getParamForSpeakerY(i), (y + kRadiusMax) / (kRadiusMax*2));
    }
	
	Trajectory::Ptr getTrajectory() { return mTrajectory; }
	void setTrajectory(Trajectory::Ptr t) { mTrajectory = t; }
    
    bool getIsSourcesChanged(){ return mIsNumberSourcesChanged;}
    bool getIsSpeakersChanged(){ return mIsNumberSpeakersChanged;}
    
    void setIsSourcesChanged(bool pIsNumberSourcesChanged){ mIsNumberSourcesChanged = pIsNumberSourcesChanged;}
    void setIsSpeakersChanged(bool pIsNumberSpeakersChanged){ mIsNumberSpeakersChanged = pIsNumberSpeakersChanged;}
    
    void setIsRecordingAutomation(bool b)   {
        m_bIsRecordingAutomation = b;
        startOrStopSourceUpdateThread();
    }
    bool getIsRecordingAutomation()         { return m_bIsRecordingAutomation;  }

    void setSourceLocationChanged(int i)    {  m_iSourceLocationChanged = i;    }
    int  getSourceLocationChanged()         { return m_iSourceLocationChanged;  }

    int getSrcSelected() const {return mSrcSelected;}
    int getSpSelected() const  {return mSpSelected;}
    
    void setSrcSelected(int p_i){
    	mSrcSelected = p_i;
        mHostChangedParameter++;
	}

	void setSpSelected(int p_i){ mSpSelected = p_i; }

    void setPreventSourceLocationUpdate(bool b){ m_bPreventSourceLocationUpdate = b; }
    
    void setIsSettingEndPoint(bool isSetting){ m_bIsSettingEndPoint = isSetting; }
    bool isSettingEndPoint(){ return m_bIsSettingEndPoint; }
    
    std::pair<float, float> getEndLocationXY01(){ return m_fEndLocationXY01; }
    void setEndLocationXY01(std::pair<float, float> pair){ m_fEndLocationXY01 = pair; }
    
    void storeCurrentLocations();
    void restoreCurrentLocations(int p_iLocToRestore = -1);
	void reset();
    
    void updateSpeakerLocation(bool p_bAlternate, bool p_bStartAtTop, bool p_bClockwise);
    
    FPoint  getOldSrcLocRT(int id){return mOldSrcLocRT[id];}
    void    setOldSrcLocRT(int id, FPoint pointRT){
        mOldSrcLocRT[id] = pointRT;
    }
    bool isPlaying(){ return m_bIsPlaying;}
    void threadUpdateNonSelectedSourcePositions();
    void startOrStopSourceUpdateThread();
	
private:

	bool m_bAllowInputOutputModeSelection;
	Trajectory::Ptr mTrajectory;

    FPoint mOldSrcLocRT[JucePlugin_MaxNumInputChannels];

    
	Array<float> mParameters;
	
	int mCalculateLevels;
#if USE_DB_METERS
	Array<float> mLevels;
#endif
	bool mApplyFilter;
	bool mLinkSurfaceOrPan;
    bool mLinkAzimSpan;
    bool mLinkElevSpan;
	int m_iMovementMode;
	bool mShowGridLines;
    bool mTrSeparateAutomationMode;
    int mGuiWidth;
    int mGuiHeight;
    int mInputOutputMode;
    int mSrcPlacementMode;
    int mSrcSelected;
    int mSpPlacementMode;
    int mSpSelected;
    int m_iTrType;
    
    int m_iTrDirection;
    int m_iTrReturn;
    
    int     m_iTrSrcSelect;
    float   m_fTrDuration;
    int     m_iTrUnits; //0 = beats, 1 = seconds
    float   m_fTrRepeats;
    float   m_fTrDampening;
    float   m_fTrTurns;
    float   m_fTrDeviation;
    float   m_fTrEllipseWidth;
    int     mTrState;
    float   m_iOscSpat1stSrcId;
    int     m_iOscSpatPort;
    String  m_sOscIpAddress;
    
    int mGuiTab;
    int mJoystickEnabled;
    int mOscJoystickSource;
    int mLeapEnabled;
	int mOscLeapSource;
	int mOscReceiveEnabled;
	int mOscReceivePort;
	int mOscSendEnabled;
	int mOscSendPort;
    String mOscSendIp;

	uint64_t mHostChangedParameter;
	uint64_t mHostChangedProperty;
	uint64_t mProcessCounter;
	
	int mProcessMode;
	int mRoutingMode;
	AudioSampleBuffer mRoutingTempAudioBuffer;
    
    bool mIsNumberSourcesChanged;
    bool mIsNumberSpeakersChanged;
	
	bool mSmoothedParametersInited;
	Array<float> mSmoothedParameters;
	JUCE_COMPILER_WARNING("these really should be vectors")
	Array<float> mLockedThetas;
    Array<float> mPrevRs;
    Array<float> mPrevTs;
    
    vector<float> allThetas;
    bool bThetasPrinted = false;
	
	#define kChunkSize (256)
	struct IOBuf { float b[kChunkSize]; };
	Array<IOBuf> mInputsCopy;
	Array<IOBuf> mSmoothedParametersRamps;
    
    float mBufferSrcLocX[JucePlugin_MaxNumInputChannels];
    float mBufferSrcLocY[JucePlugin_MaxNumInputChannels];
    float mBufferSrcLocD[JucePlugin_MaxNumInputChannels];
    float mBufferSrcLocAS[JucePlugin_MaxNumInputChannels];
    float mBufferSrcLocES[JucePlugin_MaxNumInputChannels];
    
    float mBufferSpLocX[JucePlugin_MaxNumOutputChannels];
    float mBufferSpLocY[JucePlugin_MaxNumOutputChannels];
    float mBufferSpLocM[JucePlugin_MaxNumOutputChannels];
    
    void setNumberOfSources(int p_iNewNumberOfSources, bool bUseDefaultValues);
    void setNumberOfSpeakers(int p_iNewNumberOfSpeakers, bool bUseDefaultValues);
	
	void findLeftAndRightSpeakers(float t, float *params, int &left, int &right, float &dLeft, float &dRight, int skip = -1);
    
	inline void addToOutput(float s, float **outputs, int o, int f);
	void ProcessData(float **inputs, float **outputs, float *params, float sampleRate, unsigned int frames);
	void ProcessDataFreeVolumeMode(float **inputs, float **outputs, float *params, float sampleRate, unsigned int frames);
	void ProcessDataPanVolumeMode(float **inputs, float **outputs, float *params, float sampleRate, unsigned int frames);
	void ProcessDataPanSpanMode(float **inputs, float **outputs, float *params, float sampleRate, unsigned int frames);
    int mNumberOfSources;
    int mNumberOfSpeakers;
    std::vector<FirFilter> mFilters;
    bool m_bIsRecordingAutomation;
    int m_iSourceLocationChanged;
    
    bool m_bPreventSourceLocationUpdate;
    bool m_bIsSettingEndPoint;
    std::pair <float, float> m_fEndLocationXY01;
    bool m_bJustSelectedEndPoint;

    OSCSender mOscSpatSender;
    bool m_bOscSpatSenderIsConnected;
    OscSpatThread*      m_pOscSpatThread;
    SourceUpdateThread* m_pSourceUpdateThread;
    OwnedArray<Thread>  m_OwnedThreads;
    float m_fFieldWidth;

	unique_ptr<SourceMover> m_pMover;
    bool m_bIsPlaying;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpatGrisAudioProcessor)
};



#endif  // PLUGINPROCESSOR_H_INCLUDED
