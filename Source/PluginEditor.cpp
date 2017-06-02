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


#include "PluginEditor.h"
#include "PluginProcessor.h"

//==============================================================================
SpatGrisAudioProcessorEditor::SpatGrisAudioProcessorEditor(SpatGrisAudioProcessor * filter) :
    AudioProcessorEditor (filter)
{
    
    LookAndFeel::setDefaultLookAndFeel(&this->grisFeel);
    this->filter = filter;
    


    this->spatFieldComp = new SpatComponent(this, this->filter, &this->grisFeel);
    this->addAndMakeVisible(this->spatFieldComp);
    
    //BOX-----------------------------------------------------------------
    this->boxSourceParam = new Box(&this->grisFeel, "Source parameters");
    this->addAndMakeVisible(this->boxSourceParam);
    
    this->boxOutputParam = new Box(&this->grisFeel, "Output parameters");
    this->addAndMakeVisible(this->boxOutputParam);
    
    this->boxTrajectory = new Box(&this->grisFeel, "Trajectories");
    this->addAndMakeVisible(this->boxTrajectory);
    

    //OctTabbedComponent---------------------------------------------------
    this->octTab = new OctTabbedComponent(&this->grisFeel, TabbedButtonBar::TabsAtTop, this->filter);

    this->octTab->addTab("Settings",           this->grisFeel.getBackgroundColour(), new Component(), true);
    this->octTab->addTab("Volume & Filters",   this->grisFeel.getBackgroundColour(), new Component(), true);
    this->octTab->addTab("Sources",            this->grisFeel.getBackgroundColour(), new Component(), true);
   	this->octTab->addTab("Speakers",           this->grisFeel.getBackgroundColour(), new Component(), true);
    this->octTab->addTab("Interfaces",         this->grisFeel.getBackgroundColour(), new Component(), true);
    this->octTab->setTabBarDepth(28);
    this->addAndMakeVisible(this->octTab);
    

    //Add All Component-------------------------------------------------------------------------------------
    
    //Source param
    this->labSurfaceOrPan       = addLabel("Surface", "Surface Master Soutce", 0, 0, DefaultLabWidth, DefaultLabHeight, this->boxSourceParam->getContent());
    this->togLinkSurfaceOrPan   = addToggleButton("Link", "Link other sources", 90, 0, DefaultLabWidth, DefaultLabHeight,  this->boxSourceParam->getContent());
    this->sliSurfaceOrPan       = addSlider("", "", 4, 18, 180, DefaultLabHeight, this->boxSourceParam->getContent(), MinHeigSource, MaxHeigSource,DefHeigSource, SliderInter);
    
    this->labAzimSpan           = addLabel("Azimuth Span", "Azimuth Span Selected Source", 0, 50, DefaultLabWidth, DefaultLabHeight, this->boxSourceParam->getContent());
    this->togLinkAzimSpan       = addToggleButton("Link", "Link other sources", 90, 50, DefaultLabWidth, DefaultLabHeight,  this->boxSourceParam->getContent());
    this->sliAzimSpan           = addSlider("", "", 4, 68, 180, DefaultLabHeight, this->boxSourceParam->getContent(), MinAzimSource, MaxAzimSource,DefAzimSource, SliderInter);
    
    this->labElevSpan           = addLabel("Elevation Span", "Elevation Span Selected Source", 0, 100, DefaultLabWidth, DefaultLabHeight, this->boxSourceParam->getContent());
    this->togLinkElevSpan       = addToggleButton("Link", "Link other sources", 90, 100, DefaultLabWidth, DefaultLabHeight,  this->boxSourceParam->getContent());
    this->sliAElevSpann         = addSlider("", "", 4, 118, 180, DefaultLabHeight, this->boxSourceParam->getContent(), MinElevSource, MaxElevSource,DefElevSource, SliderInter);
    //-----------------------------
    
    
    //Outputs param
    this->vecLevelOut = vector<LevelComponent *>();
    int x = 2;
    for(int i = 0; i < MaxSpeakers; i++){
        juce::Rectangle<int> level(x, 2, SizeWidthLevelComp, 134);
        
        LevelComponent * lvC = new LevelComponent(this->filter, &this->grisFeel, i+1);
        lvC->setBounds(level);
        this->boxOutputParam->getContent()->addAndMakeVisible(lvC);
        this->vecLevelOut.push_back(lvC);
        
        x+=SizeWidthLevelComp;
    }
    //-----------------------------
    
    
    //Trajectories
    //-----------------------------
    this->labMouvement      = addLabel("Mouvements :", "Mouvement with other sources", 0, 4, DefaultLabWidth, DefaultLabHeight, this->boxTrajectory->getContent());
    this->comMouvement      = addComboBox("", "Mouvement with other sources", 90, 4, DefaultLabWidth, DefaultLabHeight, this->boxTrajectory->getContent());
    
    this->labTypeTrajectory = addLabel("Types :", "Types of trajectories", 0, 30, DefaultLabWidth, DefaultLabHeight, this->boxTrajectory->getContent());
    this->comTypeTrajectory = addComboBox("", "Types of trajectories", 60, 30, DefaultLabWidth+30, DefaultLabHeight, this->boxTrajectory->getContent());
    for(int i = 0; i  < TrajectoryType::SIZE_TT; i++){
        this->comTypeTrajectory->addItem(GetTrajectoryName((TrajectoryType)i), i+1);
    }
    this->comTypeTrajectory->setSelectedId(1);
    
    
    this->labTimeTrajectory = addLabel("Time :", "Time of trajectory", 0, 50, DefaultLabWidth, DefaultLabHeight, this->boxTrajectory->getContent());
    this->texTimeTrajectory = addTextEditor("", "Time of trajectory", 60, 50, DefaultTexWidth, DefaultLabHeight, this->boxTrajectory->getContent());
    this->comTimeTrajectory = addComboBox("", "Per cycle(s)", 124, 50, DefaultLabWidth-34, DefaultLabHeight, this->boxTrajectory->getContent());
    this->comTimeTrajectory->addItem("Second(s)", 1);
    this->comTimeTrajectory->addItem("Beat(s)", 2);
    this->comTimeTrajectory->setSelectedId(1);
    
    this->labCycleTrajectory = addLabel("Cycle(s) :", "Cycle of trajectory", 0, 70, DefaultLabWidth, DefaultLabHeight, this->boxTrajectory->getContent());
    this->texCycleTrajectory = addTextEditor("", "Numbers of cycle(s) 0=inf", 60, 70, DefaultTexWidth, DefaultLabHeight, this->boxTrajectory->getContent());
    
    this->butReadyTrajectory = addButton("Ready", "Valid trajectory param", 60, 100, DefaultLabWidth, DefaultLabHeight, this->boxTrajectory->getContent());
    
    this->progressBarTraject = new ProgressBarTraj();
    this->progressBarTraject->setBounds(60, 124, DefaultLabWidth+30, DefaultLabHeight);
    this->boxTrajectory->getContent()->addAndMakeVisible(this->progressBarTraject);
    this->progressBarTraject->setVisible(false);
    
    this->sliSpeedTrajectory = addSlider("Speed :", "Speed of trajectory", 14, 150, 204, DefaultLabHeight, this->boxTrajectory->getContent(), MinSpeedTrajectory, MaxSpeedTrajectory, DefSpeedTrajectory,  0.001f, juce::Slider::TextEntryBoxPosition::TextBoxLeft);
    addLabel("Speed", "", 10, 134, DefaultLabWidth, DefaultLabHeight, this->boxTrajectory->getContent());
    
    //Other param Trajectories hided---
    int rowX = 260;
    
    this->labCyclePercent = addLabel("% cycle", "", rowX, 30, DefaultLabWidth, DefaultLabHeight, this->boxTrajectory->getContent());
    this->sliCyclePercent = addSlider("% :", "", rowX+50, 30, 180, DefaultLabHeight, this->boxTrajectory->getContent(), MinCyclePercent, MaxCyclePercent, DefCyclePercent,  1.0f, juce::Slider::TextEntryBoxPosition::TextBoxLeft);

    this->labTrajEllipseWidth = addLabel("Width :", "Width of ellipse", rowX,       50, DefaultLabWidth, DefaultLabHeight, this->boxTrajectory->getContent());
    this->texTrajEllipseWidth = addTextEditor("", "Width of ellipse", rowX+50,  50, DefaultTexWidth, DefaultLabHeight, this->boxTrajectory->getContent());
    
    this->comTrajOneWayReturn = addComboBox("", "Trajectory cycle", rowX,           50, DefaultLabWidth-30, DefaultLabHeight, this->boxTrajectory->getContent());
    this->comTrajOneWayReturn->addItem("One Way", 1);
    this->comTrajOneWayReturn->addItem("Return", 2);
    this->comTrajOneWayReturn->setSelectedId(1);

    this->labTrajRadAngEnd = addLabel("Radius / Angle of end point :", "Radius and angle end point", rowX-5, 70, 200, DefaultLabHeight, this->boxTrajectory->getContent());
    this->texTrajRadiusEnd = addTextEditor("", "Radius (0-2)", rowX, 90, 40, DefaultLabHeight, this->boxTrajectory->getContent());
    this->texTrajAngleEnd = addTextEditor("", "Angle (0-360)", rowX+50, 90, 40, DefaultLabHeight, this->boxTrajectory->getContent());

    this->butTrajSetEnd = addButton("Set", "Set end point", rowX+100, 90, 70, DefaultLabHeight, this->boxTrajectory->getContent());
    this->butTrajResetEnd = addButton("Reset", "Reset end point", rowX+180, 90, 70, DefaultLabHeight, this->boxTrajectory->getContent());

    this->labTrajPendDampe = addLabel("Dampening :", "Dampening (0-1)", rowX-5, 114, 80, DefaultLabHeight, this->boxTrajectory->getContent());
    this->texTrajPendDampe = addTextEditor("", "Dampening (0-1)", rowX+80,114, 40, DefaultLabHeight, this->boxTrajectory->getContent());

    this->labTrajPendDevia = addLabel("Deviation :", "Deviation (0-360)", rowX-5, 134, 80, DefaultLabHeight, this->boxTrajectory->getContent());
    this->texTrajPendDevia = addTextEditor("", "Deviation (0-360)", rowX+80, 134, 40, DefaultLabHeight, this->boxTrajectory->getContent());
    
    this->labTrajRandSpeed = addLabel("Speed :", "Speed Random", rowX-5, 50, 80, DefaultLabHeight, this->boxTrajectory->getContent());
    this->sliTrajRandSpeed = addSlider("Speed :", "Speed Random", rowX+48, 50, 160, DefaultLabHeight, this->boxTrajectory->getContent(), MinTrajRandomSpeed, MaxTrajRandomSpeed,  0.01f, juce::Slider::TextEntryBoxPosition::TextBoxLeft);
    this->togTrajRandSepare = addToggleButton("Separate sources :", "Force separate automation sources", rowX, 30, DefaultLabWidth, DefaultLabHeight,  this->boxTrajectory->getContent());

    //Add in list for lock
    this->listLockCompTrajectory.push_back(this->sliCyclePercent);
    this->listLockCompTrajectory.push_back(this->comTypeTrajectory);
    this->listLockCompTrajectory.push_back(this->texTimeTrajectory);
    this->listLockCompTrajectory.push_back(this->comTimeTrajectory);
    this->listLockCompTrajectory.push_back(this->texCycleTrajectory);
    
    this->listLockCompTrajectory.push_back(this->texTrajEllipseWidth);
    this->listLockCompTrajectory.push_back(this->comTrajOneWayReturn);
    
    this->listLockCompTrajectory.push_back(this->texTrajRadiusEnd);
    this->listLockCompTrajectory.push_back(this->texTrajAngleEnd);
    
    this->listLockCompTrajectory.push_back(this->butTrajSetEnd);
    this->listLockCompTrajectory.push_back(this->butTrajResetEnd);
    
    this->listLockCompTrajectory.push_back(this->texTrajPendDampe);
    this->listLockCompTrajectory.push_back(this->texTrajPendDevia);
    
    this->listLockCompTrajectory.push_back(this->sliTrajRandSpeed);
    this->listLockCompTrajectory.push_back(this->togTrajRandSepare);
    
    //OctTabbedComponent-----------------------
    
    //Settings
    //-----------------------------
    Component * settingsBox = this->octTab->getTabContentComponent(0);
    this->labTypeProcess    = addLabel("Mode :", "Process mode", 0, 4, DefaultLabWidth, DefaultLabHeight, settingsBox);
    this->comTypeProcess    = addComboBox("", "Process mode", 60, 4, DefaultLabWidth+30, DefaultLabHeight, settingsBox);
    for(int i = 0; i  < ProcessType::SIZE_PT; i++){
        this->comTypeProcess->addItem(GetProcessTypeName((ProcessType)i), i+1);
    }
    this->comTypeProcess->setSelectedId(1);
    
    this->labInOutMode      = addLabel("In / Out :", "Input/Output mode", 0, 30, DefaultLabWidth, DefaultLabHeight, settingsBox);
    this->comInOutMode      = addComboBox("", "Input/Output mode", 60, 30, DefaultLabWidth-40, DefaultLabHeight, settingsBox);
    this->butInOutMode      = addButton("Apply", "Apply Input/Output mode", 150, 30, 60, DefaultLabHeight, settingsBox);
    
    //OSC Param
    this->togOSCActive      = addToggleButton("OSC On/Off", "OSC Active (On/Off)", 240, 4, DefaultLabWidth, DefaultLabHeight, settingsBox);
    this->togOSCActive->setToggleState(this->filter->getOscOn(), dontSendNotification);
    
    this->labOSCSourceIDF   = addLabel("OSC 1er ID :", "OSC 1er Source ID", 240, 30, DefaultLabWidth, DefaultLabHeight, settingsBox);
    this->texOSCSourceIDF   = addTextEditor("", "OSC 1er Source ID", 320, 30, 60, DefaultLabHeight, settingsBox);
    this->texOSCSourceIDF->setText(String(this->filter->getOscFirstIdSource()));
    
    this->labOSCPort        = addLabel("OSC Port :", "OSC Port", 240, 50, DefaultLabWidth, DefaultLabHeight, settingsBox);
    this->texOSCPort        = addTextEditor("", "OSC Port", 320, 50, 60, DefaultLabHeight, settingsBox);
    this->texOSCPort->setText(String(this->filter->getOscPort()));
    
    //Volume and Filter
    //-----------------------------
    Component * volumeFBox = this->octTab->getTabContentComponent(1);
    this->togActiveFil  = addToggleButton("Active", "Active Filter", 4, 0, DefaultLabWidth, DefaultLabHeight, volumeFBox);
    
    this->labVolCenter  = addLabel("Volume center (dB)", "Volume center (dB)", 0, 18, DefaultLabWidth, DefaultLabHeight,  volumeFBox);
    this->sliVolCenter  = addSlider("", "Volume center (dB)", 4, 34, 172, DefaultLabHeight, volumeFBox, MinVolCenter, MaxVolCenter, DefVolCenter, SliderInterInt);
    
    this->labVolSpeaker = addLabel("Volume speakers (dB)", "Volume speakers (dB)", 180, 18, DefaultLabWidth+40, DefaultLabHeight,  volumeFBox);
    this->sliVolSpeaker = addSlider("", "Volume speakers (dB)", 184, 34, 172, DefaultLabHeight, volumeFBox, MinVolSpeaker, MaxVolSpeaker, DefVolSpeaker, SliderInterInt);
    
    this->labVolFar     = addLabel("Volume far (dB)", "Volume far (dB)", 360, 18, DefaultLabWidth, DefaultLabHeight,  volumeFBox);
    this->sliVolFar     = addSlider("", "Volume far (dB)", 364, 34, 172, DefaultLabHeight, volumeFBox, MinVolFar, MaxVolFar, DefVolFar, SliderInterInt);
    
    this->labFilCenter  = addLabel("Filter center", "Filter center", 0, 54, DefaultLabWidth, DefaultLabHeight,  volumeFBox);
    this->sliFilCenter  = addSlider("", "Filter center", 4, 70, 172, DefaultLabHeight, volumeFBox, MinFilter, MaxFilter, DefFilterCenter, SliderInterInt);
    
    this->labFilSpeaker = addLabel("Filter speakers", "Filter speakers", 180, 54, DefaultLabWidth+40, DefaultLabHeight,  volumeFBox);
    this->sliFilSpeaker = addSlider("", "Filter speakers", 184, 70, 172, DefaultLabHeight, volumeFBox, MinFilter, MaxFilter, DefFilterSpeaker, SliderInterInt);
    
    this->labFilFar     = addLabel("Filter far", "Filter far", 360, 54, DefaultLabWidth, DefaultLabHeight,  volumeFBox);
    this->sliFilFar     = addSlider("", "Filter far", 364, 70, 172, DefaultLabHeight, volumeFBox, MinFilter, MaxFilter, DefFilterFar, SliderInterInt);

    
    
    //Sources
    //-----------------------------
    Component * sourcesBox = this->octTab->getTabContentComponent(2);
    this->labSourcePos          = addLabel("Source position :", "Source position", 0, 4, DefaultLabWidth, DefaultLabHeight, sourcesBox);
    this->comSourcePos          = addComboBox("", "Source position", 110, 4, DefaultLabWidth+20, DefaultLabHeight, sourcesBox);
    for(int i = 0; i  < PositionSourceSpeaker::SIZE_PSS; i++){
        this->comSourcePos->addItem(GetPositionSourceSpeakerName((PositionSourceSpeaker)i), i+1);
    }
    this->comSourcePos->setSelectedId(1);
    
    this->butSourcePos          = addButton("Apply", "Apply Source position", 254, 4, 60, DefaultLabHeight, sourcesBox);
    
    this->labSourceSelectPos    = addLabel("Selected :", "Source Selected", 0, 30, DefaultLabWidth, DefaultLabHeight, sourcesBox);
    this->comSourceSelectPos    = addComboBox("", "Source Selected", 80, 30, 40, DefaultLabHeight, sourcesBox);
    
    this->labSourceSelectRay    = addLabel("Ray :", "Ray (0 - 2)", 0, 50, DefaultLabWidth, DefaultLabHeight, sourcesBox);
    this->comSourceSelectRay    = addTextEditor("", "Ray (0 - 2)", 50, 50, 70, DefaultLabHeight, sourcesBox);
    this->labSourceInfoRay      = addLabel("(0 - 2)", "", 120, 50, DefaultLabWidth, DefaultLabHeight, sourcesBox);
    
    this->labSourceSelectAngle  = addLabel("Angle :", "Angle (0 - 360)", 0, 70, DefaultLabWidth, DefaultLabHeight, sourcesBox);
    this->comSourceSelectAngle  = addTextEditor("", "Angle (0 - 360)", 50, 70, 70, DefaultLabHeight, sourcesBox);
    this->labSourceInfoAngle    = addLabel("(0 - 360)", "", 120, 70, DefaultLabWidth, DefaultLabHeight, sourcesBox);
    
    
    //Speakers
    //-----------------------------
    Component * speakersBox = this->octTab->getTabContentComponent(3);
    this->labSpeakerPos          = addLabel("Speaker position :", "Speaker position", 0, 4, DefaultLabWidth, DefaultLabHeight, speakersBox);
    this->comSpeakerPos          = addComboBox("", "Speaker position", 110, 4, DefaultLabWidth+20, DefaultLabHeight, speakersBox);
    for(int i = 0; i  < PositionSourceSpeaker::SIZE_PSS; i++){
        this->comSpeakerPos->addItem(GetPositionSourceSpeakerName((PositionSourceSpeaker)i), i+1);
    }
    this->comSpeakerPos->setSelectedId(1);
    
    this->butSpeakerPos          = addButton("Apply", "Apply Speaker position", 254, 4, 60, DefaultLabHeight, speakersBox);
    
    this->labSpeakerSelectPos    = addLabel("Selected :", "Speaker Selected", 0, 30, DefaultLabWidth, DefaultLabHeight, speakersBox);
    this->comSpeakerSelectPos    = addComboBox("", "Speaker Selected", 80, 30, 40, DefaultLabHeight, speakersBox);
    
    this->labSpeakerSelectRay    = addLabel("Ray :", "Ray (0 - 2)", 0, 50, DefaultLabWidth, DefaultLabHeight, speakersBox);
    this->comSpeakerSelectRay    = addTextEditor("", "Ray (0 - 2)", 50, 50, 70, DefaultLabHeight, speakersBox);
    this->labSpeakerInfoRay      = addLabel("(0 - 2)", "", 120, 50, DefaultLabWidth, DefaultLabHeight, speakersBox);
    
    this->labSpeakerSelectAngle  = addLabel("Angle :", "Angle (0 - 360)", 0, 70, DefaultLabWidth, DefaultLabHeight, speakersBox);
    this->comSpeakerSelectAngle  = addTextEditor("", "Angle (0 - 360)", 50, 70, 70, DefaultLabHeight, speakersBox);
    this->labSpeakerInfoAngle    = addLabel("(0 - 360)", "", 120, 70, DefaultLabWidth, DefaultLabHeight, speakersBox);
    
    
    //Interfaces
    //-----------------------------
    Component * interfaceBox = this->octTab->getTabContentComponent(4);
    addLabel("Comming soon...", "", 0, 4, DefaultLabWidth, DefaultLabHeight, interfaceBox);
    //------------------------------------------------------------------------------------------------------
    
    
    //Size window
    this->resizeWindow.setSizeLimits (MinFieldSize + (2*Margin), MinFieldSize + (2*Margin), 1920, 1080);
    this->addAndMakeVisible (this->resizer = new ResizableCornerComponent (this, &this->resizeWindow));
    this->setSize(DefaultUItWidth, DefaultUIHeight);
    
    this->updateComMouvement();
    this->updateSourceParam();
    this->updateTrajectoryParam();
    this->updateInputOutputMode();
    this->comInOutMode->setSelectedId(1);
    
    this->updateSelectSource();
    this->updateSelectSpeaker();
    

    
	this->startTimerHz(HertzRefresh);
}

