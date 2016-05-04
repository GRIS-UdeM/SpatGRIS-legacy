/*
 ==============================================================================
 SpatGRIS: multichannel sound spatialization plug-in.
 
 Copyright (C) 2015  GRIS-UdeM
 
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

//static const float kLevelAttackMin = 0.01;
//static const float kLevelAttackMax = 100;
static const float kLevelAttackDefault = 0.05f;

//static const float kLevelReleaseMin = 1;
//static const float kLevelReleaseMax = 500;
static const float kLevelReleaseDefault = 100;

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>

#if JUCE_MSVC
#include <sstream>
#include <string>
#include <windows.h>

template<class T>
string toString(const T &value) {
    ostringstream os;
    os << value;
    return os.str();
}

// from https://github.com/objectx/strlcpy/blob/master/strlcpy/strlcpy.c
size_t strlcpy(char * dst, const char * src, size_t dstsize)
{
    if (dst == 0 || dstsize == 0) {
        return 0;
    }
    if (src == 0) {
        dst [0] = 0;
        return 0;
    } else {
        size_t	src_len = strlen (src);
        size_t	dstlimit = dstsize - 1;
        
        if (dstlimit < src_len) {
            src_len = dstlimit;
        }
        memcpy (dst, src, src_len);
        dst [src_len] = 0;
        return src_len;
    }
}

#include <sstream>
#include <string>
#include <windows.h>

	template<class T>
	string toString(const T &value) {
		ostringstream os;
		os << value;
		return os.str();
	}
#endif

JUCE_COMPILER_WARNING("threads should probably all be in their own file?")
//==================================== SourceUpdateThread ===================================================================
class SourceUpdateThread : public Thread
{
public:
    SourceUpdateThread(SpatGrisAudioProcessor* p_pProcessor)
    : Thread ("SourceUpdateThread")
    ,m_iInterval(50)
    ,m_pProcessor(p_pProcessor)
    { }
    
    ~SourceUpdateThread() {
        stopThread (500);
    }
    
    void run() override {
        while (! threadShouldExit()) {
            m_pProcessor->updateNonSelectedSourcePositions();
            wait (m_iInterval);
        }
    }
    
private:
    int m_iInterval;
    SpatGrisAudioProcessor* m_pProcessor;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SourceUpdateThread)
};

//====================================== OscSpatThread ========================================
class OscSpatThread : public Thread {
public:
    OscSpatThread(SpatGrisAudioProcessor* p_pProcessor)
    : Thread ("OscSpatThread")
    , m_iInterval(25)
    , m_pProcessor(p_pProcessor) {

    }
    
    ~OscSpatThread() {
        // allow the thread .5 second to stop cleanly - should be plenty of time.
        stopThread (500);
    }
    
    void run() override {
        while (!threadShouldExit()) {
            wait (m_iInterval);
            
            // because this is a background thread, we mustn't do any UI work without first grabbing a MessageManagerLock
            const MessageManagerLock mml (Thread::getCurrentThread());
            
            if (!mml.lockWasGained())  // if something is trying to kill this job, the lock will fail, in which case we'd better return
                return;
            
            // now we've got the UI thread locked, we can mess about with the components
            m_pProcessor->sendOscSpatValues();
        }
    }
private:
    int m_iInterval;
    SpatGrisAudioProcessor* m_pProcessor;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OscSpatThread)
};

//==============================================================================
int IndexedAngleCompare(const void *a, const void *b)
{
	IndexedAngle *ia = (IndexedAngle*)a;
	IndexedAngle *ib = (IndexedAngle*)b;
	return (ia->a < ib->a) ? -1 : ((ia->a > ib->a) ? 1 : 0);
}

//==============================================================================
SpatGrisAudioProcessor::SpatGrisAudioProcessor()
:mFilters()
,m_bIsRecordingAutomation(false)
,m_iSourceLocationChanged(-1)
,m_bPreventSourceLocationUpdate(false)
{
//
//#if USE_LEAP
//	char winDir[MAX_PATH];//will hold path of above dll
//	GetCurrentDirectory(sizeof(winDir), winDir);//dll is in same dir as exe
//	strcat(winDir, "\\Leap.dll");//concentrate dll name with path
//	HINSTANCE DLL = LoadLibrary(winDir);//load example dll
//#endif
    
    m_pOscSpatThread        = new OscSpatThread(this);
    m_pSourceUpdateThread   = new SourceUpdateThread(this);
    m_OwnedThreads.add(m_pOscSpatThread);
    m_OwnedThreads.add(m_pSourceUpdateThread);
    
    //SET PARAMETERS
	mParameters.ensureStorageAllocated(kNumberOfParameters);
    for (int i = 0; i < kNumberOfParameters; i++){
        mParameters.add(0);
    }
    
	mParameters.set(kSmooth, normalize(kSmoothMin, kSmoothMax, kSmoothDefault));
	mParameters.set(kVolumeNear, normalize(kVolumeNearMin, kVolumeNearMax, kVolumeNearDefault));
	mParameters.set(kVolumeMid, normalize(kVolumeMidMin, kVolumeMidMax, kVolumeMidDefault));
	mParameters.set(kVolumeFar, normalize(kVolumeFarMin, kVolumeFarMax, kVolumeFarDefault));
	mParameters.set(kFilterNear, normalize(kFilterNearMin, kFilterNearMax, kFilterNearDefault));
	mParameters.set(kFilterMid, normalize(kFilterMidMin, kFilterMidMax, kFilterMidDefault));
	mParameters.set(kFilterFar, normalize(kFilterFarMin, kFilterFarMax, kFilterFarDefault));
	mParameters.set(kMaxSpanVolume, normalize(kMaxSpanVolumeMin, kMaxSpanVolumeMax, kMaxSpanVolumeDefault));
	mParameters.set(kRoutingVolume, normalize(kRoutingVolumeMin, kRoutingVolumeMax, kRoutingVolumeDefault));

	mSmoothedParametersInited = false;
	mSmoothedParameters.ensureStorageAllocated(kNumberOfParameters);
    
	for (int i = 0; i < kNumberOfParameters; i++) 
		mSmoothedParameters.add(0);

    
    mNumberOfSources = -1;
    mNumberOfSpeakers = -1;

	PluginHostType host;
	bool bIsWindows;

#if WIN32
	bIsWindows = true;
#else
	bIsWindows = false;
#endif
    
  //	if (host.isReaper() || host.isAbletonLive() || (bIsWindows && host.isDigitalPerformer())){
    if (host.isLogic() || host.isReaper() || host.isAbletonLive() || host.isDigitalPerformer()){
		m_bAllowInputOutputModeSelection = true;
	} else {
		m_bAllowInputOutputModeSelection = false;
	}
    
    //SET SOURCES AND SPEAKERS
    int iSources = getTotalNumInputChannels();
    int iSpeakers = getTotalNumOutputChannels();
    setNumberOfSources(iSources, true);
    setNumberOfSpeakers(iSpeakers, true);
    
	mCalculateLevels = 0;
	mApplyFilter = true;
	mLinkSurfaceOrPan = false;
    mLinkAzimSpan = false;
    mLinkElevSpan = false;
	setMovementMode(0);
    
    
	mShowGridLines = false;
	mTrSeparateAutomationMode = false;
    mIsNumberSourcesChanged = false;
    mIsNumberSpeakersChanged = false;
    mGuiWidth = kDefaultWidth,
    mGuiHeight = kDefaultHeight,
	mGuiTab = 0;
	mHostChangedParameter = 0;
	mHostChangedProperty = 0;
	mProcessCounter = 0;
	//mLastTimeInSamples = -1;
	mProcessMode = kPanVolumeMode;
	mRoutingMode = 0;
    //version 9
    updateInputOutputMode();
    mSrcPlacementMode = 1;
    mSrcSelected = 0;
    
    mSpPlacementMode = 1;
    mSpSelected = 1;
    m_iTrType = 0;
    m_iTrDirection = 0;

    m_iTrReturn = 0;
    m_iTrSrcSelect = -1;//0;
    m_fTrDuration = 5.f;
    m_iTrUnits = 1;     //0 = beats, 1 = seconds
    m_fTrRepeats = 1.f;
    m_fTrDampening = 0.f;
    m_iOscSpat1stSrcId = 1;
    m_iOscSpatPort = 18032;
    m_sOscIpAddress = "127.0.0.1";
    m_fTrTurns = 1.f;
    m_fTrDeviation = 0.f;
    m_fEndLocationXY01 = make_pair(.5, .5);
    m_bIsSettingEndPoint = false;
    m_bJustSelectedEndPoint = false;
    
	mOscLeapSource = 0;
	mOscReceiveEnabled = 0;
	mOscReceivePort = 8000;
	mOscSendEnabled = 0;
	mOscSendPort = 9000;
	setOscSendIp("192.168.1.100");
	
    mLeapEnabled = 0;
    mJoystickEnabled = 0;
    m_bOscSpatSenderIsConnected = false;
	mSmoothedParametersRamps.resize(kNumberOfParameters);
	
	// default values for parameters
    for (int i = 0; i < JucePlugin_MaxNumInputChannels; i++){
        float fDefaultVal = normalize(kSourceMinDistance, kSourceMaxDistance, kSourceDefaultDistance);
        mParameters.set(getParamForSourceD(i), fDefaultVal);
    }

    for (int i = 0; i < JucePlugin_MaxNumOutputChannels; i++){
        mParameters.set(getParamForSpeakerM(i), 0);
    }
    std::unique_ptr<SourceMover> pMover(new SourceMover(this));
    m_pMover = std::move(pMover);
}

SpatGrisAudioProcessor::~SpatGrisAudioProcessor() {
    Trajectory::Ptr t = getTrajectory();
    if (t){
        t->stop();
    }
}

void SpatGrisAudioProcessor::startOrStopSourceUpdateThread(){
        if (mNumberOfSources == 1 || m_bIsRecordingAutomation || m_iMovementMode == 0) {
            m_pSourceUpdateThread->stopThread(500);
        } else if (!m_pSourceUpdateThread->isThreadRunning()){
            m_pSourceUpdateThread->startThread();
        }
    }

void SpatGrisAudioProcessor::updateNonSelectedSourcePositions(){
    int iSourceChanged = getSourceLocationChanged();
    if (iSourceChanged != -1){
        JUCE_COMPILER_WARNING("performance: there is most probably a better way than begining and ending here. Also unclear at what point and why I changed the if condition above")
        m_pMover->begin(iSourceChanged, kSourceThread);
        m_pMover->move(getSourceXY01(iSourceChanged), kSourceThread);
        m_pMover->end(kSourceThread);
        setSourceLocationChanged(-1);
    }
}

//==============================================================================
void SpatGrisAudioProcessor::setCalculateLevels(bool c)
{
#if USE_DB_METERS
    if (!mCalculateLevels && c){
        for (int i = 0; i < mNumberOfSpeakers; i++){
			mLevels.setUnchecked(i, 0);
        }
    }
#endif
	// keep count of number of editors
	if (c) mCalculateLevels++;
	else mCalculateLevels--;
	
}

void SpatGrisAudioProcessor::setProcessMode(int s) {
    mProcessMode = s;
    jassert(mProcessMode >= 0 && mProcessMode < kNumberOfModes);
    
    if (mProcessMode == kOscSpatMode){
        connectOscSpat();
    } else {
        disconnectOscSpat();
    }
}

void SpatGrisAudioProcessor::connectOscSpat(){
    disconnectOscSpat();
    
    m_bOscSpatSenderIsConnected = mOscSpatSender.connect(m_sOscIpAddress, m_iOscSpatPort);
    if(m_bOscSpatSenderIsConnected){
        m_pOscSpatThread->startThread();
    } else {
        DBG("OSC cannot connect to " + String(mOscSendIp) + ", port " + String(m_iOscSpatPort));
        jassertfalse;
    }
}

void SpatGrisAudioProcessor::disconnectOscSpat(){
    m_bOscSpatSenderIsConnected = !mOscSpatSender.disconnect();
    if (m_bOscSpatSenderIsConnected){
        DBG("OSC cannot disconnect from " + String(mOscSendIp) + ", port " + String(m_iOscSpatPort));
        jassertfalse;
    }
    if (m_pOscSpatThread->isThreadRunning()){
        m_pOscSpatThread->stopThread(500);
    }
}

void SpatGrisAudioProcessor::sendOscSpatValues(){
    if  (mProcessMode != kOscSpatMode || !m_bOscSpatSenderIsConnected){
        return;
    }
    
    for(int iCurSrc = 0; iCurSrc <mNumberOfSources; ++iCurSrc){
        int   channel_osc   = getOscSpat1stSrcId()+iCurSrc-1;   //in gui the range is 1-99, for zirkonium it actually starts at 0 (or potentially lower, but Zirkosc uses 0 as starting channel)
        FPoint curPoint     = getSourceAzimElev(iCurSrc);
        float azim_osc      = curPoint.x;                       //For Zirkonium, -1 is in the back right and +1 in the back left. 0 is forward
        float elev_osc      = curPoint.y;                       //For Zirkonium, 0 is the edge of the dome, .5 is the top
        float azimspan_osc  = 2*getSourceAzimSpan01(iCurSrc);     //min azim span is 0, max is 2. I figure this is radians.
        float elevspan_osc  = getSourceElevSpan01(iCurSrc)/2;     //min elev span is 0, max is .5
        float gain_osc      = 1;                                //gain is just locked to max value
        
        OSCAddressPattern oscPattern("/pan/az");
        OSCMessage message(oscPattern);
        
        message.addInt32(channel_osc);
        message.addFloat32(azim_osc);
        message.addFloat32(elev_osc);
        message.addFloat32(azimspan_osc);
        message.addFloat32(elevspan_osc);
        message.addFloat32(gain_osc);
JUCE_COMPILER_WARNING("NOT SENDING MESSAGES BECAUSE SENDING ON 0.0.0.0")
//        if (!mOscSpatSender.send(message)) {
////        if (!mOscSpatSender.sendToIPAddress(m_sOscIpAddress, m_iOscSpatPort, message)) {
//            DBG("Error: could not send OSC message.");
//            return;
//        }
    }
}

//==============================================================================
const String SpatGrisAudioProcessor::getName() const {
	String name(JucePlugin_Name);
	name << " ";
	name << mNumberOfSources;
	name << "x";
	name << mNumberOfSpeakers;
    return name;
}

int SpatGrisAudioProcessor::getNumParameters() {
    return kNumberOfParameters;
}

float SpatGrisAudioProcessor::getParameter (int index)
{
    return mParameters[index];
}

void SpatGrisAudioProcessor::setParameter (int index, float newValue){
    
    float fOldValue = mParameters.getUnchecked(index);
    
    if (!areSame(fOldValue, newValue)){
        if (newValue == 0){
            DBG("#54: TRYING TO SET PARAMETER " << index << " TO ZERO");
            return;
        }
        
        mParameters.set(index, newValue);

        if (!m_bPreventSourceLocationUpdate){
            if (index == getParamForSourceX(0) || index == getParamForSourceY(0)) {
                setSourceLocationChanged(0);
            } else if (index == getParamForSourceX(1) || index == getParamForSourceY(1)) {
                setSourceLocationChanged(1);
            } else if (index == getParamForSourceX(2) || index == getParamForSourceY(2)) {
                setSourceLocationChanged(2);
            } else if (index == getParamForSourceX(3) || index == getParamForSourceY(3)) {
                setSourceLocationChanged(3);
            } else if (index == getParamForSourceX(4) || index == getParamForSourceY(4)) {
                setSourceLocationChanged(4);
            } else if (index == getParamForSourceX(5) || index == getParamForSourceY(5)) {
                setSourceLocationChanged(5);
            } else if (index == getParamForSourceX(6) || index == getParamForSourceY(6)) {
                setSourceLocationChanged(6);
            } else if (index == getParamForSourceX(7) || index == getParamForSourceY(7)) {
                setSourceLocationChanged(7);
            }
        }

        mHostChangedParameter++;
    }
}


void SpatGrisAudioProcessor::setParameterNotifyingHost (int index, float newValue) {
	mParameters.set(index, newValue);
    
//    if      (index == getParamForSourceX(0) || index == getParamForSourceY(0)) { setSourceLocationChanged(0);}
//    else if (index == getParamForSourceX(1) || index == getParamForSourceY(1)) { setSourceLocationChanged(1);}
//    else if (index == getParamForSourceX(2) || index == getParamForSourceY(2)) { setSourceLocationChanged(2);}
//    else if (index == getParamForSourceX(3) || index == getParamForSourceY(3)) { setSourceLocationChanged(3);}
//    else if (index == getParamForSourceX(4) || index == getParamForSourceY(4)) { setSourceLocationChanged(4);}
//    else if (index == getParamForSourceX(5) || index == getParamForSourceY(5)) { setSourceLocationChanged(5);}
//    else if (index == getParamForSourceX(6) || index == getParamForSourceY(6)) { setSourceLocationChanged(6);}
//    else if (index == getParamForSourceX(7) || index == getParamForSourceY(7)) { setSourceLocationChanged(7);}
    sendParamChangeMessageToListeners(index, newValue);
}

const String SpatGrisAudioProcessor::getParameterName (int index)
{
   
	if (index == kSmooth)		return "Smooth Param";
    if (index == kVolumeNear)	return "Volume Near";
	if (index == kVolumeMid)	return "Volume Mid";
    if (index == kVolumeFar)	return "Volume Far";
	if (index == kFilterNear)	return "Filter Near";
	if (index == kFilterMid)	return "Filter Mid";
	if (index == kFilterFar)	return "Filter Far";
	if (index == kMaxSpanVolume)return "Max span volume";
	if (index == kRoutingVolume)return "Routing volume";
	
    if (index < mNumberOfSources * kParamsPerSource)
	{
		String s("Source ");
		s << (index / kParamsPerSource + 1);
		switch(index % kParamsPerSource)
		{
			case kSourceX:          s << " - X"; break;
			case kSourceY:          s << " - Y"; break;
			case kSourceD:          s << " - S"; break;
            case kSourceAzimSpan:   s << " -AS"; break;
            case kSourceElevSpan:   s << " -ES"; break;
            default: return String::empty;
		}
		return s;
	}
	index -= mNumberOfSources * kParamsPerSource;
	
    if (index < mNumberOfSpeakers * kParamsPerSpeakers)
	{
		String s("Speaker ");
		s << (index / kParamsPerSpeakers + 1);
		switch(index % kParamsPerSpeakers)
		{
            default: return String::empty;
		}
		return s;
	}
	
    return String::empty;
}

void SpatGrisAudioProcessor::setInputOutputMode (int p_iInputOutputMode){
    
    mInputOutputMode = p_iInputOutputMode-1;
    
    switch (mInputOutputMode){
            
        case i1o2:
            setNumberOfSources(1, false);
            setNumberOfSpeakers(2, false);
            break;
        case i1o4:
            setNumberOfSources(1, false);
            setNumberOfSpeakers(4, false);
            break;
        case i1o6:
            setNumberOfSources(1, false);
            setNumberOfSpeakers(6, false);
            break;
        case i1o8:
            setNumberOfSources(1, false);
            setNumberOfSpeakers(8, false);
            break;
        case i1o16:
            setNumberOfSources(1, false);
            setNumberOfSpeakers(16, false);
            break;
        case i2o2:
            setNumberOfSources(2, false);
            setNumberOfSpeakers(2, false);
            break;
        case i2o4:
            setNumberOfSources(2, false);
            setNumberOfSpeakers(4, false);
            break;
        case i2o6:
            setNumberOfSources(2, false);
            setNumberOfSpeakers(6, false);
            break;
        case i2o8:
            setNumberOfSources(2, false);
            setNumberOfSpeakers(8, false);
            break;
        case i2o16:
            setNumberOfSources(2, false);
            setNumberOfSpeakers(16, false);
            break;
        case i4o4:
            setNumberOfSources(4, false);
            setNumberOfSpeakers(4, false);
            break;
        case i4o6:
            setNumberOfSources(4, false);
            setNumberOfSpeakers(6, false);
            break;
        case i4o8:
            setNumberOfSources(4, false);
            setNumberOfSpeakers(8, false);
            break;
        case i4o16:
            setNumberOfSources(4, false);
            setNumberOfSpeakers(16, false);
            break;
        case i6o6:
            setNumberOfSources(6, false);
            setNumberOfSpeakers(6, false);
            break;
        case i6o8:
            setNumberOfSources(6, false);
            setNumberOfSpeakers(8, false);
            break;
        case i6o16:
            setNumberOfSources(6, false);
            setNumberOfSpeakers(16, false);
            break;
        case i8o8:
            setNumberOfSources(8, false);
            setNumberOfSpeakers(8, false);
            break;
        case i8o16:
            setNumberOfSources(8, false);
            setNumberOfSpeakers(16, false);
            break;
		default:
			jassertfalse;
    }
}

void SpatGrisAudioProcessor::updateInputOutputMode (){
    if (mNumberOfSources == 1 && mNumberOfSpeakers == 2){
        mInputOutputMode =  i1o2;
        return;
    } else if (mNumberOfSources == 1 && mNumberOfSpeakers == 4){
        mInputOutputMode =  i1o4;
        return;
    } else if (mNumberOfSources == 1 && mNumberOfSpeakers == 6){
        mInputOutputMode =  i1o6;
        return;
    } else if (mNumberOfSources == 1 && mNumberOfSpeakers == 8){
        mInputOutputMode =  i1o8;
        return;
    } else if (mNumberOfSources == 1 && mNumberOfSpeakers == 16){
        mInputOutputMode =  i1o16;
        return;
    } else if (mNumberOfSources == 2 && mNumberOfSpeakers == 2){
        mInputOutputMode =  i2o2;
        return;
    } else if (mNumberOfSources == 2 && mNumberOfSpeakers == 4){
        mInputOutputMode =  i2o4;
        return;
    } else if (mNumberOfSources == 2 && mNumberOfSpeakers == 6){
        mInputOutputMode =  i2o6;
        return;
    } else if (mNumberOfSources == 2 && mNumberOfSpeakers == 8){
        mInputOutputMode =  i2o8;
        return;
    }  else if (mNumberOfSources == 2 && mNumberOfSpeakers == 16){
        mInputOutputMode =  i2o16;
        return;
    } else if (mNumberOfSources == 4 && mNumberOfSpeakers == 4){
        mInputOutputMode =  i4o4;
        return;
    } else if (mNumberOfSources == 4 && mNumberOfSpeakers == 6){
        mInputOutputMode =  i4o6;
        return;
    } else if (mNumberOfSources == 4 && mNumberOfSpeakers == 8){
        mInputOutputMode =  i4o8;
        return;
    } else if (mNumberOfSources == 4 && mNumberOfSpeakers == 16){
        mInputOutputMode =  i4o16;
        return;
    } else if (mNumberOfSources == 6 && mNumberOfSpeakers == 6){
        mInputOutputMode =  i6o6;
        return;
    } else if (mNumberOfSources == 6 && mNumberOfSpeakers == 8){
        mInputOutputMode =  i6o8;
        return;
    } else if (mNumberOfSources == 6 && mNumberOfSpeakers == 16){
        mInputOutputMode =  i6o16;
        return;
    } else if (mNumberOfSources == 8 && mNumberOfSpeakers == 8){
        mInputOutputMode =  i8o8;
        return;
    } else if (mNumberOfSources == 8 && mNumberOfSpeakers == 16){
        mInputOutputMode =  i8o16;
        return;
    }
    jassert(0);
}

void SpatGrisAudioProcessor::setSrcPlacementMode(int p_i){
    mSrcPlacementMode = p_i;
}

void SpatGrisAudioProcessor::setSpPlacementMode(int p_i){
    mSpPlacementMode = p_i;
}

void SpatGrisAudioProcessor::setNumberOfSources(int p_iNewNumberOfSources, bool bUseDefaultValues){
    
    //if new number of sources is same as before, return
    if (p_iNewNumberOfSources == mNumberOfSources){
        return;
    } else {
        mIsNumberSourcesChanged = true;
    }
    
    //prevents audio process thread from running
    suspendProcessing (true);
    {
        //prevents gui from running
        const MessageManagerLock mmLock;
        
        mNumberOfSources = p_iNewNumberOfSources;
        
        mFilters.clear();
        mFilters.resize(mNumberOfSources);
        
        mLockedThetas.ensureStorageAllocated(mNumberOfSources);
        for (int i = 0; i < mNumberOfSources; i++){
            mLockedThetas.add(0);
        }
        mInputsCopy.resize(mNumberOfSources);
        
        if (bUseDefaultValues){
            double anglePerSource = 360 / mNumberOfSources;
            double offset, axisOffset;
            if (mNumberOfSources == 1){
                setSourceRT(0, FPoint(0, 0));
                mOldSrcLocRT[0] = FPoint(0, 0);
            } else if(mNumberOfSources%2 == 0) {//if the number of speakers is even we will assign them as stereo pairs
                axisOffset = anglePerSource / 2;
                for (int i = 0; i < mNumberOfSources; i++) {
                    if(i%2 == 0) {
                        offset = 90 + axisOffset;
                    } else {
                        offset = 90 - axisOffset;
                        axisOffset += anglePerSource;
                    }
                    if (offset < 0) offset += 360;
                    else if (offset > 360) offset -= 360;
                    
                    setSourceRT(i, FPoint(kSourceDefaultRadius, offset/360*kThetaMax));
                    mOldSrcLocRT[i] = FPoint(kSourceDefaultRadius, offset/360*kThetaMax);
                }
            } else {    //odd number of speakers, assign in circular fashion

                offset = (anglePerSource + 180) / 2 - anglePerSource;
                for (int i = 0; i < mNumberOfSources; i++) {
                    if (offset < 0) offset += 360;
                    else if (offset > 360) offset -= 360;
                    
                    setSourceRT(i, FPoint(kSourceDefaultRadius, offset/360*kThetaMax));
                    mOldSrcLocRT[i] = FPoint(kSourceDefaultRadius, offset/360*kThetaMax);
                    offset += anglePerSource;
                }
            }
        }
        for (int i = 0; i < mNumberOfSources; i++){
            mLockedThetas.set(i, getSourceRT(i).y);
        }
        mHostChangedParameter++;
    }
    //restart audio processing
    suspendProcessing (false);
    
    if (mNumberOfSources > 1 && m_pSourceUpdateThread != NULL && !m_pSourceUpdateThread->isThreadRunning()){
        m_pSourceUpdateThread->startThread();
    } else if (m_pSourceUpdateThread != NULL && m_pSourceUpdateThread->isThreadRunning()){
        m_pSourceUpdateThread->stopThread(500);
    }
}

void SpatGrisAudioProcessor::setNumberOfSpeakers(int p_iNewNumberOfSpeakers, bool bUseDefaultValues){
   
    //if new number of speakers is same as before, return
    if (p_iNewNumberOfSpeakers == mNumberOfSpeakers){
        return;
    } else {
        mIsNumberSpeakersChanged = true;
    }
    
    //prevents audio process thread from running
    suspendProcessing (true);
    {
        //prevents gui from running
        const MessageManagerLock mmLock;
        
        mNumberOfSpeakers = p_iNewNumberOfSpeakers;
        
        if (mRoutingMode == 1) {
            updateRoutingTempAudioBuffer();
        }
#if USE_DB_METERS
        mLevels.ensureStorageAllocated(mNumberOfSpeakers);
        for (int i = 0; i < mNumberOfSpeakers; i++){
            mLevels.add(0);
        }
#endif
        if (bUseDefaultValues){
            updateSpeakerLocation(true, false, false);
        }
        mHostChangedParameter++;
    }
    //starts audio processing again
    suspendProcessing (false);
}

void SpatGrisAudioProcessor::updateRoutingTempAudioBuffer() {
	mRoutingTempAudioBuffer.setSize(mNumberOfSpeakers, kMaxSize);
}

void SpatGrisAudioProcessor::updateSpeakerLocation(bool p_bAlternate, bool p_bStartAtTop, bool p_bClockwise){
    float anglePerSp = kThetaMax / getNumberOfSpeakers();
    
    if (p_bAlternate)
    {
        float offset = p_bStartAtTop
        ? (p_bClockwise ? kQuarterCircle : (kQuarterCircle - anglePerSp))
        : (kQuarterCircle - anglePerSp/2);
        float start = offset;
        for (int i = p_bClockwise ? 0 : 1; i < getNumberOfSpeakers(); i += 2)
        {
            setSpeakerRT(i, FPoint(1, offset));
            offset -= anglePerSp;
        }
        
        offset = start + anglePerSp;
        for (int i = p_bClockwise ? 1 : 0; i < getNumberOfSpeakers(); i += 2)
        {
            setSpeakerRT(i, FPoint(1, offset));
            offset += anglePerSp;
        }
    }
    else
    {
        float offset = p_bStartAtTop ? kQuarterCircle : kQuarterCircle + anglePerSp/2;
        float delta = p_bClockwise ? -anglePerSp : anglePerSp;
        for (int i = 0; i < getNumberOfSpeakers(); i++)
        {
            setSpeakerRT(i, FPoint(1, offset));
            offset += delta;
        }
    }
    
}

const String SpatGrisAudioProcessor::getParameterText (int index)
{
    return String::empty;
}

const String SpatGrisAudioProcessor::getInputChannelName (int channelIndex) const
{
    return String(channelIndex + 1);
}

const String SpatGrisAudioProcessor::getOutputChannelName (int channelIndex) const
{
    return String(channelIndex + 1);
}

bool SpatGrisAudioProcessor::isInputChannelStereoPair (int index) const
{
    return index == 0 && mNumberOfSources == 2;
	//return false;
}

bool SpatGrisAudioProcessor::isOutputChannelStereoPair (int index) const
{
    return index == 0 && mNumberOfSpeakers == 2;
	//return false;
}

bool SpatGrisAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SpatGrisAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SpatGrisAudioProcessor::silenceInProducesSilenceOut() const
{
    return false;
}

double SpatGrisAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SpatGrisAudioProcessor::getNumPrograms()
{
    return 1;
}

int SpatGrisAudioProcessor::getCurrentProgram()
{
    return 1;
}

void SpatGrisAudioProcessor::setCurrentProgram (int index)
{
}

const String SpatGrisAudioProcessor::getProgramName (int index)
{
    return String::empty;
}

void SpatGrisAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void SpatGrisAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {
    if (m_bAllowInputOutputModeSelection) {
        //insure that mNumberOfSources and mNumberOfSpeakers are valid. if not, we will change mInputOutputMode.
        int iTotalSources = getTotalNumInputChannels();
        if (iTotalSources < mNumberOfSources){
            setNumberOfSources(iTotalSources, true);
        }
        int iTotalSpeakers = getTotalNumOutputChannels();
        if (iTotalSpeakers < mNumberOfSpeakers) {
            setNumberOfSpeakers(iTotalSpeakers, true);
        }
        //apply current mNumberOfSources and mNumberOfSpeakers
        updateInputOutputMode();
    } else {
        //this is basically only done in unknown daws and in AUeval (or whatever)
        setNumberOfSources(getTotalNumInputChannels(), true);
        setNumberOfSpeakers(getTotalNumOutputChannels(), true);
    }
    
    for (int i = 0; i < mNumberOfSources; i++) {
        mFilters[i].setSampleRate(static_cast<int>(sampleRate));
    }
}

void SpatGrisAudioProcessor::reset() {
#if USE_DB_METERS
    if (mCalculateLevels){
        for (int i = 0; i < mNumberOfSpeakers; i++){
            mLevels.setUnchecked(i, 0);
        }
    }
#endif
    
    mSmoothedParametersInited = false;
    
    for (int i = 0; i < mNumberOfSources; i++) {
        mFilters[i].reset();
    }
    
    
    Router::instance().reset();
}

void SpatGrisAudioProcessor::releaseResources()
{

}

void SpatGrisAudioProcessor::processBlockBypassed (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
	//fprintf(stderr, "pb bypass\n");
	//for (int c = mNumberOfSources; c < mNumberOfSpeakers; c++)
	//	buffer.clear(c, 0, buffer.getNumSamples());
}

void SpatGrisAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
	// sanity check for auval
	if (buffer.getNumChannels() < ((mRoutingMode == 1) ? mNumberOfSources : jmax(mNumberOfSources, mNumberOfSpeakers))) {
		printf("unexpected channel count %d vs %dx%d rmode: %d\n", buffer.getNumChannels(), mNumberOfSources, mNumberOfSpeakers, mRoutingMode);
		return;
	}
    
    AudioPlayHead::CurrentPositionInfo cpi;
    getPlayHead()->getCurrentPosition(cpi);
    m_bIsPlaying = cpi.isPlaying;
    
	unsigned int oriFramesToProcess = buffer.getNumSamples();
	
	if (mRoutingMode > 1) {
		buffer.clear();
		
		int outChannels = (mNumberOfSpeakers > 2) ? 2 : mNumberOfSpeakers;
		int offset = (mRoutingMode - 2) * 2;
		for (int c = 0; c < outChannels; c++) {
			buffer.copyFrom(c, 0, Router::instance().outputBuffers(oriFramesToProcess)[offset + c], oriFramesToProcess);
			Router::instance().clear(offset + c);
		}
		return;
	}
	
	double sampleRate = getSampleRate();
	unsigned int inFramesToProcess = oriFramesToProcess;
	
	Trajectory::Ptr trajectory = mTrajectory;
	if (trajectory) {
        if (m_bIsPlaying) {
			// we're playing!
			//mLastTimeInSamples = cpi.timeInSamples;
	
			double bps = cpi.bpm / 60;
			float seconds = oriFramesToProcess / sampleRate;
			float beats = seconds * bps;
			
			bool done = trajectory->process(seconds, beats);
            if (done) mTrajectory = NULL;
		}
	}
    
    if (mProcessMode == kOscSpatMode) {
        return;
    }
	
	// cache values
	float params[kNumberOfParameters];
	for (int i = 0; i < kNumberOfParameters; i++)
		params[i] = mParameters[i];
		
	if (mProcessMode != kFreeVolumeMode) {
		params[kVolumeNear] = denormalize(kVolumeNearMin, kVolumeNearMax, params[kVolumeNear]);
		params[kVolumeMid]  = denormalize(kVolumeMidMin,  kVolumeMidMax,  params[kVolumeMid]);
		params[kVolumeFar]  = denormalize(kVolumeFarMin,  kVolumeFarMax,  params[kVolumeFar]);
		params[kFilterNear] = denormalize(kFilterNearMin, kFilterNearMax, params[kFilterNear]);
		params[kFilterMid]  = denormalize(kFilterMidMin,  kFilterMidMax,  params[kFilterMid]);
		params[kFilterFar]  = denormalize(kFilterFarMin,  kFilterFarMax,  params[kFilterFar]);
	}
	if (mProcessMode == kPanSpanMode) {
		params[kMaxSpanVolume] = denormalize(kMaxSpanVolumeMin, kMaxSpanVolumeMax, params[kMaxSpanVolume]);
	}
	if (mRoutingMode == 1) {
		params[kRoutingVolume] = denormalize(kRoutingVolumeMin, kRoutingVolumeMax, params[kRoutingVolume]);
	}
	
	//float *inputs[iActualNumberOfSources];
	float **inputs = new float* [mNumberOfSources];
	for (int i = 0; i < mNumberOfSources; i++) {
		inputs[i] = buffer.getWritePointer(i);

        if (mProcessMode == kFreeVolumeMode){
            float fValue = denormalize(kSourceMinDistance, kSourceMaxDistance, params[getParamForSourceD(i)]);
            params[getParamForSourceD(i)] = fValue;
        }
		params[getParamForSourceX(i)] = params[getParamForSourceX(i)] * (2*kRadiusMax) - kRadiusMax;
		params[getParamForSourceY(i)] = params[getParamForSourceY(i)] * (2*kRadiusMax) - kRadiusMax;
	}
	
	if (mRoutingMode == 1) {
		jassert(mRoutingTempAudioBuffer.getNumSamples() >= oriFramesToProcess);
		jassert(mRoutingTempAudioBuffer.getNumChannels() >= mNumberOfSpeakers);
	}
	
	//float *outputs[iActualNumberOfSpeakers];
	float **outputs = new float*[mNumberOfSpeakers];
	for (int o = 0; o < mNumberOfSpeakers; o++)
	{

		outputs[o] = (mRoutingMode == 1) ? mRoutingTempAudioBuffer.getWritePointer(o) : buffer.getWritePointer(o);
        
		if (mProcessMode == kFreeVolumeMode)
		{
			params[getParamForSpeakerX(o)] = params[getParamForSpeakerX(o)] * (2*kRadiusMax) - kRadiusMax;
			params[getParamForSpeakerY(o)] = params[getParamForSpeakerY(o)] * (2*kRadiusMax) - kRadiusMax;
		}
		else
		{
			float x = params[getParamForSpeakerX(o)] * (2*kRadiusMax) - kRadiusMax;
			float y = params[getParamForSpeakerY(o)] * (2*kRadiusMax) - kRadiusMax;
			float t = atan2f(y, x);
			if (t < 0) t += kThetaMax;
			params[getParamForSpeakerX(o)] = t;
		}
	}
	
	// make sure the smoothed parameters table is inited
	if (!mSmoothedParametersInited)
	{
		for (int i = 0; i < kNumberOfParameters; i++)
			mSmoothedParameters.setUnchecked(i, params[i]);
		mSmoothedParametersInited = true;
	}
	
	bool processesInPlaceIsIgnored = (mRoutingMode != 1);
	
	// process data
    unsigned int numFramesToDo;
	while(1) {
        //we process either kChunkSize frames or whatever is left in inFramesToProcess
		numFramesToDo = (inFramesToProcess > kChunkSize) ? kChunkSize : inFramesToProcess;
		
		if (processesInPlaceIsIgnored) {
			float **inputsCopy = new float* [mNumberOfSources];
            JUCE_COMPILER_WARNING("why are we copying the inputs instead of using original?")
			for (int i = 0; i < mNumberOfSources; i++) {
				memcpy(mInputsCopy.getReference(i).b, inputs[i], numFramesToDo * sizeof(float));
				inputsCopy[i] = mInputsCopy.getReference(i).b;
			}
			
			ProcessData(inputsCopy, outputs, params, sampleRate, numFramesToDo);
			delete[] inputsCopy;
        } else {
			ProcessData(inputs, outputs, params, sampleRate, numFramesToDo);
        }
		
		inFramesToProcess -= numFramesToDo;
        if (inFramesToProcess == 0) {
            break;
        }
        for (int i = 0; i < mNumberOfSources; i++){
			inputs[i] += numFramesToDo;
        }
        for (int o = 0; o < mNumberOfSpeakers; o++){
            outputs[o] += numFramesToDo;
        }
	}
	
    JUCE_COMPILER_WARNING("1 = internal write")
	if (mRoutingMode == 1){
		// apply routing volume
		int i = kRoutingVolume;
		float currentParam = mSmoothedParameters[i];
		float targetParam = params[i];
		float *ramp = mSmoothedParametersRamps.getReference(i).b;
		const float smooth = denormalize(kSmoothMin, kSmoothMax, params[kSmooth]); // milliseconds
		const float sm_o = powf(0.01f, 1000.f / (smooth * sampleRate));
		const float sm_n = 1 - sm_o;
		for (unsigned int f = 0; f < oriFramesToProcess; f++)
		{
			currentParam = currentParam * sm_o + targetParam * sm_n;
			ramp[f] = dbToLinear(currentParam);
		}
		mSmoothedParameters.setUnchecked(i, currentParam);
		for (int o = 0; o < mNumberOfSpeakers; o++)
		{
			float *output = mRoutingTempAudioBuffer.getWritePointer(o);
			for (unsigned int f = 0; f < oriFramesToProcess; f++)
				output[f] *= ramp[f];
		}
	}
#if USE_DB_METERS
	if (mCalculateLevels) {
		const float attack = kLevelAttackDefault; //params[kLevelAttackParam]; // milliseconds
		const float release = kLevelReleaseDefault; //params[kLevelReleaseParam]; // milliseconds
		const float ag = powf(0.01f, 1000.f / (attack * sampleRate));
		const float rg = powf(0.01f, 1000.f / (release * sampleRate));
		
		for (int o = 0; o < mNumberOfSpeakers; o++) {
			float *output = outputs[o];
			float env = mLevels[o];
			
			for (unsigned int f = 0; f < numFramesToDo; f++) {
				float s = fabsf(output[f]);
				float g = (s > env) ? ag : rg;
				env = g * env + (1.f - g) * s;
			}
			mLevels.setUnchecked(o, env);
		}
	}
#endif
    
	if (mRoutingMode == 1) {
		// accumulate in internal buffer
		Router::instance().accumulate(mNumberOfSpeakers, oriFramesToProcess, mRoutingTempAudioBuffer);
		buffer.clear();
	}
	
	delete[] inputs;
	delete[] outputs;
	mProcessCounter++;
}

void SpatGrisAudioProcessor::ProcessData(float **inputs, float **outputs, float *params, float sampleRate, unsigned int frames) {
	switch(mProcessMode) {
		case kFreeVolumeMode:	ProcessDataFreeVolumeMode(inputs, outputs, params, sampleRate, frames);	break;
		case kPanVolumeMode:	ProcessDataPanVolumeMode(inputs, outputs, params, sampleRate, frames);	break;
		case kPanSpanMode:		ProcessDataPanSpanMode(inputs, outputs, params, sampleRate, frames);	break;
	}
}

void SpatGrisAudioProcessor::findLeftAndRightSpeakers(float p_fTargetAngle, float *params, int &p_piLeftSpeaker, int &p_piRightSpeaker,
                                                      float &p_pfDeltaAngleToLeftSpeaker, float &p_pfDeltaAngleToRightSpeaker, int p_iTargetSpeaker)
{
    int iFirstSpeaker = -1, iLastSpeaker = -1;
    //float fMaxAngle = -1, fMinAngle = 9999999;
    float fMaxAngle = p_fTargetAngle, fMinAngle = p_fTargetAngle;
    
    
    p_piLeftSpeaker = -1;
    p_piRightSpeaker = -1;
    p_pfDeltaAngleToLeftSpeaker = kThetaMax;
    p_pfDeltaAngleToRightSpeaker = kThetaMax;
    
    for (int iCurSpeaker = 0; iCurSpeaker < mNumberOfSpeakers; iCurSpeaker++){
        
        float fCurSpeakerAngle = params[getParamForSpeakerX(iCurSpeaker)];
        float fCurDeltaAngle = -1.;
        
        //find out which is the first and last speakers
        if (fCurSpeakerAngle < fMinAngle){
            fMinAngle = fCurSpeakerAngle;
            iFirstSpeaker = iCurSpeaker;
        }
        if (fCurSpeakerAngle > fMaxAngle){
            fMaxAngle = fCurSpeakerAngle;
            iLastSpeaker = iCurSpeaker;
        }
        
        //skip the rest for the target speaker
        if (iCurSpeaker == p_iTargetSpeaker){
            continue;
        }
        
        //if curSpeaker is on left of target speaker
//        if (fCurSpeakerAngle < p_fTargetSpeakerAngle){
        if (fCurSpeakerAngle >= p_fTargetAngle){
            //check if curAngle is smaller than previous smallest left angle
            //fCurDeltaAngle = p_fTargetSpeakerAngle - fCurSpeakerAngle;
            fCurDeltaAngle = fCurSpeakerAngle - p_fTargetAngle;
            if (fCurDeltaAngle < p_pfDeltaAngleToLeftSpeaker){
                p_pfDeltaAngleToLeftSpeaker = fCurDeltaAngle;
                p_piLeftSpeaker = iCurSpeaker;
            }
            
        }
        //if curSpeaker is on right of target speaker
        else {
//            fCurDeltaAngle = fCurSpeakerAngle - p_fTargetSpeakerAngle;
            fCurDeltaAngle = p_fTargetAngle - fCurSpeakerAngle;
            if (fCurDeltaAngle < p_pfDeltaAngleToRightSpeaker){
                p_pfDeltaAngleToRightSpeaker = fCurDeltaAngle;
                p_piRightSpeaker = iCurSpeaker;
            }
        }
    }
    
    //if we haven't found the right speaker and the target is the first speaker, the left speaker is the first one
    if ((areSame(fMinAngle, p_fTargetAngle) || p_iTargetSpeaker == iFirstSpeaker) && p_piRightSpeaker == -1){
        p_piRightSpeaker = iLastSpeaker;
        p_pfDeltaAngleToRightSpeaker = fMinAngle + (kThetaMax - fMaxAngle);
    }
    
    //if we haven't found the left speaker and the target is the last speaker, the left speaker is the first one
    else if ((areSame(fMaxAngle, p_fTargetAngle) || p_iTargetSpeaker == iLastSpeaker) && p_piLeftSpeaker == -1){
        p_piLeftSpeaker = iFirstSpeaker;
        p_pfDeltaAngleToLeftSpeaker = fMinAngle + (kThetaMax - fMaxAngle);
    }
    
//    //if we haven't found the right speaker and the target is the first speaker, the left speaker is the first one
//    if (p_iTargetSpeaker == iFirstSpeaker && p_piLeftSpeaker == -1){
//        p_piLeftSpeaker = iLastSpeaker;
//        p_pfDeltaAngleToLeftSpeaker = fMinAngle + (kThetaMax - fMaxAngle);
//    }
//    
//    //if we haven't found the left speaker and the target is the last speaker, the left speaker is the first one
//    else if (p_iTargetSpeaker == iLastSpeaker && p_piRightSpeaker == -1){
//        p_piRightSpeaker = iFirstSpeaker;
//        p_pfDeltaAngleToRightSpeaker = fMinAngle + (kThetaMax - fMaxAngle);
//    }
}


void SpatGrisAudioProcessor::addToOutput(float s, float **outputs, int o, int f)
{
	float *output_m = mSmoothedParametersRamps.getReference(getParamForSpeakerM(o)).b;
//	float *output_a = mSmoothedParametersRamps.getReference(getParamForSpeakerA(o)).b;
//	float a = dbToLinear(output_a[f]);
    float a = dbToLinear(kSpeakerDefaultAttenuation);
	float m = 1 - output_m[f];
	float output_adj = a * m;
	float *output = outputs[o];
	output[f] += s * output_adj;
}

void SpatGrisAudioProcessor::ProcessDataPanVolumeMode(float **inputs, float **outputs, float *params, float sampleRate, unsigned int frames) {
	
	// ramp all parameters using param smoothing parameter, except constant ones and speaker thetas
    const int sourceParameters = JucePlugin_MaxNumInputChannels * kParamsPerSource;
	const int speakerParameters = JucePlugin_MaxNumOutputChannels * kParamsPerSpeakers;
    for (int i = 0; i < (kNumberOfParameters - kConstantParameters); ++i) {
		bool isSpeakerXY = (i >= sourceParameters && i < (sourceParameters + speakerParameters) && ((i - sourceParameters) % kParamsPerSpeakers) <= kSpeakerY);
        if (isSpeakerXY) {
            continue;
        }
		float currentParam = mSmoothedParameters[i];
		float targetParam = params[i];
		float *ramp = mSmoothedParametersRamps.getReference(i).b;
	
		for (unsigned int f = 0; f < frames; ++f) {
            //this is apparently more optimal than having variables declared outside the loop, http://stackoverflow.com/questions/7959573/declaring-variables-inside-loops-good-practice-or-bad-practice-2-parter#
            const float smooth = denormalize(kSmoothMin, kSmoothMax, params[kSmooth]); // milliseconds
            const float sm_o = powf(0.01f, 1000.f / (smooth * sampleRate));
            const float sm_n = 1 - sm_o;
			currentParam = currentParam * sm_o + targetParam * sm_n;
			ramp[f] = currentParam;
		}
		
		mSmoothedParameters.setUnchecked(i, currentParam);
	}
	
	// clear outputs[]
	for (int o = 0; o < mNumberOfSpeakers; ++o) {
		float *output = outputs[o];
		memset(output, 0, frames * sizeof(float));
	}

	// Compute output[] for each source
	for (int i = 0; i < mNumberOfSources; ++i) {
		float *input = inputs[i];
		float *input_x = mSmoothedParametersRamps.getReference(getParamForSourceX(i)).b;
		float *input_y = mSmoothedParametersRamps.getReference(getParamForSourceY(i)).b;
	
		for (unsigned int f = 0; f < frames; ++f) {
			float s = input[f];     //current sample
			float x = input_x[f];   //x position of current sample
			float y = input_y[f];   //y position of current sample
			
			// could use the Accelerate framework on mac for these
			float r = hypotf(x, y);
            if (r > kRadiusMax) {
                r = kRadiusMax;
            }
			float it = atan2f(y, x);
            if (it < 0){
                it += kThetaMax;
            }
			
			if (mApplyFilter) {
				float distance;
				if (r >= 1) distance = denormalize(params[kFilterMid], params[kFilterFar], (r - 1));
				else distance = denormalize(params[kFilterNear], params[kFilterMid], r);
				s = mFilters[i].process(s, distance);
			}
			
			// adjust input volume
			{
				float dbSource;
				if (r >= 1) dbSource = denormalize(params[kVolumeMid], params[kVolumeFar], (r - 1));
				else dbSource = denormalize(params[kVolumeNear], params[kVolumeMid], r);
				s *= dbToLinear(dbSource);
			}
			
			float t;
			if (r >= kThetaLockRadius){
				t = it;
				mLockedThetas.setUnchecked(i, it);
			} else {
				float c = (r >= kThetaLockRampRadius) ? ((r - kThetaLockRampRadius) / (kThetaLockRadius - kThetaLockRampRadius)) : 0;
				float lt = mLockedThetas.getUnchecked(i);
				float dt = lt - it;
				if (dt < 0) dt = -dt;
				if (dt > kQuarterCircle)
				{
					// assume flipped side
					if (lt > it) lt -= kHalfCircle;
					else lt += kHalfCircle;
				}
				t = c * it + (1 - c) * lt;
				
				if (t < 0) t += kThetaMax;
				else if (t >= kThetaMax) t -= kThetaMax;
				
				//if (f == 0) printf("it: %f lt: %f lt2: %f t: %f c: %f\n", it, mLockedThetas.getUnchecked(i), lt, t, c);
			}
			
			if (r >= 1) {
				// find left and right speakers
				int left, right;
				float dLeft, dRight;
                findLeftAndRightSpeakers(t, params, left, right, dLeft, dRight);
                
				// add to output
				if (left >= 0 && right >= 0) {
					float dTotal = dLeft + dRight;
					float vLeft = 1 - dLeft / dTotal;
					float vRight = 1 - dRight / dTotal;
					
					addToOutput(s * vLeft, outputs, left, f);
					addToOutput(s * vRight, outputs, right, f);
				} else {
					// one side is empty!
					int o = (left >= 0) ? left : right;
					jassert(o >= 0);
					
					addToOutput(s, outputs, o, f);
				}
			} else {
				// find front left, right
				int frontLeft, frontRight;
				float dFrontLeft, dFrontRight;
                findLeftAndRightSpeakers(t, params, frontLeft, frontRight, dFrontLeft, dFrontRight);
                
				float bt = t + kHalfCircle;
				if (bt > kThetaMax) bt -= kThetaMax;
				
				// find back left, right
				int backLeft, backRight;
				float dBackLeft, dBackRight;
                findLeftAndRightSpeakers(bt, params, backLeft, backRight, dBackLeft, dBackRight);                
			
				float front = r * 0.5f + 0.5f;
				float back = 1 - front;
				
				// add to front output
				if (frontLeft >= 0 && frontRight >= 0) {
					float dTotal = dFrontLeft + dFrontRight;
					float vLeft = 1 - dFrontLeft / dTotal;
					float vRight = 1 - dFrontRight / dTotal;
					
					addToOutput(s * vLeft * front, outputs, frontLeft, f);
					addToOutput(s * vRight * front, outputs, frontRight, f);
				} else {
					// one side is empty!
					int o = (frontLeft >= 0) ? frontLeft : frontRight;
					jassert(o >= 0);
					
					addToOutput(s * front, outputs, o, f);
				}
				
				// add to back output
				if (backLeft >= 0 && backRight >= 0) {
					float dTotal = dBackLeft + dBackRight;
					float vLeft = 1 - dBackLeft / dTotal;
					float vRight = 1 - dBackRight / dTotal;
					
					addToOutput(s * vLeft * back, outputs, backLeft, f);
					addToOutput(s * vRight * back, outputs, backRight, f);
				} else {
					// one side is empty!
					int o = (backLeft >= 0) ? backLeft : backRight;
					jassert(o >= 0);
					
					addToOutput(s * back, outputs, o, f);
				}
			}
		}
	}
}

class Area
{
public:
    Area() {}
    Area(int sp, float ix1, float iy1, float ix2, float iy2)
    :	speaker(sp),
    x1(ix1), x2(ix2) //, y1(iy1), y2(iy2)
    {
        m = (iy2-iy1) / (ix2-ix1);
        b = iy1 - m * ix1;
    }
    
    float eval(float x) const { return m * x + b; }
    
    int speaker;
    float x1, x2;
    //float y1, y2;
    float m, b;
};


static void AddArea(int speaker, float ix1, float iy1, float ix2, float iy2, vector<Area> &areas, int &areaCount, int &speakerCount)
{
    jassert(ix1 < ix2);
    
    //fprintf(stderr, "speaker: %d x1: %f x2: %f dc: %f\n", speaker, ix1, ix2, ix2 - ix1);
    //fflush(stderr);
    
    if (ix1 < 0)
    {
        jassert(ix2 >= 0 && ix2 <= kThetaMax);
        jassert(ix1 + kThetaMax > 0);
        
        float yc = (0 - ix1) / (ix2 - ix1) * (iy2 - iy1) + iy1;
        
        jassert(areaCount < speakerCount * s_iMaxAreas);
        areas[areaCount++] = Area(speaker, kThetaMax + ix1, iy1, kThetaMax, yc);
        
        jassert(areaCount < speakerCount * s_iMaxAreas);
        areas[areaCount++] = Area(speaker, 0, yc, ix2, iy2);
    }
    else if (ix2 > kThetaMax)
    {
        jassert(ix1 >= 0 && ix1 <= kThetaMax);
        jassert(ix2 - kThetaMax < kThetaMax);
        
        float yc = (kThetaMax - ix1) / (ix2 - ix1) * (iy2 - iy1) + iy1;
        
        jassert(areaCount < speakerCount * s_iMaxAreas);
        areas[areaCount++] = Area(speaker, ix1, iy1, kThetaMax, yc);
        
        jassert(areaCount < speakerCount * s_iMaxAreas);
        areas[areaCount++] = Area(speaker, 0, yc, ix2 - kThetaMax, iy2);
    }
    else
    {
        jassert(ix1 >= 0 && ix2 <= kThetaMax);
        
        jassert(areaCount < speakerCount * s_iMaxAreas);
        areas[areaCount++] = Area(speaker, ix1, iy1, ix2, iy2);
    }
}
static void Integrate(float x1, float x2, const vector<Area> &areas, int areaCount, float *outFactors, float factor)
{
    if (x1 == x2)
    {
        //fprintf(stderr, "x1 == x2 (%f == %f)\n", x1, x2);
        return;
    }
    
    jassert(x1 < x2);
    jassert(x1 >= 0);
    jassert(x2 <= kThetaMax);
    
    for (int a = 0; a < areaCount; a++)
    {
        const Area &area = areas[a];
        
        if (x2 <= area.x1 || x1 >= area.x2)
            continue; // completely outside
        
        float c1 = (x1 < area.x1) ? area.x1 : x1;
        float c2 = (x2 > area.x2) ? area.x2 : x2;
        
        float y1 = area.eval(c1);
        float y2 = area.eval(c2);
        
        float dc = c2 - c1;
        //fprintf(stderr, "x1: %f x2: %f area.x1: %f area.x2: %f c1: %f c2: %f dc: %f\n", x1, x2, area.x1, area.x2, c1, c2, dc);
        //fflush(stderr);
        jassert(dc > 0);
        
        float v = dc * (y1+y2); // * 0.5f;
        if (v <= 0) v = 1e-6;
        
        outFactors[area.speaker] += v * factor;
    }
}

void SpatGrisAudioProcessor::ProcessDataPanSpanMode(float **inputs, float **outputs, float *params, float sampleRate, unsigned int frames)
{
    const float smooth = denormalize(kSmoothMin, kSmoothMax, params[kSmooth]); // milliseconds
    const float sm_o = powf(0.01f, 1000.f / (smooth * sampleRate));
    const float sm_n = 1 - sm_o;
    
    // ramp all the parameters, except constant ones and speaker thetas
    const int sourceParameters = JucePlugin_MaxNumInputChannels * kParamsPerSource;
    const int speakerParameters = JucePlugin_MaxNumOutputChannels * kParamsPerSpeakers;
//    const int sourceParameters = mNumberOfSources * kParamsPerSource;
//    const int speakerParameters = mNumberOfSpeakers * kParamsPerSpeakers;
    
    for (int iCurParamId= 0; iCurParamId < (kNumberOfParameters - kConstantParameters); iCurParamId++){
        bool isSpeakerXY = (iCurParamId >= sourceParameters && iCurParamId < (sourceParameters + speakerParameters) && ((iCurParamId - sourceParameters) % kParamsPerSpeakers) <= kSpeakerY);
        if (isSpeakerXY) continue;
        
        float currentParam = mSmoothedParameters[iCurParamId];
        float targetParam = params[iCurParamId];
        float *ramp = mSmoothedParametersRamps.getReference(iCurParamId).b;
        
        //float ori = currentParam;
        
        for (unsigned int f = 0; f < frames; f++)
        {
            currentParam = currentParam * sm_o + targetParam * sm_n;
            ramp[f] = currentParam;
        }
        
        //if (i == 0 && ori != currentParam) printf("param %i -> %f -> %f\n", i, ori, currentParam);
        
        mSmoothedParameters.setUnchecked(iCurParamId, currentParam);
    }
    
    // clear outputs
    for (int o = 0; o < mNumberOfSpeakers; o++)
    {
        float *output = outputs[o];
        memset(output, 0, frames * sizeof(float));
    }
    
    vector<Area> areas;
    areas.resize(mNumberOfSpeakers * s_iMaxAreas);

    int areaCount = 0;

    
    
    if (mNumberOfSpeakers > 2)
    {
        for (int iCurSpeaker = 0; iCurSpeaker < mNumberOfSpeakers; iCurSpeaker++)
        {
            float fCurAngle = params[getParamForSpeakerX(iCurSpeaker)];
            int left, right;
            float dLeft, dRight;
            findLeftAndRightSpeakers(fCurAngle, params, left, right, dLeft, dRight, iCurSpeaker);
 
            jassert(left >= 0 && right >= 0);
            jassert(dLeft > 0 && dRight > 0);
            
            AddArea(iCurSpeaker, fCurAngle - dLeft, 0, fCurAngle, 1, areas, areaCount, mNumberOfSpeakers);
            AddArea(iCurSpeaker, fCurAngle, 1, fCurAngle + dRight, 0, areas, areaCount, mNumberOfSpeakers);
        }
    }
    else if (mNumberOfSpeakers == 2)
    {
        int s1 = (params[getParamForSpeakerX(0)] < params[getParamForSpeakerX(1)]) ? 0 : 1;
        int s2 = 1 - s1;
        float t1 = params[getParamForSpeakerX(s1)];
        float t2 = params[getParamForSpeakerX(s2)];
        
        AddArea(s1, t2 - kThetaMax, 0, t1, 1, areas, areaCount, mNumberOfSpeakers);
        AddArea(s1, t1, 1, t2, 0, areas, areaCount, mNumberOfSpeakers);
        
        AddArea(s2, t1, 0, t2, 1, areas, areaCount, mNumberOfSpeakers);
        AddArea(s2, t2, 1, t1 + kThetaMax, 0, areas, areaCount, mNumberOfSpeakers);
    }
    else
    {
        AddArea(0, 0, 1, kThetaMax, 1, areas, areaCount, mNumberOfSpeakers);
    }
   
    
    
    jassert(areaCount > 0);
    
    
    // compute
    // in this context: source T, R are actually source X, Y
    for (int i = 0; i < mNumberOfSources; i++)
    {
        float *input = inputs[i];
        float *input_x = mSmoothedParametersRamps.getReference(getParamForSourceX(i)).b;
        float *input_y = mSmoothedParametersRamps.getReference(getParamForSourceY(i)).b;
        float *input_d = mSmoothedParametersRamps.getReference(getParamForSourceD(i)).b;
        
        for (unsigned int f = 0; f < frames; f++)
        {
            float s = input[f];
            float x = input_x[f];
            float y = input_y[f];
            float d = input_d[f];
            
            if (d > 1){
               d = normalize(kSourceMinDistance, kSourceMaxDistance, d);
            }
            
            float tv = dbToLinear(d * params[kMaxSpanVolume]);
            
            // could use the Accelerate framework on mac for these
            float r = hypotf(x, y);
            if (r > kRadiusMax) r = kRadiusMax;
            
            float it = atan2f(y, x);
            if (it < 0) it += kThetaMax;
            
            //if (r < 1 && d > 0.5) d = 0.5;
            if (d < 1e-6) d = 1e-6;
            float angle = d * M_PI;
            
            if (mApplyFilter)
            {
                float distance;
                if (r >= 1) distance = denormalize(params[kFilterMid], params[kFilterFar], (r - 1));
                else distance = denormalize(params[kFilterNear], params[kFilterMid], r);
                s = mFilters[i].process(s, distance);
            }
            
            // adjust input volume
            {
                float dbSource;
                if (r >= 1) dbSource = denormalize(params[kVolumeMid], params[kVolumeFar], (r - 1));
                else dbSource = denormalize(params[kVolumeNear], params[kVolumeMid], r);
                s *= dbToLinear(dbSource);
            }
            
            float t;
            if (r >= kThetaLockRadius)
            {
                t = it;
                mLockedThetas.setUnchecked(i, it);
            }
            else
            {
                float c = (r >= kThetaLockRampRadius) ? ((r - kThetaLockRampRadius) / (kThetaLockRadius - kThetaLockRampRadius)) : 0;
                float lt = mLockedThetas.getUnchecked(i);
                float dt = lt - it;
                if (dt < 0) dt = -dt;
                if (dt > kQuarterCircle)
                {
                    // assume flipped side
                    if (lt > it) lt -= kHalfCircle;
                    else lt += kHalfCircle;
                }
                t = c * it + (1 - c) * lt;
                
                if (t < 0) t += kThetaMax;
                else if (t >= kThetaMax) t -= kThetaMax;
                
                //if (f == 0) printf("it: %f lt: %f lt2: %f t: %f c: %f\n", it, mLockedThetas.getUnchecked(i), lt, t, c);
            }
            
            jassert(t >= 0 && t <= kThetaMax);
            jassert(angle > 0 && angle <= kHalfCircle);
            
            //float outFactors[mNumberOfSpeakers];
            //memset(outFactors, 0, sizeof(outFactors));
			float *outFactors = new float[mNumberOfSpeakers]();

            
            float factor = (r < 1) ? (r * 0.5f + 0.5f) : 1;
            
            for (int side = 0; side < 2; side++)
            {
                float tl = t - angle, tr = t + angle;
                
                if (tl < 0)
                {
                    Integrate(tl + kThetaMax, kThetaMax, areas, areaCount, outFactors, factor);
                    Integrate(0, tr, areas, areaCount, outFactors, factor);
                }
                else if (tr > kThetaMax)
                {
                    Integrate(tl, kThetaMax, areas, areaCount, outFactors, factor);
                    Integrate(0, tr - kThetaMax, areas, areaCount, outFactors, factor);
                }
                else
                {
                    Integrate(tl, tr, areas, areaCount, outFactors, factor);
                }
                
                if (r < 1)
                {
                    factor = 1 - factor;
                    t = (t < kHalfCircle) ? (t + kHalfCircle) : (t - kHalfCircle);
                }
                else break;
            }
            
            float total = 0;
            for (int o = 0; o < mNumberOfSpeakers; o++) total += outFactors[o];
            jassert(total > 0);
            
            float adj = tv / total;
            for (int o = 0; o < mNumberOfSpeakers; o++)
                if (outFactors[o])
                    addToOutput(s * outFactors[o] * adj, outputs, o, f);
			delete[] outFactors;
        }
    }
}

void SpatGrisAudioProcessor::ProcessDataFreeVolumeMode(float **inputs, float **outputs, float *params, float sampleRate, unsigned int frames)
{
	const float smooth = denormalize(kSmoothMin, kSmoothMax, params[kSmooth]); // milliseconds	
	const float sm_o = powf(0.01f, 1000.f / (smooth * sampleRate));
	const float sm_n = 1 - sm_o;
	
	// ramp all the parameters, except constant ones
	for (int i = 0; i < (kNumberOfParameters - kConstantParameters); i++)
	{
		float currentParam = mSmoothedParameters[i];
		float targetParam = params[i];
		float *ramp = mSmoothedParametersRamps.getReference(i).b;
	
		//float ori = currentParam;
		
		for (unsigned int f = 0; f < frames; f++)
		{
			currentParam = currentParam * sm_o + targetParam * sm_n;
			ramp[f] = currentParam;
		}
		
		//if (i == 0 && ori != currentParam) printf("param %i -> %f -> %f\n", i, ori, currentParam);
		
		mSmoothedParameters.setUnchecked(i, currentParam);
	}
	
	// in this context: T, R are actually X, Y
	
	// compute
	const float adj_factor = 1 / sqrtf(2);
	for (int o = 0; o < mNumberOfSpeakers; o++)
	{
		float *output = outputs[o];
		float *output_x = mSmoothedParametersRamps.getReference(getParamForSpeakerX(o)).b;
		float *output_y = mSmoothedParametersRamps.getReference(getParamForSpeakerY(o)).b;
		float output_adj[kChunkSize];
		{
			float *output_m = mSmoothedParametersRamps.getReference(getParamForSpeakerM(o)).b;
            //asdf
			//float *output_a = mSmoothedParametersRamps.getReference(getParamForSpeakerA(o)).b;
			
			for (unsigned int f = 0; f < frames; f++){
				float a = dbToLinear(kSpeakerDefaultAttenuation);
				float m = 1 - output_m[f];
				output_adj[f] = a * m;
			}
		}
		
		for (int i = 0; i < mNumberOfSources; i++)
		{
			float *input = inputs[i];
			float *input_x = mSmoothedParametersRamps.getReference(getParamForSourceX(i)).b;
			float *input_y = mSmoothedParametersRamps.getReference(getParamForSourceY(i)).b;
			float *input_d = mSmoothedParametersRamps.getReference(getParamForSourceD(i)).b;
			//DBG("input_d = " << *input_d);
			
			if (i == 0)
				for (unsigned int f = 0; f < frames; f++)
				{
					float dx = input_x[f] - output_x[f];
					float dy = input_y[f] - output_y[f];
					float d = sqrtf(dx*dx + dy*dy);
					float da = d * adj_factor * input_d[f];
					//if (da > 1) da = 0; else da = 1 - da;
					//da *= output_adj[f];
					//change distance scale for new calculations
					//"1" means far now instead of "0"
					if (da > 1) da = 1;
					//New distance vs amplitude calculations:
					//if da < 0.1 the log value will skyrocket:
					if (da < 0.1) da = 0.1;
					da = -log10f(da);
					da *= output_adj[f];
					
					output[f] = da * input[f];
				}
			else
				for (unsigned int f = 0; f < frames; f++)
				{
					float dx = input_x[f] - output_x[f];
					float dy = input_y[f] - output_y[f];
					float d = sqrtf(dx*dx + dy*dy);
					float da = d * adj_factor * input_d[f];
					//if (da > 1) da = 0; else da = 1 - da;
					//change distance scale for new calculations
					//"1" means far now instead of "0"
					if (da > 1) da = 1;
					//New distance vs amplitude calculations:
					//if da < 0.1 the log value will skyrocket:
					if (da < 0.1) da = 0.1;
					da = -log10f(da);
					da *= output_adj[f];
					
					output[f] += da * input[f];
				}
		}
	}
}

//==============================================================================
bool SpatGrisAudioProcessor::hasEditor() const
{
    return true;
}

AudioProcessorEditor* SpatGrisAudioProcessor::createEditor()
{
    return new SpatGrisAudioProcessorEditor (this, m_pMover.get());
}

//==============================================================================
static inline void appendIntData(MemoryBlock& destData, int32_t d)
{
	destData.append(&d, sizeof(d));
}
static inline int32_t readIntData(const void* &data, int &dataLength, int32_t defaultValue)
{
	int32_t r;
	if (dataLength >= sizeof(float))
	{
		const int32_t *pv = static_cast<const int32_t*>(data);
		r = *pv++;
		data = static_cast<const void*>(pv);
		dataLength -= sizeof(int32_t);
	}
	else
	{
		fprintf(stderr, "readIntData failed: not enough data...\n");
		r = defaultValue;
		dataLength = 0;
	}
	return r;
}
static inline void appendFloatData(MemoryBlock& destData, float d)
{
	destData.append(&d, sizeof(d));
}
static inline float readFloatData(const void* &data, int &dataLength, float defaultValue)
{
	float r;
	if (dataLength >= sizeof(float))
	{
		const float *pv = static_cast<const float*>(data);
		r = *pv++;
		data = static_cast<const void*>(pv);
		dataLength -= sizeof(float);
	}
	else
	{
		fprintf(stderr, "readFloatData failed: not enough data...\n");
		r = defaultValue;
		dataLength = 0;
	}
	return r;
}
static inline void appendStringData(MemoryBlock& destData, const char *d, int length) {
	destData.append(d, length);
}
static inline void readStringData(const void* &data, int &dataLength, const char *defaultValue, char *d, int length) {
	if (dataLength >= length) {
		const char *pv = static_cast<const char*>(data);
		memcpy(d, pv, length);
		d[length-1] = 0;
		data = static_cast<const void*>(pv + length);
		dataLength -= length;
	} else {
		fprintf(stderr, "readStringData failed: not enough data...\n");
		strlcpy(d, defaultValue, length);
		dataLength = 0;
	}
}

void SpatGrisAudioProcessor::storeCurrentLocations(){
    for (int i = 0; i < JucePlugin_MaxNumInputChannels; i++) {
        mBufferSrcLocX[i] = mParameters[getParamForSourceX(i)];
        mBufferSrcLocY[i] = mParameters[getParamForSourceY(i)];
        mBufferSrcLocD[i] = mParameters[getParamForSourceD(i)];
        mBufferSrcLocAS[i] = mParameters[getParamForSourceAzimSpan(i)];
        mBufferSrcLocES[i] = mParameters[getParamForSourceElevSpan(i)];

    }
    for (int i = 0; i < JucePlugin_MaxNumOutputChannels; i++) {
        mBufferSpLocX[i] =  mParameters[getParamForSpeakerX(i)];
        mBufferSpLocY[i] = mParameters[getParamForSpeakerY(i)];
        mBufferSpLocM[i] = mParameters[getParamForSpeakerM(i)];
    }
}
//p_iLocToRestore == -1 by default, meaning restore all locations
void SpatGrisAudioProcessor::restoreCurrentLocations(int p_iLocToRestore){
    if (p_iLocToRestore == -1){
        for (int i = 0; i < JucePlugin_MaxNumInputChannels; i++) {
            mParameters.set(getParamForSourceX(i), mBufferSrcLocX[i]);
            mParameters.set(getParamForSourceY(i), mBufferSrcLocY[i]);
            float fValue = mBufferSrcLocD[i];
            mParameters.set(getParamForSourceD(i), fValue);
        }
    } else {
        //only restore location for selected source
        int i = p_iLocToRestore;
        mParameters.set(getParamForSourceX(i), mBufferSrcLocX[i]);
        mParameters.set(getParamForSourceY(i), mBufferSrcLocY[i]);
        float fValue = mBufferSrcLocD[i];
        mParameters.set(getParamForSourceD(i), fValue);
    }
    
    for (int i = 0; i < JucePlugin_MaxNumOutputChannels; i++) {
        mParameters.set(getParamForSpeakerX(i), mBufferSpLocX[i]);
        mParameters.set(getParamForSpeakerY(i), mBufferSpLocY[i]);
		mParameters.set(getParamForSpeakerM(i), mBufferSpLocM[i]);
    }
}

static const int kDataVersion = 1;
void SpatGrisAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    XmlElement xml ("SPATGRIS_SETTINGS");
    
    xml.setAttribute ("kDataVersion", kDataVersion);
    xml.setAttribute ("mShowGridLines", mShowGridLines);
    xml.setAttribute ("mTrIndependentMode", mTrSeparateAutomationMode);
    xml.setAttribute ("m_iMovementMode", m_iMovementMode);
    xml.setAttribute ("mLinkSurfaceOrPan", mLinkSurfaceOrPan);
    xml.setAttribute ("mLinkAzimSpan", mLinkAzimSpan);
    xml.setAttribute ("mLinkElevSpan", mLinkElevSpan);
    xml.setAttribute ("mGuiWidth", mGuiWidth);
    xml.setAttribute ("mGuiHeight", mGuiHeight);
    xml.setAttribute ("mGuiTab", mGuiTab);
    xml.setAttribute ("mOscLeapSource", mOscLeapSource);
    xml.setAttribute ("mOscReceiveEnabled", mOscReceiveEnabled);
    xml.setAttribute ("mOscReceivePort", mOscReceivePort);
    xml.setAttribute ("mOscSendEnabled", mOscSendEnabled);
    xml.setAttribute ("mOscSendPort", mOscSendPort);
    xml.setAttribute ("mOscSendIp", mOscSendIp);
    xml.setAttribute ("mProcessMode", mProcessMode);
    xml.setAttribute ("mApplyFilter", mApplyFilter);
    xml.setAttribute ("mInputOutputMode", mInputOutputMode);
    xml.setAttribute ("mSrcPlacementMode", mSrcPlacementMode);
    xml.setAttribute ("mSpPlacementMode", mSpPlacementMode);
    xml.setAttribute ("mSrcSelected", mSrcSelected);
    xml.setAttribute ("mSpSelected", mSpSelected);
    xml.setAttribute ("mTrState", mTrState);
    xml.setAttribute ("m_iTrDirection", m_iTrDirection);
    xml.setAttribute ("m_iTrReturn", m_iTrReturn);
    xml.setAttribute ("m_iTrType", m_iTrType);
    xml.setAttribute ("m_iTrSrcSelect", m_iTrSrcSelect);
    xml.setAttribute ("m_fTrDuration", m_fTrDuration);
    xml.setAttribute ("m_iTrUnits", m_iTrUnits);
    xml.setAttribute ("m_fTrRepeats", m_fTrRepeats);
    xml.setAttribute ("m_fTrDampening", m_fTrDampening);
    xml.setAttribute ("m_fTrTurns", m_fTrTurns);
    xml.setAttribute ("m_fTrDeviation", m_fTrDeviation);
    xml.setAttribute ("m_fEndLocationX", m_fEndLocationXY01.first);
    xml.setAttribute ("m_fEndLocationY", m_fEndLocationXY01.second);
    xml.setAttribute ("mLeapEnabled", mLeapEnabled);
    xml.setAttribute ("kMaxSpanVolume", mParameters[kMaxSpanVolume]);
    xml.setAttribute ("kRoutingVolume", mParameters[kRoutingVolume]);
    xml.setAttribute ("mRoutingMode", mRoutingMode);
    xml.setAttribute ("kSmooth", mParameters[kSmooth]);
    xml.setAttribute ("kVolumeNear", mParameters[kVolumeNear]);
    xml.setAttribute ("kVolumeMid", mParameters[kVolumeMid]);
    xml.setAttribute ("kVolumeFar", mParameters[kVolumeFar]);
    xml.setAttribute ("kFilterNear", mParameters[kFilterNear]);
    xml.setAttribute ("kFilterMid", mParameters[kFilterMid]);
    xml.setAttribute ("kFilterFar", mParameters[kFilterFar]);
    xml.setAttribute ("m_iOscSpat1stSrcId", m_iOscSpat1stSrcId);
    xml.setAttribute ("m_iOscSpatPort", m_iOscSpatPort);
    
    for (int i = 0; i < JucePlugin_MaxNumInputChannels; ++i) {
		String srcX = "src" + to_string(i) + "x";
        xml.setAttribute (srcX, mParameters[getParamForSourceX(i)]);
        String srcY = "src" + to_string(i) + "y";
        xml.setAttribute (srcY, mParameters[getParamForSourceY(i)]);
        String srcD = "src" + to_string(i) + "d";
        xml.setAttribute (srcD, mParameters[getParamForSourceD(i)]);
        String srcAS = "src" + to_string(i) + "AS";
        xml.setAttribute (srcAS, mParameters[getParamForSourceAzimSpan(i)]);
        String srcES = "src" + to_string(i) + "ES";
        xml.setAttribute (srcES, mParameters[getParamForSourceElevSpan(i)]);
    }
    for (int i = 0; i < JucePlugin_MaxNumOutputChannels; ++i) {
        String spkX = "spk" + to_string(i) + "x";
        xml.setAttribute (spkX, mParameters[getParamForSpeakerX(i)]);
        String spkY = "spk" + to_string(i) + "y";
        xml.setAttribute (spkY, mParameters[getParamForSpeakerY(i)]);
        String spkM = "spk" + to_string(i) + "m";
        xml.setAttribute (spkM, mParameters[getParamForSpeakerM(i)]);
    }
    copyXmlToBinary (xml, destData);
}

void SpatGrisAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // This getXmlFromBinary() helper function retrieves our XML from the binary blob..
    ScopedPointer<XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr) {
        // make sure that it's actually our type of XML object..˝
        if (xmlState->hasTagName ("SPATGRIS_SETTINGS")) {
            int version         = xmlState->getIntAttribute ("kDataVersion", 1);
            if (version > kDataVersion){
                AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon, "SpatGRIS - Loading preset/state from newer version",
                                                  "You are attempting to load SpatGRIS with a preset from a newer version.\nDefault values will be used for all parameters.", "OK");
                return;
            }
            mShowGridLines      = xmlState->getIntAttribute ("mShowGridLines", 0);
            mTrSeparateAutomationMode  = xmlState->getIntAttribute ("mTrIndependentMode", mTrSeparateAutomationMode);
            setMovementMode(xmlState->getIntAttribute ("m_iMovementMode", 0));
            mLinkSurfaceOrPan   = xmlState->getIntAttribute ("mLinkSurfaceOrPan", 0);
            mLinkAzimSpan       = xmlState->getIntAttribute ("mLinkAzimSpan", 0);
            mLinkElevSpan       = xmlState->getIntAttribute ("mLinkElevSpan", 0);
            mGuiWidth           = xmlState->getIntAttribute ("mGuiWidth", kDefaultWidth);
            mGuiHeight          = xmlState->getIntAttribute ("mGuiHeight", kDefaultHeight);
            mGuiTab             = xmlState->getIntAttribute ("mGuiTab", 0);
            mOscLeapSource      = xmlState->getIntAttribute ("mOscLeapSource", 0);
            mOscReceiveEnabled  = xmlState->getIntAttribute ("mOscReceiveEnabled", 0);
            mOscReceivePort     = xmlState->getIntAttribute ("mOscReceivePort", 8000);
            mOscSendEnabled     = xmlState->getIntAttribute ("mOscSendEnabled", 0);
            mOscSendPort        = xmlState->getIntAttribute ("mOscSendPort", 9000);
            mOscSendIp          = xmlState->getStringAttribute ("mOscSendIp", mOscSendIp);
            mProcessMode        = xmlState->getIntAttribute ("mProcessMode", kPanVolumeMode);
            mApplyFilter        = xmlState->getIntAttribute ("mApplyFilter", 1);
            
            setInputOutputMode(xmlState->getIntAttribute ("mInputOutputMode", mInputOutputMode)+1);
            
            mSrcPlacementMode   = xmlState->getIntAttribute ("mSrcPlacementMode", 1);
            mSpPlacementMode    = xmlState->getIntAttribute ("mSpPlacementMode", 1);
            mSrcSelected        = xmlState->getIntAttribute ("mSrcSelected", 0);
            mSpSelected         = xmlState->getIntAttribute ("mSpSelected", 1);
            mTrState            = xmlState->getIntAttribute ("mTrState", 0);
            m_iTrDirection      = xmlState->getIntAttribute ("m_iTrDirection", 0);
            m_iTrReturn         = xmlState->getIntAttribute ("m_iTrReturn", 0);
            m_iTrType           = xmlState->getIntAttribute ("m_iTrType", 0);
            m_iTrSrcSelect      = xmlState->getIntAttribute ("m_iTrSrcSelect", 1);
            m_fTrDuration       = static_cast<float>(xmlState->getDoubleAttribute("m_fTrDuration", m_fTrDuration));
            m_iTrUnits          = xmlState->getIntAttribute ("m_iTrUnits", 0);
            m_fTrRepeats        = static_cast<float>(xmlState->getDoubleAttribute("m_fTrRepeats", m_fTrRepeats));
            m_fTrDampening      = static_cast<float>(xmlState->getDoubleAttribute("m_fTrDampening", m_fTrDampening));
            m_fTrDeviation      = static_cast<float>(xmlState->getDoubleAttribute("m_fTrDeviation", m_fTrDeviation));
            m_fTrTurns          = static_cast<float>(xmlState->getDoubleAttribute("m_fTrTurns", m_fTrTurns));
            m_fEndLocationXY01.first  = static_cast<float>(xmlState->getDoubleAttribute("m_fEndLocationX", m_fEndLocationXY01.first));
            m_fEndLocationXY01.second = static_cast<float>(xmlState->getDoubleAttribute("m_fEndLocationY", m_fEndLocationXY01.second));
            mLeapEnabled        = xmlState->getIntAttribute ("mLeapEnabled", 0);
            mParameters.set(kMaxSpanVolume, static_cast<float>(xmlState->getDoubleAttribute("kMaxSpanVolume", normalize(kMaxSpanVolumeMin, kMaxSpanVolumeMax, kMaxSpanVolumeDefault))));
            mParameters.set(kRoutingVolume, static_cast<float>(xmlState->getDoubleAttribute("kRoutingVolume", normalize(kRoutingVolumeMin, kRoutingVolumeMax, kRoutingVolumeDefault))));
            setRoutingMode(xmlState->getIntAttribute ("mRoutingMode", 0));
            mParameters.set(kSmooth,        static_cast<float>(xmlState->getDoubleAttribute("kSmooth", normalize(kSmoothMin, kSmoothMax, kSmoothDefault))));
            mParameters.set(kVolumeNear,    static_cast<float>(xmlState->getDoubleAttribute("kVolumeNear", normalize(kVolumeNearMin, kVolumeNearMax, kVolumeNearDefault))));
            mParameters.set(kVolumeMid,     static_cast<float>(xmlState->getDoubleAttribute("kVolumeMid", normalize(kVolumeMidMin, kVolumeMidMax, kVolumeMidDefault))));
            mParameters.set(kVolumeFar,     static_cast<float>(xmlState->getDoubleAttribute("kVolumeFar", normalize(kVolumeFarMin, kVolumeFarMax, kVolumeFarDefault))));
            mParameters.set(kFilterNear,    static_cast<float>(xmlState->getDoubleAttribute("kFilterNear", normalize(kFilterNearMin, kFilterNearMax, kFilterNearDefault))));
            mParameters.set(kFilterMid,     static_cast<float>(xmlState->getDoubleAttribute("kFilterMid", normalize(kFilterMidMin, kFilterMidMax, kFilterMidDefault))));
            mParameters.set(kFilterFar,     static_cast<float>(xmlState->getDoubleAttribute("kFilterFar", normalize(kFilterFarMin, kFilterFarMax, kFilterFarDefault))));
            m_iOscSpat1stSrcId  = xmlState->getIntAttribute("m_iOscSpat1stSrcId",   m_iOscSpat1stSrcId);
            m_iOscSpatPort      = xmlState->getIntAttribute("m_iOscSpatPort",       m_iOscSpatPort);
            for (int i = 0; i < JucePlugin_MaxNumInputChannels; ++i){
                String srcX = "src" + to_string(i) + "x";
                float fX01 = static_cast<float>(xmlState->getDoubleAttribute(srcX, 0));
                mParameters.set(getParamForSourceX(i), fX01);
                String srcY = "src" + to_string(i) + "y";
                float fY01 = static_cast<float>(xmlState->getDoubleAttribute(srcY, 0));
                mParameters.set(getParamForSourceY(i), fY01);
                FPoint curPoint = FPoint(fX01, fY01);
                mOldSrcLocRT[i] = convertXy012Rt(curPoint);
                String srcD = "src" + to_string(i) + "d";
                mParameters.set(getParamForSourceD(i), static_cast<float>(xmlState->getDoubleAttribute(srcD, normalize(kSourceMinDistance, kSourceMaxDistance, kSourceDefaultDistance))));
                String srcAS = "src" + to_string(i) + "AS";
                mParameters.set(getParamForSourceAzimSpan(i), static_cast<float>(xmlState->getDoubleAttribute(srcAS, 0)));
                String srcES = "src" + to_string(i) + "ES";
                mParameters.set(getParamForSourceElevSpan(i), static_cast<float>(xmlState->getDoubleAttribute(srcES, 0)));
            }
            for (int i = 0; i < JucePlugin_MaxNumOutputChannels; ++i){
                String spkX = "spk" + to_string(i) + "x";
                mParameters.set(getParamForSpeakerX(i), static_cast<float>(xmlState->getDoubleAttribute(spkX, 0)));
                String spkY = "spk" + to_string(i) + "y";
                mParameters.set(getParamForSpeakerY(i), static_cast<float>(xmlState->getDoubleAttribute(spkY, 0)));
                String spkM = "spk" + to_string(i) + "m";
                mParameters.set(getParamForSpeakerM(i), static_cast<float>(xmlState->getDoubleAttribute(spkM, 0)));
            }
        }
    }
    mHostChangedParameter++;
    mHostChangedProperty++;
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SpatGrisAudioProcessor();
}
