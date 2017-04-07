//
//  Source.cpp
//  SpatGRIS
//
//  Created by GRIS on 2017-04-07.
//
//

#include "Source.h"
#include "PluginProcessor.h"


Source::Source(SpatGrisAudioProcessor * filt, int idS):
filter(filt),id(idS)
{
    NormalisableRange<float> nrf = NormalisableRange<float> (-2.0f, 2.0f);
    nrf.interval = 0.00001f;
    
    String ss = "Source "+String(id)+ " - ";
    this->audPX = new AudioParameterFloat(ss+"X", ss+"X", nrf, 0.0f);
    filt->addParameter(this->audPX);
    
    this->audPY = new AudioParameterFloat(ss+"Y", ss+"Y", nrf, 1.0f);
    filt->addParameter(this->audPY);

    NormalisableRange<float> nrfSurf = NormalisableRange<float> (0.0f, 1.0f);
    nrfSurf.interval = 0.00001f;
    this->audPSurf = new AudioParameterFloat(ss+"S", ss+"S", nrfSurf, 0.5f);
    filt->addParameter(this->audPSurf);
    
    
    NormalisableRange<float> nrfAZ = NormalisableRange<float> (0.0f, 2.0f);
    nrfAZ.interval = 0.00001f;
    this->audPAzim = new AudioParameterFloat(ss+"A", ss+"A", nrfAZ, 0.0f);
    filt->addParameter(this->audPAzim);
    
    NormalisableRange<float> nrfEl= NormalisableRange<float> (0.0f, 0.5f);
    nrfEl.interval = 0.00001f;
    this->audPElev = new AudioParameterFloat(ss+"E", ss+"E", nrfEl, 0.0f);
    filt->addParameter(this->audPElev);
}
