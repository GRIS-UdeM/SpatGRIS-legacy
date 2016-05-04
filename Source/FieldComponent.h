/*
 ==============================================================================
<<<<<<< HEAD
 SpatGRIS: multichannel sound spatialization plug-in.
=======
 Octogris2: multichannel sound spatialization plug-in.
>>>>>>> 2588dc2f3221b0a2cc68818c05101612d949a534
 
 Copyright (C) 2015  GRIS-UdeM
 
 FieldComponent.h
 Created: 15 Jan 2014 10:59:44am
 
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

#ifndef FIELDCOMPONENT_H_INCLUDED
#define FIELDCOMPONENT_H_INCLUDED

#include "PluginProcessor.h"
#include "GrisLookAndFeel.h"
#include "SourceMover.h"
#include "deque"

typedef enum
{
	kNoSelection,
	kSelectedSource,
	kSelectedSpeaker
} SelectionType;

class FieldComponent : public Component
{
public:
    FieldComponent(SpatGrisAudioProcessor* filter, SourceMover *mover);
    ~FieldComponent();

    void paint (Graphics&);
	
	void mouseDown (const MouseEvent &event);
 	void mouseDrag (const MouseEvent &event);
 	void mouseUp (const MouseEvent &event);
	
	FPoint getSourcePoint(int i);
	FPoint getSpeakerPoint(int i);
	float getDistance(int source, int speaker);
    
    void clearTrajectoryPath();
    void updatePositionTrace(float p_fX, float p_fY);
private:
	SpatGrisAudioProcessor *mFilter;
	SourceMover *m_pMover;
	
	SelectionType mSelectionType;
	int mSelectedItem;
	
	ModifierKeys mLastKeys;
	float mSavedValue;

	FPoint convertSourceRT(float r, float t);

    inline double degreeToRadian (float degree){
        return ((degree/360.0)* 2 * M_PI);
    }
    
    Point <float> degreeToXy (Point <float> p, int p_iFieldWidth){
        float x,y;
        x = -((p_iFieldWidth - kSourceDiameter)/2) * sinf(degreeToRadian(p.getX())) * cosf(degreeToRadian(p.getY()));
        y = -((p_iFieldWidth - kSourceDiameter)/2) * cosf(degreeToRadian(p.getX())) * cosf(degreeToRadian(p.getY()));
        return Point <float> (x, y);
    }
    
    GrisLookAndFeel mGrisFeel;
    int m_iCurPathLines;
    int m_iMaxPathLines;
    std::deque<FPoint> m_dqAllPathPoints;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FieldComponent)
};


#endif  // FIELDCOMPONENT_H_INCLUDED
