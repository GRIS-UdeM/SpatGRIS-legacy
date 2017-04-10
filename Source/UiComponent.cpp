//
//  UiComponent.cpp
//  SpatGRIS
//
//  Created by GRIS on 2017-04-06.
//
//

#include "UiComponent.h"
#include "PluginProcessor.h"

//======================================= BOX ===========================================================================
Box::Box(GrisLookAndFeel *feel, String title) {
    
    this->title = title;
    this->grisFeel = feel;
    this->bgColour = this->grisFeel->getBackgroundColour();
    
    this->content = new Component();
    this->viewport = new Viewport();
    this->viewport->setViewedComponent(this->content, false);
    this->viewport->setScrollBarsShown(true, true);
    this->viewport->setScrollBarThickness(6);
    this->viewport->getVerticalScrollBar()->setColour(ScrollBar::ColourIds::thumbColourId, feel->getScrollBarColour());
    this->viewport->getHorizontalScrollBar()->setColour(ScrollBar::ColourIds::thumbColourId, feel->getScrollBarColour());
    
    this->viewport->setLookAndFeel(this->grisFeel);
    addAndMakeVisible(this->viewport);
}

Box::~Box(){
    this->content->deleteAllChildren();
    delete this->viewport;
    delete this->content;
}

Component * Box::getContent() {
    return this->content ? this->content : this;
}


void Box::resized() {
    if (this->viewport){
        this->viewport->setSize(getWidth(), getHeight());
    }
}

void Box::correctSize(unsigned int width, unsigned int height){
    if(this->title!=""){
        this->viewport->setTopLeftPosition(0, 20);
        this->viewport->setSize(getWidth(), getHeight()-20);
        if(width<80){
            width = 80;
        }
    }else{
        this->viewport->setTopLeftPosition(0, 0);
    }
    this->getContent()->setSize(width, height);
}


void Box::paint(Graphics &g) {
    g.setColour(this->bgColour);
    g.fillRect(getLocalBounds());
    if(this->title!=""){
        g.setColour (this->grisFeel->getWinBackgroundColour());
        g.fillRect(0,0,getWidth(),18);
        
        g.setColour (this->grisFeel->getFontColour());
        g.drawText(title, 0, 0, this->content->getWidth(), 20, juce::Justification::left);
    }
}
//=================================================================================================================


//================================== OctTabbedComponent ==============================
OctTabbedComponent::OctTabbedComponent(GrisLookAndFeel *feel, TabbedButtonBar::Orientation orientation, SpatGrisAudioProcessor *filter)
:
TabbedComponent(orientation),
filter(filter)
,mInited(false)
{
    this->setLookAndFeel(feel);
}

void OctTabbedComponent::currentTabChanged (int newCurrentTabIndex, const String& newCurrentTabName) {
    if (!mInited) {
        return;
    }
    //this->filter->setGuiTab(newCurrentTabIndex);
}

void OctTabbedComponent::initDone(){
    mInited = true;
}

//=================================================================================================================


//================================== ProgressBarTrajec ==============================
ProgressBarTraj::ProgressBarTraj():
value(0.f)
{
}
void ProgressBarTraj::paint(Graphics &g)
{
    juce::Rectangle<int> box = getLocalBounds();
    
    g.setColour(Colours::black);
    g.fillRect(box);
    
    g.setColour(Colour::fromRGB(17, 255, 159));
    box.setWidth(box.getWidth() * this->value);
    g.fillRect(box);
}
void ProgressBarTraj::setValue(float v)
{
    this->value = v;
    this->repaint();
}
//=================================================================================================================