SpatGrisAudioProcessorEditor::~SpatGrisAudioProcessorEditor()
{
    for (auto&& it : this->vecLevelOut)
    {
        delete (it);
    }
    
    //delete this->sourceMover;
    
    delete this->progressBarTraject;
    
    delete this->spatFieldComp;
    delete this->boxSourceParam;
    delete this->boxOutputParam;
    delete this->boxTrajectory;
    delete this->octTab;
}
//==============================================================================

Label* SpatGrisAudioProcessorEditor::addLabel(const String &s, const String &stooltip, int x, int y, int w, int h, Component *into)
{
    Label *lb = new Label();
    lb->setText(s, NotificationType::dontSendNotification);
    lb->setTooltip (stooltip);
    lb->setJustificationType(Justification::left);
    lb->setFont(this->grisFeel.getFont());
    lb->setLookAndFeel(&this->grisFeel);
    lb->setColour(Label::textColourId, this->grisFeel.getFontColour());
    lb->setBounds(x, y, w, h);
    into->addAndMakeVisible(lb);
    return lb;
}

TextButton* SpatGrisAudioProcessorEditor::addButton(const String &s, const String &stooltip, int x, int y, int w, int h, Component *into)
{
    TextButton *tb = new TextButton();
    tb->setTooltip (stooltip);
    tb->setButtonText(s);
    tb->setSize(w, h);
    tb->setTopLeftPosition(x, y);
    tb->addListener(this);
    tb->setColour(ToggleButton::textColourId, this->grisFeel.getFontColour());
    tb->setLookAndFeel(&this->grisFeel);
    into->addAndMakeVisible(tb);
    return tb;
}

