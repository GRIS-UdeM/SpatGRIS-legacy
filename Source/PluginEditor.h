/*
  ==============================================================================

    This file was auto-generated by the Introjucer!

    It contains the basic startup code for a Juce application.

  ==============================================================================
*/

#ifndef PLUGINEDITOR_H_INCLUDED
#define PLUGINEDITOR_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"
#include "LevelComponent.h"
#include "SourceMover.h"

class FieldComponent;

class Box;
enum
{
	kParamSource,
	kParamSpeaker,
	kParamSmooth,
	kParamVolumeFar,
	kParamVolumeMid,
	kParamVolumeNear,
	kParamFilterFar,
	kParamFilterMid,
	kParamFilterNear
};

enum placement{
    kTopClockwise = 1,
    kTopCounterClockwise,
    kLeftAlternate,
    kLeftClockwise,
    kLeftCounterClockWise
};

class MiniProgressBar;
class ParamSlider;
class OctTabbedComponent;

class HeartbeatComponent : public Component
{
public:
	virtual void heartbeat() {}
};

//==============================================================================
class OctogrisAudioProcessorEditor  : public AudioProcessorEditor,
									  public Button::Listener,
									  public ComboBox::Listener,
                                      public TextEditor::Listener,
									  private AudioProcessorListener,
									  private Timer
{
public:
    OctogrisAudioProcessorEditor (OctogrisAudioProcessor* ownerFilter);
    ~OctogrisAudioProcessorEditor();

    //==============================================================================
    void paint(Graphics& g);
	void resized();
	
	void buttonClicked (Button *button);
	void comboBoxChanged (ComboBox* comboBox);
    void textEditorFocusLost (TextEditor &textEditor);
    void textEditorReturnKeyPressed(TextEditor &textEditor);
	
	void timerCallback();
	void audioProcessorChanged (AudioProcessor* processor);
	void audioProcessorParameterChanged (AudioProcessor* processor,
                                                 int parameterIndex,
                                                 float newValue);
				
	void refreshSize();
	void fieldChanged() { mFieldNeedRepaint = true; }
	
	int getOscLeapSource() { return mFilter->getOscLeapSource(); }
	void setOscLeapSource(int s);
	SourceMover * getMover() { return &mMover; }
	
private:
    PluginHostType mHost;
	OctogrisAudioProcessor *mFilter;
	SourceMover mMover;
	
	// for memory management:
	OwnedArray<Component> mComponents;
	
	// for interactions:
	bool mNeedRepaint;
	bool mFieldNeedRepaint;
    bool m_bIsReturnKeyPressedCalledFromFocusLost;
    uint64_t mHostChangedParameter;
	uint64_t mHostChangedProperty;
	Array<Slider*> mDistances;
	Array<Component*> mLabels;
	Array<Slider*> mAttenuations;
	Array<ToggleButton*> mMutes;
	Array<LevelComponent*> mLevels;
	ToggleButton *mShowGridLines;
	ToggleButton *mLinkDistances;
	ToggleButton *mLinkMovement;
	ToggleButton *mApplyFilter;
	ComboBox *mMovementMode;
	ComboBox *mGuiSize;
    ComboBox *mInputOutputModeCombo;
	ComboBox *mProcessModeCombo;
	OctTabbedComponent *mTabs;
	Slider *mSmoothing;
	Slider *mVolumeFar;
	Slider *mVolumeMid;
	Slider *mVolumeNear;
	Slider *mFilterFar;
	Slider *mFilterMid;
	Slider *mFilterNear;

	
    // sources
    //ToggleButton *mSrcAlternate;
    //ToggleButton *mSrcStartAtTop;
    //ToggleButton *mSrcClockwise;
    //TextButton *mSrcApply;
    TextEditor *mSrcR, *mSrcT;
    //TextButton *mSrcSetRT;
    ComboBox *mSrcSelect, *mSrcPlacement;
    
    // speakers
	//ToggleButton *mSpAlternate;
	//ToggleButton *mSpStartAtTop;
	//ToggleButton *mSpClockwise;
	//TextButton *mSpApply;
	TextEditor *mSpR, *mSpT;
	//TextButton *mSpSetRT;
	ComboBox *mSpSelect, *mSpPlacement;

	// trajectories
	ComboBox *mTrType;
	TextEditor *mTrDuration;
	ComboBox *mTrUnits;
	TextEditor *mTrRepeats;
	TextButton *mTrWrite;
	MiniProgressBar *mTrProgressBar;
	ComboBox *mTrSrcSelect;
	enum
	{
		kTrReady,
		kTrWriting
	};
	int mTrState;
	
	// osc, leap
	ComboBox *mOscLeapSourceCb;
	HeartbeatComponent *mOsc;
	
	// for resizing/repaint:
	Component *mField;
    Component *mSourcesBoxLabel;
	Box *mSourcesBox;
	Component *mSpeakersBoxLabel;
	Box *mSpeakersBox;
    void updateSources();
    void updateSpeakers();
    void updateSourceLocationTextEditor();
    void updateSpeakerLocationTextEditor();
	
	Component* addLabel(const String &s, int x, int y, int w, int h, Component *into);
	ToggleButton* addCheckbox(const String &s, bool v, int x, int y, int w, int h, Component *into);
	TextButton* addButton(const String &s, int x, int y, int w, int h, Component *into);
	TextEditor* addTextEditor(const String &s, int x, int y, int w, int h, Component *into);
	Slider* addParamSlider(int paramType, int si, float v, int x, int y, int w, int h, Component *into);
};

#endif  // PLUGINEDITOR_H_INCLUDED
