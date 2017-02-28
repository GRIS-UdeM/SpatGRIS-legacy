 /*
 ==============================================================================

 SpatGRIS: multichannel sound spatialization plug-in.
 
 Copyright (C) 2015  GRIS-UdeM
 
 PluginEditor.cpp
 
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

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "FieldComponent.h"
#include "Trajectories.h"
#include "OctoLeap.h"
#include "OscComponent.h"
#include "ParamSliderGris.h"
#include <iomanip>

#if WIN32

#include <sstream>
#include <string>

template<class T>
string toString(const T &value) {
	ostringstream os;
	os << value;
	return os.str();
}

#elif USE_JOYSTICK
#include "HIDDelegate.h"
#include "HID_Utilities_External.h"
#endif

#define STRING2(x) #x
#define STRING(x) STRING2(x)
//==============================================================================
static const int kDefaultLabelHeight = 18;
static const int kParamBoxHeight = 165;
static const int hertzRefresh = 30; // 30 fps

//==============================================================================

class MiniProgressBar : public Component
{
public:
    MiniProgressBar() : mValue(0) {}
    void paint(Graphics &g)
    {

        juce::Rectangle<int> box = getLocalBounds();
        
        g.setColour(Colours::black);
        g.fillRect(box);
        
        g.setColour(Colour::fromRGB(17, 255, 159));
        box.setWidth(box.getWidth() * mValue);
        g.fillRect(box);
    }
    void setValue(float v) { mValue = v; repaint(); }
private:
    float mValue;
};

//==============================================================================
class OctTabbedComponent : public TabbedComponent
{
public:
    OctTabbedComponent(TabbedButtonBar::Orientation orientation, SpatGrisAudioProcessor *filter)
    :
    TabbedComponent(orientation),
    mFilter(filter)
    ,mInited(false)
    { }
    
    void currentTabChanged (int newCurrentTabIndex, const String& newCurrentTabName) override{
        if (!mInited) {
            return;
        }
        
        //printf("Octogris: currentTabChanged\n");
        mFilter->setGuiTab(newCurrentTabIndex);
    }
    
    void initDone() { mInited = true; }
    
private:
    SpatGrisAudioProcessor *mFilter;
    bool mInited;
};

//======================================= BOX ===========================================================================
class Box : public Component
{
public:
    Box(bool useViewport, GrisLookAndFeel *feel) {
        if (useViewport) {
            mContent = new Component();
            mViewport = new Viewport();
            mViewport->setViewedComponent(mContent, false);
            mViewport->setScrollBarsShown(true, true);
            mViewport->setScrollBarThickness(6);
            mViewport->getVerticalScrollBar()->setColour(ScrollBar::ColourIds::thumbColourId, feel->getScrollBarColour());
            mViewport->getHorizontalScrollBar()->setColour(ScrollBar::ColourIds::thumbColourId, feel->getScrollBarColour());
            
            mViewport->setLookAndFeel(feel);
            addAndMakeVisible(mViewport);
        }
        mBgColour = Colour::fromRGB(200,200,200);
    }
    
    ~Box()
    { }
    
    Component * getContent() {
        return mContent.get() ? mContent.get() : this;
    }
    
    void paint(Graphics &g) {
        g.setColour(mBgColour);
        g.fillRect(getLocalBounds());
    }
    
    void resized() {
        if (mViewport)
            mViewport->setSize(getWidth(), getHeight());
    }
    
    void setBackgroundColour(Colour pColour) {
        mBgColour = pColour;
    }
    
private:
    ScopedPointer<Component> mContent;
    ScopedPointer<Viewport> mViewport;
    Colour mBgColour;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Box)
};


//==================================== JoystickUpdateThread ===================================================================
//class JoystickUpdateThread : public Thread, public Component {
//public:
//    JoystickUpdateThread(SpatGrisAudioProcessorEditor* p_pEditor)
//    : Thread ("JoystickUpdateThread")
//    ,m_iInterval(25)
//    ,m_pEditor(p_pEditor)
//    {  }
//
//    ~JoystickUpdateThread() {
//        stopThread (500);
//    }
//
//    void run() override {
//        while (! threadShouldExit()) {
//            wait (m_iInterval);
////            m_pEditor->readAndUseJoystickValues();
//        }
//    }
//private:
//    int m_iInterval;
//    SpatGrisAudioProcessorEditor* m_pEditor;
//    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JoystickUpdateThread)
//};


//==================================== EDITOR ===================================================================

SpatGrisAudioProcessorEditor::SpatGrisAudioProcessorEditor (SpatGrisAudioProcessor* ownerFilter, SourceMover *mover):
AudioProcessorEditor (ownerFilter)
, mFilter(ownerFilter)
, m_pMover(mover)
, m_logoImage()
, mTrCycleCount(-1)
, mOsc (nullptr)
{
    LookAndFeel::setDefaultLookAndFeel(&mGrisFeel);
    
    mHostChangedParameterEditor = mFilter->getHostChangedParameter();
    mHostChangedPropertyEditor  = mFilter->getHostChangedProperty();
    
    mNeedRepaint        = false;
    mFieldNeedRepaint   = false;
	m_bLoadingPreset    = false;
    mFilter->addListener(this);
    
    // main field
    mField = new FieldComponent(mFilter, m_pMover);
    addAndMakeVisible(mField);
    mComponents.add(mField);

    //GRIS logo
    m_logoImage.setImage(ImageFileFormat::loadFrom (BinaryData::logoGris_png, (size_t) BinaryData::logoGris_pngSize));
    addAndMakeVisible(&m_logoImage);
    
    //version label
    m_VersionLabel = new Label();
    String version = STRING(JUCE_APP_VERSION);
#ifdef JUCE_DEBUG
    version += " ";
    version += STRING(__TIME__);
#endif
    

    m_VersionLabel->setText("SpatGRIS " + version,  dontSendNotification);
    m_VersionLabel->setJustificationType(Justification(Justification::right));
    m_VersionLabel->setColour(Label::textColourId, mGrisFeel.getFontColour());
    m_VersionLabel->setLookAndFeel(&mGrisFeel);
    addAndMakeVisible(m_VersionLabel);
    mComponents.add(m_VersionLabel);

    // param box
    Colour tabBg = mGrisFeel.getBackgroundColour();

    mTabs = new OctTabbedComponent(TabbedButtonBar::TabsAtTop, mFilter);
    mTabs->setLookAndFeel(&mGrisFeel);
    mTabs->addTab("Settings",           tabBg, new Component(), true);
    //mTabs->addTab("Trajectories",       tabBg, new Component(), true);
    mTabs->addTab("Volume & Filters",   tabBg, new Component(), true);
    mTabs->addTab("Sources",            tabBg, new Component(), true);
   	mTabs->addTab("Speakers",           tabBg, new Component(), true);
    mTabs->addTab("Interfaces",         tabBg, new Component(), true);
    mTabs->setTabBarDepth(28);
    mTabs->setSize(kCenterColumnWidth + kMargin + kRightColumnWidth, kParamBoxHeight);
    addAndMakeVisible(mTabs);
    mComponents.add(mTabs);
    
    
    // sources
    {
        int dh = kDefaultLabelHeight, x = 0, y = 0, w = kCenterColumnWidth;
        //SOURCE PARAMETER BOX
        mSourcesBox = new Box(true, &mGrisFeel);
        	
        mSourcesBox->setBackgroundColour(tabBg);
        addAndMakeVisible(mSourcesBox);
        mComponents.add(mSourcesBox);
        Component *boxContent = mSourcesBox->getContent();
        //main box label
        mSourcesBoxLabel = addLabel("Source parameters", 0, 0, kCenterColumnWidth, kDefaultLabelHeight, this);
        mSourcesBoxLabel->setColour(Label::textColourId, mGrisFeel.getFontColour());

        y += 5;
        m_iSelectedSrcEditor = mFilter->getSelectedSrc();
        
        //--------------------- surface/pan -----------------------
        //add surface/pan label
        mSurfaceOrPanLabel = addLabel("Surface", x, y, w*9/12, dh, boxContent);
        if (mFilter->getProcessMode() == kSpanMode){
            static_cast<Label*>(mSurfaceOrPanLabel)->setText("Pan span", dontSendNotification);
        }
        y += dh;
        //add surface/pan link button
        mSurfaceOrPanLinkButton = addCheckbox("Link", mFilter->getLinkSurfaceOrPan(), x, y, w*3/12, dh, boxContent);
        //add surface/pan slider
        float fCurDistance = mFilter->getSourceD(m_iSelectedSrcEditor);
        mSurfaceOrPanSlider = addParamSliderGRIS(kParamSource, m_iSelectedSrcEditor, fCurDistance, x + w*3/12, y, w*9/12, dh, boxContent);
        mSurfaceOrPanSlider->setIsPanSpan(true);
        y += dh + 10;
        //--------------------- azim span -----------------------
        //add azimSpan label
        mAzimSpanLabel = addLabel("Azimuth Span", x, y, w*9/12, dh, boxContent);
        y += dh;
        //add azimSpan link button
        mAzimSpanLinkButton = addCheckbox("Link", mFilter->getLinkAzimSpan(), x, y, w*3/12, dh, boxContent);
        //add azimSpan slider
        float fCurAzimSpan = mFilter->getSourceAzimSpan01(m_iSelectedSrcEditor);
        mAzimSpanSlider = addParamSliderGRIS(kParamAzimSpan, m_iSelectedSrcEditor, fCurAzimSpan, x + w*3/12, y, w*9/12, dh, boxContent);
        y += dh + 10;
        //--------------------- elev span -----------------------
        //add elevSpan label
        mElevSpanLabel = addLabel("Elevation Span", x, y, w*9/12, dh, boxContent);
        y += dh;
        //add elevSpan link button
        mElevSpanLinkButton = addCheckbox("Link", mFilter->getLinkElevSpan(), x, y, w*3/12, dh, boxContent);
        //add elevSpan slider
        float fCurElevSpan = mFilter->getSourceElevSpan01(m_iSelectedSrcEditor);
        mElevSpanSlider = addParamSliderGRIS(kParamElevSpan, m_iSelectedSrcEditor, fCurElevSpan, x + w*3/12, y, w*9/12, dh, boxContent);
        y += dh + 10;
        
        boxContent->setSize(w, y);
        
        mSrcSelectCombo = new ComboBox();
        mTabs->getTabContentComponent(2)->addAndMakeVisible(mSrcSelectCombo);
        mComponents.add(mSrcSelectCombo);
        mSrcSelectCombo->addListener(this);
        updateEditorSources(true);
    }
    
    // speakers
    {
        mSpeakersBox = new Box(true, &mGrisFeel);
        mSpeakersBox->setBackgroundColour(tabBg);
        addAndMakeVisible(mSpeakersBox);
        mComponents.add(mSpeakersBox);
        
        int dh = kDefaultLabelHeight;
        
        int x = 0, y = 5, w = kRightColumnWidth;
        
        mSpeakersBoxLabel = addLabel("Output parameters", x, y, kRightColumnWidth, kDefaultLabelHeight, this);
        mSpeakersBoxLabel->setColour(Label::textColourId,mGrisFeel.getFontColour());

        Component *ct = mSpeakersBox->getContent();
        const int muteWidth = 50;
        //addLabel("Mute", x, y, muteWidth, dh, ct);
#if USE_DB_METERS
       // addLabel("Level", x+muteWidth, y, w/3, dh, ct);
#endif
        
#if ALLOW_INTERNAL_WRITE
        addLabel("   Routing \nvolume (dB):", x+muteWidth+w/3, y, w/3, 2*dh, ct);
        y += dh;
    #if WIN32
        mRoutingVolumeSlider = addParamSliderGRIS(kParamRoutingVolume, kRoutingVolume, mFilter->getParameter(kRoutingVolume), x+muteWidth+w/3, y+20, w/4, 200, ct);
    #else
        mRoutingVolumeSlider = addParamSliderGRIS(kParamRoutingVolume, kRoutingVolume, mFilter->getParameter(kRoutingVolume), x+muteWidth+w/3, y, w/4, 200, ct);
    #endif
        mRoutingVolumeSlider->setTextBoxStyle(Slider::TextBoxBelow, false, 30, dh);
        mRoutingVolumeSlider->setSliderStyle(Slider::LinearVertical);
#endif
        
        y += dh + -22;
        x = 6;
        for (int i = 0; i < kMaxChannels; ++i){
            String s; s << i+1;
            
            if(i>8){
                Component * labS = addLabel(s, x-2, 5, muteWidth, dh, ct);
                mLabelSourceId[i] = labS;
            }else{
                Component * labS = addLabel(s, x, 5, muteWidth, dh, ct);
                mLabelSourceId[i] = labS;
            }
            
            float fMute = mFilter->getSpeakerM(i);
            ToggleButton *mute = addCheckbox("", fMute, x-1, 125, muteWidth, dh, ct);

            mute->setColour(ToggleButton::textColourId, mGrisFeel.getFontColour());
            mMuteButtons[i] = mute;
            const int muteWidth = 50;
#if USE_DB_METERS
            juce::Rectangle<int> level(x+3, muteWidth-30, dh - 6, w/3 - 10 );
        
            LevelComponent *lc = new LevelComponent(mFilter, i);
            lc->setBounds(level);
            ct->addAndMakeVisible(lc);
            mComponents.add(lc);
            mLevelComponents[i] = lc;
#endif
            x += dh + 5;
        }
        

        
        mSpSelectCombo = new ComboBox();
        mTabs->getTabContentComponent(3)->addAndMakeVisible(mSpSelectCombo);
        mComponents.add(mSpSelectCombo);
        mSpSelectCombo->addListener(this);
        updateEditorSpeakers(true);
    }
    
    
    int dh = kDefaultLabelHeight;
    int iButtonW = 50;

    //Trajectory
    {
        mTrajectoryBox = new Box(true, &mGrisFeel);
        mTrajectoryBox->setBackgroundColour(tabBg);
        addAndMakeVisible(mTrajectoryBox);
        mComponents.add(mTrajectoryBox);
        
        Component *box = mTrajectoryBox->getContent();
        mTrajectoryBoxLabel = addLabel("Trajectories", 0,0, 150, dh, this);
        mTrajectoryBoxLabel->setColour(Label::textColourId, mGrisFeel.getFontColour());
        box->setSize(500, 160);
        
        //---------- ROW 1 -------------
        int x = kMargin, y = kMargin, w = (box->getWidth() - kMargin) / 3 - kMargin;
        
    
        addLabel("Movements:", x, y, w-50, dh, box);
        mMovementModeCombo = new ComboBox();
        updateMovementModeCombo();
        box->addAndMakeVisible(mMovementModeCombo);
        mComponents.add(mMovementModeCombo);
        mMovementModeCombo->addListener(this);
        mMovementModeCombo->setBounds(x+80, y, w-12, kDefaultLabelHeight);
      
        y += dh + 4;
        x = kMargin;
        //-------
        int cbw = 130;
        {
            ComboBox *cb = new ComboBox();
            int index = 1;
            for (int i = 1; i < Trajectory::NumberOfTrajectories(); i++){
                cb->addItem(Trajectory::GetTrajectoryName(i), index++);
            }
            cb->setSelectedId(mFilter->getTrType());
            cb->setSize(cbw, dh);
            cb->setTopLeftPosition(x, y);
            box->addAndMakeVisible(cb);
            mComponents.add(cb);
            
            mTrTypeComboBox = cb;
            mTrTypeComboBox->addListener(this);
        }
        
        {
            ComboBox *cb = new ComboBox();
            cb->setSize(cbw, dh);
            cb->setTopLeftPosition(x+cbw+5, y);
            box->addAndMakeVisible(cb);
            mComponents.add(cb);
            
            mTrDirectionComboBox = cb;
            mTrDirectionComboBox->addListener(this);
        }
        
        {
            ComboBox *cb = new ComboBox();
            cb->setSize(cbw-40, dh);
            cb->setTopLeftPosition(x+2*(cbw+5), y);
            box->addAndMakeVisible(cb);
            mComponents.add(cb);
            
            mTrReturnComboBox = cb;
            mTrReturnComboBox->addListener(this);
        }
        
        
        mTrSeparateAutomationModeButton = addCheckbox("Force separate automation", mFilter->getShowGridLines(), x+3*(cbw+5)-40, y, cbw+20, dh, box);
        
        int tewShort = 30;
        int x2 = x+3*(cbw+5);
        mTrDampeningTextEditor = addTextEditor(String(mFilter->getTrDampening()), x2, y, tewShort, dh, box);
        mTrDampeningTextEditor->addListener(this);
        mTrDampeningLabel = addLabel("dampening", x2 + tewShort, y, w, dh, box);
        
        mTrTurnsTextEditor = addTextEditor(String(mFilter->getTrTurns()), x2, y, tewShort, dh, box);
        mTrTurnsTextEditor->addListener(this);
        mTrTurnsLabel = addLabel("turn(s)", x2 + tewShort, y, w, dh, box);
        
        
        //---------- ROW 2 -------------
        y += dh + 4;
        x = kMargin;
        int tew = 80;
        
        mTrDuration = addTextEditor(String(mFilter->getTrDuration()), x, y, tew, dh, box);
        mTrDuration->addListener(this);
        x += tew + kMargin;
        {
            ComboBox *cb = new ComboBox();
            int index = 1;
            cb->addItem("Beat(s)", index++);
            cb->addItem("Second(s)", index++);
            
            cb->setSelectedId(mFilter->getTrUnits());
            cb->setSize(tew, dh);
            cb->setTopLeftPosition(x, y);
            box->addAndMakeVisible(cb);
            mComponents.add(cb);
            
            mTrUnits = cb;
            mTrUnits->addListener(this);
        }
        x += tew + kMargin;
        addLabel("per cycle", x, y, w, dh, box);
        
        mTrDeviationTextEditor = addTextEditor(String(mFilter->getTrDeviation()*360), x2, y, tewShort, dh, box);
        mTrDeviationTextEditor->addListener(this);
        mTrDeviationLabel = addLabel("deviation", x2 + tewShort, y, w, dh, box);
        
        
        mTrEllipseWidthTextEditor = addTextEditor(String(mFilter->getTrEllipseWidth()*2), x2, y, tewShort, dh, box);
        mTrEllipseWidthTextEditor->addListener(this);
        mTrEllipseWidthLabel = addLabel("width factor", x2 + tewShort, y, w, dh, box);
        
        
        //---------- ROW 3 -------------
        y += dh + 4;
        x = kMargin;
        
        mTrRepeats = addTextEditor(String(mFilter->getTrRepeats()), x, y, tew, dh, box);
        mTrRepeats->addListener(this);
        x += tew + kMargin;
        
        addLabel("cycle(s)", x, y, w, dh, box);
        
        //---------- ROW 4 -------------
        y += dh + 4;
        x = kMargin;
        
        mTrEndPointButton = addButton("Set end point", x, y, cbw, dh, box);
        mTrEndPointButton->setClickingTogglesState(true);
        
        x += cbw + kMargin;
        m_pTrEndRayTextEditor = addTextEditor("", x, y, cbw/2, dh, box);
        m_pTrEndRayTextEditor->setTextToShowWhenEmpty("Ray", juce::Colour::greyLevel(.6));
        m_pTrEndRayTextEditor->addListener(this);
        
        x += cbw/2 + kMargin;
        m_pTrEndAngleTextEditor = addTextEditor("", x, y, cbw/2, dh, box);
        m_pTrEndAngleTextEditor->setTextToShowWhenEmpty("Angle", juce::Colour::greyLevel(.6));
        m_pTrEndAngleTextEditor->addListener(this);
        updateEndLocationTextEditors();
        
        x += cbw/2 + kMargin;
        m_pTrResetEndButton = addButton("Reset end point", x, y, cbw, dh, box);
        
        x = kMargin;
        y += dh + 4;
        
        mTrWriteButton = addButton("Ready", x, y, cbw, dh, box);
        mTrWriteButton->setClickingTogglesState(true);
        
        mTrEndPointLabel = addLabel("Click anywhere on circle to set end point", x+cbw+kMargin-2, y-4, 300, dh, box);
        mTrEndPointLabel->setVisible(false);
        
        y += dh + 4;
        
        mTrProgressBar = new MiniProgressBar();
        mTrProgressBar->setSize(cbw , dh-5);//tew
        mTrProgressBar->setTopLeftPosition(x, y);
        mTrProgressBar->setVisible(false);
        
        x = 1*cbw + 2*kMargin;
        
        addLabel("Speed", x-4, y-14, w+10, dh, box);
        
        Slider *ds = addParamSliderGRIS(kParamTrajSpeed, 2.5f, mFilter->getSpeedTraject(), x, y, w, dh-5, box);
        ds->setTextBoxStyle(Slider::TextBoxLeft, false, 40, dh);
        ds->setRange(-2.5f, 2.5f);
        
        //ds->setTextBoxStyle (Slider::TextBoxBelow, false, 90, 20);
        mSpeedTrajectory = ds;
        
        
        box->addChildComponent(mTrProgressBar);
        mComponents.add(mTrProgressBar);
        
        if(mFilter->getTrState() == kTrWriting){
            updateTrajectoryStartComponent(kRunning);
            mTrWriteButton->setToggleState(true, dontSendNotification);
        }
        
        x = 2*cbw + 2*kMargin;
        y = kMargin + dh + 5;
        /*addLabel("Movements:", x, y, w-50, dh, box);

        mMovementModeCombo = new ComboBox();
        updateMovementModeCombo();
        box->addAndMakeVisible(mMovementModeCombo);
        mComponents.add(mMovementModeCombo);
        mMovementModeCombo->addListener(this);
        y += dh + 4;
        mMovementModeCombo->setBounds(x, y, w, kDefaultLabelHeight);*/

        
    }
    
    
    
    //--------------- SETTINGS TAB ---------------- //
    Component *box = mTabs->getTabContentComponent(0);
    {
        int x = kMargin, y = kMargin, w = (box->getWidth() - kMargin) / 3 - kMargin;
        
        //-----------------------------
        // start 1st column        
        {
            mSmoothingLabel = addLabel("Param smoothing (ms)", x, y, w, dh, box);
            y += dh + 4;
            
            Slider *ds = addParamSliderGRIS(kParamSmooth, kSmooth, mFilter->getParameter(kSmooth), x, y, w, dh, box);
            ds->setTextBoxStyle(Slider::TextBoxLeft, false, 40, dh);
            ds->setLookAndFeel(&mGrisFeel);
        
            mSmoothingSlider = ds;
            y += dh + 4;
        }
        mApplyOutputRamping = addCheckbox("Output Ramping", mFilter->getApplyOutRamp(), x, y, w, dh, box);
        mApplyOutputRamping->setTooltip("Warning : destroy performence");
        y += dh + 4;
        mShowGridLines = addCheckbox("Show grid lines", mFilter->getShowGridLines(), x, y, w, dh, box);

        //-----------------------------
        // start 2nd column
        y = kMargin;
        x += w + kMargin;
        //need to initialize those before calling updateInputOuputCombo() below
        mSrcPlacementCombo = new ComboBox();
        mSpPlacementCombo = new ComboBox();

        if (mFilter->getIsAllowInputOutputModeSelection()) {
            addLabel("Input/Output mode", x, y, w, dh, box);
            y += dh + 4;
            
            mInputOutputModeCombo = new ComboBox();
            mInputOutputModeCombo->setSize(w - iButtonW, dh);
            mInputOutputModeCombo->setTopLeftPosition(x, y);
            box->addAndMakeVisible(mInputOutputModeCombo);
            mComponents.add(mInputOutputModeCombo);
            mApplyInputOutputModeButton = addButton("Apply", x + w - iButtonW, y, iButtonW, dh, box);
            y += dh + 4;
            updateInputOutputCombo();
        }
        #if ALLOW_INTERNAL_WRITE
        mRoutingModeLabel = addLabel("Routing mode", x, y, w, dh, box);
        y += dh + 4;
        {
            ComboBox *cb = new ComboBox();
            int index = 1;
            cb->addItem("Normal", index++);
            cb->addItem("Internal write", index++);
            cb->addItem("Internal read 1-2", index++);
            cb->addItem("Internal read 3-4", index++);
            cb->addItem("Internal read 5-6", index++);
            cb->addItem("Internal read 7-8", index++);
            cb->addItem("Internal read 9-10", index++);
            cb->addItem("Internal read 11-12", index++);
            cb->addItem("Internal read 13-14", index++);
            cb->addItem("Internal read 15-16", index++);
            cb->setSelectedId(mFilter->getRoutingMode() + 1);
            cb->setSize(w, dh);
            cb->setTopLeftPosition(x, y);
            box->addAndMakeVisible(cb);
            mComponents.add(cb);
            y += dh + 4;
            
            cb->addListener(this);
            mRoutingModeCombo = cb;
        }
#endif
        
        mOscActiveButton = addCheckbox("Osc Active", mFilter->getOscActive(), x, y, w, dh, box);
         y += dh + 4;
        mOscSpat1stSrcIdLabel = addLabel("1st source ID:", x, y, w*2/3 - 5, dh, box);
        mOscSpat1stSrcIdTextEditor = addTextEditor(String(mFilter->getOscSpat1stSrcId()), x + w*2/3, y, w/3, dh, box);
        mOscSpat1stSrcIdTextEditor->addListener(this);
        y += dh + 4;
        
        mOscSpatPortLabel       = addLabel("OSC Spat port:", x, y, w*2/3 - 5, dh, box);
        mOscSpatPortTextEditor  = addTextEditor(String(mFilter->getOscSpatPort()), x + w*2/3, y, w/3, dh, box);
        mOscSpatPortTextEditor->addListener(this);
        y += dh + 4;
        //-----------------------------
        // start 3rd column
        y = kMargin;
        x += w + kMargin;
        
        addLabel("Process mode", x, y, w, dh, box);
        y += dh + 4;
        
        {
            mProcessModeCombo = new ComboBox();
            int index = 1;
            mProcessModeCombo->addItem("Free volume", index++);
#if ALLOW_PAN_MODE
            mProcessModeCombo->addItem("Pan volume", index++);
#endif
            mProcessModeCombo->addItem("Pan span", index++);
            mProcessModeCombo->addItem("OSC Spatialization", index++);
            mProcessModeCombo->setSelectedId(mFilter->getProcessMode() + 1);
            mProcessModeCombo->setSize(w, dh);
            mProcessModeCombo->setTopLeftPosition(x, y);
            box->addAndMakeVisible(mProcessModeCombo);
            mComponents.add(mProcessModeCombo);
            y += dh + 4;
            
            mProcessModeCombo->addListener(this);
        }
        
        {
            mMaxSpanVolumeLabel = addLabel("Max span volume (dB)", x, y, w, dh, box);
            y += dh + 4;
            
            Slider *ds = addParamSliderGRIS(kParamMaxSpanVolume, kMaxSpanVolume, mFilter->getParameter(kMaxSpanVolume), x, y, w, dh, box);
            ds->setTextBoxStyle(Slider::TextBoxLeft, false, 40, dh);
            mMaxSpanVolumeSlider = ds;
            y += dh + 4;
        }
        
    }
    
    //--------------- TRAJECTORIES TAB ---------------- //
    //box = mTabs->getTabContentComponent(1);
    //{
        /*
        //---------- ROW 1 -------------
        int x = kMargin, y = kMargin, w = (box->getWidth() - kMargin) / 3 - kMargin;
        
        int cbw = 130;
        {
            ComboBox *cb = new ComboBox();
            int index = 1;
            for (int i = 1; i < Trajectory::NumberOfTrajectories(); i++){
                cb->addItem(Trajectory::GetTrajectoryName(i), index++);
            }
            cb->setSelectedId(mFilter->getTrType());
            cb->setSize(cbw, dh);
            cb->setTopLeftPosition(x, y);
            box->addAndMakeVisible(cb);
            mComponents.add(cb);
            
            mTrTypeComboBox = cb;
            mTrTypeComboBox->addListener(this);
        }
        
        {
            ComboBox *cb = new ComboBox();
            cb->setSize(cbw, dh);
            cb->setTopLeftPosition(x+cbw+5, y);
            box->addAndMakeVisible(cb);
            mComponents.add(cb);
            
            mTrDirectionComboBox = cb;
            mTrDirectionComboBox->addListener(this);
        }
        
        {
            ComboBox *cb = new ComboBox();
            cb->setSize(cbw-40, dh);
            cb->setTopLeftPosition(x+2*(cbw+5), y);
            box->addAndMakeVisible(cb);
            mComponents.add(cb);
            
            mTrReturnComboBox = cb;
            mTrReturnComboBox->addListener(this);
        }
        
        
        mTrSeparateAutomationModeButton = addCheckbox("Force separate automation", mFilter->getShowGridLines(), x+3*(cbw+5)-40, y, cbw+20, dh, box);
        
        int tewShort = 30;
        int x2 = x+3*(cbw+5);
        mTrDampeningTextEditor = addTextEditor(String(mFilter->getTrDampening()), x2, y, tewShort, dh, box);
        mTrDampeningTextEditor->addListener(this);
        mTrDampeningLabel = addLabel("dampening", x2 + tewShort, y, w, dh, box);
        
        mTrTurnsTextEditor = addTextEditor(String(mFilter->getTrTurns()), x2, y, tewShort, dh, box);
        mTrTurnsTextEditor->addListener(this);
        mTrTurnsLabel = addLabel("turn(s)", x2 + tewShort, y, w, dh, box);

        
        //---------- ROW 2 -------------
        y += dh + 4;
        x = kMargin;
        int tew = 80;
        
        mTrDuration = addTextEditor(String(mFilter->getTrDuration()), x, y, tew, dh, box);
        mTrDuration->addListener(this);
        x += tew + kMargin;
        {
            ComboBox *cb = new ComboBox();
            int index = 1;
            cb->addItem("Beat(s)", index++);
            cb->addItem("Second(s)", index++);
            
            cb->setSelectedId(mFilter->getTrUnits());
            cb->setSize(tew, dh);
            cb->setTopLeftPosition(x, y);
            box->addAndMakeVisible(cb);
            mComponents.add(cb);
            
            mTrUnits = cb;
            mTrUnits->addListener(this);
        }
        x += tew + kMargin;
        addLabel("per cycle", x, y, w, dh, box);
        
        mTrDeviationTextEditor = addTextEditor(String(mFilter->getTrDeviation()*360), x2, y, tewShort, dh, box);
        mTrDeviationTextEditor->addListener(this);
        mTrDeviationLabel = addLabel("deviation", x2 + tewShort, y, w, dh, box);
        
        
        mTrEllipseWidthTextEditor = addTextEditor(String(mFilter->getTrEllipseWidth()*2), x2, y, tewShort, dh, box);
        mTrEllipseWidthTextEditor->addListener(this);
        mTrEllipseWidthLabel = addLabel("width factor", x2 + tewShort, y, w, dh, box);

        
        //---------- ROW 3 -------------
        y += dh + 4;
        x = kMargin;
        
        mTrRepeats = addTextEditor(String(mFilter->getTrRepeats()), x, y, tew, dh, box);
        mTrRepeats->addListener(this);
        x += tew + kMargin;
        
        addLabel("cycle(s)", x, y, w, dh, box);
        
        //---------- ROW 4 -------------
        y += dh + 4;
        x = kMargin;
        
        mTrEndPointButton = addButton("Set end point", x, y, cbw, dh, box);
        mTrEndPointButton->setClickingTogglesState(true);
        
        x += cbw + kMargin;
        m_pTrEndRayTextEditor = addTextEditor("", x, y, cbw/2, dh, box);
        m_pTrEndRayTextEditor->setTextToShowWhenEmpty("Ray", juce::Colour::greyLevel(.6));
        m_pTrEndRayTextEditor->addListener(this);
        
        x += cbw/2 + kMargin;
        m_pTrEndAngleTextEditor = addTextEditor("", x, y, cbw/2, dh, box);
        m_pTrEndAngleTextEditor->setTextToShowWhenEmpty("Angle", juce::Colour::greyLevel(.6));
        m_pTrEndAngleTextEditor->addListener(this);
        updateEndLocationTextEditors();

        x += cbw/2 + kMargin;
        m_pTrResetEndButton = addButton("Reset end point", x, y, cbw, dh, box);
        
        x = kMargin;
        y += dh + 4;
        
        mTrWriteButton = addButton("Ready", x, y, cbw, dh, box);
        mTrWriteButton->setClickingTogglesState(true);
        
        mTrEndPointLabel = addLabel("Click anywhere on circle to set end point", x+cbw+kMargin-2, y-4, 300, dh, box);
        mTrEndPointLabel->setVisible(false);
        
        y += dh + 4;
        
        mTrProgressBar = new MiniProgressBar();
        mTrProgressBar->setSize(cbw , dh-5);//tew
        mTrProgressBar->setTopLeftPosition(x, y);
        mTrProgressBar->setVisible(false);
        
        x = 1*cbw + 2*kMargin;
        
        addLabel("Speed", x-4, y-14, w+10, dh, box);
        
        Slider *ds = addParamSliderGRIS(kParamTrajSpeed, kTrajectorySpeed, mFilter->getParameter(kTrajectorySpeed), x, y, w, dh-5, box);
        ds->setTextBoxStyle(Slider::TextBoxLeft, false, 40, dh);
        ds->setRange(-2.5f, 2.5f);
        
        //ds->setTextBoxStyle (Slider::TextBoxBelow, false, 90, 20);
        mSpeedTrajectory = ds;

        
        box->addChildComponent(mTrProgressBar);
        mComponents.add(mTrProgressBar);
        
        if(mFilter->getTrState() == kTrWriting){
            updateTrajectoryStartComponent(kRunning);
            mTrWriteButton->setToggleState(true, dontSendNotification);
        }
       
        x = 2*cbw + 2*kMargin;
        y = kMargin + dh + 5;
        addLabel("Movements:", x, y, w-50, dh, box);
        */
 
       
        
    //}
    
    //--------------- V & F TAB ---------------- //
    box = mTabs->getTabContentComponent(1);
    {
        int x = kMargin, y = kMargin, w = (box->getWidth() - kMargin) / 3 - kMargin;
        
        //-----------------------------
        // start 1st column
        
        {
            addLabel("Volume center (dB)", x, y, w, dh, box);
            y += dh + 4;
            
            Slider *ds = addParamSliderGRIS(kParamVolumeNear, kVolumeNear, mFilter->getParameter(kVolumeNear), x, y, w, dh, box);
            ds->setTextBoxStyle(Slider::TextBoxLeft, false, 40, dh);
            mVolumeNear = ds;
            y += dh + 4;
        }
        
        {
            addLabel("Filter center", x, y, w, dh, box);
            y += dh + 4;
            
            Slider *ds = addParamSliderGRIS(kParamFilterNear, kFilterNear, mFilter->getParameter(kFilterNear), x, y, w, dh, box);
            ds->setTextBoxStyle(Slider::TextBoxLeft, false, 40, dh);
            mFilterNear = ds;
            y += dh + 4;
        }
        
        mApplyFilterButton = addCheckbox("Apply Filter", mFilter->getApplyFilter(), x, y, w, dh, box);
        y += dh + 4;
        
        //-----------------------------
        // start 2nd column
        y = kMargin;
        x += w + kMargin;
        
        {
            addLabel("Volume speakers (dB)", x, y, w, dh, box);
            y += dh + 4;
            

            Slider *ds = addParamSliderGRIS(kParamVolumeMid, kVolumeMid, mFilter->getParameter(kVolumeMid), x, y, w, dh, box);
            ds->setTextBoxStyle(Slider::TextBoxLeft, false, 40, dh);
            mVolumeMid = ds;
            y += dh + 4;
        }
        
        {
            addLabel("Filter speakers", x, y, w, dh, box);
            y += dh + 4;
            
            Slider *ds = addParamSliderGRIS(kParamFilterMid, kFilterMid, mFilter->getParameter(kFilterMid), x, y, w, dh, box);
            ds->setTextBoxStyle(Slider::TextBoxLeft, false, 40, dh);
            mFilterMid = ds;
            y += dh + 4;
        }

        //-----------------------------
        // start 3rd column
        y = kMargin;
        x += w + kMargin;
        
        {
            addLabel("Volume far (dB)", x, y, w, dh, box);
            y += dh + 4;
            
            Slider *ds = addParamSliderGRIS(kParamVolumeFar, kVolumeFar, mFilter->getParameter(kVolumeFar), x, y, w, dh, box);
            ds->setTextBoxStyle(Slider::TextBoxLeft, false, 40, dh);
            mVolumeFar = ds;
            y += dh + 4;
        }
        
        {
            addLabel("Filter far", x, y, w, dh, box);
            y += dh + 4;
            
            Slider *ds = addParamSliderGRIS(kParamFilterFar, kFilterFar, mFilter->getParameter(kFilterFar), x, y, w, dh, box);
            ds->setTextBoxStyle(Slider::TextBoxLeft, false, 40, dh);
            mFilterFar = ds;
            y += dh + 4;
        }
    }
    
    //--------------- SOURCES TAB ---------------- //
    box = mTabs->getTabContentComponent(2);
    {
        int x = kMargin, y = kMargin, w = (box->getWidth() - kMargin) / 3 - kMargin;
        int selectw = 50;
        
        // column 1
        addLabel("Source placement", x, y, w, dh, box);
        y += dh + 4;

        //mSrcPlacementCombo = new ComboBox();
        mSrcPlacementCombo->addItem("Left Alternate", kLeftAlternate);
        mSrcPlacementCombo->addItem("Left Clockwise", kLeftClockwise);
        mSrcPlacementCombo->addItem("Left Counter Clockwise", kLeftCounterClockWise);
        mSrcPlacementCombo->addItem("Top Clockwise", kTopClockwise);
        mSrcPlacementCombo->addItem("Top Counter Clockwise", kTopCounterClockwise);
        
        mSrcPlacementCombo->setSelectedId(mFilter->getSrcPlacementMode());
        box->addAndMakeVisible(mSrcPlacementCombo);
        mComponents.add(mSrcPlacementCombo);
        mSrcPlacementCombo->setSize(w, dh);
        mSrcPlacementCombo->setTopLeftPosition(x, y);
        mSrcPlacementCombo->setExplicitFocusOrder(5);
        //mSrcPlacementCombo->addListener(this);
        
        y += dh + 4;
        mApplySrcPlacementButton = addButton("Apply", x, y, iButtonW, dh, box);
        
        // column 2
        y = kMargin;
        x += w + kMargin;
        
        addLabel("Set RA position", x, y, w - selectw, dh, box);
        mSrcSelectCombo->setSelectedId(m_iSelectedSrcEditor+1);
        mSrcSelectCombo->setSize(selectw, dh);
        mSrcSelectCombo->setTopLeftPosition(x + w - selectw, y);
        mSrcSelectCombo->setExplicitFocusOrder(5);
        
        int lw = 60;
        y += dh + 4;
        
        addLabel("Ray", x, y, lw, dh, box);
        mSrcR = addTextEditor("1", x + (w - selectw -25), y, 75, dh, box);
        mSrcR->setExplicitFocusOrder(6);
        mSrcR->addListener(this);
        addLabel("(0 - 2)", x +(w - selectw+50), y, lw, dh, box);
        y += dh + 4;
        
        addLabel("Angle", x, y, lw, dh, box);
        mSrcT = addTextEditor("0", x + (w - selectw -25), y, 75, dh, box);
        mSrcT->setExplicitFocusOrder(7);
        mSrcT->addListener(this);
        addLabel("(0 - 360)", x +(w - selectw+50), y, lw, dh, box);
        
    }
    //--------------- SPEAKERS TAB ---------------- //
    box = mTabs->getTabContentComponent(3);
    {
        int x = kMargin, y = kMargin, w = (box->getWidth() - kMargin) / 3 - kMargin;
        int selectw = 50;
        
        //-------- column 1 --------
        addLabel("Speaker placement", x, y, w, dh, box);
        y += dh + 4;
        
        mSpPlacementCombo->addItem("Left Alternate", kLeftAlternate);
        mSpPlacementCombo->addItem("Left Clockwise", kLeftClockwise);
        mSpPlacementCombo->addItem("Left Counter Clockwise", kLeftCounterClockWise);
        mSpPlacementCombo->addItem("Top Clockwise", kTopClockwise);
        mSpPlacementCombo->addItem("Top Counter Clockwise", kTopCounterClockwise);
        
        mSpPlacementCombo->setSelectedId(mFilter->getSpPlacementMode());
        
        box->addAndMakeVisible(mSpPlacementCombo);
        mComponents.add(mSpPlacementCombo);
        mSpPlacementCombo->setSize(w, dh);
        mSpPlacementCombo->setTopLeftPosition(x, y);
        mSpPlacementCombo->setExplicitFocusOrder(5);
        //mSpPlacementCombo->addListener(this);
        y += dh + 4;
        mApplySpPlacementButton = addButton("Apply", x, y, iButtonW, dh, box);
        
        
        //-------- column 2 --------
        y = kMargin;
        x += w + kMargin;
        
        addLabel("Set RA position", x, y, w - selectw, dh, box);
        mSpSelectCombo->setSelectedId(mFilter->getSpSelected());
        mSpSelectCombo->setSize(selectw, dh);
        mSpSelectCombo->setTopLeftPosition(x + w - selectw, y);
        mSpSelectCombo->setExplicitFocusOrder(5);
        
        int lw = 60;
        y += dh + 4;
        
        addLabel("Ray", x, y, lw, dh, box);
        mSpR = addTextEditor("1", x + (w - selectw -25), y, 75, dh, box);
        mSpR->setExplicitFocusOrder(6);
        mSpR->addListener(this);
        addLabel("(0 - 2)", x +(w - selectw+50), y, lw, dh, box);
        y += dh + 4;
        
        addLabel("Angle", x, y, lw, dh, box);
        mSpT = addTextEditor("0", x + (w - selectw -25), y, 75, dh, box);
        mSpT->setExplicitFocusOrder(7);
        mSpT->addListener(this);
        addLabel("(0 - 360)", x +(w - selectw+50), y, lw, dh, box);
        
        
        
    }
    
    //--------------- INTERFACE TAB ---------------- //
    box = mTabs->getTabContentComponent(4);
    {
        int x = kMargin, y = kMargin;
        const int m = 10, dh = 18, cw = 300;
        int comboW = 40, w = (box->getWidth() - kMargin) / 3 - kMargin;
#if USE_LEAP
        addLabel("OSC source", x, y, w-comboW, dh, box);
#else
        addLabel("OSC/Leap source", x, y, w-comboW, dh, box);
#endif
        {
            mOscLeapSourceCb = new ComboBox();
            int index = 1;
            for (int i = 0; i < mFilter->getNumberOfSources(); i++)
            {
                String s; s << i+1;
                mOscLeapSourceCb->addItem(s, index++);
            }
            
            mOscLeapSourceCb->setSelectedId(mFilter->getOscLeapSource() + 1);
            mOscLeapSourceCb->setSize(comboW, dh);
            mOscLeapSourceCb->setTopLeftPosition(x+w-comboW, y);
            box->addAndMakeVisible(mOscLeapSourceCb);
            mComponents.add(mOscLeapSourceCb);            
            mOscLeapSourceCb->addListener(this);
        }
        y += dh + 4;
#if USE_LEAP           
        mEnableLeap = new ToggleButton();
        mEnableLeap->setButtonText("Enable Leap");
        mEnableLeap->setSize(130, dh);
        mEnableLeap->setTopLeftPosition(x, y);
        mEnableLeap->addListener(this);
        mEnableLeap->setToggleState(false, dontSendNotification);
        mEnableLeap->setColour(ToggleButton::textColourId, mGrisFeel.getFontColour());
        box->addAndMakeVisible(mEnableLeap);
        mComponents.add(mEnableLeap);
        
        mStateLeap = new Label();
        mStateLeap->setText("", dontSendNotification);
        mStateLeap->setSize(cw, dh);
        mStateLeap->setJustificationType(Justification::left);
        mStateLeap->setMinimumHorizontalScale(1);
        mStateLeap->setTopLeftPosition(x+80+ m, y);
        box->addAndMakeVisible(mStateLeap);
        mComponents.add(mStateLeap);
       
        y += dh;
#endif
#if USE_JOYSTICK  
		y += 10;
        mEnableJoystick = new ToggleButton();
        mEnableJoystick->setButtonText("Enable Joystick");
        mEnableJoystick->setSize(130, dh);
        mEnableJoystick->setTopLeftPosition(x, y);
        mEnableJoystick->addListener(this);
        mEnableJoystick->setToggleState(false, dontSendNotification);
        mEnableJoystick->setColour(ToggleButton::textColourId, mGrisFeel.getFontColour());
        box->addAndMakeVisible(mEnableJoystick);
        mComponents.add(mEnableJoystick);
        
        mStateJoystick = new Label();
        mStateJoystick->setText("", dontSendNotification);
        mStateJoystick->setSize(cw, dh);
        mStateJoystick->setJustificationType(Justification::left);
        mStateJoystick->setMinimumHorizontalScale(1);
        mStateJoystick->setTopLeftPosition(x+100+ m, y);
        box->addAndMakeVisible(mStateJoystick);
        mComponents.add(mStateJoystick);
        
        y += dh;
#endif      
#if USE_TOUCH_OSC
        mOsc = CreateOscComponent(mFilter, this);
        if (mOsc) {
            mOsc->setTopLeftPosition(x+m+170, kMargin+10);
            mOsc->setSize(box->getWidth(), box->getHeight()-y);
            box->addAndMakeVisible(mOsc);
            mComponents.add(mOsc);
        }
#endif
    }

    
    int selectedTab = mFilter->getGuiTab();
    if (selectedTab >= 0 && selectedTab < mTabs->getNumTabs())
    {
        bool sendChangeMessage = false;
        mTabs->setCurrentTabIndex(selectedTab, sendChangeMessage);
    }
    
    mTabs->initDone();
    updateMovementModeComboPosition();

    mFilter->setCalculateLevels(true);
    
    //resizable corner
