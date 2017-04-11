/*
 ==============================================================================
 SpatGRIS: multichannel sound spatialization plug-in.
 
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

#ifndef SPATCOMPONENT_H_INCLUDED
#define SPATCOMPONENT_H_INCLUDED


#define STRING2(x) #x
#define STRING(x) STRING2(x)

#include "../JuceLibraryCode/JuceHeader.h"
#include "../../GrisCommonFiles/GrisLookAndFeel.h"

#include "DefaultParam.h"

using namespace std;

class SpatGrisAudioProcessor;
class SpatGrisAudioProcessorEditor;

static float AngleInCircle(FPoint p) {
    return  -atan2(( - p.y * 2.0f), (p.x * 2.0f ));
}
static float DegreeToRadian (float degree){
    return ((degree * M_PI ) / 180.0f) ;
}
static float RadianToDegree (float radian){
    return ((radian * 180.0f ) / M_PI);
}

static void NormalizeXYSourceWithScreen(FPoint &p, float w){
    p.x = ((w) + ((w/2.0f)*p.x))+SourceRadius;
    p.y = ((w) - ((w/2.0f)*p.y))+SourceRadius;
}
static void NormalizeScreenWithSpat(FPoint &p, float w){
    p.x = (p.x - SourceRadius - w) / (w/2.0f);
    p.y = -(p.y - SourceRadius - w) / (w/2.0f);
}

static FPoint DegreeToXy (FPoint p, int FieldWidth){
    float x,y;
    x = -((FieldWidth - SourceDiameter)/2) * sinf(DegreeToRadian(p.x)) * cosf(DegreeToRadian(p.y));
    y = -((FieldWidth - SourceDiameter)/2) * cosf(DegreeToRadian(p.x)) * cosf(DegreeToRadian(p.y));
    return FPoint(x, y);
}

static FPoint GetSourceAzimElev(FPoint pXY, bool bUseCosElev = false) {

    //calculate azim in range [0,1], and negate it because zirkonium wants -1 on right side
    float fAzim = -atan2f(pXY.x, pXY.y)/M_PI;
    
    //calculate xy distance from origin, and clamp it to 2 (ie ignore outside of circle)
    float hypo = hypotf(pXY.x, pXY.y);
    if (hypo > RadiusMax){
        hypo = RadiusMax;
    }
    
    float fElev;
    if (bUseCosElev){
        fElev = acosf(hypo/RadiusMax);   //fElev is elevation in radian, [0,pi/2)
        fElev /= (M_PI/2.f);                      //making range [0,1]
        fElev /= 2.f;                            //making range [0,.5] because that's what the zirkonium wants
    } else {
        fElev = (RadiusMax-hypo)/4.0f;
    }
    
    return FPoint(fAzim, fElev);
}

class SpatComponent :   public Component
{
public:
    SpatComponent(SpatGrisAudioProcessorEditor * edit, SpatGrisAudioProcessor * filt, GrisLookAndFeel *feel);
    ~SpatComponent();
    
    //======================================================
    void paint(Graphics &g) ;
    void resized(int fieldSize);
    
    //======================================================
    void mouseDown (const MouseEvent &event);
    void mouseDrag (const MouseEvent &event);
    void mouseUp (const MouseEvent &event);
    
private:
    Colour getColor(int i);
    
    void drawAzimElevSource(Graphics &g, const int i, const int fieldWH, const int fieldCenter);
    
    SpatGrisAudioProcessorEditor * editor;
    SpatGrisAudioProcessor * filter;
    GrisLookAndFeel *grisFeel;
    ImageComponent logoImg;
    Label       labVersion;
    
    
};
#endif  // SPATCOMPONENT_H_INCLUDED
