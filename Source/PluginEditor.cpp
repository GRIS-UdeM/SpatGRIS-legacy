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

    this->spatFieldComp = new SpatComponent(this->filter, &this->grisFeel);
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
    this->labSurfaceOrPan       = addLabel("Surface", "Surface Master Soutce", 0, 0, CenterColumnWidth, DefaultLabHeight, this->boxSourceParam->getContent());
    this->togLinkSurfaceOrPan   = addToggleButton("Link", "Link other sources", 0, 20, CenterColumnWidth, DefaultLabHeight,  this->boxSourceParam->getContent());
    this->sliSurfaceOrPan       = addSlider("", "", 50, 18, 130, DefaultLabHeight, this->boxSourceParam->getContent());
    
    this->labAzimSpan           = addLabel("Azimuth Span", "Azimuth Span Master Soutce", 0, 50, CenterColumnWidth, DefaultLabHeight, this->boxSourceParam->getContent());
    this->togLinkAzimSpan       = addToggleButton("Link", "Link other sources", 0, 70, CenterColumnWidth, DefaultLabHeight,  this->boxSourceParam->getContent());
    this->sliSurfaceOrPan       = addSlider("", "", 50, 68, 130, DefaultLabHeight, this->boxSourceParam->getContent());
    
    this->labElevSpan           = addLabel("Elevation Span", "Elevation Span Master Soutce", 0, 100, CenterColumnWidth, DefaultLabHeight, this->boxSourceParam->getContent());
    this->togLinkElevSpan       = addToggleButton("Link", "Link other sources", 0, 120, CenterColumnWidth, DefaultLabHeight,  this->boxSourceParam->getContent());
    this->sliSurfaceOrPan       = addSlider("", "", 50, 118, 130, DefaultLabHeight, this->boxSourceParam->getContent());
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
    
    
    
    //OctTabbedComponent-----------------------
    
    //Settings
    //-----------------------------
    
    //------------------------------------------------------------------------------------------------------
    
    
    
    //Size window
    this->resizeWindow.setSizeLimits (MinFieldSize + (2*Margin), MinFieldSize + (2*Margin), 1920, 1080);
    this->addAndMakeVisible (this->resizer = new ResizableCornerComponent (this, &this->resizeWindow));
    this->setSize(DefaultUItWidth, DefaultUIHeight);

	this->startTimerHz(HertzRefresh);
}

SpatGrisAudioProcessorEditor::~SpatGrisAudioProcessorEditor()
{
    for (auto&& it : this->vecLevelOut)
    {
        delete (it);
    }
  
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

Slider* SpatGrisAudioProcessorEditor::addSlider(const String &s, const String &stooltip, int x, int y, int w, int h, Component *into, juce::Slider::TextEntryBoxPosition tebp)
{
    Slider *sd = new Slider();
    sd->setTooltip (stooltip);
    //sd->setTextValueSuffix(s);
    sd->setSize(w, h);
    sd->setTopLeftPosition(x, y);
    //sd->setSliderStyle(Slider::Rotary);
    //sd->setRotaryParameters(M_PI * 1.3f, M_PI * 2.7f, true);
    sd->setTextBoxStyle (tebp, false, 60, 20);
    sd->setColour(ToggleButton::textColourId, this->grisFeel.getFontColour());
    sd->setLookAndFeel(&this->grisFeel);
    sd->addListener(this);
    into->addAndMakeVisible(sd);
    return sd;
}


//==============================================================================

void SpatGrisAudioProcessorEditor::buttonClicked (Button *button)
{
}
void SpatGrisAudioProcessorEditor::sliderValueChanged (Slider *slider)
{
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
    this->boxOutputParam->setBounds(x, y, w-(fieldSize+ CenterColumnWidth + (2 * 7)), 160);
    this->boxOutputParam->correctSize(((unsigned int )this->vecLevelOut.size()*(SizeWidthLevelComp))+4, 130);
    
    x = Margin + fieldSize + Margin + Margin;
    this->boxTrajectory->setBounds(x, 170, w-(fieldSize + (2 * 5)), 200);
    this->boxTrajectory->correctSize(w-(fieldSize + (2 * 5)), 170);
    
    //OctTabbedComponent-----------------------
    this->octTab->setBounds(x, 170+210, w-(fieldSize + (2 * 4)), h - (170+200+(2*6)) );

    
    this->resizer->setBounds (w - 16, h - 16, 16, 16);

}
//==============================================================================