//    m_oResizeLimits.setSizeLimits (960-150, 420-150, 1560, 1020);
    m_oResizeLimits.setSizeLimits (kMinFieldSize + (2*kMargin), kMinFieldSize + (2*kMargin), 1560, 1020);
    addAndMakeVisible (m_pResizer = new ResizableCornerComponent (this, &m_oResizeLimits));
    setSize (mFilter->getGuiWidth(), mFilter->getGuiHeight());
    
 
    startTimerHz (hertzRefresh);
    
}


void SpatGrisAudioProcessorEditor::updateMovementModeComboPosition(){
    /*
    if(mFilter->getGuiTab() == 0){
        int w = (mTabs->getTabContentComponent(0)->getWidth() - kMargin) / 3 - kMargin;
        mMovementModeCombo->setBounds(kMargin, kMargin+kDefaultLabelHeight+5, w, kDefaultLabelHeight);
        mTabs->getTabContentComponent(0)->addAndMakeVisible(mMovementModeCombo);
        mTabs->getTabContentComponent(1)->removeChildComponent(mMovementModeCombo);
    } else if(mFilter->getGuiTab() == 1){
        int cbw = 130;
        int x = 2*cbw + 2*kMargin;
        int y = kMargin + 2 * (kDefaultLabelHeight + 5);
        int w = (mTabs->getTabContentComponent(1)->getWidth() - kMargin) / 3 - kMargin;
        mMovementModeCombo->setBounds(x, y, w, kDefaultLabelHeight);
        mTabs->getTabContentComponent(1)->addAndMakeVisible(mMovementModeCombo);
        mTabs->getTabContentComponent(0)->removeChildComponent(mMovementModeCombo);
    }*/
}

