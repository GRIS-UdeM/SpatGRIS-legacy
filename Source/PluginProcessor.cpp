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
#include "Areas.h"
#include "Router.h"
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

#endif

class sourceParameter : public AudioParameterFloat
{
public :
    void setParent(SpatGrisAudioProcessor * f){ this->mFilter = f;}
    void setValue(float newValue){
        
        if (this->mFilter->getMovementMode() != Independent){
            for (int iCurSource = 0; iCurSource < this->mFilter->getNumberOfSources(); ++iCurSource){
                if (iCurSource != this->mFilter->getSelectedSrc()){
                    if (index == this->mFilter->getParamForSourceX(iCurSource) || index == this->mFilter->getParamForSourceY(iCurSource) ||
                        index == this->mFilter->getParamForSourceAzimSpan(iCurSource) || index == this->mFilter->getParamForSourceElevSpan(iCurSource)) {
                        return;
                    }
                }
            }
        }
        
    }
private :
    SpatGrisAudioProcessor * mFilter;
    int mSourceNumber;
};

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
        // allow the thread 2 seconds to stop cleanly - should be plenty of time.
        stopThread (2000);
    }
    
    void run() override {
        while (!threadShouldExit()) {
            if (!m_bBypass){
                m_pProcessor->threadUpdateNonSelectedSourcePositions();
            }
            wait (m_iInterval);
        }
    }
    void setThreadBypass(bool p_bBypass){
        m_bBypass = p_bBypass;
    }
    bool getBypass(){
        return m_bBypass;
    }
    
private:
    int m_iInterval;
    bool m_bBypass;
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
        // allow the thread 2 seconds to stop cleanly - should be plenty of time.
        stopThread (2000);
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
:m_bIsRecordingAutomation(false)
,m_iSourceLocationChanged(-1)
,m_iSourceAzimSpanChanged(-1)
,m_iSourceElevSpanChanged(-1)
,m_bPreventSourceLocationUpdate(false)
,m_bPreventSourceAzimElevSpanUpdate(false)
{

#if WIN32 & USE_LEAP
	//char winDir[MAX_PATH];							//will hold path of above dll
	//GetCurrentDirectory(sizeof(winDir), winDir);	//dll is in same dir as exe
	////strcat(winDir, "\\Leap.dll");					//concentrate dll name with path
	////HINSTANCE DLL = LoadLibrary(winDir);			//load example dll

	//char sysDir[MAX_PATH];
	//sysDir[0] = 0;
	//GetSystemDirectory(sysDir, (UINT)numElementsInArray(sysDir));
	//strcat(sysDir, "\\Leap.dll");
	//HINSTANCE DLL = LoadLibrary(sysDir);			//load example dll
	//HINSTANCE DLL = LoadLibrary("C:\\Users\\barth\\Documents\\Leap.dll");			//load example dll
	//DBG(sysDir);
#endif
 
    m_pOscSpatThread        = new OscSpatThread(this);
    m_pSourceUpdateThread   = new SourceUpdateThread(this);
    m_OwnedThreads.add(m_pOscSpatThread);
    m_OwnedThreads.add(m_pSourceUpdateThread);
    
   

    //SET PARAMETERS
	//mParameters.resize(kNumberOfParameters);
    for (int i = 0; i < kNumberOfParameters; i++){
        mParameters.push_back(new AudioParameterFloat("source "+to_string(i), "Source "+to_string(i),0.f, 1.f, 0));
    }
    mParameters.push_back(new AudioParameterFloat("kSmooth","KSmooth",kSmoothMin, kSmoothMax, kSmoothDefault));
    mParameters.push_back(new AudioParameterFloat("kVolumeNear","kVolumeNear",kVolumeNearMin, kVolumeNearMax, kVolumeNearDefault));
    mParameters.push_back(new AudioParameterFloat("kVolumeMid","kVolumeMid",kVolumeMidMin, kVolumeMidMax, kVolumeMidDefault));
    mParameters.push_back(new AudioParameterFloat("kVolumeFar","kVolumeFar",kVolumeFarMin, kVolumeFarMax, kVolumeFarDefault));
    mParameters.push_back(new AudioParameterFloat("kFilterNear","kFilterNear",kFilterNearMin, kFilterNearMax, kFilterNearDefault));
    mParameters.push_back(new AudioParameterFloat("kFilterMid","kFilterMid",kFilterMidMin, kFilterMidMax, kFilterMidDefault));
    mParameters.push_back(new AudioParameterFloat("kFilterFar","kFilterFar",kFilterFarMin, kFilterFarMax, kFilterFarDefault));
    mParameters.push_back(new AudioParameterFloat("kMaxSpanVolume","kMaxSpanVolume",kMaxSpanVolumeMin, kMaxSpanVolumeMax, kMaxSpanVolumeDefault));
    mParameters.push_back(new AudioParameterFloat("kRoutingVolume","kRoutingVolume",kRoutingVolumeMin, kRoutingVolumeMax, kRoutingVolumeDefault));
    
    for(AudioParameterFloat * t :mParameters){
        addParameter(t);
    }
   
    /*
	mParameters.set(kSmooth,        normalize(kSmoothMin, kSmoothMax, kSmoothDefault));
	mParameters.set(kVolumeNear,    normalize(kVolumeNearMin, kVolumeNearMax, kVolumeNearDefault));
	mParameters.set(kVolumeMid,     normalize(kVolumeMidMin, kVolumeMidMax, kVolumeMidDefault));
	mParameters.set(kVolumeFar,     normalize(kVolumeFarMin, kVolumeFarMax, kVolumeFarDefault));
	mParameters.set(kFilterNear,    normalize(kFilterNearMin, kFilterNearMax, kFilterNearDefault));
	mParameters.set(kFilterMid,     normalize(kFilterMidMin, kFilterMidMax, kFilterMidDefault));
	mParameters.set(kFilterFar,     normalize(kFilterFarMin, kFilterFarMax, kFilterFarDefault));
	mParameters.set(kMaxSpanVolume, normalize(kMaxSpanVolumeMin, kMaxSpanVolumeMax, kMaxSpanVolumeDefault));
	mParameters.set(kRoutingVolume, normalize(kRoutingVolumeMin, kRoutingVolumeMax, kRoutingVolumeDefault));*/
#if ALLOW_MVT_MODE_AUTOMATION
    addParameter(kMovementModeChoice  = new AudioParameterChoice("mouvement","Mouvement",AllMovementModesStrings,kMovementMode));
    //mParameters.set(kMovementMode,  normalize(kRoutingVolumeMin, kRoutingVolumeMax, kRoutingVolumeDefault));
#endif

	mSmoothedParameters.resize(kNumberOfParameters);
    for (int i = 0; i < kNumberOfParameters; ++i){
		mSmoothedParameters.add(0);
    }
    
    if (host.isLogic() || host.isReaper() || host.isAbletonLive() || host.isDigitalPerformer()){
		m_bAllowInputOutputModeSelection = true;
	} else {
		m_bAllowInputOutputModeSelection = false;
	}

    std::unique_ptr<SourceMover> pMover(new SourceMover(this));
    m_pMover = std::move(pMover);
    
    //SET SOURCES AND SPEAKERS
    int iSources  = getTotalNumInputChannels();
    if (iSources == 0){
        iSources = 2;
    }
    int iSpeakers = getTotalNumOutputChannels();
    if (iSpeakers == 0){
        iSpeakers = 2;
    }
    setNumberOfSources(iSources, true);
    setNumberOfSpeakers(iSpeakers, true);
    
	mCalculateLevels = false;
	mApplyFilter = true;
	mLinkSurfaceOrPan = false;
    mLinkAzimSpan = false;
    mLinkElevSpan = false;
#if ALLOW_MVT_MODE_AUTOMATION
	setMovementMode(0, false);
#else
    setMovementMode(0);
#endif
    
	mShowGridLines  = false;
    m_bOscActive    = true;
	mTrSeparateAutomationMode = false;
    mGuiWidth = kDefaultWidth,
    mGuiHeight = kDefaultHeight,
	mHostChangedParameterProcessor = 0;
	mHostChangedPropertyProcessor = 0;
    setGuiTab(0);
	mProcessCounter = 0;
	//mLastTimeInSamples = -1;
	setProcessMode(kPanVolumeMode);
#if ALLOW_INTERNAL_WRITE
	mRoutingMode = kNormalRouting;
#endif
    //version 9
    updateInputOutputMode();
    mSrcPlacementMode = 1;
    mSelectedSrc = 0;
    
    mSpPlacementMode = 1;
    mSpSelected = 1;
    
    mTrState = kTrReady;
    m_iTrType = 0;
    m_iTrDirection = 0;
    m_iTrReturn = 0;
    m_iTrSrcSelect = -1;//0;
    m_fTrDuration = 5.f;
    m_iTrUnits = 1;     //0 = beats, 1 = seconds
    m_fTrRepeats = 1.f;
    m_fTrDampening = 0.f;
    
    
    m_fTrTurns = 1.f;
    m_fTrDeviation = 0.f;
    m_fTrEllipseWidth = .25f;
    m_fEndLocationXY01 = FPoint(.5, .5);
    m_bIsSettingEndPoint = false;
    
    m_iOscSpat1stSrcId = 1;
    m_iOscSpatPort = 18032;
    m_sOscIpAddress = "127.0.0.1";

	mOscLeapSource = 0;
	mOscReceiveEnabled = 0;
	mOscReceivePort = 8000;
	mOscSendEnabled = 0;
	mOscSendPort = 9000;
	setOscSendIp("192.168.1.100");
	
    mLeapEnabled = 0;
    mJoystickEnabled = 0;
    m_bOscSpatSenderIsConnected = false;
	
	// default values for parameters
    for (int i = 0; i < JucePlugin_MaxNumInputChannels; i++){
        float fDefaultVal = normalize(kSourceMinDistance, kSourceMaxDistance, kSourceDefaultDistance);
        mParameters[getParamForSourceD(i)]->setValueNotifyingHost(fDefaultVal);
        //mParameters.set(getParamForSourceD(i), fDefaultVal);
    }

    for (int i = 0; i < JucePlugin_MaxNumOutputChannels; i++){
        mParameters[getParamForSpeakerM(i)]->setValueNotifyingHost(0);
        //mParameters.set(getParamForSpeakerM(i), 0);
    }
    
     m_pSourceUpdateThread->startThread();
//    mAllAreas.resize(kMaxChannels * MAX_AREAS);
}

SpatGrisAudioProcessor::~SpatGrisAudioProcessor() {
    if (mTrajectory){
        mTrajectory->stop(false);
        mTrajectory = nullptr;
    }
}

void SpatGrisAudioProcessor::bypassOrNotSourceUpdateThread(){
    //we don't want the thread to run when we only have 1 source, when we're recording automation (because then the regular move
    //will already move all sources according to movement constraints, and when we're in independent mode.
    if (m_bIsRecordingAutomation){
        m_pSourceUpdateThread->setThreadBypass(true);
    } else {
        if (mNumberOfSources == 1 || getMovementMode() == 0) {
            m_pSourceUpdateThread->setThreadBypass(true);
        } else {
            m_pSourceUpdateThread->setThreadBypass(false);
        }
    }
}

