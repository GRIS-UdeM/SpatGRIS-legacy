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
    this->togLinkSurfaceOrPan   = addToggleButton("Link", "Link other sources", 0, 20, DefaultLabWidth, DefaultLabHeight,  this->boxSourceParam->getContent());
    this->sliSurfaceOrPan       = addSlider("", "", 50, 18, 130, DefaultLabHeight, this->boxSourceParam->getContent(), MinSurfSource, MaxSurfSource, DefaultSliderInter);
    
    this->labAzimSpan           = addLabel("Azimuth Span", "Azimuth Span Master Soutce", 0, 50, DefaultLabWidth, DefaultLabHeight, this->boxSourceParam->getContent());
    this->togLinkAzimSpan       = addToggleButton("Link", "Link other sources", 0, 70, DefaultLabWidth, DefaultLabHeight,  this->boxSourceParam->getContent());
    this->sliAzimSpan           = addSlider("", "", 50, 68, 130, DefaultLabHeight, this->boxSourceParam->getContent(), MinAzimSource, MaxAzimSource, DefaultSliderInter);
    
    this->labElevSpan           = addLabel("Elevation Span", "Elevation Span Master Soutce", 0, 100, DefaultLabWidth, DefaultLabHeight, this->boxSourceParam->getContent());
    this->togLinkElevSpan       = addToggleButton("Link", "Link other sources", 0, 120, DefaultLabWidth, DefaultLabHeight,  this->boxSourceParam->getContent());
    this->sliAElevSpann         = addSlider("", "", 50, 118, 130, DefaultLabHeight, this->boxSourceParam->getContent(), MinElevSource, MaxElevSource, DefaultSliderInter);
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
    this->labMouvement      = addLabel("Mouvements :", "Mouvement with other sources", 0, 0, DefaultLabWidth, DefaultLabHeight, this->boxTrajectory->getContent());
    this->comMouvement      = addComboBox("", "Mouvement with other sources", 90, 0, DefaultLabWidth, DefaultLabHeight, this->boxTrajectory->getContent());
    
    this->labTypeTrajectory = addLabel("Types :", "Types of trajectories", 0, 30, DefaultLabWidth, DefaultLabHeight, this->boxTrajectory->getContent());
    this->comTypeTrajectory = addComboBox("", "Types of trajectories", 60, 30, DefaultLabWidth+30, DefaultLabHeight, this->boxTrajectory->getContent());
    for(int i = 0; i  < TrajectoryType::SIZE_TT; i++){
        this->comTypeTrajectory->addItem(Trajectory::GetTrajectoryName((TrajectoryType)i), i+1);
    }
    this->comTypeTrajectory->setSelectedId(1);
    
    
    this->labTimeTrajectory = addLabel("Time :", "Time of trajectory", 0, 50, DefaultLabWidth, DefaultLabHeight, this->boxTrajectory->getContent());
    this->texTimeTrajectory = addTextEditor("", "", "Time of trajectory", 60, 50, DefaultTexWidth, DefaultLabHeight, this->boxTrajectory->getContent());
    this->comTimeTrajectory = addComboBox("", "Per cycle(s)", 124, 50, DefaultLabWidth-34, DefaultLabHeight, this->boxTrajectory->getContent());
    this->comTimeTrajectory->addItem("Second(s)", 1);
    this->comTimeTrajectory->addItem("Beat(s)", 2);
    this->comTimeTrajectory->setSelectedId(1);
    
    this->labCycleTrajectory = addLabel("Cycle(s) :", "Cycle of trajectory", 0, 70, DefaultLabWidth, DefaultLabHeight, this->boxTrajectory->getContent());
    this->texCycleTrajectory = addTextEditor("", "", "Numbers of cycle(s) 0=inf", 60, 70, DefaultTexWidth, DefaultLabHeight, this->boxTrajectory->getContent());
    
    this->butReadyTrajectory = addButton("Ready", "Valid trajectory param", 60, 100, DefaultLabWidth, DefaultLabHeight, this->boxTrajectory->getContent());
    
    this->progressBarTraject = new ProgressBarTraj();
    this->progressBarTraject->setBounds(60, 124, DefaultLabWidth+30, DefaultLabHeight);
    this->boxTrajectory->getContent()->addAndMakeVisible(this->progressBarTraject);
    
    this->sliSpeedTrajectory = addSlider("Speed :", "Speed of trajectory", 14, 150, 204, DefaultLabHeight, this->boxTrajectory->getContent(), MinSpeedTrajectory, MaxSpeedTrajectory,  0.001f, juce::Slider::TextEntryBoxPosition::TextBoxLeft);
    
    //Other param Trajectories hided
    int rowX = 260;
    
    this->labTrajEllipseWidth = addLabel("Width :", "Width of ellipse", rowX, 30, DefaultLabWidth, DefaultLabHeight, this->boxTrajectory->getContent());
    this->texTrajEllipseWidth = addTextEditor("", "", "Width of ellipse", rowX+50, 30, DefaultTexWidth, DefaultLabHeight, this->boxTrajectory->getContent());
    
    this->comTrajOneWayReturn = addComboBox("", "Trajectory cycle", rowX, 30, DefaultLabWidth-30, DefaultLabHeight, this->boxTrajectory->getContent());
    this->comTrajOneWayReturn->addItem("One Way", 1);
    this->comTrajOneWayReturn->addItem("Return", 2);
    this->comTrajOneWayReturn->setSelectedId(1);

    this->labTrajRadAngEnd = addLabel("Radius / Angle of end point :", "Radius and angle end point", rowX-5, 50, 200, DefaultLabHeight, this->boxTrajectory->getContent());
    this->texTrajRadiusEnd = addTextEditor("", "", "Radius (0-2)", rowX, 70, 40, DefaultLabHeight, this->boxTrajectory->getContent());
    this->texTrajAngleEnd = addTextEditor("", "", "Angle (0-360)", rowX+50, 70, 40, DefaultLabHeight, this->boxTrajectory->getContent());

    this->butTrajSetEnd = addButton("Set", "Set end point", rowX+100, 70, 70, DefaultLabHeight, this->boxTrajectory->getContent());
    this->butTrajResetEnd = addButton("Reset", "Reset end point", rowX+180, 70, 70, DefaultLabHeight, this->boxTrajectory->getContent());

    this->labTrajPendDampe = addLabel("Dampening :", "Dampening (0-1)", rowX-5, 94, 80, DefaultLabHeight, this->boxTrajectory->getContent());
    this->texTrajPendDampe = addTextEditor("", "", "Dampening (0-1)", rowX+80, 94, 40, DefaultLabHeight, this->boxTrajectory->getContent());

    this->labTrajPendDevia = addLabel("Deviation :", "Deviation (0-360)", rowX-5, 114, 80, DefaultLabHeight, this->boxTrajectory->getContent());
    this->texTrajPendDevia = addTextEditor("", "", "Deviation (0-360)", rowX+80, 114, 40, DefaultLabHeight, this->boxTrajectory->getContent());
    
    this->labTrajRandSpeed = addLabel("Speed :", "Speed Random", rowX-5, 30, 80, DefaultLabHeight, this->boxTrajectory->getContent());
    this->sliTrajRandSpeed = addSlider("Speed :", "Speed Random", rowX+48, 30, 160, DefaultLabHeight, this->boxTrajectory->getContent(), MinTrajRandomSpeed, MaxTrajRandomSpeed,  0.01f, juce::Slider::TextEntryBoxPosition::TextBoxLeft);
    this->togTrajRandSepare = addToggleButton("Separate sources :", "Force separate automation sources", rowX, 50, DefaultLabWidth, DefaultLabHeight,  this->boxTrajectory->getContent());

    
    //OctTabbedComponent-----------------------
    
    //Settings
    //-----------------------------
    
    //------------------------------------------------------------------------------------------------------
    
    
    
    //Size window
    this->resizeWindow.setSizeLimits (MinFieldSize + (2*Margin), MinFieldSize + (2*Margin), 1920, 1080);
    this->addAndMakeVisible (this->resizer = new ResizableCornerComponent (this, &this->resizeWindow));
    this->setSize(DefaultUItWidth, DefaultUIHeight);
    
    this->updateComMouvement();
    this->updateSourceParam();
    
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