ToggleButton* SpatGrisAudioProcessorEditor::addToggleButton(const String &s, const String &stooltip, int x, int y, int w, int h, Component *into, bool toggle)
{
    ToggleButton *tb = new ToggleButton();
    tb->setTooltip (stooltip);
    tb->setButtonText(s);
    tb->setToggleState(toggle, dontSendNotification);
    tb->setSize(w, h);
    tb->setTopLeftPosition(x, y);
    tb->addListener(this);
    tb->setColour(ToggleButton::textColourId, this->grisFeel.getFontColour());
    tb->setLookAndFeel(&this->grisFeel);
    into->addAndMakeVisible(tb);
    return tb;
}

TextEditor* SpatGrisAudioProcessorEditor::addTextEditor(const String &emptyS, const String &stooltip, int x, int y, int w, int h, Component *into, int wLab)
{
    TextEditor *te = new TextEditor();
    te->setTooltip (stooltip);
    te->setTextToShowWhenEmpty(emptyS, this->grisFeel.getOffColour());
    te->setColour(ToggleButton::textColourId, this->grisFeel.getFontColour());
    te->setLookAndFeel(&this->grisFeel);
    te->setBounds(x, y, w, h);
    te->addListener(this);
    into->addAndMakeVisible(te);
    return te;
}