void SpatGrisAudioProcessorEditor::updateInputOutputCombo(){

	if (mInputOutputModeCombo->getNumItems() > 0){
		mInputOutputModeCombo->clear();
	}
    //insert all modes available, based on iMaxSources and iMaxSpeakers
    int iMaxSources = mFilter->getTotalNumInputChannels();
    int iMaxSpeakers;
#if ALLOW_INTERNAL_WRITE
    if (mFilter->getRoutingMode() == kInternalWrite){
        iMaxSpeakers = 16;
    } else {
        iMaxSpeakers = mFilter->getTotalNumOutputChannels();
    }
#else
    iMaxSpeakers = mFilter->getTotalNumOutputChannels();
#endif

    if (iMaxSpeakers >=1)  { mInputOutputModeCombo->addItem("1x1",  i1o1+1);  }
    if (iMaxSpeakers >=2)  { mInputOutputModeCombo->addItem("1x2",  i1o2+1);  }
    if (iMaxSpeakers >=4)  { mInputOutputModeCombo->addItem("1x4",  i1o4+1);  }
    if (iMaxSpeakers >=6)  { mInputOutputModeCombo->addItem("1x6",  i1o6+1);  }
    if (iMaxSpeakers >=8)  { mInputOutputModeCombo->addItem("1x8",  i1o8+1);  }
    if (iMaxSpeakers >=12) { mInputOutputModeCombo->addItem("1x12", i1o12+1); }
    if (iMaxSpeakers >=16) { mInputOutputModeCombo->addItem("1x16", i1o16+1); }
    
    if (iMaxSources >=2 && iMaxSpeakers >=2)  { mInputOutputModeCombo->addItem("2x2",  i2o2+1);  }  //the id here cannot be 0
    if (iMaxSources >=2 && iMaxSpeakers >=4)  { mInputOutputModeCombo->addItem("2x4",  i2o4+1);  }
    if (iMaxSources >=2 && iMaxSpeakers >=6)  { mInputOutputModeCombo->addItem("2x6",  i2o6+1);  }
    if (iMaxSources >=2 && iMaxSpeakers >=8)  { mInputOutputModeCombo->addItem("2x8",  i2o8+1);  }
    if (iMaxSources >=2 && iMaxSpeakers >=12) { mInputOutputModeCombo->addItem("2x12", i2o12+1); }
    if (iMaxSources >=2 && iMaxSpeakers >=16) { mInputOutputModeCombo->addItem("2x16", i2o16+1); }
    
    if (iMaxSources >=4 && iMaxSpeakers >=4)  { mInputOutputModeCombo->addItem("4x4",  i4o4+1);  }
    if (iMaxSources >=4 && iMaxSpeakers >=6)  { mInputOutputModeCombo->addItem("4x6",  i4o6+1);  }
    if (iMaxSources >=4 && iMaxSpeakers >=8)  { mInputOutputModeCombo->addItem("4x8",  i4o8+1);  }
    if (iMaxSources >=4 && iMaxSpeakers >=12) { mInputOutputModeCombo->addItem("4x12", i4o12+1); }
    if (iMaxSources >=4 && iMaxSpeakers >=16) { mInputOutputModeCombo->addItem("4x16", i4o16+1); }
    
    if (iMaxSources >=6 && iMaxSpeakers >=6)  { mInputOutputModeCombo->addItem("6x6",  i6o6+1);  }
    if (iMaxSources >=6 && iMaxSpeakers >=8)  { mInputOutputModeCombo->addItem("6x8",  i6o8+1);  }
    if (iMaxSources >=6 && iMaxSpeakers >=12) { mInputOutputModeCombo->addItem("6x12", i6o12+1); }
    if (iMaxSources >=6 && iMaxSpeakers >=16) { mInputOutputModeCombo->addItem("6x16", i6o16+1); }
    
    if (iMaxSources >=8 && iMaxSpeakers >=8)  { mInputOutputModeCombo->addItem("8x8",  i8o8+1);  }
    if (iMaxSources >=8 && iMaxSpeakers >=12) { mInputOutputModeCombo->addItem("8x12", i8o12+1); }
    if (iMaxSources >=8 && iMaxSpeakers >=16) { mInputOutputModeCombo->addItem("8x16", i8o16+1); }
   
    /*if (iMaxSources >=12 && iMaxSpeakers >=12) { mInputOutputModeCombo->addItem("12x12", i12o12+1); }
    if (iMaxSources >=16 && iMaxSpeakers >=16) { mInputOutputModeCombo->addItem("16x16", i16o16+1); }*/

    //then select the current mode, if it is valid. otherwise change it in mFilter
    int mode = mFilter->getInputOutputMode();
    if (mode > mInputOutputModeCombo->getNumItems()){
        int last = mInputOutputModeCombo->getNumItems();
        int id = mInputOutputModeCombo->getItemId(last-1);
        mFilter->setInputOutputMode(id);
        mInputOutputModeCombo->setSelectedId(id);
    }else{
        mInputOutputModeCombo->setSelectedId(mode);
    }
    
    //updateEditorSources(true);
    updateEditorSpeakers(true);
    //repaintTheStuff();
}

