//
//  Speaker.cpp
//  SpatGRIS
//
//  Created by GRIS on 2017-04-06.
//
//

#include "Speaker.h"
#include "PluginProcessor.h"


Speaker::Speaker(SpatGrisAudioProcessor * filt, unsigned int idS):
filter(filt), id(idS)
{
    this->px = 0.f;
    this->py = 0.f;
    this->muted = false;
}