Slider* SpatGrisAudioProcessorEditor::addSlider(const String &s, const String &stooltip, int x, int y, int w, int h, Component *into, float minF, float maxF, float defF, float incr, juce::Slider::TextEntryBoxPosition tebp)
{
    Slider *sd = new SliderGRIS(defF);
    sd->setTooltip (stooltip);
    //sd->setTextValueSuffix(s);
    sd->setSize(w, h);
    sd->setTopLeftPosition(x, y);
    sd->setRange(minF, maxF, incr);
    //sd->setSliderStyle(Slider::Rotary);
    //sd->setRotaryParameters(M_PI * 1.3f, M_PI * 2.7f, true);
    sd->setTextBoxStyle (tebp, false, 40, DefaultLabHeight);
    sd->setColour(ToggleButton::textColourId, this->grisFeel.getFontColour());
    sd->setLookAndFeel(&this->grisFeel);
    sd->addListener(this);
    into->addAndMakeVisible(sd);
    return sd;
}
ComboBox* SpatGrisAudioProcessorEditor::addComboBox(const String &s, const String &stooltip, int x, int y, int w, int h, Component *into)
{
    ComboBox *cb = new ComboBox();
    cb->setTooltip(stooltip);
    cb->setSize(w, h);
    cb->setTopLeftPosition(x, y);
    cb->setLookAndFeel(&this->grisFeel);
    cb->addListener(this);
    into->addAndMakeVisible(cb);
    return cb;
}
//==============================================================================
void SpatGrisAudioProcessorEditor::updateSourceParam()
{
    this->togLinkSurfaceOrPan->setToggleState(this->filter->getLinkHeight(),   dontSendNotification);
    this->togLinkAzimSpan->setToggleState(this->filter->getLinkAzimuth(),       dontSendNotification);
    this->togLinkElevSpan->setToggleState(this->filter->getLinkElevation(),     dontSendNotification);
    

    const int idS = this->filter->getSelectItem()->selectIdSource;
    this->sliSurfaceOrPan->setValue(*(this->filter->getListSource()[idS]->getHeigt()),dontSendNotification);
    this->sliSurfaceOrPan->setTooltip("S:"+String(this->sliSurfaceOrPan->getValue(),2));
    
    this->sliAzimSpan->setValue(*(this->filter->getListSource()[idS]->getAzim()),    dontSendNotification);
    this->sliAzimSpan->setTooltip("A:"+String(this->sliAzimSpan->getValue(),2));
    
    this->sliAElevSpann->setValue(*(this->filter->getListSource()[idS]->getElev()),  dontSendNotification);
    this->sliAElevSpann->setTooltip("E:"+String(this->sliAElevSpann->getValue(),2));

}