void SpatGrisAudioProcessorEditor::updateEndLocationTextEditors(){
    FPoint endLocation = mFilter->getEndLocationXY01();
    FPoint pointRT = mFilter->convertXy012Rt(FPoint(endLocation.x, 1-endLocation.y), false);
    pointRT.y *= 360/(2*M_PI);
#if WIN32
	m_pTrEndRayTextEditor->setText(toString(pointRT.x));
	m_pTrEndAngleTextEditor->setText(toString(pointRT.y));
#else
    {
        ostringstream oss;
        oss << std::fixed << std::right << std::setw( 4 ) << setprecision(2) << std::setfill( ' ' ) << "" <<  pointRT.x;
        m_pTrEndRayTextEditor->setText(oss.str());
    }
    {
        ostringstream oss;
        oss << std::fixed << std::right << std::setw( 4 ) << setprecision(2) << std::setfill( ' ' ) << "" << pointRT.y;
        m_pTrEndAngleTextEditor->setText(oss.str());
    }
#endif
}

void SpatGrisAudioProcessorEditor::updateRoutingModeComponents(){
    updateInputOutputCombo();
}


void SpatGrisAudioProcessorEditor::updateProcessModeComponents(){
    int iSelectedMode = mFilter->getProcessMode();
    if (iSelectedMode == kOscSpatMode){
        mOscSpat1stSrcIdLabel->setVisible(true);
        mOscSpat1stSrcIdTextEditor->setVisible(true);
        mOscSpatPortLabel->setVisible(true);
        mOscSpatPortTextEditor->setVisible(true);
        mOscActiveButton->setVisible(true);
        
        mAzimSpanSlider->setEnabled(true);
        mAzimSpanLabel->setEnabled(true);
        mAzimSpanLinkButton->setEnabled(true);
        mElevSpanSlider->setEnabled(true);
        mElevSpanLabel->setEnabled(true);
        mElevSpanLinkButton->setEnabled(true);
        
        mSpeakersBox->setEnabled(false);
        mSmoothingLabel->setEnabled(false);
        mSmoothingSlider->setEnabled(false);
#if ALLOW_INTERNAL_WRITE
        mRoutingModeCombo->setEnabled(false);
        mRoutingModeLabel->setEnabled(false);
#endif
        mMaxSpanVolumeLabel->setEnabled(false);
        mMaxSpanVolumeSlider->setEnabled(false);
        mTabs->getTabContentComponent(1)->setEnabled(false);
        mTabs->getTabContentComponent(3)->setEnabled(false);
    } else {
        mOscSpat1stSrcIdLabel->setVisible(false);
        mOscSpat1stSrcIdTextEditor->setVisible(false);
        mOscSpatPortLabel->setVisible(false);
        mOscSpatPortTextEditor->setVisible(false);
        mOscActiveButton->setVisible(false);

        mAzimSpanSlider->setEnabled(false);
        mAzimSpanLabel->setEnabled(false);
        mAzimSpanLinkButton->setEnabled(false);
        mElevSpanSlider->setEnabled(false);
        mElevSpanLabel->setEnabled(false);
        mElevSpanLinkButton->setEnabled(false);

        mSpeakersBox->setEnabled(true);
        mSmoothingLabel->setEnabled(true);
        mSmoothingSlider->setEnabled(true);
#if ALLOW_INTERNAL_WRITE
        mRoutingModeCombo->setEnabled(true);
        mRoutingModeLabel->setEnabled(true);
#endif
        mMaxSpanVolumeLabel->setEnabled(true);
        mMaxSpanVolumeSlider->setEnabled(true);
        mTabs->getTabContentComponent(1)->setEnabled(true);
        mTabs->getTabContentComponent(3)->setEnabled(true);
    }
#if ALLOW_PAN_MODE
    if (iSelectedMode == kPanMode){
        mSurfaceOrPanSlider->setEnabled(false);
        mSurfaceOrPanLabel->setEnabled(false);
        mSurfaceOrPanLinkButton->setEnabled(false);
    } else {
        mSurfaceOrPanSlider->setEnabled(true);
        mSurfaceOrPanLabel->setEnabled(true);
        mSurfaceOrPanLinkButton->setEnabled(true);
        mSurfaceOrPanSlider->valueChanged();
    }
#endif
    if (mFilter->getProcessMode() == kSpanMode){
        mSurfaceOrPanLabel->setEnabled(true);
        mSurfaceOrPanSlider->setIsPanSpan(true, false);
        mSurfaceOrPanSlider->setEnabled(true);
        mSurfaceOrPanLinkButton->setEnabled(true);
        static_cast<Label*>(mSurfaceOrPanLabel)->setText("Pan span", dontSendNotification);
    } else if (mFilter->getProcessMode() == kFreeVolumeMode){
        mSurfaceOrPanSlider->setIsPanSpan(false);
        mSurfaceOrPanLabel->setEnabled(true);
        mSurfaceOrPanSlider->setEnabled(true);
        mSurfaceOrPanLinkButton->setEnabled(true);
        static_cast<Label*>(mSurfaceOrPanLabel)->setText("Surface", dontSendNotification);
    } else {
        mSurfaceOrPanSlider->setEnabled(false);
        mSurfaceOrPanLabel->setEnabled(false);
        mSurfaceOrPanLinkButton->setEnabled(false);
    }
    mNeedRepaint = true;
    mFieldNeedRepaint = true;
}

