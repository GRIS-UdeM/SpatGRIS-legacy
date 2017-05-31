/*
 ==============================================================================
 SpatGRIS: multichannel sound spatialization plug-in.
 
 Copyright (C) 2015  GRIS-UdeM
 
 LevelComponent.cpp
 Created: 23 Jan 2014 8:09:25am
 
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

#include "../JuceLibraryCode/JuceHeader.h"
#include "LevelComponent.h"


//======================================= LevelBox =====================================================================
LevelBox::LevelBox(LevelComponent * parent, GrisLookAndFeel *feel):
mainParent(parent),
grisFeel(feel)
{
    
}

LevelBox::~LevelBox(){
    
}


void LevelBox::setBounds(const Rectangle<int> &newBounds){
    this->juce::Component::setBounds(newBounds);
    colorGrad = ColourGradient(Colours::red, 0.f, 0.f, Colour::fromRGB(17, 255, 159), 0.f, getHeight(), false);
    colorGrad.addColour(0.1, Colours::yellow);
}

void LevelBox::paint (Graphics& g){
    if(this->mainParent->isMuted()){
        g.fillAll (grisFeel->getWinBackgroundColour());
    }
    else{
        float level = this->mainParent->getLevel();
        g.setGradientFill(colorGrad);
        g.fillRect(0, 0, getWidth() ,getHeight());
        
        if (level < MinLevelComp){
            level = MinLevelComp;
        }
        if (level < 0.9f){
            level = -abs(level);
            g.setColour(grisFeel->getDarkColour());
            g.fillRect(0, 0, getWidth() ,(int)(getHeight()*(level/MinLevelComp)));
        }
    }
}

//==============================================================================
LevelComponent::LevelComponent(SpatGrisAudioProcessor * filt, GrisLookAndFeel *feel, int index)
:
filter(filt),
grisFeel(feel),
indexLev(index)

{
    this->labId = new Label();
    this->labId->setText(String(indexLev), NotificationType::dontSendNotification);
    this->labId->setTooltip (String(indexLev));
    this->labId->setJustificationType(Justification::centred);
    this->labId->setFont(feel->getFont());
    this->labId->setLookAndFeel(feel);
    this->labId->setColour(Label::textColourId, this->grisFeel->getFontColour());
    this->labId->setBounds(0, 0, 16, 16);
    this->addAndMakeVisible(this->labId);
    
    //ToggleButton=========================================================
    this->muteToggleBut = new ToggleButton();
    this->muteToggleBut->setButtonText("M");
    this->muteToggleBut->setSize(16, 16);
    this->muteToggleBut->setTooltip ("Mute "+String(indexLev));
    this->muteToggleBut->addListener(this);
    this->muteToggleBut->setToggleState(false, dontSendNotification);
    this->muteToggleBut->setLookAndFeel(this->grisFeel);
    this->muteToggleBut->setColour(ToggleButton::textColourId, this->grisFeel->getFontColour());
    this->addAndMakeVisible(this->muteToggleBut);
    
    this->levelBox = new LevelBox(this, this->grisFeel);
    this->addAndMakeVisible(this->levelBox);

}

LevelComponent::~LevelComponent()
{
    delete this->labId;
    delete this->muteToggleBut;
    delete this->levelBox;
}


void LevelComponent::buttonClicked(Button *button){
    if (button == this->muteToggleBut) {
        this->filter->getListSpeaker()[this->indexLev]->setMuted(this->muteToggleBut->getToggleState());
        this->levelBox->repaint();
    }
}

void LevelComponent::setBounds(const Rectangle<int> &newBounds){
    this->juce::Component::setBounds(newBounds);
    
    juce::Rectangle<int> labRect(WidthRect/2, 0, newBounds.getWidth()-WidthRect, this->labId->getHeight());
    this->labId->setBounds(labRect);

    this->muteToggleBut->setBounds((newBounds.getWidth()/2)-6, getHeight()-22, this->muteToggleBut->getWidth(), this->muteToggleBut->getHeight());
 
    juce::Rectangle<int> level(WidthRect/2, 16, newBounds.getWidth()-WidthRect, getHeight()-40 );
    this->levelBox->setBounds(level);
}

float LevelComponent::getLevel(){
    return this->level;
}

void LevelComponent::update(){
    float l = -30.0f;// this->filter->getLevel(indexLev);
    if(isnan(l)){ return; }
    if(!this->muteToggleBut->getToggleState() && this->level != l){
        this->repaint();
    }
    this->level = l;
}

bool LevelComponent::isMuted(){
    return this->muteToggleBut->getToggleState();
}