void SpatGrisAudioProcessorEditor::updateComMouvement()
{
    this->comMouvement->clear();
    for(int i = 0; i  < MouvementMode::SIZE_MM; i++){
        if(((MouvementMode)i == MouvementMode::SymmetricX || (MouvementMode)i == MouvementMode::SymmetricY)){
            if (this->filter->getNumSourceUsed() == 2){
                this->comMouvement->addItem(GetMouvementModeName((MouvementMode)i), i+1);
            }
        }
        else{
            this->comMouvement->addItem(GetMouvementModeName((MouvementMode)i), i+1);
        }
    }
    this->comMouvement->setSelectedId(this->filter->getSourceMover()->getMouvementModeIndex());
}

void SpatGrisAudioProcessorEditor::updateTrajectoryParam()
{
    this->comTypeTrajectory->setSelectedId(this->filter->getTrajectory()->getTrajectoryType()+1);
    this->texTimeTrajectory->setText(String(this->filter->getTrajectory()->getTimeDuration()));
    this->comTimeTrajectory->setSelectedId(this->filter->getTrajectory()->getInSeconds() ? 1 : 2);
    this->texCycleTrajectory->setText(String(this->filter->getTrajectory()->getCycle()));
    this->sliSpeedTrajectory->setValue(this->filter->getTrajectory()->getSpeed());
    
    this->texTrajEllipseWidth->setText(String(this->filter->getTrajectory()->getEllipseWidth()));
    this->comTrajOneWayReturn->setSelectedId(this->filter->getTrajectory()->getInOneWay() ? 1 : 2);
    this->texTrajRadiusEnd->setText(String(this->filter->getTrajectory()->getRadiusEnd()));
    this->texTrajAngleEnd->setText(String(this->filter->getTrajectory()->getAngleEnd()));
    
    this->texTrajPendDampe->setText(String(this->filter->getTrajectory()->getPendDampening()));
    this->texTrajPendDevia->setText(String(this->filter->getTrajectory()->getPendDeviation()));
    
    this->sliTrajRandSpeed->setValue(this->filter->getTrajectory()->getRandSpeed());
    this->togTrajRandSepare->setToggleState(this->filter->getTrajectory()->getRandSeparate(),dontSendNotification);
    
    if(!(this->filter->getTrajectory()->getProcessTrajectory())){   //Start ready...
        for (auto&& it : this->listLockCompTrajectory)
        {
            it->setEnabled(false);
        }
        
        this->progressBarTraject->setVisible(true);
        this->filter->getTrajectory()->setProcessTrajectory(true);
        
    }else{
        for (auto&& it : this->listLockCompTrajectory)
        {
            it->setEnabled(true);
        }
        
        this->progressBarTraject->setVisible(false);
        this->filter->getTrajectory()->setProcessTrajectory(false);
        this->filter->getTrajectory()->restorePosSources();
    }
    
    this->butReadyTrajectory->setToggleState(this->filter->getTrajectory()->getProcessTrajectory(), dontSendNotification);
    
}

void SpatGrisAudioProcessorEditor::updateInputOutputMode()
{
    int iMaxSources = this->filter->getTotalNumInputChannels();
    int iMaxSpeakers  = this->filter->getTotalNumOutputChannels();
    
    
    if (iMaxSpeakers >=1)  { this->comInOutMode->addItem("1x1",  1);  }
    if (iMaxSpeakers >=2)  { this->comInOutMode->addItem("1x2",  2);  }
    if (iMaxSpeakers >=4)  { this->comInOutMode->addItem("1x4",  3);  }
    if (iMaxSpeakers >=6)  { this->comInOutMode->addItem("1x6",  4);  }
    if (iMaxSpeakers >=8)  { this->comInOutMode->addItem("1x8",  5);  }
    if (iMaxSpeakers >=12) { this->comInOutMode->addItem("1x12", 7); }
    if (iMaxSpeakers >=16) { this->comInOutMode->addItem("1x16", 8); }
    
    if (iMaxSources >=2 && iMaxSpeakers >=2)  { this->comInOutMode->addItem("2x2",  9);  }  //the id here cannot be 0
    if (iMaxSources >=2 && iMaxSpeakers >=4)  { this->comInOutMode->addItem("2x4",  10);  }
    if (iMaxSources >=2 && iMaxSpeakers >=6)  { this->comInOutMode->addItem("2x6",  11);  }
    if (iMaxSources >=2 && iMaxSpeakers >=8)  { this->comInOutMode->addItem("2x8",  12);  }
    if (iMaxSources >=2 && iMaxSpeakers >=12) { this->comInOutMode->addItem("2x12", 13); }
    if (iMaxSources >=2 && iMaxSpeakers >=16) { this->comInOutMode->addItem("2x16", 14); }
    
    if (iMaxSources >=4 && iMaxSpeakers >=4)  { this->comInOutMode->addItem("4x4",  15);  }
    if (iMaxSources >=4 && iMaxSpeakers >=6)  { this->comInOutMode->addItem("4x6",  16);  }
    if (iMaxSources >=4 && iMaxSpeakers >=8)  { this->comInOutMode->addItem("4x8",  17);  }
    if (iMaxSources >=4 && iMaxSpeakers >=12) { this->comInOutMode->addItem("4x12", 18); }
    if (iMaxSources >=4 && iMaxSpeakers >=16) { this->comInOutMode->addItem("4x16", 19); }
    
    if (iMaxSources >=6 && iMaxSpeakers >=6)  { this->comInOutMode->addItem("6x6",  20);  }
    if (iMaxSources >=6 && iMaxSpeakers >=8)  { this->comInOutMode->addItem("6x8",  21);  }
    if (iMaxSources >=6 && iMaxSpeakers >=12) { this->comInOutMode->addItem("6x12", 22); }
    if (iMaxSources >=6 && iMaxSpeakers >=16) { this->comInOutMode->addItem("6x16", 23); }
    
    if (iMaxSources >=8 && iMaxSpeakers >=8)  { this->comInOutMode->addItem("8x8",  24);  }
    if (iMaxSources >=8 && iMaxSpeakers >=12) { this->comInOutMode->addItem("8x12", 25); }
    if (iMaxSources >=8 && iMaxSpeakers >=16) { this->comInOutMode->addItem("8x16", 26); }
    
    this->comSourceSelectPos->clear();
    for(int i = 0; i  < this->filter->getNumSourceUsed(); i++){
        this->comSourceSelectPos->addItem(String(i+1), i+1);
    }
    
    this->comSpeakerSelectPos->clear();
    for(int i = 0; i  < this->filter->getNumSpeakerUsed(); i++){
        this->comSpeakerSelectPos->addItem(String(i+1), i+1);
    }
}

