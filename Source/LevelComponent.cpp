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



//==============================================================================
LevelComponent::LevelComponent(SpatGrisAudioProcessor* filter, int index)
:
	mFilter(filter),
	mIndex(index),
	/*mLevelAdjustment(1),
	mShowLevel(0),
	mLastProcessCounter(0),*/
    muted(false)
{
    
}

LevelComponent::~LevelComponent()
{
	
}

void LevelComponent::setBounds(const Rectangle<int> &newBounds){
    this->juce::Component::setBounds(newBounds); 
    colorGrad = ColourGradient(Colours::red, 0.f, 0.f, Colour::fromRGB(17, 255, 159), 0.f, getHeight(), false);
    colorGrad.addColour(0.1, Colours::yellow);
}

void LevelComponent::setMute(bool b){
    muted = b;
}
/*
void LevelComponent::refreshIfNeeded(){
    float level;
    uint64_t processCounter = mFilter->getProcessCounter();
    if (mLastProcessCounter != processCounter) {
        mLastProcessCounter = processCounter;
        mLevelAdjustment = 1;
        level = mFilter->getLevel(mIndex);
    } else {
        mLevelAdjustment *= 0.8;
        level = mLevelAdjustment * mFilter->getLevel(mIndex);
    }
    
    if (mShowLevel != level) {
        mShowLevel = level;
        repaint();
    }
}*/

void LevelComponent::paint (Graphics& g)
{
    /*
	const float yellowStart = -6;
	float hue;
	if (level > 0)
        hue = 0;
	else if (level < yellowStart)
        hue = 1 / 3.f;
	else
	{
		float p = (level - yellowStart) / (-yellowStart); // 0 .. 1
		hue = (1 - p) / 3.f;
	}*/
	//fprintf(stderr, "speaker %d linear: %.3f dB: %.1f hue: %.3f\n", mIndex, mLastLevel, level, hue);

    if(muted){
        g.fillAll (mLookAndFeel.getWinBackgroundColour());
        
    }else{
        float level = linearToDb(mFilter->getLevel(mIndex));
        
        /*if (isnan(level)){
            level = 0;
        }*/
        
        if (level < kMinLevel){
            level = kMinLevel;
        }
        else if (level > kMaxLevel){
            level = kMaxLevel;
        }


        g.setGradientFill(colorGrad);
        g.fillRect(0, 0, getWidth() ,getHeight());
        
        g.setColour(mLookAndFeel.getDarkColour());
        g.fillRect(0, 0, getWidth() ,(int)(getHeight()*(level/kMinLevel)));
    }
    
	
 
}
