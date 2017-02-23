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
#include "../../GrisCommonFiles/GrisLookAndFeel.h"

static const float kMinLevel = -60.f;
static const float kMaxLevel = 1.f;
static const float kMaxMin = kMaxLevel - kMinLevel;

//==============================================================================
/*
*/
class LevelComponent : public Component
{
public:
    LevelComponent(SpatGrisAudioProcessor* filter, int index);
    ~LevelComponent();
    
    void setMute(bool b);
    void paint (Graphics&);
    void setBounds(const Rectangle<int> &newBounds);
    //void refreshIfNeeded();
	
private:
	SpatGrisAudioProcessor *mFilter;
	int mIndex;
    bool muted;
    ColourGradient colorGrad;
	/*float mLevelAdjustment;
	float mShowLevel;
	uint64_t mLastProcessCounter;*/
    GrisLookAndFeel mLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelComponent)
};


#endif  // LEVELCOMPONENT_H_INCLUDED