void SpatGrisAudioProcessor::threadUpdateNonSelectedSourcePositions(){
    int iSourceChanged = getSourceLocationChanged();
    if (getMovementMode() != 0 && iSourceChanged != -1){
        //with kSourceThread, these begin and end only set the movertype to kSourceThread, they don't start or end automations.
        //and since they are only called when a source was actually moved, it's fine to use (and block) the mover for that
        m_pMover->begin(iSourceChanged, kSourceThread);
        m_pMover->move(getSourceXY01(iSourceChanged), kSourceThread);
        m_pMover->end(kSourceThread);
        setSourceLocationChanged(-1);
    }

    int iSourceAzimSpanChanged = getSourceAzimSpanChanged();
    if (iSourceAzimSpanChanged != -1){
        if (mLinkAzimSpan){
            float fCurAzimSpan = getParameter(getParamForSourceAzimSpan(iSourceAzimSpanChanged));
            for (int i = 0; i < getNumberOfSources(); ++i) {
                if (i == iSourceAzimSpanChanged){
                    continue;
                }
                int paramIndex = getParamForSourceAzimSpan(i);
                if (getParameter(paramIndex) != fCurAzimSpan){
                    setParameterNotifyingHost(paramIndex, fCurAzimSpan);
                }
            }
        }
        setSourceAzimSpanChanged(-1);
    }
    
    int iSourceElevSpanChanged = getSourceElevSpanChanged();
    if (iSourceElevSpanChanged != -1){
        if (mLinkElevSpan){
            float fCurElevSpan = getParameter(getParamForSourceElevSpan(iSourceElevSpanChanged));
            for (int i = 0; i < getNumberOfSources(); ++i) {
                if (i == iSourceElevSpanChanged){
                    continue;
                }
                int paramIndex = getParamForSourceElevSpan(i);
                if (getParameter(paramIndex) != fCurElevSpan){
                    setParameterNotifyingHost(paramIndex, fCurElevSpan);
                }
            }
        }
        setSourceElevSpanChanged(-1);
    }
}

//==============================================================================
void SpatGrisAudioProcessor::setCalculateLevels(bool c) {
    JUCE_COMPILER_WARNING("what does this function do?")
#if USE_DB_METERS
    if (!mCalculateLevels && c){
        for (int i = 0; i < mNumberOfSpeakers; i++){
			mLevels.setUnchecked(i, 0);
        }
    }
#endif
    
	// keep count of number of editors
    mCalculateLevels = c;
    //(c) ? mCalculateLevels++ :  mCalculateLevels--;
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
        m_pOscSpatThread->stopThread(2000);
    }
}

void SpatGrisAudioProcessor::sendOscSpatValues(){
    if  (mProcessMode != kOscSpatMode || !m_bOscActive || !m_bOscSpatSenderIsConnected){
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
        if (!mOscSpatSender.send(message)) {
            DBG("Error: could not send OSC message.");
            return;
        }
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
/*
int SpatGrisAudioProcessor::getNumParameters() {
    return kNumberOfParameters;
}*/
/*
float SpatGrisAudioProcessor::getParameter (int index) {
    return mParameters[index];
}
*/

#if ALLOW_MVT_MODE_AUTOMATION
bool SpatGrisAudioProcessor::isNewMovementMode(float m_fNewValue){
    for (int iCurMode = 0; iCurMode < TotalNumberMovementModes; ++iCurMode) {
        float fCurMode = normalize(Independent, TotalNumberMovementModes-1, iCurMode);
        if (areSameStepParameterValues(m_fNewValue, fCurMode, TotalNumberMovementModes)){
            //m_fNewValue encodes the movement mode fCurMode. Is fCurMode the same as the currently selected movement mode?
            return !(areSame(fCurMode, getParameter(kMovementMode)));

        }
    }
    return false;
}
#endif

bool SpatGrisAudioProcessor::isKnownHost(){
    return (host.isLogic() || host.isReaper() || host.isAbletonLive() || host.isDigitalPerformer() ||
            host.isAdobeAudition() || host.isArdour() || host.isCubase() || host.isFruityLoops() ||
            host.isNuendo() || host.isSonar());
}

/*
void SpatGrisAudioProcessor::setParameter (int index, float newValue){
    //unknown host is logic's au eval tool
    if (!isKnownHost()){
        mParameters[index]->setValueNotifyingHost(newValue);
        //mParameters.set(index, newValue);
        return;
    }
    
    //return if mode is non-indedent and DAW is attempting to set position or azim/elev span for non-selected source
    if (getMovementMode() != Independent){
        for (int iCurSource = 0; iCurSource < getNumberOfSources(); ++iCurSource){
            if (iCurSource != getSelectedSrc()){
                if (index == getParamForSourceX(iCurSource) || index == getParamForSourceY(iCurSource) ||
                    index == getParamForSourceAzimSpan(iCurSource) || index == getParamForSourceElevSpan(iCurSource)) {
                    return;
                }
            }
        }
    }
    setParameterInternal (index, newValue);
}*/


void SpatGrisAudioProcessor::setParameterInternal (const int index, const float newValue){
    
    //unknown host is logic's au eval tool. we just shouldn't get here with that.
    if (!isKnownHost()){
        return;
    }
    
    float fOldValue = mParameters[(index)]->get();
        
#if ALLOW_MVT_MODE_AUTOMATION
    if (index == kMovementMode && !isNewMovementMode(newValue)){
        return;
    }
#endif
    if (!areSame(fOldValue, newValue)){
        if (newValue == 0 && isSourceLocationParameter(index)){
            DBG("#54: TRYING TO SET PARAMETER " << index << " " << getParameterName(index) << " TO ZERO");
            return;
        }
        mParameters[index]->setValueNotifyingHost(newValue);
        //mParameters.set(index, newValue);
        
#if ALLOW_MVT_MODE_AUTOMATION
        if (index == kMovementMode && m_pMover){
            m_pMover->storeAllDownPositions();
            bypassOrNotSourceUpdateThread();
        }
#endif

        if (!m_bPreventSourceLocationUpdate){
            for (int iCurSource = 0; iCurSource < getNumberOfSources(); ++iCurSource){
                if (index == getParamForSourceX(iCurSource) || index == getParamForSourceY(iCurSource)) {
                    setSourceLocationChanged(iCurSource);
                    break;
                }
            }
        }
        
        if (!m_bPreventSourceAzimElevSpanUpdate){
            for (int iCurSource = 0; iCurSource < getNumberOfSources(); ++iCurSource){
                if (index == getParamForSourceAzimSpan(iCurSource)){
                    setSourceAzimSpanChanged(iCurSource);
                    break;
                }
                if (index == getParamForSourceElevSpan(iCurSource)){
                    setSourceElevSpanChanged(iCurSource);
                    break;
                }
            }
        }
        ++mHostChangedParameterProcessor;
    }
}

bool SpatGrisAudioProcessor::isSourceLocationParameter(const int &index){
    for (int iCurSource = 0; iCurSource < mNumberOfSources; ++iCurSource) {
        if (index == getParamForSourceX(iCurSource) || index == getParamForSourceY(iCurSource)){
            return true;
        }
    }
    return false;
}


void SpatGrisAudioProcessor::setParameterNotifyingHost (int index, float newValue) {
    if (!isKnownHost()){
        //if in logic au test tool, return
        return;
    }
    mParameters[index]->setValueNotifyingHost(newValue);
    //mParameters.set(index, newValue);
    switch(index % kParamsPerSource) {
        case kSourceX:
        case kSourceY:
        case kSourceD:
        case kSourceAzimSpan:
        case kSourceElevSpan:
        case kTrajectorySpeed:
            ++mHostChangedParameterProcessor;
            break;
        default:
            break;
    }
#if ALLOW_MVT_MODE_AUTOMATION
    if (index == kMovementMode && m_pMover){
        m_pMover->storeAllDownPositions();
    }
#endif
    sendParamChangeMessageToListeners(index, newValue);
 
}

const String SpatGrisAudioProcessor::getParameterName (int index) {
   
	if (index == kSmooth)		return "Smooth Param";
    if (index == kVolumeNear)	return "Volume Near";
	if (index == kVolumeMid)	return "Volume Mid";
    if (index == kVolumeFar)	return "Volume Far";
	if (index == kFilterNear)	return "Filter Near";
	if (index == kFilterMid)	return "Filter Mid";
	if (index == kFilterFar)	return "Filter Far";
	if (index == kMaxSpanVolume)return "Max span volume";
	if (index == kRoutingVolume)return "Routing volume";
#if ALLOW_MVT_MODE_AUTOMATION
    if (index == kMovementMode) return "Movement Mode";
#endif
    if (index < mNumberOfSources * kParamsPerSource) {
		String s("Source ");
		s << (index / kParamsPerSource + 1);
		switch(index % kParamsPerSource) {
			case kSourceX:          s << " - X"; break;
			case kSourceY:          s << " - Y"; break;
			case kSourceD:          s << " - S"; break;
            case kSourceAzimSpan:   s << " - AS"; break;
            case kSourceElevSpan:   s << " - ES"; break;
            default: return String::empty;
		}
		return s;
	}
	index -= mNumberOfSources * kParamsPerSource;
	
    if (index < mNumberOfSpeakers * kParamsPerSpeakers) {
		String s("Speaker ");
		s << (index / kParamsPerSpeakers + 1);
		switch(index % kParamsPerSpeakers) {
            default: return String::empty;
		}
		return s;
	}
	
    return String::empty;
}
#if ALLOW_MVT_MODE_AUTOMATION
void SpatGrisAudioProcessor::setMovementMode(int i, bool p_bNotifyHost) {
    if (p_bNotifyHost){
        setParameterNotifyingHost(kMovementMode, normalize(kMovementModeMin, kMovementModeMax, i));
    } else {
        setParameterInternal(kMovementMode, normalize(kMovementModeMin, kMovementModeMax, i));
    }
}
#endif

void SpatGrisAudioProcessor::setInputOutputMode (int p_iInputOutputMode){
    if(mTrajectory){
        mTrajectory->stop();
    }
    const MessageManagerLock mmLock;            //prevents gui from running
    suspendProcessing (true);                   //prevents audio process thread from running
    mInputOutputMode = p_iInputOutputMode-1;
    switch (mInputOutputMode){
        case i1o1:
            setNumberOfSources(1, false);
            setNumberOfSpeakers(1, false);
            break;
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
        case i1o12:
            setNumberOfSources(1, false);
            setNumberOfSpeakers(12, false);
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
        case i2o12:
            setNumberOfSources(2, false);
            setNumberOfSpeakers(12, false);
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
        case i4o12:
            setNumberOfSources(4, false);
            setNumberOfSpeakers(12, false);
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
        case i6o12:
            setNumberOfSources(6, false);
            setNumberOfSpeakers(12, false);
            break;
        case i6o16:
            setNumberOfSources(6, false);
            setNumberOfSpeakers(16, false);
            break;
        case i8o8:
            setNumberOfSources(8, false);
            setNumberOfSpeakers(8, false);
            break;
        case i8o12:
            setNumberOfSources(8, false);
            setNumberOfSpeakers(12, false);
            break;
        case i8o16:
            setNumberOfSources(8, false);
            setNumberOfSpeakers(16, false);
            break;
        case i12o12:
            setNumberOfSources(12, false);
            setNumberOfSpeakers(12, false);
            break;
        default:
            jassertfalse;
    }
    //restart audio processing
    suspendProcessing (false);

}

void SpatGrisAudioProcessor::updateInputOutputMode (){
    if (mNumberOfSources == 1 && mNumberOfSpeakers == 1){
        mInputOutputMode =  i1o1;
        return;
    } else if (mNumberOfSources == 1 && mNumberOfSpeakers == 2){
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
    } else if (mNumberOfSources == 1 && mNumberOfSpeakers == 12){
        mInputOutputMode =  i1o12;
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
    } else if (mNumberOfSources == 2 && mNumberOfSpeakers == 12){
        mInputOutputMode =  i2o12;
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
    } else if (mNumberOfSources == 4 && mNumberOfSpeakers == 12){
        mInputOutputMode =  i4o12;
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
    } else if (mNumberOfSources == 6 && mNumberOfSpeakers == 12){
        mInputOutputMode =  i6o12;
        return;
    } else if (mNumberOfSources == 6 && mNumberOfSpeakers == 16){
        mInputOutputMode =  i6o16;
        return;
    } else if (mNumberOfSources == 8 && mNumberOfSpeakers == 8){
        mInputOutputMode =  i8o8;
        return;
    } else if (mNumberOfSources == 8 && mNumberOfSpeakers == 12){
        mInputOutputMode =  i8o12;
        return;
    } else if (mNumberOfSources == 8 && mNumberOfSpeakers == 16){
        mInputOutputMode =  i8o16;
        return;
    }
    else if (mNumberOfSources == 12 && mNumberOfSpeakers == 12){
        mInputOutputMode =  i12o12;
        return;
    }
    
    jassertfalse;
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
    }
    mNumberOfSources = p_iNewNumberOfSources;

    m_pMover->updateNumberOfSources();
    if (bUseDefaultValues){
        double anglePerSource = 360 / mNumberOfSources;
        double offset, axisOffset;
        if (mNumberOfSources == 1){
            setSourceRT(0, FPoint(0, 0));
            m_pMover->storeDownPosition(0, FPoint(0,0));
            storeCurrentLocations();
        } else if (mNumberOfSources % 2 == 0) {//if the number of sources is even we will assign them as stereo pairs
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
                m_pMover->storeDownPosition(i, FPoint(kSourceDefaultRadius, offset/360*kThetaMax));
            }
        } else {    //odd number of speakers, assign in circular fashion
            offset = (anglePerSource + 180) / 2 - anglePerSource;
            for (int i = 0; i < mNumberOfSources; i++) {
                if (offset < 0) offset += 360;
                else if (offset > 360) offset -= 360;
                
                setSourceRT(i, FPoint(kSourceDefaultRadius, offset/360*kThetaMax));
                m_pMover->storeDownPosition(i, FPoint(kSourceDefaultRadius, offset/360*kThetaMax));
                offset += anglePerSource;
            }
        }
    }
    for (int i = 0; i < mNumberOfSources; i++){
        mLockedThetas.set(i, getSourceRT(i).y);
        mPrevRs.set(i, getSourceRT(i).x);
        mPrevTs.set(i, getSourceRT(i).y);
    }
    
    mHostChangedParameterProcessor++;
    bypassOrNotSourceUpdateThread();
}