void SpatGrisAudioProcessorEditor::updateTrajectoryTypeComponents(){
    int iSelectedTrajectory = mFilter->getTrType();
    //if pendulum is selected

    if (iSelectedTrajectory == Pendulum){
        setDefaultPendulumEndpoint();
        updateEndLocationTextEditors();
        
        mTrDampeningTextEditor->setVisible(true);
        mTrDampeningLabel->setVisible(true);
        mTrDeviationTextEditor->setVisible(true);
        mTrDeviationLabel->setVisible(true);
    } else {
        mTrDampeningTextEditor->setVisible(false);
        mTrDampeningLabel->setVisible(false);
        mTrDeviationTextEditor->setVisible(false);
        mTrDeviationLabel->setVisible(false);
    }
    
    if (iSelectedTrajectory == Circle || iSelectedTrajectory == EllipseTr || iSelectedTrajectory == Spiral){
        mTrTurnsTextEditor->setVisible(true);
        mTrTurnsLabel->setVisible(true);
    } else {
        mTrTurnsTextEditor->setVisible(false);
        mTrTurnsLabel->setVisible(false);
    }

    if (iSelectedTrajectory == EllipseTr){
        mTrEllipseWidthTextEditor->setVisible(true);
        mTrEllipseWidthLabel->setVisible(true);
    } else {
        mTrEllipseWidthTextEditor->setVisible(false);
        mTrEllipseWidthLabel->setVisible(false);
    }
    
    if (iSelectedTrajectory == Spiral || iSelectedTrajectory == Pendulum){
        mTrEndPointButton->setVisible(true);
        m_pTrEndRayTextEditor->setVisible(true);
        m_pTrEndAngleTextEditor->setVisible(true);
        m_pTrResetEndButton->setVisible(true);
    } else {
        mTrEndPointButton->setVisible(false);
        m_pTrEndRayTextEditor->setVisible(false);
        m_pTrEndAngleTextEditor->setVisible(false);
        m_pTrResetEndButton->setVisible(false);
    }

    if (iSelectedTrajectory == RandomTarget || iSelectedTrajectory == RandomTrajectory){
        mTrSeparateAutomationModeButton->setVisible(true);
    } else {
        mTrSeparateAutomationModeButton->setVisible(false);
    }
    
    unique_ptr<vector<String>> allDirections = Trajectory::getAllPossibleDirections(iSelectedTrajectory);
    if (allDirections != nullptr){
        mTrDirectionComboBox->clear();
        for(auto it = allDirections->begin(); it != allDirections->end(); ++it){
            mTrDirectionComboBox->addItem(*it, it - allDirections->begin()+1);
        }
        mTrDirectionComboBox->setVisible(true);
        
        int iSelectedDirection = mFilter->getTrDirection()+1;
        
        if (iSelectedDirection > allDirections->size()){
            iSelectedDirection = 1;
            mFilter->setTrDirection(iSelectedDirection);
        }
        mTrDirectionComboBox->setSelectedId(iSelectedDirection);
        
    } else {
        mTrDirectionComboBox->setVisible(false);
    }
    
    unique_ptr<vector<String>> allReturns = Trajectory::getAllPossibleReturns(iSelectedTrajectory);
    if (allReturns != nullptr){
        mTrReturnComboBox->clear();
        for(auto it = allReturns->begin(); it != allReturns->end(); ++it){
            mTrReturnComboBox->addItem(*it, it - allReturns->begin()+1);
        }
        mTrReturnComboBox->setVisible(true);
        mTrReturnComboBox->setSelectedId(mFilter->getTrReturn()+1);
    } else {
        mTrReturnComboBox->setVisible(false);
    }
}