void SpatGrisAudioProcessorEditor::updateSelectSource()
{
    this->comSourceSelectPos->setSelectedId(this->filter->getSelectItem()->selectIdSource+1, dontSendNotification);
    FPoint rayAngleS = this->filter->getRayAngleSource(this->filter->getSelectItem()->selectIdSource);
    
    if(this->comSourceSelectRay->getText() != String(rayAngleS.x,4)){
        this->comSourceSelectRay->setText(String(rayAngleS.x,4), dontSendNotification);
    }
    if(this->comSourceSelectAngle->getText() != String(RadianToDegree(rayAngleS.y))){
        this->comSourceSelectAngle->setText(String(RadianToDegree(rayAngleS.y) ,4), dontSendNotification);
    }
}

void SpatGrisAudioProcessorEditor::updateSelectSpeaker()
{
    this->comSpeakerSelectPos->setSelectedId(this->filter->getSelectItem()->selectIdSpeaker+1, dontSendNotification);
    FPoint xyS = this->filter->getListSpeaker()[this->filter->getSelectItem()->selectIdSpeaker]->getPosXY();
    
    FPoint rayAngleS = FPoint(GetRaySpat(xyS.x, xyS.y), GetAngleSpat(xyS.x, xyS.y));
    
    
    if(this->comSpeakerSelectRay->getText() != String(rayAngleS.x,4)){
        this->comSpeakerSelectRay->setText(String(rayAngleS.x,4), dontSendNotification);
    }
    if(this->comSpeakerSelectAngle->getText() != String(RadianToDegree(rayAngleS.y))){
        this->comSpeakerSelectAngle->setText(String(RadianToDegree(rayAngleS.y) ,4), dontSendNotification);
    }
}

