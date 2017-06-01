/*
 ==============================================================================
 SpatGRIS: multichannel sound spatialization plug-in.
 
 Copyright (C) 2015  GRIS-UdeM
 
 ParamSliderGris.h
 Created: 2016-03-20
 
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



#ifndef SliderGris_h
#define SliderGris_h

#include "../JuceLibraryCode/JuceHeader.h"
#include "../../GrisCommonFiles/GrisLookAndFeel.h"

#include "DefaultParam.h"


class SpatGrisAudioProcessorEditor;
//================================================== ParamSliderGRIS ======================================================
class SliderGRIS : public Slider
{
    
public:
    SliderGRIS(float defaultV = 0.0f);

    void mouseDown (const MouseEvent &e);
    
private:

    float defaultValue;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SliderGRIS)
};


#endif /* SliderGRIS */