TextEditor* SpatGrisAudioProcessorEditor::addTextEditor(const String &s, const String &emptyS, const String &stooltip, int x, int y, int w, int h, Component *into, int wLab)
{
    TextEditor *te = new TextEditor();
    te->setTooltip (stooltip);
    te->setTextToShowWhenEmpty(emptyS, this->grisFeel.getOffColour());
    te->setColour(ToggleButton::textColourId, this->grisFeel.getFontColour());
    te->setLookAndFeel(&this->grisFeel);
    
    if (s.isEmpty()){
        te->setBounds(x, y, w, h);
    }else{
        te->setBounds(x+wLab, y, w, h);
        Label *lb =addLabel(s, "", x, y, wLab, h, into);
        lb->setJustificationType(Justification::centredRight);
    }
    
    te->addListener(this);
    into->addAndMakeVisible(te);
    return te;
}

Slider* SpatGrisAudioProcessorEditor::addSlider(const String &s, const String &stooltip, int x, int y, int w, int h, Component *into, float minF, float maxF, float defF, juce::Slider::TextEntryBoxPosition tebp)
{
    Slider *sd = new Slider();
    sd->setTooltip (stooltip);
    //sd->setTextValueSuffix(s);
    sd->setSize(w, h);
    sd->setTopLeftPosition(x, y);
    sd->setRange(minF, maxF, defF);
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
    this->togLinkSurfaceOrPan->setToggleState(this->filter->getLinkSurface(),   dontSendNotification);
    this->togLinkAzimSpan->setToggleState(this->filter->getLinkAzimuth(),       dontSendNotification);
    this->togLinkElevSpan->setToggleState(this->filter->getLinkElevation(),     dontSendNotification);
    
    const int idS = this->filter->getSelectItem()->selectID;
    this->sliSurfaceOrPan->setValue(*(this->filter->getListSource().at(idS)->getSurf()),dontSendNotification);
    this->sliSurfaceOrPan->setTooltip("S:"+String(this->sliSurfaceOrPan->getValue(),2));
    
    this->sliAzimSpan->setValue(*(this->filter->getListSource().at(idS)->getAzim()),    dontSendNotification);
    this->sliAzimSpan->setTooltip("A:"+String(this->sliAzimSpan->getValue(),2));
    
    this->sliAElevSpann->setValue(*(this->filter->getListSource().at(idS)->getElev()),  dontSendNotification);
    this->sliAElevSpann->setTooltip("E:"+String(this->sliAElevSpann->getValue(),2));
}