//==============================================================================
void SpatGrisAudioProcessorEditor::buttonClicked (Button *button)
{
    if(this->togLinkAzimSpan == button){
        this->filter->setLinkAzimuth(this->togLinkAzimSpan->getToggleState());
        
    }
    else if(this->togLinkElevSpan == button){
        this->filter->setLinkElevation(this->togLinkElevSpan->getToggleState());
        
    }
    else if(this->togLinkSurfaceOrPan == button){
        this->filter->setLinkHeight(this->togLinkSurfaceOrPan->getToggleState());
    }
    else if(this->togTrajRandSepare == button){
        this->filter->getTrajectory()->setRandSeparate(this->togTrajRandSepare->getToggleState());
        
    }
    else if(this->butReadyTrajectory == button){
        this->butReadyTrajectory->setToggleState(this->filter->getTrajectory()->getProcessTrajectory(), dontSendNotification);
        this->updateTrajectoryParam();
    }
    else if(this->butSourcePos == button){
        this->filter->getSourceMover()->setSourcesPosition((PositionSourceSpeaker)this->comSourcePos->getSelectedItemIndex());
        this->updateSelectSource();
    }
    else if(this->butSpeakerPos == button){
        this->filter->getSourceMover()->setSpeakersPosition((PositionSourceSpeaker)this->comSpeakerPos->getSelectedItemIndex());
        this->updateSelectSpeaker();
    }
    else if(this->togOSCActive == button){
        this->filter->setOscOn(this->togOSCActive->getToggleState());
    }
    else {
        cout << "buttonClicked not found !" << newLine;
    }
}
void SpatGrisAudioProcessorEditor::sliderValueChanged (Slider *slider)
{
    if(this->sliSurfaceOrPan == slider){
        this->filter->setHeightSValue(this->sliSurfaceOrPan->getValue());
        this->sliSurfaceOrPan->setTooltip("S:"+String(this->sliSurfaceOrPan->getValue(),2));
        
    }
    else if(this->sliAzimSpan == slider){
        this->filter->setAzimuthValue(this->sliAzimSpan->getValue());
        this->sliAzimSpan->setTooltip("A:"+String(this->sliAzimSpan->getValue(),2));
        
    }
    else if(this->sliAElevSpann == slider){
        this->filter->setElevationValue(this->sliAElevSpann->getValue());
        this->sliAElevSpann->setTooltip("E:"+String(this->sliAElevSpann->getValue(),2));
        
    }
    else if(this->sliSpeedTrajectory == slider){
        this->filter->getTrajectory()->setSpeed(this->sliSpeedTrajectory->getValue());
        
    }
    else if(this->sliTrajRandSpeed == slider){
        this->filter->getTrajectory()->setRandSpeed(this->sliTrajRandSpeed->getValue());

    }
    else if(this->sliCyclePercent == slider){
        this->filter->getTrajectory()->setCyclePercent(this->sliCyclePercent->getValue());
        
    }
    else {
        cout << "sliderValueChanged not found !" << newLine;
    }
}
void SpatGrisAudioProcessorEditor::comboBoxChanged (ComboBox* comboBox)
{
    if(this->comMouvement == comboBox){
        this->filter->getSourceMover()->setMouvementMode((MouvementMode)this->comMouvement->getSelectedItemIndex());
        
    }
    else if(this->comTypeTrajectory == comboBox){
        
        this->labCyclePercent->setVisible(false);
        this->sliCyclePercent->setVisible(false);
        this->labTrajEllipseWidth->setVisible(false);
        this->texTrajEllipseWidth->setVisible(false);
        this->comTrajOneWayReturn->setVisible(false);
        this->labTrajRadAngEnd->setVisible(false);
        this->texTrajRadiusEnd->setVisible(false);
        this->texTrajAngleEnd->setVisible(false);
        this->butTrajSetEnd->setVisible(false);
        this->butTrajResetEnd->setVisible(false);
        this->labTrajPendDampe->setVisible(false);
        this->texTrajPendDampe->setVisible(false);
        this->labTrajPendDevia->setVisible(false);
        this->texTrajPendDevia->setVisible(false);
        this->labTrajRandSpeed->setVisible(false);
        this->sliTrajRandSpeed->setVisible(false);
        this->togTrajRandSepare->setVisible(false);
        
        switch ((TrajectoryType)this->comTypeTrajectory->getSelectedItemIndex()) {
            case Circle:
                this->labCyclePercent->setVisible(true);
                this->sliCyclePercent->setVisible(true);
                
            case Ellipse:
                this->labCyclePercent->setVisible(true);
                this->sliCyclePercent->setVisible(true);
                this->labTrajEllipseWidth->setVisible(true);
                this->texTrajEllipseWidth->setVisible(true);
                break;
                
            case Spiral:
                this->labCyclePercent->setVisible(true);
                this->sliCyclePercent->setVisible(true);
                this->comTrajOneWayReturn->setVisible(true);
                this->labTrajRadAngEnd->setVisible(true);
                this->texTrajRadiusEnd->setVisible(true);
                this->texTrajAngleEnd->setVisible(true);
                this->butTrajSetEnd->setVisible(true);
                this->butTrajResetEnd->setVisible(true);
                break;
                
            case Pendulum:
                this->labCyclePercent->setVisible(true);
                this->sliCyclePercent->setVisible(true);
                this->comTrajOneWayReturn->setVisible(true);
                this->labTrajRadAngEnd->setVisible(true);
                this->texTrajRadiusEnd->setVisible(true);
                this->texTrajAngleEnd->setVisible(true);
                this->butTrajSetEnd->setVisible(true);
                this->butTrajResetEnd->setVisible(true);
                
                this->labTrajPendDampe->setVisible(true);
                this->texTrajPendDampe->setVisible(true);
                this->labTrajPendDevia->setVisible(true);
                this->texTrajPendDevia->setVisible(true);
                break;
                
            case RandomTraj:
                this->labTrajRandSpeed->setVisible(true);
                this->sliTrajRandSpeed->setVisible(true);
                this->togTrajRandSepare->setVisible(true);
                break;
                
            case RandomTarget:
                this->comTrajOneWayReturn->setVisible(true);
                this->togTrajRandSepare->setVisible(true);
                break;
            
            case SymXTarget:
                this->labCyclePercent->setVisible(true);
                this->sliCyclePercent->setVisible(true);
                break;
                
            case SymYTarget:
                this->labCyclePercent->setVisible(true);
                this->sliCyclePercent->setVisible(true);
                break;
                
            case FreeDrawing:
                this->labCyclePercent->setVisible(true);
                this->sliCyclePercent->setVisible(true);
                this->comTrajOneWayReturn->setVisible(true);
                break;
                
            default:
                break;
        }
        this->filter->getTrajectory()->setTrajectoryType((TrajectoryType)this->comTypeTrajectory->getSelectedItemIndex());
        
    }
    else if(this->comTimeTrajectory == comboBox){
        this->filter->getTrajectory()->setInSeconds(!(this->comTimeTrajectory->getSelectedItemIndex()==1));
        
    }
    else if(this->comTrajOneWayReturn == comboBox){
        this->filter->getTrajectory()->setInOneWay(!(this->comTrajOneWayReturn->getSelectedItemIndex()==1));
    }
    else if(this->comSourceSelectPos == comboBox){
        this->filter->getSelectItem()->selectIdSource = this->comSourceSelectPos->getSelectedId()-1;
        this->updateSelectSource();
    }
    else if(this->comSpeakerSelectPos == comboBox){
        this->filter->getSelectItem()->selectIdSpeaker = this->comSpeakerSelectPos->getSelectedId()-1;
        this->updateSelectSpeaker();
    }
    
    else if(this->comTypeProcess == comboBox){

        switch ((ProcessType)this->comTypeProcess->getSelectedItemIndex()) {
                
            case FreeVolum:
                this->labSurfaceOrPan->setEnabled(true);
                this->labSurfaceOrPan->setText("Surface", dontSendNotification);
                this->togLinkSurfaceOrPan->setEnabled(true);
                this->sliSurfaceOrPan->setEnabled(true);
                
                this->labAzimSpan->setEnabled(false);
                this->togLinkAzimSpan->setEnabled(false);
                this->sliAzimSpan->setEnabled(false);
                
                this->labElevSpan->setEnabled(false);
                this->togLinkElevSpan->setEnabled(false);
                this->sliAElevSpann->setEnabled(false);
                
                this->boxOutputParam->setEnabled(true);
                
                this->togOSCActive->setEnabled(false);
                //Filter
                this->octTab->getTabContentComponent(1)->setEnabled(false);
                //Speakers
                this->octTab->getTabContentComponent(3)->setEnabled(true);
                this->labSpeakerSelectRay->setEnabled(true);
                this->comSpeakerSelectRay->setEnabled(true);
                this->labSpeakerInfoRay->setText("(0 - 2)", dontSendNotification);
                this->labSpeakerInfoRay->setEnabled(true);
                
                break;
            
            case PanSpan:
                this->labSurfaceOrPan->setEnabled(false);
                this->togLinkSurfaceOrPan->setEnabled(false);
                this->sliSurfaceOrPan->setEnabled(false);
                
                this->labAzimSpan->setEnabled(true);
                this->labAzimSpan->setText("Span", dontSendNotification);
                this->togLinkAzimSpan->setEnabled(true);
                this->sliAzimSpan->setEnabled(true);
                
                this->labElevSpan->setEnabled(false);
                this->togLinkElevSpan->setEnabled(false);
                this->sliAElevSpann->setEnabled(false);
                
                this->boxOutputParam->setEnabled(true);
                
                this->togOSCActive->setEnabled(false);
                //Filter
                this->octTab->getTabContentComponent(1)->setEnabled(true);
    
                //Speakers
                this->octTab->getTabContentComponent(3)->setEnabled(true);
                this->labSpeakerSelectRay->setEnabled(false);
                this->comSpeakerSelectRay->setEnabled(false);
                this->labSpeakerInfoRay->setText("(1)", dontSendNotification);
                this->labSpeakerInfoRay->setEnabled(false);
                
                this->filter->getSourceMover()->setSpeakersPosition((PositionSourceSpeaker)this->comSpeakerPos->getSelectedItemIndex());
                this->updateSelectSpeaker();
                
                
                break;
               
            case OSCSpatServer:
                this->labSurfaceOrPan->setEnabled(true);
                this->labSurfaceOrPan->setText("Height", dontSendNotification);
                this->togLinkSurfaceOrPan->setEnabled(true);
                this->sliSurfaceOrPan->setEnabled(true);
                
                this->labAzimSpan->setEnabled(true);
                this->labAzimSpan->setText("Azimuth Span", dontSendNotification);
                this->togLinkAzimSpan->setEnabled(true);
                this->sliAzimSpan->setEnabled(true);
                
                this->labElevSpan->setEnabled(true);
                this->togLinkElevSpan->setEnabled(true);
                this->sliAElevSpann->setEnabled(true);
                
                this->boxOutputParam->setEnabled(false);
                
                this->togOSCActive->setEnabled(true);
                //Filter
                this->octTab->getTabContentComponent(1)->setEnabled(false);
                //Speakers
                this->octTab->getTabContentComponent(3)->setEnabled(false);
                break;
                
            case OSCZirkonium:
                this->labSurfaceOrPan->setEnabled(false);
                this->togLinkSurfaceOrPan->setEnabled(false);
                this->sliSurfaceOrPan->setEnabled(false);
                
                this->labAzimSpan->setEnabled(true);
                this->labAzimSpan->setText("Azimuth Span", dontSendNotification);
                this->togLinkAzimSpan->setEnabled(true);
                this->sliAzimSpan->setEnabled(true);
                
                this->labElevSpan->setEnabled(true);
                this->togLinkElevSpan->setEnabled(true);
                this->sliAElevSpann->setEnabled(true);
                
                this->boxOutputParam->setEnabled(false);
                
                this->togOSCActive->setEnabled(true);
                //Filter
                this->octTab->getTabContentComponent(1)->setEnabled(false);
                //Speakers
                this->octTab->getTabContentComponent(3)->setEnabled(false);
                break;
                
            default:
                jassert(false);
                break;
        }
        this->filter->setTypeProcess((ProcessType)this->comTypeProcess->getSelectedItemIndex());
        
    }
}

