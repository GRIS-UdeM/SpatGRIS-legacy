/*
 ==============================================================================
<<<<<<< HEAD
 SpatGRIS: multichannel sound spatialization plug-in.
=======
 Octogris2: multichannel sound spatialization plug-in.
>>>>>>> 2588dc2f3221b0a2cc68818c05101612d949a534
 
 Copyright (C) 2015  GRIS-UdeM
 
 PluginEditor.h
 
<<<<<<< HEAD
 Developers: Antoine Missout, Vincent Berthiaume, Antoine Landrieu
=======
 Developers: Antoine Missout, Vincent Berthiaume
>>>>>>> 2588dc2f3221b0a2cc68818c05101612d949a534
 
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
#include "PluginProcessor.h"
#include "LevelComponent.h"
#include "SourceMover.h"
#include "../../GrisCommonFiles/GrisLookAndFeel.h"

#if USE_LEAP
#include "Leap.h"
#endif

class FieldComponent;
class SourceUpdateThread;
//class JoystickUpdateThread;
class Box;

<<<<<<< HEAD
enum paramTypes {
=======
enum
{
>>>>>>> 2588dc2f3221b0a2cc68818c05101612d949a534
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
<<<<<<< HEAD
	kParamRoutingVolume,
    kParamAzimSpan,
    kParamElevSpan,
=======
	kParamRoutingVolume
>>>>>>> 2588dc2f3221b0a2cc68818c05101612d949a534
};

enum placement{
    kLeftAlternate = 1,
    kLeftClockwise,
    kLeftCounterClockWise,
    kTopClockwise,
    kTopCounterClockwise,

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
<<<<<<< HEAD
    SpatGrisAudioProcessorEditor (SpatGrisAudioProcessor* ownerFilter, SourceMover *mover);
    ~SpatGrisAudioProcessorEditor();
=======
    OctogrisAudioProcessorEditor (OctogrisAudioProcessor* ownerFilter, SourceMover* p_pMover);
    ~OctogrisAudioProcessorEditor();
>>>>>>> 2588dc2f3221b0a2cc68818c05101612d949a534

    //==============================================================================
    void paint(Graphics& g);
	void resized();
	
    //! Method called by button listener when a button is clicked
	void buttonClicked (Button *button);
    //! Method called by button listener when a combobox is changed
	void comboBoxChanged (ComboBox* comboBox);
    void textEditorFocusLost (TextEditor &textEditor);
    void textEditorReturnKeyPressed(TextEditor &textEditor);
	
    //! Called every 50ms;
	void timerCallback();
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
<<<<<<< HEAD
	SpatGrisAudioProcessor *mFilter;

    LookAndFeel_V2 mV2Feel;
    GrisLookAndFeel mGrisFeel;
    
	SourceMover *m_pMover;
=======
	OctogrisAudioProcessor *mFilter;
	SourceMover* m_pMover;
>>>>>>> 2588dc2f3221b0a2cc68818c05101612d949a534
	
	// for memory management:
	OwnedArray<Component> mComponents;
//    OwnedArray<LookAndFeel> lookAndFeels;
	
	// for interactions:
	bool mNeedRepaint;
	bool mFieldNeedRepaint;
    bool m_bIsReturnKeyPressedCalledFromFocusLost;
    bool m_bLoadingPreset;
    uint64_t mHostChangedParameter;
	uint64_t mHostChangedProperty;
<<<<<<< HEAD
    
    Slider          *mSurfaceOrPanSlider;
    ToggleButton    *mSurfaceOrPanLinkButton;
    Component       *mSurfaceOrPanLabel;

//    Slider          *mAzimuthSlider;
//    ToggleButton    *mAzimuthLinkButton;
//    Component       *mAzimuthLabel;
//    Slider          *mElevationSlider;
//    ToggleButton    *mElevationLinkButton;
//    Component       *mElevationLabel;
    
    Slider          *mAzimSpanSlider;
    ToggleButton    *mAzimSpanLinkButton;
    Component       *mAzimSpanLabel;
    
    Slider          *mElevSpanSlider;
    ToggleButton    *mElevSpanLinkButton;
    Component       *mElevSpanLabel;
    
=======
	Array<Slider*> mDistances;
	Array<Component*> mLabels;
>>>>>>> 2588dc2f3221b0a2cc68818c05101612d949a534
	Array<ToggleButton*> mMutes;
    
#if USE_DB_METERS
	Array<LevelComponent*> mLevels;
<<<<<<< HEAD
=======
   	Array<Slider*> mAttenuations;
#endif
    
>>>>>>> 2588dc2f3221b0a2cc68818c05101612d949a534
    ToggleButton *mEnableJoystick;
    ToggleButton *mEnableLeap;
	ToggleButton *mShowGridLines;
    ToggleButton *mTrSeparateAutomationMode;
<<<<<<< HEAD

=======
	ToggleButton *mLinkDistances;
>>>>>>> 2588dc2f3221b0a2cc68818c05101612d949a534
    ToggleButton *mApplyFilter;
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
<<<<<<< HEAD
	Slider *mMaxSpanVolumeSlider;
    Component *mMaxSpanVolumeLabel;
=======
	Slider *mMaxSpanVolume;
>>>>>>> 2588dc2f3221b0a2cc68818c05101612d949a534
    //Label *mShowChange;
    Label *mStateLeap;
    Label *mStateJoystick;
    Label *m_VersionLabel;
<<<<<<< HEAD
	
	ComboBox *mRoutingModeCombo;
    Component *mRoutingModeLabel;
	Slider *mRoutingVolumeSlider;
    
    ImageComponent m_logoImage;
	
=======
	
	ComboBox *mRoutingMode;
	Slider *mRoutingVolume;
    
    ImageComponent m_logoImage;
	
>>>>>>> 2588dc2f3221b0a2cc68818c05101612d949a534
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
	ComboBox *mTrUnits;
	TextEditor *mTrRepeats;
    TextEditor *mTrDampeningTextEditor;
    Component  *mTrDampeningLabel;
    
    TextEditor *mTrDeviationTextEditor;
    Component  *mTrDeviationLabel;
    
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
    void updateSources(bool p_bCalledFromConstructor);
    void updateSpeakers(bool p_bCalledFromConstructor);
    void updateSourceLocationTextEditor(bool p_bUpdateFilter);
    void updateSpeakerLocationTextEditor();
    void updateMovementModeCombo();
    void updateTrajectoryComponents();
    void updateEndLocationTextEditors();
<<<<<<< HEAD
    void updateInputOutputCombo(bool p_bResetSrcAndSpkPositions = true);
    void updateProcessModeComponents();
=======
>>>>>>> 2588dc2f3221b0a2cc68818c05101612d949a534
	
	Component* addLabel(const String &s, int x, int y, int w, int h, Component *into);
	ToggleButton* addCheckbox(const String &s, bool v, int x, int y, int w, int h, Component *into);
	TextButton* addButton(const String &s, int x, int y, int w, int h, Component *into);
    TextEditor* addTextEditor(const String &s, int x, int y, int w, int h, Component *into);
<<<<<<< HEAD
	Slider* addParamSliderGRIS(int paramType, int si, float v, int x, int y, int w, int h, Component *into);
=======
	Slider* addParamSlider(int paramType, int si, float v, int x, int y, int w, int h, Component *into);
>>>>>>> 2588dc2f3221b0a2cc68818c05101612d949a534
    
//    JoystickUpdateThread*   m_pJoystickUpdateThread;
    
    //! Bounds of the resizable window
    ComponentBoundsConstrainer m_oResizeLimits;
    ScopedPointer<ResizableCornerComponent> m_pResizer;
    
<<<<<<< HEAD


    Component  *mOscSpat1stSrcIdLabel;
    TextEditor *mOscSpat1stSrcIdTextEditor;
    
    Component  *mOscSpatPortLabel;
    TextEditor *mOscSpatPortTextEditor;
    
    void applyCurrentSrcPlacement();
    void applyCurrentSpkPlacement();

=======
    LookAndFeel_V2 mV2Feel;
    GrisLookAndFeel mGrisFeel;
    
    std::vector<string> mTimingVector;
>>>>>>> 2588dc2f3221b0a2cc68818c05101612d949a534
};

#endif  // PLUGINEDITOR_H_INCLUDED