void SpatGrisAudioProcessor::setNumberOfSpeakers(int p_iNewNumberOfSpeakers, bool bUseDefaultValues){
    
    mNumberOfSpeakers = p_iNewNumberOfSpeakers;
#if ALLOW_INTERNAL_WRITE
    if (mRoutingMode == kInternalWrite) {
        updateRoutingTempAudioBuffer();
    }
#endif
#if USE_DB_METERS
    mLevels.resize(mNumberOfSpeakers);
    for (int i = 0; i < mNumberOfSpeakers; i++){
        mLevels.add(0);
    }
#endif
    if (bUseDefaultValues){
        updateSpeakerLocation(true, false, false);
    }

#if OUTPUT_RAMPING
#if USE_VECTORS
    mSpeakerVolumes.clear();
    for (int i = 0; i < mNumberOfSources; i++) {
        mSpeakerVolumes.add(Array<float>());
        for (int j = 0; j < mNumberOfSpeakers; j++){
            mSpeakerVolumes[j].add(0);
        }
    }
#else
    for (int i = 0; i < mNumberOfSources; i++) {
        for (int j = 0; j < mNumberOfSpeakers; j++){
            mSpeakerVolumes[i][j] = 0.f;
        }
    }
#endif
#endif
    
    mHostChangedParameterProcessor++;
}

#if ALLOW_INTERNAL_WRITE
void SpatGrisAudioProcessor::updateRoutingTempAudioBuffer() {
	mRoutingTempAudioBuffer.setSize(mNumberOfSpeakers, kMaxBufferSize);
}
#endif

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

int SpatGrisAudioProcessor::getParameterNumSteps (int index){
    return getDefaultNumParameterSteps();
}

const String SpatGrisAudioProcessor::getParameterText (int index)
{
    switch (index) {
#if ALLOW_MVT_MODE_AUTOMATION
        case kMovementMode:
            return GetMovementModeName(getMovementMode());
            break;
#endif
            
        default:
            return String::empty;
    }

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

void SpatGrisAudioProcessor::reset() {
#if USE_DB_METERS
    if (mCalculateLevels){
        for (int i = 0; i < mNumberOfSpeakers; i++){
            mLevels.setUnchecked(i, 0);
        }
    }
#endif
    
    for (int i = 0; i < mNumberOfSources; ++i) {
        mFilters[i].reset();
        for (int j = 0; j < mNumberOfSpeakers; ++j){
#if OUTPUT_RAMPING
#if USE_VECTORS
            mSpeakerVolumes.getReference(i).set(j, 0);
#else
            mSpeakerVolumes[i][j] = 0.f;
#endif
#endif
        }
    }
    
    Router::instance().reset();
}

//==============================================================================
void SpatGrisAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {

    m_dSampleRate    = sampleRate;
    m_iDawBufferSize = samplesPerBlock;
    
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
        setNumberOfSources (getTotalNumInputChannels(),  true);
        setNumberOfSpeakers(getTotalNumOutputChannels(), true);
    }
    
    for (int i = 0; i < mNumberOfSources; i++) {
        mFilters[i].setSampleRate(static_cast<int>(m_dSampleRate));
    }

#if TIME_PROCESS
    DBG("SPATgris\ntrajectories\tparamCopy\tprepareSrcSpk\ttotProcesData\tAvgParamRamp\tAvgFilter\tAvgVolume\tAvgSpatial\tAvgAddOutputs\tDbMeters");
#endif
    
    //---------- INIT MEMORY STUFF -------
    memcpy (mSmoothedParameters.getRawDataPointer(), &mParameters, kNumberOfParameters * sizeof(float));

    updateInputOutputRampsSizes();
}

void SpatGrisAudioProcessor::updateInputOutputRampsSizes(){
#if USE_VECTORS
    //resize parameter ramps
    mParameterRamps.resize(kNumberOfParameters);
    for (auto &curParameterRamp : mParameterRamps){
        curParameterRamp.resize(m_iDawBufferSize);
    }
    //resize inputcopy and outputs
    for (auto &curInput : mInputsCopy){
        curInput.resize(m_iDawBufferSize);
    }
#endif
}


void SpatGrisAudioProcessor::releaseResources() {
#if USE_VECTORS
    mParameterRamps.clear();
#endif
}

void SpatGrisAudioProcessor::processBlockBypassed (AudioBuffer<float> &buffer, MidiBuffer& midiMessages)
{
}