void SpatGrisAudioProcessorEditor::textEditorFocusLost (TextEditor &textEditor)
{
    textEditorReturnKeyPressed(textEditor);
}
void SpatGrisAudioProcessorEditor::textEditorReturnKeyPressed (TextEditor &textEditor)
{
    if(this->texTimeTrajectory == &textEditor){
        this->filter->getTrajectory()->setTimeDuration(this->texTimeTrajectory->getText().getFloatValue());
        
    }else if(this->texCycleTrajectory == &textEditor){
        this->filter->getTrajectory()->setCycle(this->texCycleTrajectory->getText().getFloatValue());
        
    }else if(this->texTrajEllipseWidth == &textEditor){
        float w = GetValueInRange(this->texTrajEllipseWidth->getText().getFloatValue(), MinTrajWidthEllipse, MaxTrajWidthEllipse);
        this->filter->getTrajectory()->setEllipseWidth(w);
        this->texTrajEllipseWidth->setText(String(w,2), dontSendNotification);
        
    }else if(this->texTrajRadiusEnd == &textEditor){
        this->filter->getTrajectory()->setRadiusEnd(this->texTrajRadiusEnd->getText().getFloatValue());
    }
    else if(this->texTrajAngleEnd == &textEditor){
        this->filter->getTrajectory()->setAngleEnd(this->texTrajAngleEnd->getText().getFloatValue());
    }
    else if(this->texTrajPendDampe == &textEditor){
        this->filter->getTrajectory()->setPendDampening(this->texTrajPendDampe->getText().getFloatValue());
    }
    else if(this->texTrajPendDevia == &textEditor){
        this->filter->getTrajectory()->setPendDeviation(this->texTrajPendDevia->getText().getFloatValue());
    }
    else if(this->comSourceSelectRay == &textEditor || this->comSourceSelectAngle == &textEditor){
        float r = GetValueInRange(this->comSourceSelectRay->getText().getFloatValue(), 0.0f, RadiusMax);
        float a = GetValueInRange(this->comSourceSelectAngle->getText().getFloatValue(), 0.0f, AngleDegMax);
        this->filter->setPosRayAngSource(this->comSourceSelectPos->getSelectedId()-1, r, a, false);
        this->comSourceSelectRay->setText(String(r,4), dontSendNotification);
        this->comSourceSelectAngle->setText(String(a ,4), dontSendNotification);
    }
    
    else if(this->comSpeakerSelectRay == &textEditor || this->comSpeakerSelectAngle == &textEditor){
        float r = GetValueInRange(this->comSpeakerSelectRay->getText().getFloatValue(), 0.0f, RadiusMax);
        float a = GetValueInRange(this->comSpeakerSelectAngle->getText().getFloatValue(), 0.0f, AngleDegMax);
        this->filter->getListSpeaker()[this->comSpeakerSelectPos->getSelectedId()-1]->setPosXY( GetXYFromRayAng(r, DegreeToRadian(a)));
        this->comSpeakerSelectRay->setText(String(r,4), dontSendNotification);
        this->comSpeakerSelectAngle->setText(String(a ,4), dontSendNotification);
    }
    
    else if(this->texOSCSourceIDF == &textEditor){
        this->filter->setOscFirstIdSource(this->texOSCSourceIDF->getText().getIntValue());
    }
    
    else if(this->texOSCPort == &textEditor){
        int v = GetValueInRange(this->texOSCPort->getText().getIntValue(), OscMinPort, OscMaxPort);
        this->filter->setOscPort(v);
        if(this->filter->getOscRun()){
            this->labOSCPort->setColour(Label::textColourId, this->grisFeel.getFontColour());
        }else{
            this->labOSCPort->setColour(Label::textColourId, Colours::red);
        }
        this->texOSCPort->setText(String(v), dontSendNotification);
    }
}

void SpatGrisAudioProcessorEditor::timerCallback()
{
	this->spatFieldComp->repaint();
    
    if(this->filter->getTrajectory()->getProcessTrajectory()){
        this->progressBarTraject->setValue(this->filter->getTrajectory()->getProgressBar());
    }
    
    if(this->comMouvement->getSelectedId() != this->filter->getSourceMover()->getMouvementMode()){
        this->comMouvement->setSelectedId(this->filter->getSourceMover()->getMouvementModeIndex(), dontSendNotification);
    }
    this->updateSourceParam();
    
    if(this->filter->isPlaying()){
        //Sources
        if(octTab->getCurrentTabIndex() == 2){
            this->updateSelectSource();
        }
        
        //Speakers
        if(octTab->getCurrentTabIndex() == 3){
            this->updateSelectSpeaker();
        }
    }
}

void SpatGrisAudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll(this->grisFeel.getWinBackgroundColour());
}

void SpatGrisAudioProcessorEditor::resized()
{
    const int w = getWidth();
    const int h = getHeight();
    
    const int fieldWidth = w - (Margin + Margin + CenterColumnWidth + Margin + RightColumnWidth + Margin);
    const int fieldHeight = h - (Margin + Margin);
    int fieldSize = jmin(fieldWidth, fieldHeight);
    if (fieldSize < MinFieldSize){
        fieldSize = MinFieldSize;
    }
    int x = Margin + fieldSize + Margin + Margin;
    int y = Margin;
    
    //SpatComponent----------------------
    this->spatFieldComp->setBounds(Margin, Margin, fieldSize, fieldSize);
    this->spatFieldComp->resized(fieldSize);
    
    //BOX------------------------------------
    this->boxSourceParam->setBounds(x, y, CenterColumnWidth, 160);
    this->boxSourceParam->correctSize(CenterColumnWidth, 140);
    
    x += CenterColumnWidth + Margin + Margin;
    this->boxOutputParam->setBounds(x, y, w-(fieldSize+ CenterColumnWidth + (Margin * 7)), 160);
    this->boxOutputParam->correctSize(((unsigned int )this->vecLevelOut.size()*(SizeWidthLevelComp))+4, 130);
    
    x = Margin + fieldSize + Margin + Margin;
    this->boxTrajectory->setBounds(x, 170, w-(fieldSize + (Margin * 5)), 200);
    this->boxTrajectory->correctSize(510, 170);    //w-(fieldSize + (Margin * 5))
    
    //OctTabbedComponent-----------------------
    this->octTab->setBounds(x, 170+206, w-(fieldSize + (Margin * 4)), h - (170+196+(Margin*6)) );

    
    this->resizer->setBounds (w - 16, h - 16, 16, 16);

}
//==============================================================================
