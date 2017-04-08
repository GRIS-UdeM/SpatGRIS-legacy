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

typedef enum
{
    NoSelection,
    SelectedSource,
    SelectedSpeaker
} SelectionType;

struct SelectItem{
    int selectID;
    SelectionType selecType;
};

static float angleInCircle(double abx, double aby ) {
    return  -atan2(( - aby * 2.0f), (abx * 2.0f ));
}

class SpatComponent :   public Component
{
public:
    SpatComponent(SpatGrisAudioProcessor * filt, GrisLookAndFeel *feel);
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
    
    SpatGrisAudioProcessor * filter;
    GrisLookAndFeel *grisFeel;
    ImageComponent logoImg;
    Label       labVersion;
    
    SelectItem selectItem;
};
#endif  // SPATCOMPONENT_H_INCLUDED