void SpatGrisAudioProcessor::processBlock(AudioBuffer<float> &pBuffer, MidiBuffer& midiMessages) {
#if TIME_PROCESS
    Time beginTime = Time::getCurrentTime();
#endif

    //==================================== CHECK SOME STUFF ===========================================
	// sanity check for auval
#if ALLOW_INTERNAL_WRITE
	if (pBuffer.getNumChannels() < ((mRoutingMode == kInternalWrite) ? mNumberOfSources : jmax(mNumberOfSources, mNumberOfSpeakers))) {
#else
    if (pBuffer.getNumChannels() < jmax(mNumberOfSources, mNumberOfSpeakers)) {
#endif
		printf("unexpected channel count %d vs %dx%d rmode: %d\n", pBuffer.getNumChannels(), mNumberOfSources, mNumberOfSpeakers, mRoutingMode);
		return;
	}
    //if we're in any of the internal READ modes, copy stuff from Router into buffer and return
#if ALLOW_INTERNAL_WRITE
	if (mProcessMode != kOscSpatMode && mRoutingMode >= kInternalRead12) {
		pBuffer.clear();
        //maximum number of output channels when writing to internal is 2. Higher outputs will be ignored
		int outChannels = (mNumberOfSpeakers > 2) ? 2 : mNumberOfSpeakers;
        //here, e.g., internalRead12 = 2, so offset = 0, internalRead34 = 3, so offset = 2; internalRead45 = 4 so offset = 4
		int offset = (mRoutingMode - 2) * 2;
        //for every output channel
		for (int c = 0; c < outChannels; c++) {
            //copy oriFramesToProcess samples from the router's channel (offset+c) into buffer channel c, starting at sample 0
			pBuffer.copyFrom(c, 0, Router::instance().outputBuffers(m_iDawBufferSize)[offset + c], m_iDawBufferSize);
			Router::instance().clear(offset + c);
		}
		return;
	}
#endif

    //==================================== PROCESS TRAJECTORIES ===========================================
    processTrajectory();

#if TIME_PROCESS
    Time time1Trajectories = Time::getCurrentTime();
#endif
    
    //if we're in osc spat mode, return and don't process any audio
    if (mProcessMode == kOscSpatMode) {
        return;
    }
	
    //==================================== COPY ALL PARAMETERS INTO PARAMCOPY ===========================================
	// copy mParameters into paramCopy, because we will transform those
	float paramCopy[kNumberOfParameters];
    memcpy (paramCopy, &mParameters, kNumberOfParameters * sizeof(float));
    
    //depending on what mode we are, denormalize parameters we will need
	if (mProcessMode != kFreeVolumeMode) {
		paramCopy[kVolumeNear] = denormalize(kVolumeNearMin, kVolumeNearMax, paramCopy[kVolumeNear]);
		paramCopy[kVolumeMid]  = denormalize(kVolumeMidMin,  kVolumeMidMax,  paramCopy[kVolumeMid]);
		paramCopy[kVolumeFar]  = denormalize(kVolumeFarMin,  kVolumeFarMax,  paramCopy[kVolumeFar]);
		paramCopy[kFilterNear] = denormalize(kFilterNearMin, kFilterNearMax, paramCopy[kFilterNear]);
		paramCopy[kFilterMid]  = denormalize(kFilterMidMin,  kFilterMidMax,  paramCopy[kFilterMid]);
		paramCopy[kFilterFar]  = denormalize(kFilterFarMin,  kFilterFarMax,  paramCopy[kFilterFar]);
	}
	if (mProcessMode == kPanSpanMode) {
		paramCopy[kMaxSpanVolume] = denormalize(kMaxSpanVolumeMin, kMaxSpanVolumeMax, paramCopy[kMaxSpanVolume]);
	}
#if ALLOW_INTERNAL_WRITE
	if (mRoutingMode == kInternalWrite) {
		paramCopy[kRoutingVolume] = denormalize(kRoutingVolumeMin, kRoutingVolumeMax, paramCopy[kRoutingVolume]);
        jassert(mRoutingTempAudioBuffer.getNumSamples() >= m_iDawBufferSize);
        jassert(mRoutingTempAudioBuffer.getNumChannels() >= mNumberOfSpeakers);
	}
#endif
    
#if TIME_PROCESS
    Time time2ParamCopy = Time::getCurrentTime();
#endif
    
    //==================================== PREPARE SOURCE AND SPEAKER PARAMETERS ===========================================
    if (m_iDawBufferSize != pBuffer.getNumSamples()){
        m_iDawBufferSize = pBuffer.getNumSamples();
        updateInputOutputRampsSizes();
    }
        
    if (mNumberOfSpeakers < pBuffer.getNumChannels()){
        for (int i = mNumberOfSpeakers; i < pBuffer.getNumChannels(); ++i) {
            memset(pBuffer.getWritePointer(i), 0, m_iDawBufferSize * sizeof(float));
        }
    }
    
    for (int iCurChannel = 0; iCurChannel < mNumberOfSpeakers; ++iCurChannel) {
        if (iCurChannel < mNumberOfSources){
#if USE_VECTORS
            vector<float> &curInput = mInputsCopy[iCurChannel];
            for (int iCurSample = 0; iCurSample < m_iDawBufferSize; ++iCurSample){
                curInput[iCurSample] = pBuffer.getSample(iCurChannel, iCurSample);
            }

//            mInputsCopy[iCurChannel].assign(pBuffer.getReadPointer(iCurChannel), pBuffer.getReadPointer(iCurChannel) + m_iDawBufferSize);
#else
            jassert(m_iDawBufferSize <= kMaxBufferSize);
            memcpy(mInputsCopy[iCurChannel], pBuffer.getWritePointer(iCurChannel), m_iDawBufferSize * sizeof(float));
#endif
            //denormalize source position
            paramCopy[getParamForSourceX(iCurChannel)] = paramCopy[getParamForSourceX(iCurChannel)] * (2*kRadiusMax) - kRadiusMax;
            paramCopy[getParamForSourceY(iCurChannel)] = paramCopy[getParamForSourceY(iCurChannel)] * (2*kRadiusMax) - kRadiusMax;
        }
        
        //if we're in internal write, get pointer to audio data from mRoutingTempAudioBuffer, otherwise get it from pBuffer
#if ALLOW_INTERNAL_WRITE
        if (mRoutingMode == kInternalWrite){
            mOutputs[iCurChannel] = mRoutingTempAudioBuffer.getWritePointer(iCurChannel);
        } else {
#endif
            //copy pointers to pBuffer[mNumberOfSources][DAW buffer size] into mOutputs[mNumberOfSources]
            mOutputs[iCurChannel] = pBuffer.getWritePointer(iCurChannel);
#if ALLOW_INTERNAL_WRITE
        }
#endif
        if (mProcessMode == kFreeVolumeMode) {
            if (iCurChannel < mNumberOfSources) {paramCopy[getParamForSourceD(iCurChannel)] = denormalize(kSourceMinDistance, kSourceMaxDistance, paramCopy[getParamForSourceD(iCurChannel)]);}
            //in free volume, speakers can be anywhere, so we have an x and a y
            paramCopy[getParamForSpeakerX(iCurChannel)] = paramCopy[getParamForSpeakerX(iCurChannel)] * (2*kRadiusMax) - kRadiusMax;
            paramCopy[getParamForSpeakerY(iCurChannel)] = paramCopy[getParamForSpeakerY(iCurChannel)] * (2*kRadiusMax) - kRadiusMax;
        } else {
            //in non-free volume, speakers are bounded to the unit circle, so we only need their angle on that circle, which we store in params[getParamForSpeakerX(iCurOutput)]
            float x = paramCopy[getParamForSpeakerX(iCurChannel)] * (2*kRadiusMax) - kRadiusMax;
            float y = paramCopy[getParamForSpeakerY(iCurChannel)] * (2*kRadiusMax) - kRadiusMax;
            float t = atan2f(y, x);
            if (t < 0) t += kThetaMax;
            paramCopy[getParamForSpeakerX(iCurChannel)] = t;
        }
    }

#if TIME_PROCESS
    Time time3SourceSpeakers = Time::getCurrentTime();
#endif
    ProcessData(paramCopy);
    
#if TIME_PROCESS
    Time time4ProcessData = Time::getCurrentTime();
#endif
    
    //==================================== INTERNAL WRITE STUFF ===========================================
#if ALLOW_INTERNAL_WRITE
    if (mRoutingMode == kInternalWrite){
		// apply routing volume
		float currentParam  = mSmoothedParameters[kRoutingVolume];
		float targetParam   = paramCopy[kRoutingVolume];
#if USE_VECTORS
        float *ramp         = mParameterRamps[kRoutingVolume].data();
#else
        float *ramp         = mParameterRamps[kRoutingVolume];
#endif
		const float smooth  = denormalize(kSmoothMin, kSmoothMax, paramCopy[kSmooth]); // milliseconds
		const float sm_o    = powf(0.01f, 1000.f / (smooth * m_dSampleRate));
		const float sm_n    = 1 - sm_o;
		for (unsigned int f = 0; f < m_iDawBufferSize; f++) {
			currentParam  = currentParam * sm_o + targetParam * sm_n;
			ramp[f]       = dbToLinear(currentParam);
		}
		mSmoothedParameters.setUnchecked(kRoutingVolume, currentParam);
		for (int o = 0; o < mNumberOfSpeakers; o++) {
			float *output = mRoutingTempAudioBuffer.getWritePointer(o);
            for (unsigned int f = 0; f < m_iDawBufferSize; f++){
				output[f] *= ramp[f];
            }
		}
	}
#endif
    
#if USE_DB_METERS
    //==================================== DB METER STUFF ===========================================
	if (mCalculateLevels) {
        //envelope constants
		const float attack  = kLevelAttackDefault;
		const float release = kLevelReleaseDefault;
		const float ag      = powf(0.01f, 1000.f / (attack * m_dSampleRate));
		const float rg      = powf(0.01f, 1000.f / (release * m_dSampleRate));
		//for each speaker
		for (int o = 0; o < mNumberOfSpeakers; o++) {
            //get pointer to current spot in output buffer
			float *output = mOutputs[o];
			float env = mLevels[o];
            
			//for each frame that are left to process
            for (unsigned int f = 0; f < m_iDawBufferSize; f++) {
                //figure out enveloppe level
				float s = fabsf(output[f]);
				float g = (s > env) ? ag : rg;
				env = g * env + (1.f - g) * s;
			}
			mLevels.setUnchecked(o, env);
		}
	}
#endif
#if ALLOW_INTERNAL_WRITE
	if (mRoutingMode == kInternalWrite) {
		Router::instance().accumulate(mNumberOfSpeakers, m_iDawBufferSize, mRoutingTempAudioBuffer);
		pBuffer.clear();
	}
#endif
	
    //this is only used for the level components, ie the db meters
	mProcessCounter++;

#if TIME_PROCESS
    Time time5DbMeters = Time::getCurrentTime();
    int n = 50;
    mAvgTime[0] += (time1Trajectories     - beginTime).inMilliseconds()/(float)n;
    mAvgTime[1] += (time2ParamCopy        - time1Trajectories).inMilliseconds()/(float)n;
    mAvgTime[2] += (time3SourceSpeakers   - time2ParamCopy).inMilliseconds()/(float)n;
    mAvgTime[3] += (time4ProcessData      - time3SourceSpeakers).inMilliseconds()/(float)n;
    
    //from processData
    mAvgTime[4] += timeAvgParamRamp/(float)n;
    mAvgTime[5] += timeAvgFilter/(float)n;
    mAvgTime[6] += timeAvgVolume/(float)n;
    mAvgTime[7] += timeAvgSpatial/(float)n;
    mAvgTime[8] += timeAvgOutputs/(float)n;
    timeAvgParamRamp     = 0.f;
    timeAvgFilter   = 0.f;
    timeAvgVolume   = 0.f;
    timeAvgSpatial  = 0.f;
    timeAvgOutputs  = 0.f;
    mAvgTime[9] += (time5DbMeters         - time4ProcessData).inMilliseconds()/(float)n;
        
    if (mProcessCounter % n == 0){
        for (int i = 3; i < kTimeSlots; ++i){
            cout << mAvgTime[i] << "\t";
            mAvgTime[i] = 0;
        }
        cout << "\n";
    }
#endif
}

void SpatGrisAudioProcessor::processTrajectory(){
    //check whether we're currently playing
    AudioPlayHead::CurrentPositionInfo cpi;
    getPlayHead()->getCurrentPosition(cpi);
    m_bIsPlaying = cpi.isPlaying;
    
    // process trajectory if there is one going on

    if (mTrajectory) {
        if (m_bIsPlaying) {
            mTrajectory->setSpeed(getParameter(kTrajectorySpeed));
            double bps = cpi.bpm / 60;
            float seconds = m_iDawBufferSize / m_dSampleRate;
            float beats = seconds * bps;
            
            bool done = mTrajectory->process(seconds, beats);
            if (done){
                //mTrajectory.~ReferenceCountedObjectPtr();
                mTrajectory = nullptr;
            }
        }
    }
}
                       
void SpatGrisAudioProcessor::ProcessData(float *params) {
	switch(mProcessMode) {
        case kFreeVolumeMode:	ProcessDataFree(params);	break;
        case kPanVolumeMode:	ProcessDataPan (params);	break;
        case kPanSpanMode:		ProcessDataSpan(params);	break;
        default: jassertfalse;
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
        if (fCurSpeakerAngle >= p_fTargetAngle){
            //check if curAngle is smaller than previous smallest left angle
            fCurDeltaAngle = fCurSpeakerAngle - p_fTargetAngle;
            if (fCurDeltaAngle < p_pfDeltaAngleToLeftSpeaker){
                p_pfDeltaAngleToLeftSpeaker = fCurDeltaAngle;
                p_piLeftSpeaker = iCurSpeaker;
            }
            
        }
        //if curSpeaker is on right of target speaker
        else {
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
}

#if OUTPUT_RAMPING
void SpatGrisAudioProcessor::setSpeakerVolume(const int &source, const float &targetVolume, const float &sm_o, const int &o, vector<bool> *p_pvSpeakersCurrentlyInUse) {
#if USE_VECTORS
    mSpeakerVolumes.getReference(source).set(o, sm_o * mSpeakerVolumes[source][o] + (1-sm_o) * targetVolume);     // with exp. smoothing on volume
    if (p_pvSpeakersCurrentlyInUse){
        p_pvSpeakersCurrentlyInUse->at(o) = true;
    }
#else
    mSpeakerVolumes[source][o] = sm_o * mSpeakerVolumes[source][o] + (1-sm_o) * targetVolume;     // with exp. smoothing on volume
    if (p_pvSpeakersCurrentlyInUse){
        p_pvSpeakersCurrentlyInUse->at(o) = true;
    }
#endif
}
#endif

#if OUTPUT_RAMPING
void SpatGrisAudioProcessor::addToOutputs(const int &source, const float &sample, const int &f) {
#if USE_VECTORS
    const Array<float> &volumes = mSpeakerVolumes[source];
    for (int o = 0; o < mNumberOfSpeakers; ++o) {
        float m = 1 - mParameterRamps[getParamForSpeakerM(o)][f];
        mOutputs[o][f] += sample * volumes[o] * m;
    }
#else
    const float* volumes = mSpeakerVolumes[source];
    for (int o = 0; o < mNumberOfSpeakers; ++o) {
        float m = 1 - mParameterRamps[getParamForSpeakerM(o)][f];
        mOutputs[o][f] += sample * volumes[o] * m;
    }
#endif
}
#endif
    
void SpatGrisAudioProcessor::addToOutput (const float &sample, const int &speaker, const int &f){
    float m = 1 - mParameterRamps[getParamForSpeakerM(speaker)][f];
    mOutputs[speaker][f] += sample * m;
}
    
void SpatGrisAudioProcessor::createParameterRamps(float *p_pfParamCopy, const float &fOldValuesPortion){
    
    //init stuff
    const int kiTotalSourceParameters   = JucePlugin_MaxNumInputChannels  * kParamsPerSource;
    const int kiTotalSpeakerParameters  = JucePlugin_MaxNumOutputChannels * kParamsPerSpeakers;
    const float fNewValuePortion        = 1 - fOldValuesPortion;
    
    //for each kNonConstantParameters parameter, ie, IDs 0 to 120
    for (int iCurParamId = 0; iCurParamId < kNonConstantParameters; ++iCurParamId) {
        //skip all parameters but those: for each source (kSourceX,kSourceY,kSourceD,kSourceAzimSpan,kSourceElevSpan) and for each speaker kSpeakerM
        if (iCurParamId >= kiTotalSourceParameters && iCurParamId < (kiTotalSourceParameters + kiTotalSpeakerParameters) && (
          ((iCurParamId - kiTotalSourceParameters) % kParamsPerSpeakers) == kSpeakerX ||
          ((iCurParamId - kiTotalSourceParameters) % kParamsPerSpeakers) == kSpeakerY ||
          ((iCurParamId - kiTotalSourceParameters) % kParamsPerSpeakers) == kSpeakerUnused1 ||
          ((iCurParamId - kiTotalSourceParameters) % kParamsPerSpeakers) == kSpeakerUnused2)){
            continue;
        }
        
        //mSmoothedParameters contains the old parameter value, p_pfParamCopy contains the target value
        float currentParamValue = mSmoothedParameters[iCurParamId];
        float targetParamValue  = p_pfParamCopy[iCurParamId];
        
        for (unsigned int iCurSampleId = 0; iCurSampleId < m_iDawBufferSize; ++iCurSampleId) {
            //mParameterRamps contains an asymptotic interpolation between the current and target values, ramped over all m_iDawBufferSize values
            if (!areSame(currentParamValue, targetParamValue)){
                currentParamValue = currentParamValue * fOldValuesPortion + targetParamValue * fNewValuePortion;
            } else {
                currentParamValue = targetParamValue;
            }
            mParameterRamps[iCurParamId][iCurSampleId] = currentParamValue;
        }
        mSmoothedParameters.setUnchecked(iCurParamId, currentParamValue);    //store old value for next time
    }
}
    
    //sizes are p_ppfInputs[mNumberOfSources][p_iTotalSamples] and p_ppfOutputs[mNumberOfSpeakers][p_iTotalSamples], and p_pfParams[kNumberOfParameters];
void SpatGrisAudioProcessor::ProcessDataPan(float *p_pfParamCopy) {
    // clear mOutputs[]
    for (int iCurOutput = 0; iCurOutput < mNumberOfSpeakers; ++iCurOutput) {
        float *output = mOutputs[iCurOutput];
        memset(output, 0, m_iDawBufferSize * sizeof(float));
    }
    
    //if a given speaker is currently in use, we flag it in here, so that we know which speakers are not in use and can set their output to 0
#if OUTPUT_RAMPING
    vector<bool> vSpeakersCurrentlyInUse;
#endif
    
    //------------------------------- DISTRIBUTE PARAMETER CHANGE OVER SAMPLES IN THE BUFFER ------------------------------------------
    //kSmooth is in ms. when multiplied by sampling rate (samples/s), fSmoothingSamples is a number of samples over which we will smooth. it is only an approximation since the curve is exponential
    float fSmoothingSamples = denormalize(kSmoothMin, kSmoothMax, p_pfParamCopy[kSmooth]) * m_dSampleRate;
    const float fOldValuesPortion = powf(0.01f, 1000.f / fSmoothingSamples);
    createParameterRamps(p_pfParamCopy, fOldValuesPortion);
    
    
	//------------------------------- FOR EACH SOUND SOURCE ------------------------------------------
	for (int iCurSource = 0; iCurSource < mNumberOfSources; ++iCurSource) {
#if USE_VECTORS
        float *xCurSource = mParameterRamps[getParamForSourceX(iCurSource)].data();
        float *yCurSource = mParameterRamps[getParamForSourceY(iCurSource)].data();
#else
        float *xCurSource = mParameterRamps[getParamForSourceX(iCurSource)];
        float *yCurSource = mParameterRamps[getParamForSourceY(iCurSource)];
#endif
        
        //------------------------------- FOR EACH SAMPLE ------------------------------------------
		for (unsigned int iSampleId = 0; iSampleId < m_iDawBufferSize; ++iSampleId) {
#if TIME_PROCESS
            Time timeBeginSample = Time::getCurrentTime();
#endif

#if OUTPUT_RAMPING
            //reset vSpeakersCurrentlyInUse
            vSpeakersCurrentlyInUse.assign(mNumberOfSpeakers,false);
#endif
            
            //figure out current sample value and its Ray and Theta coordinates

            float fCurSampleValue    = mInputsCopy[iCurSource][iSampleId];   //current sample
            float fCurSampleX        = xCurSource[iSampleId];           //x position of current sample
            float fCurSampleY        = yCurSource[iSampleId];           //y position of current sample
			float fCurSampleR = hypotf(fCurSampleX, fCurSampleY);
            if (fCurSampleR > kRadiusMax) {
                fCurSampleR = kRadiusMax;
            }
			float fCurSampleThetaTemp = atan2f(fCurSampleY, fCurSampleX);       //atan2f is the arctangent with 2 variables
            if (fCurSampleThetaTemp < 0){
                fCurSampleThetaTemp += kThetaMax;
            }
            
            //lock angle when passing in center
            float fCurSampleTheta = fCurSampleThetaTemp;
            JUCE_COMPILER_WARNING("this code was a failed attempt to fix #137")
//            if (fCurSampleR >= kThetaRampRadius) {
//                fCurSampleTheta = fCurSampleThetaTemp;
//                mLockedThetas.setUnchecked(iCurSource, fCurSampleThetaTemp);
//            } else {
//
//                float fNewProportion = (fCurSampleR >= kThetaLockRadius) ? ((fCurSampleR - kThetaLockRadius) / (kThetaRampRadius - kThetaLockRadius)) : .5;
//                
//                float oldTheta = mLockedThetas.getUnchecked(iCurSource);
//                float deltaTheta = oldTheta - fCurSampleThetaTemp;
//                
//                if (deltaTheta < 0) deltaTheta = -deltaTheta;
//                
//                if (deltaTheta > kQuarterCircle) {
//                    // assume flipped side
//                    if (oldTheta > fCurSampleThetaTemp) oldTheta -= kHalfCircle;
//                    else oldTheta += kHalfCircle;
//                }
//                
//                fCurSampleTheta = fNewProportion * fCurSampleThetaTemp + (1 - fNewProportion) * oldTheta;
//                
//                if (fCurSampleTheta < 0) fCurSampleTheta += kThetaMax;
//                else if (fCurSampleTheta >= kThetaMax) fCurSampleTheta -= kThetaMax;
//            }
            
            jassert(fCurSampleTheta >= 0 && fCurSampleTheta <= kThetaMax);
            
#if TIME_PROCESS
            Time timeParameterRamps = Time::getCurrentTime();
#endif
            //apply filter to fCurSampleValue if needed
			if (mApplyFilter) {
				float distance;
                if (fCurSampleR >= 1) {
                    distance = denormalize(p_pfParamCopy[kFilterMid], p_pfParamCopy[kFilterFar], (fCurSampleR - 1));
                } else {
                    distance = denormalize(p_pfParamCopy[kFilterNear], p_pfParamCopy[kFilterMid], fCurSampleR);
                }
				fCurSampleValue = mFilters[iCurSource].process(fCurSampleValue, distance);
			}
            
#if TIME_PROCESS
            Time timeFilter = Time::getCurrentTime();
#endif
            // adjust volume of fCurSampleValue based on volume options from 'volume and filters' tab
            float dbSource;
            if (fCurSampleR >= 1) {
                dbSource = denormalize(p_pfParamCopy[kVolumeMid], p_pfParamCopy[kVolumeFar], (fCurSampleR - 1));
            } else {
                dbSource = denormalize(p_pfParamCopy[kVolumeNear], p_pfParamCopy[kVolumeMid], fCurSampleR);
            }
            fCurSampleValue *= dbToLinear(dbSource);
#if TIME_PROCESS
            Time timeVolume = Time::getCurrentTime();
#endif
#if OUTPUT_RAMPING
            spatializeSample(fCurSampleValue, iSampleId, iCurSource, fCurSampleTheta, fCurSampleR, &p_pfParamCopy, vSpeakersCurrentlyInUse, fOldValuesPortion);
#else
            vector<bool> empty;
            spatializeSample(fCurSampleValue, iSampleId, iCurSource, fCurSampleTheta, fCurSampleR, &p_pfParamCopy, empty, fOldValuesPortion);
#endif
            
#if TIME_PROCESS
            Time timeSpatial = Time::getCurrentTime();
#endif
            
#if OUTPUT_RAMPING
//            for (int o = 0; o < mNumberOfSpeakers; o++){
//                if (!vSpeakersCurrentlyInUse[o]){
//                    setSpeakerVolume(iCurSource, 0, fOldValuesPortion, o, nullptr);
//                }
//            }
            addToOutputs(iCurSource, fCurSampleValue, iSampleId);
#endif
            
#if TIME_PROCESS
            Time timeOutput = Time::getCurrentTime();
            timeAvgParamRamp+= 1000*(timeParameterRamps - timeBeginSample).     inMilliseconds()/(float)m_iDawBufferSize;
            timeAvgFilter   += 1000*(timeFilter         - timeParameterRamps).  inMilliseconds()/(float)m_iDawBufferSize;
            timeAvgVolume   += 1000*(timeVolume         - timeFilter).          inMilliseconds()/(float)m_iDawBufferSize;
            timeAvgSpatial  += 1000*(timeSpatial        - timeVolume).          inMilliseconds()/(float)m_iDawBufferSize;
            timeAvgOutputs  += 1000*(timeOutput         - timeSpatial).         inMilliseconds()/(float)m_iDawBufferSize;
#endif
		}
	}
}

void SpatGrisAudioProcessor::spatializeSample(const float &p_fCurSampleValue, const int &p_iSampleId, const int &p_iCurSource, const float &fCurSampleT, const float &fCurSampleR, float **p_pfParams, vector<bool> &vSpeakersCurrentlyInUse, const float &fOldValuesPortion){
    //if we're outside the main, first circle, only 2 speakers will play
    if (fCurSampleR >= 1 || mNumberOfSpeakers == 2) {
        // find left and right speakers
        int left, right;
        float dLeft, dRight;
        
        findLeftAndRightSpeakers(fCurSampleT, *p_pfParams, left, right, dLeft, dRight);

        // add to output
        if (left >= 0 && right >= 0) {
            float dTotal = dLeft + dRight;
            float vLeft = 1 - dLeft / dTotal;
            float vRight = 1 - dRight / dTotal;
#if OUTPUT_RAMPING
            setSpeakerVolume(p_iCurSource, vLeft,  fOldValuesPortion, left,  &vSpeakersCurrentlyInUse);
            setSpeakerVolume(p_iCurSource, vRight, fOldValuesPortion, right, &vSpeakersCurrentlyInUse);
#else
            addToOutput(p_fCurSampleValue * vLeft,  left, p_iSampleId);
            addToOutput(p_fCurSampleValue * vRight,  right, p_iSampleId);
#endif
        } else {
            // one side is empty!
            int o = (left >= 0) ? left : right;
            jassert(o >= 0);
#if OUTPUT_RAMPING
            setSpeakerVolume(p_iCurSource, 1, fOldValuesPortion, o, &vSpeakersCurrentlyInUse);
#else
            addToOutput(p_fCurSampleValue, o, p_iSampleId);
#endif
        }
    }
    //if we're inside the main circle, 4 speakers will play
    else {
        // find front left and right speakers and angles
        int iFrontLeftSpID, iFrontRightSpId;
        float fFrontLeftSpAngle, fFrontRightSpAngle;
        findLeftAndRightSpeakers(fCurSampleT, *p_pfParams, iFrontLeftSpID, iFrontRightSpId, fFrontLeftSpAngle, fFrontRightSpAngle);
        
        //find back left and right speakers and angles
        float fCurSampleBackTheta = fCurSampleT + kHalfCircle;
        if (fCurSampleBackTheta > kThetaMax) fCurSampleBackTheta -= kThetaMax;
        int iBackLeftSpID, iBackRightSpId;
        float dBackLeft, dBackRight;
        findLeftAndRightSpeakers(fCurSampleBackTheta, *p_pfParams, iBackLeftSpID, iBackRightSpId, dBackLeft, dBackRight);
        
        float fFrontVol = fCurSampleR * 0.5f + 0.5f;
        float fBackVol = 1 - fFrontVol;
        
        // add to front output
        if (iFrontLeftSpID >= 0 && iFrontRightSpId >= 0) {
            float dTotal = fFrontLeftSpAngle + fFrontRightSpAngle;
            float vLeft = 1 - fFrontLeftSpAngle / dTotal;
            float vRight = 1 - fFrontRightSpAngle / dTotal;
#if OUTPUT_RAMPING
            setSpeakerVolume(p_iCurSource, vLeft * fFrontVol, fOldValuesPortion, iFrontLeftSpID, &vSpeakersCurrentlyInUse);
            setSpeakerVolume(p_iCurSource, vRight * fFrontVol, fOldValuesPortion, iFrontRightSpId, &vSpeakersCurrentlyInUse);
#else
            addToOutput(p_fCurSampleValue * vLeft * fFrontVol, iFrontLeftSpID, p_iSampleId);
            addToOutput(p_fCurSampleValue * vRight * fFrontVol, iFrontRightSpId, p_iSampleId);
#endif
        } else {
            // one side is empty!
            int o = (iFrontLeftSpID >= 0) ? iFrontLeftSpID : iFrontRightSpId;
            jassert(o >= 0);
#if OUTPUT_RAMPING
            setSpeakerVolume(p_iCurSource, fFrontVol, fOldValuesPortion, o, &vSpeakersCurrentlyInUse);
#else
            addToOutput(p_fCurSampleValue * fFrontVol, o, p_iSampleId);
#endif
        }
        
        // add to back output
        if (iBackLeftSpID >= 0 && iBackRightSpId >= 0) {
            float dTotal = dBackLeft + dBackRight;
            float vLeft  = 1 - dBackLeft / dTotal;
            float vRight = 1 - dBackRight / dTotal;
#if OUTPUT_RAMPING
            setSpeakerVolume(p_iCurSource, vLeft * fBackVol, fOldValuesPortion, iBackLeftSpID, &vSpeakersCurrentlyInUse);
            setSpeakerVolume(p_iCurSource, vRight * fBackVol, fOldValuesPortion, iBackRightSpId, &vSpeakersCurrentlyInUse);
#else
            addToOutput(p_fCurSampleValue * vLeft * fBackVol, iBackLeftSpID, p_iSampleId);
            addToOutput(p_fCurSampleValue * vRight * fBackVol, iBackRightSpId, p_iSampleId);
#endif
        } else {
            // one side is empty!
            int o = (iBackLeftSpID >= 0) ? iBackLeftSpID : iBackRightSpId;
            jassert(o >= 0);
#if OUTPUT_RAMPING
            setSpeakerVolume(p_iCurSource, fBackVol, fOldValuesPortion, o, &vSpeakersCurrentlyInUse);
#else
            addToOutput(p_fCurSampleValue * fBackVol, o, p_iSampleId);
#endif
        }
    }
}

void SpatGrisAudioProcessor::ProcessDataSpan(float *params) {
    // clear mOutputs
    for (int o = 0; o < mNumberOfSpeakers; o++) {
        float *output = mOutputs[o];
        memset(output, 0, m_iDawBufferSize * sizeof(float));
    }
    
    //------------------------------- DISTRIBUTE PARAMETER CHANGE OVER SAMPLES IN THE BUFFER ------------------------------------------
    const float fOldValuesPortion = powf(0.01f, 1000.f / (denormalize(kSmoothMin, kSmoothMax, params[kSmooth]) * m_dSampleRate));
    createParameterRamps(params, fOldValuesPortion);
    
#if OUTPUT_RAMPING
    vector<bool> vSpeakersCurrentlyInUse;
#endif
        kMovementModeChoice->juce::AudioProcessorParameter::setValue(<#float newValue#>)
    int areaCount = 0;
    //--------------------------------- CREATE AREAS ------------------------------------------
    if (mNumberOfSpeakers > 2) {
        for (int iCurSpeaker = 0; iCurSpeaker < mNumberOfSpeakers; iCurSpeaker++) {
            float fCurAngle = params[getParamForSpeakerX(iCurSpeaker)];
            int left, right;
            float dLeft, dRight;
            findLeftAndRightSpeakers(fCurAngle, params, left, right, dLeft, dRight, iCurSpeaker);
 
            jassert(left >= 0 && right >= 0);
            jassert(dLeft > 0 && dRight > 0);
            
            AddArea(iCurSpeaker, fCurAngle - dLeft, 0, fCurAngle, 1, mAllAreas, areaCount, mNumberOfSpeakers);
            AddArea(iCurSpeaker, fCurAngle, 1, fCurAngle + dRight, 0, mAllAreas, areaCount, mNumberOfSpeakers);
        }
    } else if (mNumberOfSpeakers == 2) {
        int s1 = (params[getParamForSpeakerX(0)] < params[getParamForSpeakerX(1)]) ? 0 : 1;
        int s2 = 1 - s1;
        float t1 = params[getParamForSpeakerX(s1)];
        float t2 = params[getParamForSpeakerX(s2)];
        
        AddArea(s1, t2 - kThetaMax, 0, t1, 1, mAllAreas, areaCount, mNumberOfSpeakers);
        AddArea(s1, t1, 1, t2, 0, mAllAreas, areaCount, mNumberOfSpeakers);
        
        AddArea(s2, t1, 0, t2, 1, mAllAreas, areaCount, mNumberOfSpeakers);
        AddArea(s2, t2, 1, t1 + kThetaMax, 0, mAllAreas, areaCount, mNumberOfSpeakers);
    } else {
        AddArea(0, 0, 1, kThetaMax, 1, mAllAreas, areaCount, mNumberOfSpeakers);
    }
    
    jassert(areaCount > 0);
    
    //------------------------------- FOR EACH SOUND SOURCE ------------------------------------------
    // in this context: source T, R are actually source X, Y
    for (int iCurSource = 0; iCurSource < mNumberOfSources; iCurSource++) {
#if USE_VECTORS
        float *input = mInputsCopy[iCurSource].data();
        float *input_x = mParameterRamps[getParamForSourceX(iCurSource)].data();
        float *input_y = mParameterRamps[getParamForSourceY(iCurSource)].data();
        float *input_d = mParameterRamps[getParamForSourceD(iCurSource)].data();
#else
        float *input = mInputsCopy[i];
        float *input_x = mParameterRamps[getParamForSourceX(i)];
        float *input_y = mParameterRamps[getParamForSourceY(i)];
        float *input_d = mParameterRamps[getParamForSourceD(i)];
#endif
        
        //------------------------------- FOR EACH SAMPLE ------------------------------------------
        for (unsigned int iCurSampleId = 0; iCurSampleId < m_iDawBufferSize; ++iCurSampleId) {
#if OUTPUT_RAMPING
            vSpeakersCurrentlyInUse.assign(mNumberOfSpeakers, false);
#endif
            float s = input[iCurSampleId];
            float x = input_x[iCurSampleId];
            float y = input_y[iCurSampleId];
            float d = input_d[iCurSampleId];
           
            float tv = dbToLinear((1-d) * params[kMaxSpanVolume]);
            
            // could use the Accelerate framework on mac for these
            float r = hypotf(x, y);
            if (r > kRadiusMax) r = kRadiusMax;
            
            float it = atan2f(y, x);
            if (it < 0) it += kThetaMax;
            
            //if (r < 1 && d > 0.5) d = 0.5;
            if (d < 1e-6) d = 1e-6;          
            float angle = d * M_PI;
            
            if (mApplyFilter) {
                float distance;
                if (r >= 1) distance = denormalize(params[kFilterMid], params[kFilterFar], (r - 1));
                else distance = denormalize(params[kFilterNear], params[kFilterMid], r);
                s = mFilters[iCurSource].process(s, distance);
            }
            
            //adjust input volume
            float dbSource;
            if (r >= 1) {
                dbSource = denormalize(params[kVolumeMid], params[kVolumeFar], (r - 1));
            } else {
                dbSource = denormalize(params[kVolumeNear], params[kVolumeMid], r);
            }
            s *= dbToLinear(dbSource);
            
            //lock angle when passing in center
            float t;
            if (r >= kThetaRampRadius) {
                t = it;
                mLockedThetas.setUnchecked(iCurSource, it);
            } else {
                float c = (r >= kThetaLockRadius) ? ((r - kThetaLockRadius) / (kThetaRampRadius - kThetaLockRadius)) : 0;
                float lt = mLockedThetas.getUnchecked(iCurSource);
                float dt = lt - it;
                if (dt < 0) dt = -dt;
                if (dt > kQuarterCircle) {
                    // assume flipped side
                    if (lt > it) lt -= kHalfCircle;
                    else lt += kHalfCircle;
                }
                t = c * it + (1 - c) * lt;
                
                if (t < 0) t += kThetaMax;
                else if (t >= kThetaMax) t -= kThetaMax;
            }
            
            jassert(t >= 0 && t <= kThetaMax);
            jassert(angle > 0 && angle <= kHalfCircle);
            
            memset(mOutFactors, 0, kMaxChannels * sizeof(float));
            
            float factor = (r < 1) ? (r * 0.5f + 0.5f) : 1;
            
            for (int side = 0; side < 2; side++) {
                float tl = t - angle;
                float tr = t + angle;
                
                if (tl < 0) {
                    Integrate(tl + kThetaMax, kThetaMax, mAllAreas, areaCount, mOutFactors, factor);
                    Integrate(0, tr, mAllAreas, areaCount, mOutFactors, factor);
                } else if (tr > kThetaMax) {
                    Integrate(tl, kThetaMax, mAllAreas, areaCount, mOutFactors, factor);
                    Integrate(0, tr - kThetaMax, mAllAreas, areaCount, mOutFactors, factor);
                } else {
                    Integrate(tl, tr, mAllAreas, areaCount, mOutFactors, factor);
                }
                
                if (r < 1) {
                    factor = 1 - factor;
                    t = (t < kHalfCircle) ? (t + kHalfCircle) : (t - kHalfCircle);
                }
                else break;
            }
            
            float total = 0;
            for (int o = 0; o < mNumberOfSpeakers; o++) {
                total += mOutFactors[o];
            }
            jassert(total > 0);
            float adj = tv / total;
            
#if OUTPUT_RAMPING
            for (int o = 0; o < mNumberOfSpeakers; o++){
                setSpeakerVolume(iCurSource, mOutFactors[o] * adj, fOldValuesPortion, o, NULL);
            }
            addToOutputs(iCurSource, s, iCurSampleId);
#else
            for (int o = 0; o < mNumberOfSpeakers; o++){
                if (mOutFactors[o]){
                    addToOutput(s * mOutFactors[o] * adj, o, iCurSampleId);
                }
            }
#endif
        }
    }
}

void SpatGrisAudioProcessor::ProcessDataFree(float *params) {
	// ramp all non constant parameters
    const float smooth = denormalize(kSmoothMin, kSmoothMax, params[kSmooth]); // milliseconds
	const float fOldValuesPortion = powf(0.01f, 1000.f / (smooth * m_dSampleRate));
	const float sm_n = 1 - fOldValuesPortion;
	for (int i = 0; i < kNonConstantParameters; i++) {
		float currentParam = mSmoothedParameters[i];
		float targetParam = params[i];
#if USE_VECTORS
        float *ramp         = mParameterRamps[i].data();
#else
        float *ramp         = mParameterRamps[i];
#endif
        
		for (unsigned int f = 0; f < m_iDawBufferSize; f++) {
			currentParam = currentParam * fOldValuesPortion + targetParam * sm_n;
			ramp[f] = currentParam;
		}
		mSmoothedParameters.setUnchecked(i, currentParam);
	}
    
	// in this context: T, R are actually X, Y
	const float adj_factor = 1 / sqrtf(2);
	for (int o = 0; o < mNumberOfSpeakers; o++) {
		float *output = mOutputs[o];

#if USE_VECTORS
        float *output_x = mParameterRamps[getParamForSpeakerX(o)].data();
        float *output_y = mParameterRamps[getParamForSpeakerY(o)].data();
        float *output_m = mParameterRamps[getParamForSpeakerM(o)].data();
#else
        float *output_x = mParameterRamps[getParamForSpeakerX(o)];
        float *output_y = mParameterRamps[getParamForSpeakerY(o)];
        float *output_m = mParameterRamps[getParamForSpeakerM(o)];
#endif
        vector<float> output_adj(m_iDawBufferSize, 0);
        for (unsigned int f = 0; f < m_iDawBufferSize; f++){
            output_adj[f] = 1 - output_m[f];
        }
        
        
        for (int i = 0; i < mNumberOfSources; i++) {
#if USE_VECTORS
            float *input = mInputsCopy[i].data();
            float *input_x = mParameterRamps[getParamForSourceX(i)].data();
            float *input_y = mParameterRamps[getParamForSourceY(i)].data();
            float *input_d = mParameterRamps[getParamForSourceD(i)].data();
#else
            float *input = mInputsCopy[i];
            float *input_x = mParameterRamps[getParamForSourceX(i)];
            float *input_y = mParameterRamps[getParamForSourceY(i)];
            float *input_d = mParameterRamps[getParamForSourceD(i)];
#endif
            
            for (unsigned int f = 0; f < m_iDawBufferSize; f++){
                float dx = input_x[f] - output_x[f];
                float dy = input_y[f] - output_y[f];
                float d = sqrtf(dx*dx + dy*dy);
                float da = d * adj_factor * input_d[f];
                if (da > 1) da = 1;
                if (da < 0.1) da = 0.1;
                da = -log10f(da);
                da *= output_adj[f];
                if (i == 0){
                    output[f] = da * input[f];
                } else {
                    output[f] += da * input[f];
                }
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

void SpatGrisAudioProcessor::storeCurrentLocations(){
    for (int i = 0; i < JucePlugin_MaxNumInputChannels; i++) {
        mBufferSrcLocX[i]  = mParameters[getParamForSourceX(i)]->get();
        mBufferSrcLocY[i]  = mParameters[getParamForSourceY(i)]->get();
        mBufferSrcLocD[i]  = mParameters[getParamForSourceD(i)]->get();
        mBufferSrcLocAS[i] = mParameters[getParamForSourceAzimSpan(i)]->get();
        mBufferSrcLocES[i] = mParameters[getParamForSourceElevSpan(i)]->get();

    }
    for (int i = 0; i < JucePlugin_MaxNumOutputChannels; i++) {
        mBufferSpLocX[i] =  mParameters[getParamForSpeakerX(i)]->get();
        mBufferSpLocY[i] = mParameters[getParamForSpeakerY(i)]->get();
        mBufferSpLocM[i] = mParameters[getParamForSpeakerM(i)]->get();
    }
}
//p_iLocToRestore == -1 by default, meaning restore all locations
void SpatGrisAudioProcessor::restoreCurrentLocations(int p_iLocToRestore){
    if (p_iLocToRestore == -1){
        for (int i = 0; i < JucePlugin_MaxNumInputChannels; i++) {
            mParameters[getParamForSourceX(i)]->setValueNotifyingHost(mBufferSrcLocX[i]);
            mParameters[getParamForSourceY(i)]->setValueNotifyingHost(mBufferSrcLocY[i]);
            //mParameters.set(getParamForSourceX(i), mBufferSrcLocX[i]);
            //mParameters.set(getParamForSourceY(i), mBufferSrcLocY[i]);
            float fValue = mBufferSrcLocD[i];
            mParameters[getParamForSourceD(i)]->setValueNotifyingHost(fValue);
            //mParameters.set(getParamForSourceD(i), fValue);
        }
    } else {
        //only restore location for selected source
        int i = p_iLocToRestore;
        mParameters[getParamForSourceX(i)]->setValueNotifyingHost(mBufferSrcLocX[i]);
        mParameters[getParamForSourceY(i)]->setValueNotifyingHost(mBufferSrcLocY[i]);
        //mParameters.set(getParamForSourceX(i), mBufferSrcLocX[i]);
        //mParameters.set(getParamForSourceY(i), mBufferSrcLocY[i]);
        float fValue = mBufferSrcLocD[i];
        //mParameters.set(getParamForSourceD(i), fValue);
        mParameters[getParamForSourceD(i)]->setValueNotifyingHost(fValue);
    }
    
    for (int i = 0; i < JucePlugin_MaxNumOutputChannels; i++) {
        mParameters[getParamForSpeakerX(i)]->setValueNotifyingHost(mBufferSpLocX[i]);
        mParameters[getParamForSpeakerY(i)]->setValueNotifyingHost(mBufferSpLocY[i]);
        mParameters[getParamForSpeakerM(i)]->setValueNotifyingHost(mBufferSpLocM[i]);
        /*
        mParameters.set(getParamForSpeakerX(i), mBufferSpLocX[i]);
        mParameters.set(getParamForSpeakerY(i), mBufferSpLocY[i]);
		mParameters.set(getParamForSpeakerM(i), mBufferSpLocM[i]);*/
    }
}

static const int kDataVersion = 1;
void SpatGrisAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    XmlElement xml ("SPATGRIS_SETTINGS");
    
    xml.setAttribute ("kDataVersion", kDataVersion);
    xml.setAttribute ("mShowGridLines", mShowGridLines);
    xml.setAttribute ("m_bOscActive", m_bOscActive);
    xml.setAttribute ("mTrIndependentMode", mTrSeparateAutomationMode);
    xml.setAttribute ("m_iMovementMode", getMovementMode());
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
    xml.setAttribute ("mSrcSelected", mSelectedSrc);
    xml.setAttribute ("mSpSelected", mSpSelected);
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
    xml.setAttribute ("m_fTrEllipseWidth", m_fTrEllipseWidth);
    xml.setAttribute ("m_fEndLocationX", m_fEndLocationXY01.x);
    xml.setAttribute ("m_fEndLocationY", m_fEndLocationXY01.y);
    xml.setAttribute ("mLeapEnabled", mLeapEnabled);
    xml.setAttribute ("kMaxSpanVolume", mParameters[kMaxSpanVolume]->get());
    xml.setAttribute ("kRoutingVolume", mParameters[kRoutingVolume]->get());
    xml.setAttribute ("mRoutingMode", mRoutingMode);
    xml.setAttribute ("kSmooth", mParameters[kSmooth]->get());
    xml.setAttribute ("kVolumeNear", mParameters[kVolumeNear]->get());
    xml.setAttribute ("kVolumeMid", mParameters[kVolumeMid]->get());
    xml.setAttribute ("kVolumeFar", mParameters[kVolumeFar]->get());
    xml.setAttribute ("kFilterNear", mParameters[kFilterNear]->get());
    xml.setAttribute ("kFilterMid", mParameters[kFilterMid]->get());
    xml.setAttribute ("kFilterFar", mParameters[kFilterFar]->get());
    xml.setAttribute ("m_iOscSpat1stSrcId", m_iOscSpat1stSrcId);
    xml.setAttribute ("m_iOscSpatPort", m_iOscSpatPort);
    xml.setAttribute ("kTrajectorySpeed", getParameter(kTrajectorySpeed));
    
    for (int i = 0; i < JucePlugin_MaxNumInputChannels; ++i) {
		String srcX = "src" + to_string(i) + "x";
        float x = mParameters[getParamForSourceX(i)]->get();
        xml.setAttribute (srcX, x);
        String srcY = "src" + to_string(i) + "y";
        float y = mParameters[getParamForSourceY(i)]->get();
        xml.setAttribute (srcY, y);
        String srcD = "src" + to_string(i) + "d";
        xml.setAttribute (srcD, mParameters[getParamForSourceD(i)]->get());
        String srcAS = "src" + to_string(i) + "AS";
        xml.setAttribute (srcAS, mParameters[getParamForSourceAzimSpan(i)]->get());
        String srcES = "src" + to_string(i) + "ES";
        xml.setAttribute (srcES, mParameters[getParamForSourceElevSpan(i)]->get());
    }
    for (int i = 0; i < JucePlugin_MaxNumOutputChannels; ++i) {
        String spkX = "spk" + to_string(i) + "x";
        xml.setAttribute (spkX, mParameters[getParamForSpeakerX(i)]->get());
        String spkY = "spk" + to_string(i) + "y";
        xml.setAttribute (spkY, mParameters[getParamForSpeakerY(i)]->get());
        String spkM = "spk" + to_string(i) + "m";
        xml.setAttribute (spkM, mParameters[getParamForSpeakerM(i)]->get());
    }
    copyXmlToBinary (xml, destData);
}

void SpatGrisAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {
    if(mTrajectory){
        mTrajectory->stop();
    }
    // This getXmlFromBinary() helper function retrieves our XML from the binary blob..
    ScopedPointer<XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr) {
        bLevelUiLock = true;
        // make sure that it's actually our type of XML object..˝
        if (xmlState->hasTagName ("SPATGRIS_SETTINGS")) {
            int version         = xmlState->getIntAttribute ("kDataVersion", 1);
            if (version > kDataVersion){
                AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon, "SpatGRIS - Loading preset/state from newer version",
                                                  "You are attempting to load SpatGRIS with a preset from a newer version.\nDefault values will be used for all parameters.", "OK");
                return;
            }
            mShowGridLines      = xmlState->getIntAttribute ("mShowGridLines", 0);
            m_bOscActive        = xmlState->getIntAttribute ("m_bOscActive", 1);
            mTrSeparateAutomationMode  = xmlState->getIntAttribute ("mTrIndependentMode", mTrSeparateAutomationMode);
#if ALLOW_MVT_MODE_AUTOMATION
            setMovementMode(xmlState->getIntAttribute ("m_iMovementMode", 0), false);
#endif
            mLinkSurfaceOrPan   = xmlState->getIntAttribute ("mLinkSurfaceOrPan", 0);
            mLinkAzimSpan       = xmlState->getIntAttribute ("mLinkAzimSpan", 0);
            mLinkElevSpan       = xmlState->getIntAttribute ("mLinkElevSpan", 0);
            mGuiWidth           = xmlState->getIntAttribute ("mGuiWidth", kDefaultWidth);
            mGuiHeight          = xmlState->getIntAttribute ("mGuiHeight", kDefaultHeight);
            setGuiTab(xmlState->getIntAttribute ("mGuiTab", 0));
            mOscLeapSource      = xmlState->getIntAttribute ("mOscLeapSource", 0);
            mOscReceiveEnabled  = xmlState->getIntAttribute ("mOscReceiveEnabled", 0);
            mOscReceivePort     = xmlState->getIntAttribute ("mOscReceivePort", 8000);
            mOscSendEnabled     = xmlState->getIntAttribute ("mOscSendEnabled", 0);
            mOscSendPort        = xmlState->getIntAttribute ("mOscSendPort", 9000);
            mOscSendIp          = xmlState->getStringAttribute ("mOscSendIp", mOscSendIp);
            setProcessMode(xmlState->getIntAttribute ("mProcessMode", kPanVolumeMode));
            mApplyFilter        = xmlState->getIntAttribute ("mApplyFilter", 1);
            
            setInputOutputMode(xmlState->getIntAttribute ("mInputOutputMode", mInputOutputMode)+1);
            
            mSrcPlacementMode   = xmlState->getIntAttribute ("mSrcPlacementMode", 1);
            mSpPlacementMode    = xmlState->getIntAttribute ("mSpPlacementMode", 1);
            mSelectedSrc        = xmlState->getIntAttribute ("mSrcSelected", 0);
            mSpSelected         = xmlState->getIntAttribute ("mSpSelected", 1);
            m_iTrDirection      = xmlState->getIntAttribute ("m_iTrDirection", 0);
            m_iTrReturn         = xmlState->getIntAttribute ("m_iTrReturn", 0);
            m_iTrType           = xmlState->getIntAttribute ("m_iTrType", 0);
            m_iTrSrcSelect      = xmlState->getIntAttribute ("m_iTrSrcSelect", 1);
            m_fTrDuration       = static_cast<float>(xmlState->getDoubleAttribute("m_fTrDuration", m_fTrDuration));
            m_iTrUnits          = xmlState->getIntAttribute ("m_iTrUnits", 0);
            m_fTrRepeats        = static_cast<float>(xmlState->getDoubleAttribute("m_fTrRepeats", m_fTrRepeats));
            m_fTrDampening      = static_cast<float>(xmlState->getDoubleAttribute("m_fTrDampening", m_fTrDampening));
            m_fTrDeviation      = static_cast<float>(xmlState->getDoubleAttribute("m_fTrDeviation", m_fTrDeviation));
            m_fTrEllipseWidth   = static_cast<float>(xmlState->getDoubleAttribute("m_fTrEllipseWidth", m_fTrEllipseWidth));
            m_fTrTurns          = static_cast<float>(xmlState->getDoubleAttribute("m_fTrTurns", m_fTrTurns));
            m_fEndLocationXY01.x  = static_cast<float>(xmlState->getDoubleAttribute("m_fEndLocationX", m_fEndLocationXY01.x));
            m_fEndLocationXY01.y = static_cast<float>(xmlState->getDoubleAttribute("m_fEndLocationY", m_fEndLocationXY01.y));
            mLeapEnabled        = xmlState->getIntAttribute ("mLeapEnabled", 0);
            
            mParameters[kMaxSpanVolume]->setValueNotifyingHost(static_cast<float>(xmlState->getDoubleAttribute("kMaxSpanVolume", normalize(kMaxSpanVolumeMin, kMaxSpanVolumeMax, kMaxSpanVolumeDefault))));
            mParameters[kRoutingVolume]->setValueNotifyingHost(static_cast<float>(xmlState->getDoubleAttribute("kRoutingVolume", normalize(kRoutingVolumeMin, kRoutingVolumeMax, kRoutingVolumeDefault))));

            mParameters[kSmooth ] ->setValueNotifyingHost(        static_cast<float>(xmlState->getDoubleAttribute("kSmooth", normalize(kSmoothMin, kSmoothMax, kSmoothDefault))));
            mParameters[kVolumeNear ] ->setValueNotifyingHost(    static_cast<float>(xmlState->getDoubleAttribute("kVolumeNear", normalize(kVolumeNearMin, kVolumeNearMax, kVolumeNearDefault))));
            mParameters[kVolumeMid ]->setValueNotifyingHost(     static_cast<float>(xmlState->getDoubleAttribute("kVolumeMid", normalize(kVolumeMidMin, kVolumeMidMax, kVolumeMidDefault))));
            mParameters[kVolumeFar]->setValueNotifyingHost(    static_cast<float>(xmlState->getDoubleAttribute("kVolumeFar", normalize(kVolumeFarMin, kVolumeFarMax, kVolumeFarDefault))));
            mParameters[kFilterNear] ->setValueNotifyingHost(    static_cast<float>(xmlState->getDoubleAttribute("kFilterNear", normalize(kFilterNearMin, kFilterNearMax, kFilterNearDefault))));
            mParameters[kFilterMid]->setValueNotifyingHost(     static_cast<float>(xmlState->getDoubleAttribute("kFilterMid", normalize(kFilterMidMin, kFilterMidMax, kFilterMidDefault))));
            mParameters[kFilterFar] ->setValueNotifyingHost(     static_cast<float>(xmlState->getDoubleAttribute("kFilterFar", normalize(kFilterFarMin, kFilterFarMax, kFilterFarDefault))));
            mParameters[kTrajectorySpeed] ->setValueNotifyingHost(static_cast<float>(xmlState->getDoubleAttribute("kTrajectorySpeed", kSpeedDefault)));
            //mParameters.set(kMaxSpanVolume, static_cast<float>(xmlState->getDoubleAttribute("kMaxSpanVolume", normalize(kMaxSpanVolumeMin, kMaxSpanVolumeMax, kMaxSpanVolumeDefault))));
            //mParameters.set(kRoutingVolume, static_cast<float>(xmlState->getDoubleAttribute("kRoutingVolume", normalize(kRoutingVolumeMin, kRoutingVolumeMax, kRoutingVolumeDefault))));
#if ALLOW_INTERNAL_WRITE
            setRoutingMode(xmlState->getIntAttribute ("mRoutingMode", kNormalRouting));
#endif
           /* mParameters.set(kSmooth,        static_cast<float>(xmlState->getDoubleAttribute("kSmooth", normalize(kSmoothMin, kSmoothMax, kSmoothDefault))));
            mParameters.set(kVolumeNear,    static_cast<float>(xmlState->getDoubleAttribute("kVolumeNear", normalize(kVolumeNearMin, kVolumeNearMax, kVolumeNearDefault))));
            mParameters.set(kVolumeMid,     static_cast<float>(xmlState->getDoubleAttribute("kVolumeMid", normalize(kVolumeMidMin, kVolumeMidMax, kVolumeMidDefault))));
            mParameters.set(kVolumeFar,     static_cast<float>(xmlState->getDoubleAttribute("kVolumeFar", normalize(kVolumeFarMin, kVolumeFarMax, kVolumeFarDefault))));
            mParameters.set(kFilterNear,    static_cast<float>(xmlState->getDoubleAttribute("kFilterNear", normalize(kFilterNearMin, kFilterNearMax, kFilterNearDefault))));
            mParameters.set(kFilterMid,     static_cast<float>(xmlState->getDoubleAttribute("kFilterMid", normalize(kFilterMidMin, kFilterMidMax, kFilterMidDefault))));
            mParameters.set(kFilterFar,     static_cast<float>(xmlState->getDoubleAttribute("kFilterFar", normalize(kFilterFarMin, kFilterFarMax, kFilterFarDefault))));
            mParameters.set(kTrajectorySpeed,static_cast<float>(xmlState->getDoubleAttribute("kTrajectorySpeed", kSpeedDefault)));*/
            
            m_iOscSpat1stSrcId  = xmlState->getIntAttribute("m_iOscSpat1stSrcId",   m_iOscSpat1stSrcId);
            m_iOscSpatPort      = xmlState->getIntAttribute("m_iOscSpatPort",       m_iOscSpatPort);
//            int iMax = JucePlugin_MaxNumInputChannels;
//            int iMax = getTotalNumInputChannels();
            int iMax = getNumberOfSources();
            for (int iCurSource = 0; iCurSource < iMax; ++iCurSource){
                String srcX = "src" + to_string(iCurSource) + "x";
                float fX01 = static_cast<float>(xmlState->getDoubleAttribute(srcX, 0));
                mParameters[getParamForSourceX(iCurSource)] ->setValueNotifyingHost(fX01);
                //mParameters.set(getParamForSourceX(iCurSource), fX01);
                String srcY = "src" + to_string(iCurSource) + "y";
                float fY01 = static_cast<float>(xmlState->getDoubleAttribute(srcY, 0));
                //mParameters.set(getParamForSourceY(iCurSource), fY01);
                mParameters[getParamForSourceY(iCurSource)] ->setValueNotifyingHost(fY01);
                FPoint curPoint = FPoint(fX01, fY01);
                if (m_pMover){
                    m_pMover->storeDownPosition(iCurSource, convertXy012Rt(curPoint));
                }
                String srcD = "src" + to_string(iCurSource) + "d";
                //mParameters.set(getParamForSourceD(iCurSource), static_cast<float>(xmlState->getDoubleAttribute(srcD, normalize(kSourceMinDistance, kSourceMaxDistance, kSourceDefaultDistance))));
                mParameters[getParamForSourceD(iCurSource)] ->setValueNotifyingHost(static_cast<float>(xmlState->getDoubleAttribute(srcD, normalize(kSourceMinDistance, kSourceMaxDistance, kSourceDefaultDistance))));
                
                String srcAS = "src" + to_string(iCurSource) + "AS";
                //mParameters.set(getParamForSourceAzimSpan(iCurSource), static_cast<float>(xmlState->getDoubleAttribute(srcAS, 0)));
                mParameters[getParamForSourceAzimSpan(iCurSource)] ->setValueNotifyingHost(static_cast<float>(xmlState->getDoubleAttribute(srcAS, 0)));
                String srcES = "src" + to_string(iCurSource) + "ES";
                //mParameters.set(getParamForSourceElevSpan(iCurSource), static_cast<float>(xmlState->getDoubleAttribute(srcES, 0)));
                mParameters[getParamForSourceElevSpan(iCurSource)] ->setValueNotifyingHost(static_cast<float>(xmlState->getDoubleAttribute(srcES, 0)));
            }
            for (int iCurSpeaker = 0; iCurSpeaker < JucePlugin_MaxNumOutputChannels; ++iCurSpeaker){
                String spkX = "spk" + to_string(iCurSpeaker) + "x";
                //mParameters.set(getParamForSpeakerX(iCurSpeaker), static_cast<float>(xmlState->getDoubleAttribute(spkX, 0)));
                String spkY = "spk" + to_string(iCurSpeaker) + "y";
                //mParameters.set(getParamForSpeakerY(iCurSpeaker), static_cast<float>(xmlState->getDoubleAttribute(spkY, 0)));
                String spkM = "spk" + to_string(iCurSpeaker) + "m";
                //mParameters.set(getParamForSpeakerM(iCurSpeaker), static_cast<float>(xmlState->getDoubleAttribute(spkM, 0)));
                
                mParameters[getParamForSpeakerX(iCurSpeaker)] ->setValueNotifyingHost( static_cast<float>(xmlState->getDoubleAttribute(spkX, 0)));
                mParameters[getParamForSpeakerY(iCurSpeaker)] ->setValueNotifyingHost(static_cast<float>(xmlState->getDoubleAttribute(spkY, 0)));
                mParameters[getParamForSpeakerM(iCurSpeaker)] ->setValueNotifyingHost(static_cast<float>(xmlState->getDoubleAttribute(spkM, 0)));
            }
        }
    }
    bLevelUiLock = false;
    mHostChangedParameterProcessor++;
    mHostChangedPropertyProcessor++;
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
	return new SpatGrisAudioProcessor();
}
