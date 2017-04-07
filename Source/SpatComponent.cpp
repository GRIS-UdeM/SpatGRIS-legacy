/*
 ==============================================================================
 SpatGRIS: multichannel sound spatialization plug-in.
 
 Copyright (C) 2015  GRIS-UdeM
 
 FieldComponent.cpp
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


#include "SpatComponent.h"
#include "PluginProcessor.h"

SpatComponent::SpatComponent(SpatGrisAudioProcessor * filt,GrisLookAndFeel *feel):
filter(filt),
grisFeel(feel)
{
    //Gris Logo
    this->logoImg.setImage(ImageFileFormat::loadFrom (BinaryData::logoGris_png, (size_t) BinaryData::logoGris_pngSize));
    this->addAndMakeVisible(&this->logoImg);
    
    //Version label
    String version = STRING(JUCE_APP_VERSION);
#ifdef JUCE_DEBUG
    version += " ";
    version += STRING(__TIME__);
#endif
    version = "SpatGRIS "+version;
    this->labVersion.setText(version, NotificationType::dontSendNotification);
    this->labVersion.setJustificationType(Justification(Justification::right));
    this->labVersion.setFont(this->grisFeel->getFont());
    this->labVersion.setLookAndFeel(this->grisFeel);
    this->labVersion.setColour(Label::textColourId, this->grisFeel->getFontColour());
    this->addAndMakeVisible(this->labVersion);
}

SpatComponent::~SpatComponent()
{
}

//======================================================================================================================
void SpatComponent::paint(Graphics &g)
{
    const int fieldWH = getWidth(); //Same getHeight
    float w,x;
    
    g.fillAll(this->grisFeel->getBackgroundColour());
    
    // - - - - - - - - - - - -
    // draw grid
    // - - - - - - - - - - - -
    g.setColour(this->grisFeel->getLightColour().withBrightness(0.5));
    w = (fieldWH - SourceDiameter) / 1.3f;
    x = (fieldWH - w) / 2.0f;
    g.drawEllipse(x, x, w, w, 1);
    w = (fieldWH - SourceDiameter) / 4.0f;
    x = (fieldWH - w) / 2.0f;
    g.drawEllipse(x, x, w, w, 1);
    
    w = (fieldWH - SourceDiameter);
    x = (fieldWH - w) / 2.0f;
    float r = (w/2)*0.296f;
    //4 lines
    g.drawLine(x+r, x+r, (w+x)-r, (w+x)-r);
    g.drawLine(x+r, (w+x)-r, (w+x)-r , x+r);
    g.drawLine(x, fieldWH/2, w+x , fieldWH/2);
    g.drawLine(fieldWH/2, x, fieldWH/2 , w+x);

    
    // - - - - - - - - - - - -
    // draw big background circles
    // - - - - - - - - - - - -
    g.setColour(this->grisFeel->getLightColour());
    g.drawEllipse(x, x, w, w, 1);
    
    // - - - - - - - - - - - -
    // draw little background circles
    // - - - - - - - - - - - -
    w = (fieldWH - SourceDiameter) / 2.0f;
    x = (fieldWH - w) / 2.0f;
    g.drawEllipse(x, x, w, w, 1);
    
    
    // - - - - - - - - - - - -
    // draw Speaker
    // - - - - - - - - - - - -
    for(unsigned int i = 0; i < this->filter->getNumSpeakerUsed(); ++i){
        
    }
    
    // - - - - - - - - - - - -
    // draw Source
    // - - - - - - - - - - - -
    String stringVal;
    w = (fieldWH - SourceDiameter);
    float hueSele;
    
    for(int i = 0; i < this->filter->getNumSourceUsed(); ++i){
       
        Point<float> sourceP = Point<float>(this->filter->getPosXSource(i), this->filter->getPosYSource(i));
        sourceP.x = (w/2.0f) + ((w/4.0f)*sourceP.x);
        sourceP.y = (w/2.0f) - ((w/4.0f)*sourceP.y);

        g.setColour(this->getColor(i));
        g.fillEllipse(sourceP.x , sourceP.y , SourceDiameter, SourceDiameter);
        
        stringVal.clear();
        
        stringVal << i+1;
        
        g.setColour(Colours::black);
        g.setFont(this->grisFeel->getFont());
        g.drawText(stringVal, sourceP.x+1 , sourceP.y + 1, SourceDiameter, SourceDiameter, Justification(Justification::centred), false);
        
        g.setColour(Colours::white);
        g.drawText(stringVal, sourceP.x , sourceP.y, SourceDiameter, SourceDiameter, Justification(Justification::centred), false);

    }
    
    
}

void SpatComponent::resized(int fieldSize)
{
    int iLabelX = 2*(float)fieldSize/3;
    
    this->logoImg.setBounds(8, 8, (float)fieldSize/7, (float)fieldSize/7);
    this->labVersion.setBounds(iLabelX,5,fieldSize-iLabelX,25);
}

//======================================================================================================================
void SpatComponent::mouseDown(const MouseEvent &event)
{
    const int fieldWH = getWidth();
    const Point<int> mouseP(event.x, event.y);
    
    const float w = (fieldWH - SourceDiameter) /2.0f;
    
    this->selectItem.selectID = -1;
    this->selectItem.selecType = NoSelection;
    
    for(int i = 0; i < this->filter->getNumSourceUsed(); ++i){
        Point<float> sourceP = Point<float>(this->filter->getPosXSource(i), this->filter->getPosYSource(i));
        sourceP.x = ((w) + ((w/2.0f)*sourceP.x))+SourceRadius;
        sourceP.y = ((w) - ((w/2.0f)*sourceP.y))+SourceRadius;
        float dx = mouseP.x - sourceP.x;
        float dy = mouseP.y - sourceP.y;
        float distanceSquared = dx*dx + dy*dy;
        if(distanceSquared < SourceRadius*SourceRadius){
            
            this->selectItem.selectID = i;
            this->selectItem.selecType = SelectedSource;
 
        }
    }
}

void SpatComponent::mouseDrag(const MouseEvent &event)
{
    const int fieldWH = getWidth();
    const Point<int> mouseP(event.x, event.y);
    
    const float w = (fieldWH - SourceDiameter)/2.0f;
    const float x = (fieldWH - w) / 2.0f;

    
    switch(this->selectItem.selecType)
    {
        case SelectedSource:

            float dx =  (x+(w/2.0f)) - mouseP.x;
            float dy =  (x+(w/2.0f)) - mouseP.y;
            float dist = sqrt(dx*dx + dy*dy);
            
            float vx =  (mouseP.x - SourceRadius - w) / (w/2.0f);//-2 and 2
            float vy = -(mouseP.y - SourceRadius - w) / (w/2.0f);
            float ang = angleInCircle(vx,vy);
            
            dist = dist/(w/2.0f);
            if(dist > 2.0f){ dist = 2.0f; }

            this->filter->setPosXYSource(this->selectItem.selectID, dist*cosf(ang), dist* sinf(ang));    //(-2, 2)

            break;
    }
    this->repaint();
}

void SpatComponent::mouseUp(const MouseEvent &event)
{
    
}

//=============================================================

Colour SpatComponent::getColor(int i) {
    float hueSelect = (float)i / this->filter->getNumSourceUsed() + 0.577251;
    if (hueSelect > 1){
        hueSelect -= 1;
    }
    return Colour::fromHSV(hueSelect, 0.85f, 0.85f, 1);
}