SpatGrisAudioProcessorEditor::~SpatGrisAudioProcessorEditor()
{
    
    mFilter->setCalculateLevels(false);
    mFilter->removeListener(this);

#if USE_JOYSTICK
    if(mEnableJoystick->getToggleState()) {
//        m_pJoystickUpdateThread->stopThread(100);
        IOHIDManagerUnscheduleFromRunLoop(gIOHIDManagerRef, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
        IOHIDManagerRegisterInputValueCallback(gIOHIDManagerRef, NULL,this);
        IOHIDManagerClose(gIOHIDManagerRef, kIOHIDOptionsTypeNone);
        gIOHIDManagerRef = NULL;
        gDeviceCFArrayRef = NULL;
        gElementCFArrayRef = NULL;
    }
    mJoystick = NULL;
#endif
#if USE_LEAP
    if(mController) {
		mController->enableGesture(Leap::Gesture::TYPE_INVALID);
		mController->removeListener(*mleap);
		gIsLeapConnected = 0;
		mController.release();
	}
    m_pMover->end(kLeap);
    m_pMover->end(kHID);
#endif
    
}

void SpatGrisAudioProcessorEditor::resized()
{
    int w = getWidth();
    int h = getHeight();
    
    mFilter->setGuiWidth(w);
    mFilter->setGuiHeight(h);
    
    m_pResizer->setBounds (w - 16, h - 16, 16, 16);
    
    int fieldWidth = w - (kMargin + kMargin + kCenterColumnWidth + kMargin + kRightColumnWidth + kMargin);
    int fieldHeight = h - (kMargin + kMargin);
    int fieldSize = jmin(fieldWidth, fieldHeight);
    if (fieldSize < kMinFieldSize){
        fieldSize = kMinFieldSize;
    }

    mField->setBounds(kMargin, kMargin, fieldSize, fieldSize);
    mFilter->setFieldWidth(fieldSize);

    m_logoImage.setBounds(15, 15, (float)fieldSize/7, (float)fieldSize/7);
    
    int iLabelX = 2*(float)fieldSize/3;
    m_VersionLabel->setBounds(iLabelX,5,fieldSize-iLabelX,25);
    
    int x = kMargin + fieldSize + kMargin + kMargin;
    int y = kMargin;
    int iExtraSpace = 2;
    mSourcesBoxLabel->setTopLeftPosition(x, y);
    
    int lh = mSourcesBoxLabel->getHeight();
    mSourcesBox->setBounds(x, y + lh, kCenterColumnWidth, 155);
    
    mTrajectoryBoxLabel->setTopLeftPosition(x, 176);
    mTrajectoryBox->setBounds(x, 192 , w-(fieldSize + (iExtraSpace * 5)), 170);

    mTabs->setBounds(x-1, 180+184+iExtraSpace, w-(fieldSize + (iExtraSpace * 5)), h - (180+186 + iExtraSpace));

    x += kCenterColumnWidth + kMargin + kMargin;
    mSpeakersBoxLabel->setTopLeftPosition(x, y);
    mSpeakersBox->setBounds(x, y + lh, w-(fieldSize+ kCenterColumnWidth + (iExtraSpace * 7)), 155);
}

void SpatGrisAudioProcessorEditor::updateEditorSources(bool p_bCalledFromConstructor){
    const MessageManagerLock mmLock;
    
    //if we're not in constructor, clear source and movement constraint combos, and ensure movement constraint is valid
    if (!p_bCalledFromConstructor){
        mSrcSelectCombo->clear(dontSendNotification);
        mMovementModeCombo->clear(dontSendNotification);
        updateMovementModeCombo();
        //update number of sources in mover. this puts all sources at 0,0, ie, bottom left corner
        m_pMover->updateNumberOfSources();
        //if we're not in constructor, reset source placement
        buttonClicked(mApplySrcPlacementButton);
    }

    //update content of source combobox
    int index = 1;
    int iNumSources = mFilter->getNumberOfSources();
    for (int i = 0; i < iNumSources; i++){
        String s; s << i+1;
        mSrcSelectCombo->addItem(s, index++);
    }
    mSrcSelectCombo->setSelectedId(m_iSelectedSrcEditor+1);
}


void SpatGrisAudioProcessorEditor::updateEditorSpeakers(bool p_bCalledFromConstructor){
    const MessageManagerLock mmLock;
    //remove old stuff
    Component *ct = mSpeakersBox->getContent();
    
//    for (int iCurLevelComponent = 0; iCurLevelComponent < mMuteButtons.size(); ++iCurLevelComponent){
//        ct->removeChildComponent(mMuteButtons.getUnchecked(iCurLevelComponent));
//#if USE_DB_METERS
//        ct->removeChildComponent(mLevelComponents.getUnchecked(iCurLevelComponent));
//        mComponents.removeObject(mLevelComponents.getUnchecked(iCurLevelComponent));
//#endif
//    }
    
//    mMuteButtons.clear();
    mSpSelectCombo->clear();
#if USE_DB_METERS
//    mLevelComponents.clear();
#endif
    
    //put new stuff
    int iCurSpeakers = mFilter->getNumberOfSpeakers();
   	int dh = kDefaultLabelHeight, x = 0, y = 0, w = kRightColumnWidth;
    
    const int muteWidth = 50;
    y += dh + 4;

    for (int i = 0; i < kMaxChannels; i++){
//        String s; s << i+1;
//#if USE_DB_METERS
//        s << ":";
//#endif
//        
//		float fMute = mFilter->getSpeakerM(i);
//		ToggleButton *mute = addCheckbox(s, fMute, x, y, muteWidth, dh, ct);
//        mute->setColour(ToggleButton::textColourId, mGrisFeel.getFontColour());
//        mMuteButtons.add(mute);
//        const int muteWidth = 50;
//#if USE_DB_METERS
//        juce::Rectangle<int> level(x+muteWidth, y + 3, w/3 - 10, dh - 6);
//        LevelComponent *lc = new LevelComponent(mFilter, i);
//        lc->setBounds(level);
//        ct->addAndMakeVisible(lc);
//        mComponents.add(lc);
//        mLevelComponents.add(lc);
//#endif
        if (i < iCurSpeakers){
            y += dh + 4;
            mLevelComponents[i]->setVisible(true);
            mMuteButtons[i]->setVisible(true);
            mLabelSourceId[i]->setVisible(true);
        } else {
            mLevelComponents[i]->setVisible(false);
            mMuteButtons[i]->setVisible(false);
             mLabelSourceId[i]->setVisible(false);
        }
    }
    //ensure box height is not smaller than mRoutingVolumeSlider
    if (y < 200 + 2*dh + 10){
        y = 200 + 2*dh + 10;
    }
    ct->setSize(y, 148);
    
    if (!p_bCalledFromConstructor){
        buttonClicked(mApplySpPlacementButton); 
    }
    
    //speaker position combo box in speakers tab
    int index = 1;
    for (int i = 0; i < iCurSpeakers; i++){
        String s; s << i+1;
        mSpSelectCombo->addItem(s, index++);
    }
    mSpSelectCombo->setSelectedId(mFilter->getSpSelected());
}

void SpatGrisAudioProcessorEditor::updateMovementModeCombo(){
    mMovementModeCombo->addItem("Independent", Independent+1);
    if (mFilter->getNumberOfSources() > 1){
        mMovementModeCombo->addItem("Circular",                 Circular+1);
        mMovementModeCombo->addItem("Circular Fixed Radius",    CircularFixedRadius+1);
        mMovementModeCombo->addItem("Circular Fixed Angle",     CircularFixedAngle+1);
        mMovementModeCombo->addItem("Circular Fully Fixed",     CircularFullyFixed+1);
        mMovementModeCombo->addItem("Delta Lock",               DeltaLock+1);
        if (mFilter->getNumberOfSources() == 2){
            mMovementModeCombo->addItem("Symmetric X",          SymmetricX+1);
            mMovementModeCombo->addItem("Symmetric Y",          SymmetricY+1);
        }
    }
    int iCurMode = mFilter->getMovementMode() + 1;
    if (mMovementModeCombo->getItemText(iCurMode-1) == ""){
        mMovementModeCombo->setSelectedId(1);
        mFilter->setMovementMode(Independent);
    } else {
        mMovementModeCombo->setSelectedId(iCurMode);
    }
}


void SpatGrisAudioProcessorEditor::setOscLeapSource(int s)
{
    if (s < 0) s = 0;
    if (s >= mFilter->getNumberOfSources()) s = mFilter->getNumberOfSources() - 1;
    mFilter->setOscLeapSource(s);
    
    const MessageManagerLock mmLock;
    mOscLeapSourceCb->setSelectedId(s + 1);
}



//==============================================================================
Component* SpatGrisAudioProcessorEditor::addLabel(const String &s, int x, int y, int w, int h, Component *into)
{
    Label *label = new Label();
    label->setText(s, dontSendNotification);
    label->setSize(w, h);
    label->setJustificationType(Justification::left);
    label->setMinimumHorizontalScale(1);
    label->setTopLeftPosition(x, y);
    label->setColour(Label::textColourId, mGrisFeel.getFontColour());
    label->setLookAndFeel(&mGrisFeel);
    into->addAndMakeVisible(label);
    mComponents.add(label);
    return label;
}

ToggleButton* SpatGrisAudioProcessorEditor::addCheckbox(const String &s, bool v, int x, int y, int w, int h, Component *into)
{
    ToggleButton *tb = new ToggleButton();
    tb->setButtonText(s);
    tb->setSize(w, h);
    tb->setTopLeftPosition(x, y);
    tb->addListener(this);
    tb->setToggleState(v, dontSendNotification);
    tb->setColour(ToggleButton::textColourId, mGrisFeel.getFontColour());
    tb->setLookAndFeel(&mGrisFeel);
    into->addAndMakeVisible(tb);
    mComponents.add(tb);
    return tb;
}

TextButton* SpatGrisAudioProcessorEditor::addButton(const String &s, int x, int y, int w, int h, Component *into)
{
    TextButton *tb = new TextButton();
    tb->setButtonText(s);
    tb->setSize(w, h);
    tb->setTopLeftPosition(x, y);
    tb->addListener(this);
    tb->setLookAndFeel(&mGrisFeel);
    into->addAndMakeVisible(tb);
    mComponents.add(tb);
    return tb;
}


TextEditor* SpatGrisAudioProcessorEditor::addTextEditor(const String &s, int x, int y, int w, int h, Component *into)
{
    TextEditor *te = new TextEditor();
    te->setFont(mGrisFeel.getFont());
    te->setText(s);
    te->setSize(w, h);
    te->setTopLeftPosition(x, y);
    te->setLookAndFeel(&mGrisFeel);
    into->addAndMakeVisible(te);
    mComponents.add(te);
    return te;
}

ParamSliderGRIS* SpatGrisAudioProcessorEditor::addParamSliderGRIS(paramTypes paramType, int si, float v, int x, int y, int w, int h, Component *into)
{
    int index ;
    ParamSliderGRIS *ds;
    
    switch(paramType) {
        case kParamSource :
            index = mFilter->getParamForSourceD(si);
            ds = new ParamSliderGRIS(index, paramType, mSurfaceOrPanLinkButton, mFilter);
            break;
        
        case kParamAzimSpan :
            index = mFilter->getParamForSourceAzimSpan(si);
            ds = new ParamSliderGRIS(index, paramType, mAzimSpanLinkButton, mFilter);
            break;
            
        case kParamElevSpan :
            index = mFilter->getParamForSourceElevSpan(si);
            ds = new ParamSliderGRIS(index, paramType, mElevSpanLinkButton, mFilter);
            break;
        
        default:
            index = si;
            ds = new ParamSliderGRIS(index, paramType, NULL, mFilter);
        
    }
    
    ds->setRange(0, 1);

    ds->setValue(v);
    ds->setTextBoxStyle(Slider::NoTextBox, true,0,0);
    ds->setSize(w, h);
    ds->setTopLeftPosition(x, y);
    ds->setLookAndFeel(&mGrisFeel);
    into->addAndMakeVisible(ds);
    mComponents.add(ds);
    return ds;
}


//==============================================================================
void SpatGrisAudioProcessorEditor::textEditorReturnKeyPressed(TextEditor & textEditor){
    if (&textEditor == mSrcR || &textEditor == mSrcT){
        int src = mSrcSelectCombo->getSelectedId() - 1;
        float r = mSrcR->getText().getFloatValue();
        float t = mSrcT->getText().getFloatValue();
        if (r < 0) r = 0; else if (r > kRadiusMax) r = kRadiusMax;

        m_pMover->begin(src, kField);
        m_pMover->move(mFilter->convertRt2Xy01(r, t * M_PI / 180.), kField);
        m_pMover->end(kField);
    }
    else if (&textEditor == mSpR || &textEditor == mSpT) {
        int sp = mSpSelectCombo->getSelectedId() - 1;
        float r = mSpR->getText().getFloatValue();
        float t = mSpT->getText().getFloatValue();
        if (r < 0) r = 0; else if (r > kRadiusMax) r = kRadiusMax;
        mFilter->setSpeakerRT(sp, FPoint(r, t * M_PI / 180.));
    }
    else if (&textEditor == mTrDuration){
        float duration = mTrDuration->getText().getFloatValue();
        if (duration >= 0 && duration <= 10000){
            mFilter->setTrDuration(duration);
        }
        mTrDuration->setText(String(mFilter->getTrDuration()));
    }
    else if (&textEditor == mTrRepeats){
        float repeats = mTrRepeats->getText().getFloatValue();
        if (repeats >= 0 && repeats <= 10000){
            mFilter->setTrRepeats(repeats);
        }
        mTrRepeats->setText(String(mFilter->getTrRepeats()));
    }
    else if (&textEditor == mTrDampeningTextEditor){
        float dampening = mTrDampeningTextEditor->getText().getFloatValue();
        if (dampening >= 0 && dampening <= 1){
            mFilter->setTrDampening(dampening);
        }
        mTrDampeningTextEditor->setText(String(mFilter->getTrDampening()));
    }
    else if (&textEditor == mTrDeviationTextEditor){
        float deviation = mTrDeviationTextEditor->getText().getFloatValue()/360;
        if (deviation >= 0 && deviation <= 1){
            mFilter->setTrDeviation(deviation);
        }
        mTrDeviationTextEditor->setText(String(mFilter->getTrDeviation()*360));
    }
    else if (&textEditor == mTrEllipseWidthTextEditor){
        float fHalfWidth = mTrEllipseWidthTextEditor->getText().getFloatValue()/2;
        if (fHalfWidth >= 0.05 && fHalfWidth <= 1){
            mFilter->setTrEllipseWidth(fHalfWidth);
        }
        mTrEllipseWidthTextEditor->setText(String(mFilter->getTrEllipseWidth()*2));
    }
    else if (&textEditor == mTrTurnsTextEditor){
        float Turns = mTrTurnsTextEditor->getText().getFloatValue();
        int iSelectedTrajectory = mFilter->getTrType();
        int iUpperLimit = 1;
        if (iSelectedTrajectory == 3){
            iUpperLimit = 10;
        }
        if (Turns > 0 && Turns <= iUpperLimit){
            mFilter->setTrTurns(Turns);
        }
        mTrTurnsTextEditor->setText(String(mFilter->getTrTurns()));
    }
    else if (&textEditor == mOscSpat1stSrcIdTextEditor){
        int i1stSrcId = mOscSpat1stSrcIdTextEditor->getText().getIntValue();
        if (i1stSrcId >= 1 && i1stSrcId <= 99-7){
            mFilter->setOscSpat1stSrcId(i1stSrcId);
            mFieldNeedRepaint = true;
        }
        mOscSpat1stSrcIdTextEditor->setText(String(mFilter->getOscSpat1stSrcId()));
    }
    else if (&textEditor == mOscSpatPortTextEditor){
        int iPort = mOscSpatPortTextEditor->getText().getIntValue();
        if (iPort >= 1 && iPort <= 100000){
            mFilter->setOscSpatPort(iPort);
            mFilter->connectOscSpat();
        }
        mOscSpatPortTextEditor->setText(String(mFilter->getOscSpatPort()));
    }
    else if (&textEditor == m_pTrEndRayTextEditor || &textEditor == m_pTrEndAngleTextEditor){
        if (mTrEndPointButton->getToggleState()){
            return;
        }
        float fEndRay   = m_pTrEndRayTextEditor->getText().getFloatValue();
        float fEndAngle = m_pTrEndAngleTextEditor->getText().getFloatValue();
        if (fEndRay >= 0 && fEndRay <= 2 && fEndAngle >= 0 && fEndAngle <= 360 ){
            FPoint endPoint = mFilter->convertRt2Xy01(fEndRay, fEndAngle*M_PI/180);
            mFilter->setEndLocationXY01(FPoint (endPoint.x, 1-endPoint.y));
        }
        updateEndLocationTextEditors();
    } else {
        printf("unknown TextEditor clicked...\n");
    }
    
    //if called from actually pressing enter, put focus on something else
    if (!m_bIsReturnKeyPressedCalledFromFocusLost){
        mMovementModeCombo->grabKeyboardFocus();
    }
}


void SpatGrisAudioProcessorEditor::buttonClicked (Button *button){
    for (int i = 0; i < mFilter->getNumberOfSpeakers(); i++) {
        if (button == mMuteButtons[i]) {
            float v = button->getToggleState() ? 1.f : 0.f;
            mFilter->setParameterNotifyingHost(mFilter->getParamForSpeakerM(i), v);
            mFieldNeedRepaint = true;
            mLevelComponents[i]->setMute(button->getToggleState());
            return;
        }
    }
    
    if (button == mTrWriteButton) {
        Trajectory::Ptr t = mFilter->getTrajectory();
        //if a trajectory exists
        if (t) {
            t->stop();                              //stop it
            mFilter->setTrajectory(nullptr);        //delete it
            updateTrajectoryStartComponent(kSetOff);  //re-activate trajectory components
            mFieldNeedRepaint = true;
        }
        //a trajectory does not exist, create one
        else {
            TrajectoryProperties properties;
            properties.type         = mTrTypeComboBox->getSelectedId();
            properties.filter       = mFilter;
            properties.mover        = m_pMover;
            properties.duration     = mTrDuration->getText().getFloatValue();
            properties.beats        = mTrUnits->getSelectedId() == 1;
            properties.direction    = Trajectory::getCurDirection(properties.type, mTrDirectionComboBox->getSelectedId());
            properties.bReturn      = (mTrReturnComboBox->getSelectedId() == 2);
            properties.repeats      = mTrRepeats->getText().getFloatValue();
            properties.dampening    = mTrDampeningTextEditor->getText().getFloatValue();
            properties.deviation    = mTrDeviationTextEditor->getText().getFloatValue()/360;
            properties.turns        = mTrTurnsTextEditor->getText().getFloatValue();
            properties.width        = mTrEllipseWidthTextEditor->getText().getFloatValue();
            properties.endPoint     = mFilter->getEndLocationXY01();
            
            mFilter->setTrajectory(Trajectory::CreateTrajectory(properties));
            
            updateTrajectoryStartComponent(kSetOn);
        }
    }
    else if (button == mTrEndPointButton) {
        if (mTrEndPointButton->getToggleState()){
            mTrEndPointButton->setButtonText("Cancel");
            mFilter->setIsSettingEndPoint(true);
            mTrEndPointLabel->setVisible(true);
            m_pTrEndRayTextEditor->setText("");
            m_pTrEndAngleTextEditor->setText("");
            m_pTrEndRayTextEditor->setReadOnly(true);
            m_pTrEndAngleTextEditor->setReadOnly(true);
            mTrEndPointLabel->setVisible(true);
        } else {
            mTrEndPointButton->setButtonText("Set end point");
            mFilter->setIsSettingEndPoint(false);
            mTrEndPointLabel->setVisible(false);
            updateEndLocationTextEditors();
            mTrEndPointLabel->setVisible(false);
            m_pTrEndRayTextEditor->setReadOnly(false);
            m_pTrEndAngleTextEditor->setReadOnly(false);
        }
    }
    else if (button == m_pTrResetEndButton) {
        if (mFilter->getTrType() == Pendulum){
            setDefaultPendulumEndpoint();
        } else {
            FPoint point (.5, .5);
            mFilter->setEndLocationXY01(point);
        }
        updateEndLocationTextEditors();
    }
    else if (mFilter->getIsAllowInputOutputModeSelection() && button == mApplyInputOutputModeButton) {
        //update mode in processor, which will update its number of sources and speakers
        int iSelectedMode = mInputOutputModeCombo->getSelectedId();
        mFilter->setInputOutputMode(iSelectedMode);
        
		updateEditorSources(false);
		updateEditorSpeakers(false);
        
        //if we're loading a preset, make sure we keep the current source and speaker locations
        if (m_bLoadingPreset){
            mFilter->restoreCurrentLocations(-1);
            m_bLoadingPreset = false;
        }
        //repaint the field
        mFieldNeedRepaint = true;
        
        //ensure movement mode stays valid
        if (iSelectedMode == i1o1 || iSelectedMode == i1o2 || iSelectedMode == i1o4 || iSelectedMode == i1o6 || iSelectedMode == i1o8 || iSelectedMode == i1o16){
            mMovementModeCombo->setSelectedId(1);
        }
    }
    else if (button == mApplySrcPlacementButton) {
        applyCurrentSrcPlacement();
    }
    else if (button == mApplySpPlacementButton) {
        applyCurrentSpkPlacement();
    }
    else if (button == mShowGridLines) {
        mFilter->setShowGridLines(button->getToggleState());
        mFieldNeedRepaint = true;
    }
    else if (button == mOscActiveButton) {
        mFilter->setOscActive(button->getToggleState());
    }
    else if (button == mTrSeparateAutomationModeButton) {
        mFilter->setIndependentMode(button->getToggleState());
    }
    else if (button == mSurfaceOrPanLinkButton) {
        mFilter->setLinkSurfaceOrPan(button->getToggleState());
    }
    else if (button == mAzimSpanLinkButton) {
        mFilter->setLinkAzimSpan(button->getToggleState());
    }
    else if (button == mElevSpanLinkButton) {
        mFilter->setLinkElevSpan(button->getToggleState());
    }
    else if (button == mApplyFilterButton) {
        mFilter->setApplyFilter(button->getToggleState());
    }
    else if (button == mApplyOutputRamping) {
        mFilter->setApplyOutRamp(button->getToggleState());
    }
    
#if USE_JOYSTICK
    //Changements lié a l'ajout de joystick à l'onglet interface
    else if(button == mEnableJoystick) {
        bool bJoystickEnabled = mEnableJoystick->getToggleState();
        mFilter->setIsJoystickEnabled(bJoystickEnabled);
        if (bJoystickEnabled) {
            if (!gIOHIDManagerRef) {
                mStateJoystick->setText("not found", dontSendNotification);
                mStateJoystick->setColour(Label::textColourId,  Colour(235, 3, 25));
                gIOHIDManagerRef = IOHIDManagerCreate(CFAllocatorGetDefault(),kIOHIDOptionsTypeNone);
                if(!gIOHIDManagerRef) {
                    printf("Could not create IOHIDManager");
                } else {
                    mJoystick = HIDDelegate::CreateHIDDelegate(mFilter, this);
                    mJoystick->Initialize_HID(this);
                    if(mJoystick->getDeviceSetRef()) {
                        mStateJoystick->setText("connected", dontSendNotification);
                        mStateJoystick->setColour(Label::textColourId, Colour(3,235, 25));
                    } else {
                        mStateJoystick->setText("not found", dontSendNotification);
                        mEnableJoystick->setToggleState(false, dontSendNotification);
                        gIOHIDManagerRef = NULL;
                        mStateJoystick->setColour(Label::textColourId,  Colour(235, 3, 25));
                    }
                }
            } else {
                mEnableJoystick->setToggleState(false, dontSendNotification);
                mStateJoystick->setText("Joystick connected to another Octogris", dontSendNotification);
            }
        } else {
            if(gIOHIDManagerRef) {
                IOHIDManagerUnscheduleFromRunLoop(gIOHIDManagerRef, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
                IOHIDManagerRegisterInputValueCallback(gIOHIDManagerRef, NULL,this);
                IOHIDManagerClose(gIOHIDManagerRef, kIOHIDOptionsTypeNone);
                gIOHIDManagerRef = NULL;
                gDeviceCFArrayRef = NULL;
                gElementCFArrayRef = NULL;
                mJoystick = NULL;
                mStateJoystick->setText("", dontSendNotification);
            }
        }
    }
#endif   
    
#if USE_LEAP
    else if(button == mEnableLeap) {
        bool state = mEnableLeap->getToggleState();
        
        if (state) {
            if (!gIsLeapConnected) {
                mStateLeap->setText("not found", dontSendNotification);
                mStateLeap->setColour(Label::textColourId,  Colour(235, 3, 25));
                mController = new Leap::Controller();
                if(!mController) {
                    printf("Could not create leap controler");
                } else {
                    mleap = OctoLeap::CreateLeapComponent(mFilter, this);
                    if(mleap) {
                        gIsLeapConnected = 1;
                        mController->addListener(*mleap);
                    } else {
                        mStateLeap->setText("not found", dontSendNotification);
                        mStateLeap->setColour(Label::textColourId,  Colour(235, 3, 25));
                    }
                }
            } else {
                mStateLeap->setText("already used", dontSendNotification);
                mStateLeap->setColour(Label::textColourId, Colour(255, 125,0));
                mEnableLeap->setToggleState(false, dontSendNotification);
               
            }
        } else {
            if(gIsLeapConnected) {
                mController->enableGesture(Leap::Gesture::TYPE_INVALID);
                mController->removeListener(*mleap);
				gIsLeapConnected = 0;
				mController.release();
                mStateLeap->setText("", dontSendNotification);
            }
        }
    }
	//fin de changements lié a l'ajout de joystick à l'onglet leap
#endif    
    
 else {
		printf("unknown button clicked...\n");
	}
}

void SpatGrisAudioProcessorEditor::applyCurrentSrcPlacement(){
    //if only one source, put it in middle
    if (mFilter->getNumberOfSources() == 1){
        mFilter->setSourceRT(0, FPoint(0, 0));
        return;
    }
    //figure out current options
    bool alternate = false;
    bool startAtTop = false;
    bool clockwise = false;
    
    bool bIsStuffConstructedYet = (mSrcPlacementCombo->getNumItems() !=0) ? true : false;
    
    int iCurrentOption = (bIsStuffConstructedYet) ? mSrcPlacementCombo->getSelectedId() : kLeftAlternate;
    
    switch (iCurrentOption){
        case kLeftAlternate:
            alternate = true;
            break;
        case kTopClockwise:
            startAtTop = true;
            clockwise = true;
            break;
        case kTopCounterClockwise:
            startAtTop = true;
            break;
        case kLeftClockwise:
            clockwise = true;
            break;
        case kLeftCounterClockWise:
            break;
            
    }
    float anglePerSp = kThetaMax / mFilter->getNumberOfSources();
    JUCE_COMPILER_WARNING("this stuff is kind of a replication of processor::setNumberOfSources, although setNumberOfSources is only for default placement")
    if (alternate) {
        float offset = startAtTop
        ? (clockwise ? kQuarterCircle : (kQuarterCircle - anglePerSp))
        : (kQuarterCircle - anglePerSp/2);
        float start = offset;
        for (int i = clockwise ? 0 : 1; i < mFilter->getNumberOfSources(); i += 2) {
            mFilter->setSourceRT(i, FPoint(kSourceDefaultRadius, offset));
            offset -= anglePerSp;
        }
        
        offset = start + anglePerSp;
        for (int i = clockwise ? 1 : 0; i < mFilter->getNumberOfSources(); i += 2) {
            mFilter->setSourceRT(i, FPoint(kSourceDefaultRadius, offset));
            offset += anglePerSp;
        }
    } else {
        float offset = startAtTop ? kQuarterCircle : kQuarterCircle + anglePerSp/2;
        float delta = clockwise ? -anglePerSp : anglePerSp;
        for (int i = 0; i < mFilter->getNumberOfSources(); i++) {
            mFilter->setSourceRT(i, FPoint(1, offset));
            offset += delta;
        }
    }
    if (bIsStuffConstructedYet){
        updateSourceLocationTextEditor(false);
        mFilter->setSrcPlacementMode(mSrcPlacementCombo->getSelectedId());
    }
}

void SpatGrisAudioProcessorEditor::applyCurrentSpkPlacement(){
    bool alternate = false;
    bool startAtTop = false;
    bool clockwise = false;
    
    bool bIsStuffConstructedYet = (mSpPlacementCombo->getNumItems() !=0) ? true : false;
    int iCurrentOption = (bIsStuffConstructedYet) ? mSpPlacementCombo->getSelectedId() : kLeftAlternate;
    
    switch (iCurrentOption){
        case kLeftAlternate:
            alternate = true;
            break;
        case kTopClockwise:
            startAtTop = true;
            clockwise = true;
            break;
        case kTopCounterClockwise:
            startAtTop = true;
            break;
        case kLeftClockwise:
            clockwise = true;
            break;
        case kLeftCounterClockWise:
            break;
    }
    
    mFilter->updateSpeakerLocation(alternate, startAtTop, clockwise);
    if (bIsStuffConstructedYet){
        updateSpeakerLocationTextEditor();
        mFilter->setSpPlacementMode(mSpPlacementCombo->getSelectedId());
    }
}

void SpatGrisAudioProcessorEditor::setDefaultPendulumEndpoint(){
    FPoint pointRT      = mFilter->getSourceRT(m_iSelectedSrcEditor);
    pointRT.y += M_PI;
    FPoint pointXY = mFilter->convertRt2Xy01(pointRT.x, pointRT.y);
    mFilter->setEndLocationXY01(FPoint(pointXY.x, 1-pointXY.y));
}


void SpatGrisAudioProcessorEditor::textEditorFocusLost (TextEditor &textEditor){

    m_bIsReturnKeyPressedCalledFromFocusLost = true;
    textEditorReturnKeyPressed(textEditor);
    m_bIsReturnKeyPressedCalledFromFocusLost = false;
}

void SpatGrisAudioProcessorEditor::comboBoxChanged (ComboBox* comboBox)
{
    if (comboBox == mMovementModeCombo) {
        int iSelectedMode = comboBox->getSelectedId() - 1;
        mFilter->setMovementMode(iSelectedMode);
        if(mFilter->getNumberOfSources() > 1){
            switch (iSelectedMode) {
                case CircularFixedRadius:
                    m_pMover->setEqualRadius();
                    break;
                case CircularFixedAngle:
                    m_pMover->setEqualAngles();
                    break;
                case CircularFullyFixed:
                    m_pMover->setEqualRadiusAndAngles();
                    break;
                case SymmetricX:
                    m_pMover->setSymmetricX();
                    break;
                case SymmetricY:
                    m_pMover->setSymmetricY();
                    break;
                default:
                    break;
            }
        }
    }
#if ALLOW_INTERNAL_WRITE
    else if (comboBox == mRoutingModeCombo) {
        JUCE_COMPILER_WARNING("this will update the number of speakers in the processor, not sure this is the smartest place to do that")
		mFilter->setRoutingMode(comboBox->getSelectedId() - 1);
        updateRoutingModeComponents();
	}
#endif
    else if (comboBox == mProcessModeCombo) {
        int iSelectedMode = comboBox->getSelectedId() - 1;
        mFilter->setProcessMode(iSelectedMode);
        updateProcessModeComponents();
	}
	else if (comboBox == mOscLeapSourceCb)
	{
		mFilter->setOscLeapSource(comboBox->getSelectedId() - 1);
	}

    else if (comboBox == mSrcSelectCombo){
        updateSourceLocationTextEditor(true);
    }
    else if (comboBox == mSpSelectCombo){
        updateSpeakerLocationTextEditor();
    }
    else if (comboBox == mTrUnits)
    {
        mFilter->setTrUnits(mTrUnits->getSelectedId());
    }
    else if (comboBox == mTrTypeComboBox)
    {
        int type = mTrTypeComboBox->getSelectedId();
        mFilter->setTrType(type);
        updateTrajectoryTypeComponents();
    }
    else if (comboBox ==  mTrDirectionComboBox)
    {
        int direction = mTrDirectionComboBox->getSelectedId()-1;
        mFilter->setTrDirection(direction);
    }
    else if (comboBox == mTrReturnComboBox)
    {
        int iReturn = mTrReturnComboBox->getSelectedId()-1;
        mFilter->setTrReturn(iReturn);
    }
    else
    {
        printf("unknown combobox clicked...\n");
    }
}


void SpatGrisAudioProcessorEditor::updateSourceLocationTextEditor(bool p_bUpdateFilter){
    int iSelectedSrc = mSrcSelectCombo->getSelectedId();
    iSelectedSrc = (iSelectedSrc <= 0) ? 1: iSelectedSrc;
    if (p_bUpdateFilter){
        mFilter->setSelectedSrc(iSelectedSrc-1);
    }
    FPoint curPosition = mFilter->getSourceRT(iSelectedSrc-1);
    mSrcR->setText(String(curPosition.x));
    mSrcT->setText(String(curPosition.y * 180. / M_PI));
}

void SpatGrisAudioProcessorEditor::updateSpeakerLocationTextEditor(){
    FPoint curPosition = mFilter->getSpeakerRT(mSpSelectCombo->getSelectedId()-1);
    mSpR->setText(String(curPosition.x));
    mSpT->setText(String(curPosition.y * 180. / M_PI));
}

void SpatGrisAudioProcessorEditor::updateSingleTrajectoryStartComponent(Component* p_oComponent, bool p_bIsStarting){    
    if (p_bIsStarting){
        if (TextEditor* te = dynamic_cast<TextEditor*>(p_oComponent)) {
            te->setColour (TextEditor::textColourId, juce::Colour::greyLevel(.6));
            te->applyFontToAllText (mGrisFeel.getFont());
        }
        if(p_oComponent){ p_oComponent->setEnabled(false); }
    } else {
        if (TextEditor* te = dynamic_cast<TextEditor*>(p_oComponent)) {
            te->setColour (TextEditor::textColourId, juce::Colours::black);
            te->applyFontToAllText (mGrisFeel.getFont());
        }
        if(p_oComponent){ p_oComponent->setEnabled(true); }
    }
}

void SpatGrisAudioProcessorEditor::updateTrajectoryStartComponent(trajectoryStatus p_bIsStarting){
    
    bool isStarting = false;
    switch (p_bIsStarting){
        case kSetOn:
            mFilter->storeCurrentLocations();
            setTrStateEditor(kTrWriting);
            mTrProgressBar->setValue(0);
            mFilter->setIsRecordingAutomation(true);    //this starts the source update thread
            mTrWriteButton->setButtonText("Cancel");
            mTrProgressBar->setVisible(true);
            isStarting = true;
            break;
            
        case kSetOff:
            mFilter->setIsRecordingAutomation(false);
            mFilter->restoreCurrentLocations(-1);
            setTrStateEditor(kTrReady);
            mTrWriteButton->setButtonText("Ready");
            mTrProgressBar->setVisible(false);
            isStarting = false;
            break;
            
        case kRunning:
            setTrStateEditor(kTrWriting);
            mTrWriteButton->setButtonText("Cancel");
            mTrProgressBar->setVisible(true);
            isStarting = true;
            break;
    }
    
    updateSingleTrajectoryStartComponent(mTrDampeningTextEditor,    isStarting);
    updateSingleTrajectoryStartComponent(mTrDeviationTextEditor,    isStarting);
    updateSingleTrajectoryStartComponent(mTrTurnsTextEditor,        isStarting);
    updateSingleTrajectoryStartComponent(mTrEllipseWidthTextEditor, isStarting);
    updateSingleTrajectoryStartComponent(mTrEndPointButton,         isStarting);
    updateSingleTrajectoryStartComponent(m_pTrResetEndButton,       isStarting);
    updateSingleTrajectoryStartComponent(mTrSeparateAutomationModeButton, isStarting);
    updateSingleTrajectoryStartComponent(mTrDirectionComboBox,      isStarting);
    updateSingleTrajectoryStartComponent(mTrReturnComboBox,         isStarting);
    updateSingleTrajectoryStartComponent(mTrTypeComboBox,           isStarting);
    updateSingleTrajectoryStartComponent(mTrDirectionComboBox,      isStarting);
    updateSingleTrajectoryStartComponent(mTrDuration,               isStarting);
    updateSingleTrajectoryStartComponent(mTrUnits,                  isStarting);
    updateSingleTrajectoryStartComponent(mTrRepeats,                isStarting);
    updateSingleTrajectoryStartComponent(m_pTrEndRayTextEditor,     isStarting);
    updateSingleTrajectoryStartComponent(m_pTrEndAngleTextEditor,   isStarting);
    updateSingleTrajectoryStartComponent(m_pTrEndRayTextEditor,     isStarting);
    updateSingleTrajectoryStartComponent(m_pTrEndAngleTextEditor,   isStarting);
}

//==============================================================================
void SpatGrisAudioProcessorEditor::timerCallback(){
//---------------------------------------- TRAJECTORIES -----------------------------------------
#if TIME_GUI
    std::ostringstream oss;
    Time init = Time::getCurrentTime();
#endif

    updateTrajectoryStuff();
    
#if TIME_GUI
    Time timeTraj = Time::getCurrentTime();
    oss << "traj\t" << (timeTraj - init).inMilliseconds() << "\t";
#endif

//---------------------------------------- DB METERS -----------------------------------------
#if USE_DB_METERS
    
    if (!mFilter->getIsRecordingAutomation()){
        const int nbrS = mFilter->getNumberOfSpeakers();
        for (int i = 0; i < nbrS; ++i){
            mLevelComponents[i]->repaint();
        }
    }
    
    /*
    if (!mFilter->getIsRecordingAutomation() && !mFilter->isLevelUilcok()){
        for (int i = 0; i < mFilter->getNumberOfSpeakers(); i++){
            if(!mFilter->isLevelUilcok() && mLevelComponents[i] != NULL){
                mLevelComponents[i]->repaint();
            }
        }
    }*/
#endif
    
#if TIME_GUI
    Time timeDbMeters = Time::getCurrentTime();
    oss << "db meters\t" << (timeDbMeters - timeTraj).inMilliseconds() << "\t";
#endif

//---------------------------------------- PROPERTIES -----------------------------------------
    if (mField->justSelectedEndPoint()){
        updateEndLocationTextEditors();
        mTrEndPointButton->setToggleState(false, dontSendNotification);
        mTrEndPointButton->setButtonText("Set end point");
        mTrEndPointLabel->setVisible(false);
        mField->setJustSelectedEndPoint(false);
    }
    
    //properties are only updated when loading a preset, vs parameters are updated all the time
    //parameters are anything that can be automated or changed randomly by the processor
    //properties are things that the user can only change with the gui, and save as preset
	uint64_t hcpProcessor = mFilter->getHostChangedProperty();
	if (hcpProcessor != mHostChangedPropertyEditor) {
        mHostChangedPropertyEditor = hcpProcessor;
        propertyChanged();
    }
    
#if TIME_GUI
    Time timeProperties = Time::getCurrentTime();
    oss << "properties\t" << (timeProperties - timeDbMeters).inMilliseconds() << "\t";
#endif
    
//---------------------------------------- FIELD -----------------------------------------
    
    hcpProcessor = mFilter->getHostChangedParameter();
    if (hcpProcessor != mHostChangedParameterEditor) {
        mHostChangedParameterEditor = hcpProcessor;
        mNeedRepaint        = true;
        //repainting the field is required for reading movement automations (and potentially other things)
        mFieldNeedRepaint   = true;
    }
    
    if (mFieldNeedRepaint){
        mField->repaint();
    }

#if TIME_GUI
    Time timeField = Time::getCurrentTime();
    oss << "properties\t" << (timeField - timeProperties).inMilliseconds() << "\t";
#endif
    
//---------------------------------------- REPAINT -----------------------------------------
    
    if (mNeedRepaint){
        repaintTheStuff();
    }

    if (mOsc) {
        mOsc->heartbeat();
    }
    
#if TIME_GUI
    Time repaint = Time::getCurrentTime();
    oss << "repaintTOTAL\t" << (repaint - timeField).inMilliseconds() << "\t";
    
    m_iGuiRefreshCounter++;
    if (m_iGuiRefreshCounter % (2 * hertzRefresh) == 0){
        cout << oss.str() << newLine;
        m_iGuiRefreshCounter = 0;
    }
#endif
    
    mNeedRepaint        = false;
    mFieldNeedRepaint   = false;
}

void SpatGrisAudioProcessorEditor::updateTrajectoryStuff(){
    if (mTrStateEditor == kTrWriting){
        Trajectory::Ptr t = mFilter->getTrajectory();
        if (t) {
            //if we're writing a trajectory, update the progressbar and trajectory path
            mTrProgressBar->setValue(t->progress());
            int iCurCycle = t->progressCycle();
            if (mTrCycleCount != iCurCycle){
                mField->clearTrajectoryPath();
                mTrCycleCount = iCurCycle;
            }
        } else {
            updateTrajectoryStartComponent(kSetOff);                          //re-activate trajectory components
            mTrWriteButton->setToggleState(false, dontSendNotification);    //untoggle button
        }
        mFieldNeedRepaint = true;
    }
}

//this is essentially called when loading a preset
void SpatGrisAudioProcessorEditor::propertyChanged(){
    //update input/output mode, making sure to store and restore current locations
    if (mFilter->getIsAllowInputOutputModeSelection()){
        int iNewMode = mFilter->getInputOutputMode();
        if (iNewMode != mInputOutputModeCombo->getSelectedId()){
            
            mFilter->storeCurrentLocations();
            m_bLoadingPreset = true;
            mFilter->setInputOutputMode(iNewMode);
            updateInputOutputCombo();
        }
    }
    
    updateMovementModeComboPosition();
    
    mProcessModeCombo-> setSelectedId(mFilter->getProcessMode() + 1);
    mOscLeapSourceCb->  setSelectedId(mFilter->getOscLeapSource() + 1);
    mSrcSelectCombo->   setSelectedId(m_iSelectedSrcEditor+1);
    mSpSelectCombo->    setSelectedId(mFilter->getSpSelected());
    mSrcPlacementCombo->setSelectedId(mFilter->getSrcPlacementMode(), dontSendNotification);
    mSpPlacementCombo-> setSelectedId(mFilter->getSpPlacementMode(), dontSendNotification);
    
    
    mTrTypeComboBox->           setSelectedId(mFilter->getTrType());
    mTrDuration->               setText(String(mFilter->getTrDuration()));
    mTrUnits->                  setSelectedId(mFilter->getTrUnits());
    mTrRepeats->                setText(String(mFilter->getTrRepeats()));
    mTrDampeningTextEditor->    setText(String(mFilter->getTrDampening()));
    mTrDeviationTextEditor->    setText(String(mFilter->getTrDeviation()*360));
    mTrEllipseWidthTextEditor-> setText(String(mFilter->getTrEllipseWidth()*2));
    mTrTurnsTextEditor->        setText(String(mFilter->getTrTurns()));
    mOscSpat1stSrcIdTextEditor->setText(String(mFilter->getOscSpat1stSrcId()));
    mOscSpatPortTextEditor->    setText(String(mFilter->getOscSpatPort()));
    
#if USE_TOUCH_OSC
    updateOscComponent(mOsc);
#endif
    mShowGridLines->setToggleState(mFilter->getShowGridLines(),             dontSendNotification);
    mOscActiveButton->setToggleState(mFilter->getOscActive(),               dontSendNotification);
    mTrSeparateAutomationModeButton->setToggleState(mFilter->getIndependentMode(),dontSendNotification);
    mSurfaceOrPanLinkButton->setToggleState(mFilter->getLinkSurfaceOrPan(),     dontSendNotification);
    mAzimSpanLinkButton->setToggleState(mFilter->getLinkAzimSpan(),         dontSendNotification);
    mElevSpanLinkButton->setToggleState(mFilter->getLinkElevSpan(),         dontSendNotification);
    mApplyFilterButton->setToggleState(mFilter->getApplyFilter(),           dontSendNotification);
    mApplyOutputRamping->setToggleState(mFilter->getApplyOutRamp(),           dontSendNotification);
}

void SpatGrisAudioProcessorEditor::repaintTheStuff(){
#if TIME_GUI
    ostringstream oss;
    Time repaintInit = Time::getCurrentTime();
#endif
    
    mMovementModeCombo->setSelectedId(mFilter->getMovementMode() + 1);
    
    //apparently all these sliders are automatable
    mSmoothingSlider->      setValue(mFilter->getParameter(kSmooth));
    mVolumeFar->            setValue(mFilter->getParameter(kVolumeFar));
    mVolumeMid->            setValue(mFilter->getParameter(kVolumeMid));
    mVolumeNear->           setValue(mFilter->getParameter(kVolumeNear));
    mMaxSpanVolumeSlider->  setValue(mFilter->getParameter(kMaxSpanVolume));
    mFilterNear->           setValue(mFilter->getParameter(kFilterNear));
    mFilterMid->            setValue(mFilter->getParameter(kFilterMid));
    mFilterFar->            setValue(mFilter->getParameter(kFilterFar));
    mSpeedTrajectory->      setValue(mFilter->getSpeedTraject());
#if ALLOW_INTERNAL_WRITE
    mRoutingVolumeSlider->  setValue(mFilter->getParameter(kRoutingVolume));
#endif
    
#if TIME_GUI
    Time timeSliders = Time::getCurrentTime();
    oss << "repaint sliders\t" << (timeSliders - repaintInit).inMilliseconds() << "\t";
#endif
    
    //so these text editors will update only when we're not playing and moving stuff around
    if (!mFilter->isPlaying()){
        updateSourceLocationTextEditor(false);
        updateSpeakerLocationTextEditor();
    }
    
#if TIME_GUI
    Time timeTextEd = Time::getCurrentTime();
    oss << "repaint TextEd\t" << (timeTextEd - timeSliders).inMilliseconds() << "\t";
#endif
    
    //update sliders and mute, these could be automated
    int iSelSrc = mFilter->getSelectedSrc();
    if (iSelSrc != m_iSelectedSrcEditor){
        m_iSelectedSrcEditor = iSelSrc;
        mSurfaceOrPanSlider->setParamIndex(mFilter->getParamForSourceD(m_iSelectedSrcEditor));
        mAzimSpanSlider->    setParamIndex(mFilter->getParamForSourceAzimSpan(m_iSelectedSrcEditor));
        mElevSpanSlider->    setParamIndex(mFilter->getParamForSourceElevSpan(m_iSelectedSrcEditor));
    }
    if (mFilter->getProcessMode() == kFreeVolumeMode){
        mSurfaceOrPanSlider->setValue(1.f - mFilter->getSourceD(iSelSrc), dontSendNotification);
    } else {
        mSurfaceOrPanSlider->setValue(mFilter->getSourceD(iSelSrc), dontSendNotification);
    }
    mAzimSpanSlider->    setValue(mFilter->getSourceAzimSpan01(iSelSrc), dontSendNotification);
    mElevSpanSlider->    setValue(mFilter->getSourceElevSpan01(iSelSrc), dontSendNotification);
    for (int i = 0; i < mFilter->getNumberOfSpeakers(); i++){
        mMuteButtons[i]->setToggleState((mFilter->getSpeakerM(i) > .5), dontSendNotification);
        mLevelComponents[i]->setMute(mMuteButtons[i]->getToggleState());
    }
    
#if TIME_GUI
    Time timeSelectedSrc = Time::getCurrentTime();
    oss << "repaint sel src\t" << (timeSelectedSrc - timeTextEd).inMilliseconds() << "\t";
#endif
}

void SpatGrisAudioProcessorEditor::audioProcessorChanged (AudioProcessor* processor){
    mNeedRepaint = true;
}

//void SpatGrisAudioProcessorEditor::readAndUseJoystickValues(){
//    mJoystick->readAndUseJoystickValues();
//}

void SpatGrisAudioProcessorEditor::audioProcessorParameterChanged(AudioProcessor* processor, int parameterIndex, float newValue){
//    mNeedRepaint = true;
//    mFieldNeedRepaint   = true;
}

//==============================================================================
void SpatGrisAudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll (mGrisFeel.getWinBackgroundColour());
}
#if USE_JOYSTICK
void SpatGrisAudioProcessorEditor::uncheckJoystickButton()
{
    mEnableJoystick->setToggleState(false, dontSendNotification);
    buttonClicked(mEnableJoystick);
}
#endif
int SpatGrisAudioProcessorEditor::getNbSources()
{
    return mFilter->getNumberOfSources();
}


