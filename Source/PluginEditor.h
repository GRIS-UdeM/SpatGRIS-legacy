/*
 ==============================================================================
 SpatGRIS: multichannel sound spatialization plug-in.
 
 Copyright (C) 2015  GRIS-UdeM
 
 PluginEditor.h
 
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

#ifndef PLUGINEDITOR_H_INCLUDED
#define PLUGINEDITOR_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"
#include "../../GrisCommonFiles/GrisLookAndFeel.h"
#include "PluginProcessor.h"
#include "LevelComponent.h"
#include "SourceMover.h"

#if USE_LEAP
#include "Leap.h"
#endif

class FieldComponent;
class SourceUpdateThread;
//class JoystickUpdateThread;
class Box;

enum paramTypes {
	kParamSource,
	kParamUnused,
	kParamSmooth,
	kParamVolumeFar,
	kParamVolumeMid,
	kParamVolumeNear,
	kParamFilterFar,
	kParamFilterMid,
	kParamFilterNear,
	kParamMaxSpanVolume,
	kParamRoutingVolume,
    kParamAzimSpan,
    kParamElevSpan,
    kParamTrajSpeed,
    kParamDirRandom,
};

enum placement{
    kLeftAlternate = 1,
    kLeftClockwise,
    kLeftCounterClockWise,
    kTopClockwise,
    kTopCounterClockwise,

};

enum trajectoryStatus{
    kSetOn = 0,
    kSetOff,
    kRunning,
};

class MiniProgressBar;
class ParamSliderGRIS;
class OctTabbedComponent;
class HIDDelegate;
class OctoLeap;

class HeartbeatComponent : public Component
{
public:
	virtual void heartbeat() {}
};

//==============================================================================
class SpatGrisAudioProcessorEditor  : public AudioProcessorEditor,
									  public Button::Listener,
									  public ComboBox::Listener,
                                      public TextEditor::Listener,
									  private AudioProcessorListener,
									  private Timer
{
public:
    SpatGrisAudioProcessorEditor (SpatGrisAudioProcessor* ownerFilter, SourceMover *mover);
    ~SpatGrisAudioProcessorEditor();

    //==============================================================================
    void paint(Graphics& g);
	void resized();
	
    //! Method called by button listener when a button is clicked
	void buttonClicked (Button *button);
    //! Method called by button listener when a combobox is changed
	void comboBoxChanged (ComboBox* comboBox);
    void textEditorFocusLost (TextEditor &textEditor);
    void textEditorReturnKeyPressed(TextEditor &textEditor);
	
    //! 30Hz refresh (see hertzRefresh)
	void timerCallback();
    void propertyChanged();
    void updateTrajectoryStuff();
    void repaintTheStuff();
	void audioProcessorChanged (AudioProcessor* processor);
	void audioProcessorParameterChanged (AudioProcessor* processor, int parameterIndex, float newValue);
				
	//void refreshSize();
	void fieldChanged() { mFieldNeedRepaint = true; }
	
    //! Return the number of the source selected for the Leap Motion
	int getOscLeapSource() { return mFilter->getOscLeapSource(); }
    //! Set the number of the source selected for the Leap Motion
	void setOscLeapSource(int s);
	SourceMover * getMover() { return m_pMover; }
    Label * getmStateLeap() {return mStateLeap;}
    
#if USE_JOYSTICK
    HIDDelegate * getHIDDel() {return mJoystick;};
#endif    
    //! Method unchecking the joystick check box
    void uncheckJoystickButton();
    //! Return the number of sources form the processor
    int getNbSources();
    
    void setDefaultPendulumEndpoint();
    
private:
	SpatGrisAudioProcessor *mFilter;

    GrisLookAndFeel mGrisFeel;
    
	SourceMover *m_pMover;
	
	// for memory management:
	OwnedArray<Component> mComponents;
	
	// for interactions:
	bool mNeedRepaint;
	bool mFieldNeedRepaint;
    bool m_bIsReturnKeyPressedCalledFromFocusLost;
    bool m_bLoadingPreset;
    uint64_t mHostChangedParameterEditor;
	uint64_t mHostChangedPropertyEditor;
    
    ParamSliderGRIS          *mSurfaceOrPanSlider;
    ToggleButton    *mSurfaceOrPanLinkButton;
    Component       *mSurfaceOrPanLabel;
   
    ParamSliderGRIS          *mAzimSpanSlider;
    ToggleButton    *mAzimSpanLinkButton;
    Component       *mAzimSpanLabel;
    
    ParamSliderGRIS          *mElevSpanSlider;
    ToggleButton    *mElevSpanLinkButton;
    Component       *mElevSpanLabel;

    LevelComponent* mLevelComponents[kMaxChannels];
    ToggleButton*   mMuteButtons[kMaxChannels];
    Component*          mLabelSourceId[kMaxChannels];

    ToggleButton *mEnableJoystick;
    ToggleButton *mEnableLeap;
	ToggleButton *mShowGridLines;
    ToggleButton *mOscActiveButton;
    ToggleButton *mTrSeparateAutomationModeButton;
    ToggleButton *mApplyFilterButton;
    //ToggleButton *mApplyOutputRamping;
	ComboBox *mMovementModeCombo;
    ComboBox *mInputOutputModeCombo;
    TextButton *mApplyInputOutputModeButton;
	ComboBox *mProcessModeCombo;
	OctTabbedComponent *mTabs;
	Slider *mSmoothingSlider;
    Component *mSmoothingLabel;
	Slider *mVolumeFar;
	Slider *mVolumeMid;
	Slider *mVolumeNear;
	Slider *mFilterFar;
	Slider *mFilterMid;
	Slider *mFilterNear;
	Slider *mMaxSpanVolumeSlider;
    Slider *mSpeedTrajectory;
    Slider *mDirRandTrajectory;
    Component *mMaxSpanVolumeLabel;
    //Label *mShowChange;
    Label *mStateLeap;
    Label *mStateJoystick;
    Label *m_VersionLabel;
    
    ImageComponent m_logoImage;

#if USE_LEAP
    ScopedPointer<Leap::Controller> mController;
    Leap::Listener leapList;
#endif
    // sources
    TextButton *mApplySrcPlacementButton;
    TextEditor *mSrcR, *mSrcT;
    ComboBox *mSrcSelectCombo, *mSrcPlacementCombo;
    
    // speakers
    TextButton *mApplySpPlacementButton;
	TextEditor *mSpR, *mSpT;
	ComboBox *mSpSelectCombo, *mSpPlacementCombo;

	// trajectories
	ComboBox *mTrTypeComboBox;
    ComboBox* mTrDirectionComboBox;
    ComboBox* mTrReturnComboBox;
    
	TextEditor *mTrDuration;
	ComboBox   *mTrUnits;
	TextEditor *mTrRepeats;
    TextEditor *mTrDampeningTextEditor;
    Component  *mTrDampeningLabel;
    
    TextEditor *mTrDeviationTextEditor;
    Component  *mTrDeviationLabel;
    
    TextEditor *mTrEllipseWidthTextEditor;
    Component  *mTrEllipseWidthLabel;
    
    TextEditor *mTrTurnsTextEditor;
    Component  *mTrTurnsLabel;
    
	TextButton *mTrWriteButton;
	MiniProgressBar *mTrProgressBar;
    
    TextButton *mTrEndPointButton;
    TextEditor* m_pTrEndRayTextEditor;
    TextEditor* m_pTrEndAngleTextEditor;
    TextButton* m_pTrResetEndButton;
    Component*  mTrEndPointLabel;

    int mTrStateEditor;
    void setTrStateEditor(int p_iState){
        mTrStateEditor = p_iState;
        mFilter->setTrState(mTrStateEditor);
    }
    
    int mTrCycleCount;
	
	// osc, leap
	ComboBox *mOscLeapSourceCb;
#if USE_LEAP    
	//leap
    ReferenceCountedObjectPtr<OctoLeap> mleap;
#endif
#if USE_JOYSTICK
	ReferenceCountedObjectPtr<HIDDelegate>  mJoystick;
#endif
	HeartbeatComponent *mOsc;

	// for resizing/repaint:
    FieldComponent *mField;
    Component *mSourcesBoxLabel;
	Box *mSourcesBox;
	Component *mSpeakersBoxLabel;
	Box *mSpeakersBox;
    Box *mTrajectoryBox;
    Component *mTrajectoryBoxLabel;
    void updateEditorSources(bool p_bCalledFromConstructor);
    void updateEditorSpeakers(bool p_bCalledFromConstructor);
    void updateSourceLocationTextEditor(bool p_bUpdateFilter);
    void updateSpeakerLocationTextEditor();
    void updateMovementModeCombo();
    void updateTrajectoryTypeComponents();
    void updateTrajectoryStartComponent(trajectoryStatus p_bIsStarting);
    void updateSingleTrajectoryStartComponent(Component* p_oComponent, bool p_bIsStarting);
    void updateEndLocationTextEditors();
    void updateInputOutputCombo();
    void updateMovementModeComboPosition();
    void updateProcessModeComponents();
    void updateRoutingModeComponents();
	
	Component* addLabel(const String &s, int x, int y, int w, int h, Component *into);
	ToggleButton* addCheckbox(const String &s, bool v, int x, int y, int w, int h, Component *into);
	TextButton* addButton(const String &s, int x, int y, int w, int h, Component *into);
    TextEditor* addTextEditor(const String &s, int x, int y, int w, int h, Component *into);
	ParamSliderGRIS* addParamSliderGRIS(paramTypes paramType, int si, float v, int x, int y, int w, int h, Component *into);
    
//    JoystickUpdateThread*   m_pJoystickUpdateThread;
    
    //! Bounds of the resizable window
    ComponentBoundsConstrainer m_oResizeLimits;
    ScopedPointer<ResizableCornerComponent> m_pResizer;
    
    Component  *mOscSpat1stSrcIdLabel;
    TextEditor *mOscSpat1stSrcIdTextEditor;
    
    Component  *mOscSpatPortLabel;
    TextEditor *mOscSpatPortTextEditor;
    
    void applyCurrentSrcPlacement();
    void applyCurrentSpkPlacement();
    
    Component * mrayLab;
    Component * mrayvalLab;
    
    
    //TextEditor *mSpeedStartAccel;
    //TextEditor *mSpeedEndtAccel;
    //TextEditor *mTimeStartAccel;
    //ComboBox *mTypeAccel;
    
#if TIME_GUI
    int m_iGuiRefreshCounter = 0;
#endif
    
    int m_iSelectedSrcEditor;
};

#endif  // PLUGINEDITOR_H_INCLUDED
