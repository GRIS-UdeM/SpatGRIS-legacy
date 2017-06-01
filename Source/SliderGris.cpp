//
//  SliderGris.cpp
//  SpatGRIS
//
//  Created by GRIS on 2017-06-01.
//
//

#include "SliderGris.h"
#include "PluginEditor.h"


SliderGRIS::SliderGRIS( float defaultV)
{
    this->defaultValue = defaultV;
}

void SliderGRIS::mouseDown (const MouseEvent &e)
{
    if(e.mods.isAltDown()){
        Slider::setValue(this->defaultValue);
    }else{
        Slider::mouseDown(e);
    }
}
