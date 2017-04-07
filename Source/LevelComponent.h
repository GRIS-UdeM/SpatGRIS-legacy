/*
 ==============================================================================
 SpatGRIS: multichannel sound spatialization plug-in.
 
 Copyright (C) 2015  GRIS-UdeM
 
 LevelComponent.h
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

#ifndef LEVELCOMPONENT_H_INCLUDED
#define LEVELCOMPONENT_H_INCLUDED

#include "PluginProcessor.h"
#include "GrisLookAndFeel.h"

class LevelComponent;

static const float MinLevelComp  = -60.f;
static const float MaxLevelComp  = 1.f;
static const float MaxMinLevComp = MaxLevelComp - MinLevelComp;
static const int   WidthRect     = 2;

//======================================= LevelBox ===================================
class LevelBox : public Component
{
public:
    LevelBox(LevelComponent* parent, GrisLookAndFeel *feel);
    ~LevelBox();
    
    void setBounds(const Rectangle<int> &newBounds);
    void paint (Graphics& g);
    
private:
    LevelComponent *mainParent;
    GrisLookAndFeel *grisFeel;
    ColourGradient colorGrad;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelBox)
};

//====================================================================================
class LevelComponent :  public Component,
                        public ToggleButton::Listener
{
public:
    LevelComponent(SpatGrisAudioProcessor * filt, GrisLookAndFeel *feel, int index);
    ~LevelComponent();
    
    
    void buttonClicked(Button *button) override;
    void setBounds(const Rectangle<int> &newBounds);
    
    void update();
    bool isMuted();
    float getLevel();

    
private:
    SpatGrisAudioProcessor* filter;
    GrisLookAndFeel * grisFeel;
    LevelBox * levelBox;
    Label * labId;
    ToggleButton * muteToggleBut;

    float level = MinLevelComp;
    int indexLev;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelComponent)
};



#endif  // LEVELCOMPONENT_H_INCLUDED