void SpatGrisAudioProcessorEditor::updateComMouvement()
{
    this->comMouvement->clear();
    for(int i = 0; i  < MouvementMode::SIZE_MM; i++){
        if(((MouvementMode)i == MouvementMode::SymmetricX || (MouvementMode)i == MouvementMode::SymmetricY)){
            if (this->filter->getNumSourceUsed() == 2){
                this->comMouvement->addItem(this->sourceMover->getMouvementModeName((MouvementMode)i), i+1);
            }
        }
        else{
            this->comMouvement->addItem(this->sourceMover->getMouvementModeName((MouvementMode)i), i+1);
        }
    }
    this->comMouvement->setSelectedId(1);
}
//==============================================================================

void SpatGrisAudioProcessorEditor::buttonClicked (Button *button)
{
    if(this->togLinkSurfaceOrPan == button){
        this->filter->setLinkSurface(this->togLinkSurfaceOrPan->getToggleState());
        
    }else if(this->togLinkAzimSpan == button){
        this->filter->setAzimuthValue(this->togLinkAzimSpan->getToggleState());
        
    }else if(this->togLinkElevSpan == button){
        this->filter->setElevationValue(this->togLinkElevSpan->getToggleState());
        
    }else {
        cout << "buttonClicked not found !" << newLine;
    }
}
void SpatGrisAudioProcessorEditor::sliderValueChanged (Slider *slider)
{
    
    if(this->sliSurfaceOrPan == slider){
        this->filter->setSurfaceValue(this->sliSurfaceOrPan->getValue());
        this->sliSurfaceOrPan->setTooltip("S:"+String(this->sliSurfaceOrPan->getValue(),2));
        
    }else if(this->sliAzimSpan == slider){
        this->filter->setAzimuthValue(this->sliAzimSpan->getValue());
        this->sliAzimSpan->setTooltip("A:"+String(this->sliAzimSpan->getValue(),2));
        
    }else if(this->sliAElevSpann == slider){
        this->filter->setElevationValue(this->sliAElevSpann->getValue());
        this->sliAElevSpann->setTooltip("E:"+String(this->sliAElevSpann->getValue(),2));
        
    }else {
        cout << "sliderValueChanged not found !" << newLine;
    }
}
void SpatGrisAudioProcessorEditor::comboBoxChanged (ComboBox* comboBox)
{
    if(this->comTypeTrajectory == comboBox){

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
                
            case Ellipse:
                this->labTrajEllipseWidth->setVisible(true);
                this->texTrajEllipseWidth->setVisible(true);
                break;
                
            case Spiral:
                this->comTrajOneWayReturn->setVisible(true);
                this->labTrajRadAngEnd->setVisible(true);
                this->texTrajRadiusEnd->setVisible(true);
                this->texTrajAngleEnd->setVisible(true);
                this->butTrajSetEnd->setVisible(true);
                this->butTrajResetEnd->setVisible(true);
                break;
                
            case Pendulum:
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
            
            case FreeDrawing:
                this->comTrajOneWayReturn->setVisible(true);
                break;
                
            default:
                break;
        }
        
    }
}
void SpatGrisAudioProcessorEditor::textEditorFocusLost (TextEditor &textEditor)
{
}
void SpatGrisAudioProcessorEditor::textEditorReturnKeyPressed (TextEditor &textEditor)
{
}

void SpatGrisAudioProcessorEditor::timerCallback()
{
	this->spatFieldComp->repaint();
}

void SpatGrisAudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll(this->grisFeel.getWinBackgroundColour());
}

void SpatGrisAudioProcessorEditor::resized()
{
    int w = getWidth();
    int h = getHeight();
    
    int fieldWidth = w - (Margin + Margin + CenterColumnWidth + Margin + RightColumnWidth + Margin);
    int fieldHeight = h - (Margin + Margin);
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
    this->boxTrajectory->correctSize(w-(fieldSize + (Margin * 5)), 170);
    
    //OctTabbedComponent-----------------------
    this->octTab->setBounds(x, 170+210, w-(fieldSize + (Margin * 4)), h - (170+200+(Margin*6)) );

    
    this->resizer->setBounds (w - 16, h - 16, 16, 16);

}
//==============================================================================
