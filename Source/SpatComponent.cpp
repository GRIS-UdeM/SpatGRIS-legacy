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
#include "PluginEditor.h"

SpatComponent::SpatComponent(SpatGrisAudioProcessorEditor * edit, SpatGrisAudioProcessor * filt,GrisLookAndFeel *feel):
editor(edit),
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
    const int fieldWH = getWidth();     //Same getHeight
    const int fieldCenter = fieldWH/2;
    float w,x;

    
    g.fillAll(this->grisFeel->getBackgroundColour());
    
    // - - - - - - - - - - - -
    // draw line and light circle
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
    // draw big background circle
    // - - - - - - - - - - - -
    g.setColour(this->grisFeel->getLightColour());
    g.drawEllipse(x, x, w, w, 1);
    
    // - - - - - - - - - - - -
    // draw little background circle
    // - - - - - - - - - - - -
    w = (fieldWH - SourceDiameter) / 2.0f;
    x = (fieldWH - w) / 2.0f;
    g.drawEllipse(x, x, w, w, 1);
    
    // - - - - - - - - - - - -
    // draw fill center cirlce
    // - - - - - - - - - - - -
    g.setColour(this->grisFeel->getBackgroundColour());
    w = (fieldWH - SourceDiameter) / 4.0f;
    w -= 2;
    x = (fieldWH - w) / 2.0f;
    g.fillEllipse(x, x, w, w);
    
    
    // - - - - - - - - - - - -
    // draw Speaker
    // - - - - - - - - - - - -
    for(unsigned int i = 0; i < this->filter->getNumSpeakerUsed(); ++i){
        
    }
    
    
    
    // - - - - - - - - - - - -
    // draw translucid circles (mode)
    // - - - - - - - - - - - -
    for (int i = 0; i < filter->getNumSourceUsed(); ++i) {
        drawAzimElevSource(g, i, fieldWH, fieldCenter);
    }
    
    // - - - - - - - - - - - -
    // draw Source
    // - - - - - - - - - - - -
    String stringVal;
    w = (fieldWH - SourceDiameter);
    
    for(int i = 0; i < this->filter->getNumSourceUsed(); ++i){
        
        FPoint sourceP = FPoint(*(this->filter->getListSource().at(i)->getX()), *(this->filter->getListSource().at(i)->getY()));
        sourceP.x = (w/2.0f) + ((w/4.0f)*sourceP.x);
        sourceP.y = (w/2.0f) - ((w/4.0f)*sourceP.y);
        
        // - - - - - - - - - - - -
        // draw Select Source
        // - - - - - - - - - - - -
        if(this->filter->getSelectItem()->selectID == i && this->filter->getSelectItem()->selecType == SelectedSource){
            
            g.setColour(this->getColor(i));
            float Radius = SourceDiameter/2;
            g.drawLine   (fieldCenter, fieldCenter,  sourceP.x+Radius , sourceP.y+Radius , 1);
            
            g.setColour(this->grisFeel->getLightColour());
            g.drawEllipse(sourceP.x-2 , sourceP.y-2 , SourceDiameter+4, SourceDiameter+4, 1);
        }
        
        g.setColour(this->getColor(i));
        g.fillEllipse(sourceP.x , sourceP.y , SourceDiameter, SourceDiameter);
        
        stringVal.clear();
        stringVal << i+1;
        
        g.setColour(Colours::black);
        g.setFont(this->grisFeel->getFont());
        
        int tx = sourceP.x;
        int ty = sourceP.y;
        g.drawText(stringVal, tx+1 , ty+1, SourceDiameter, SourceDiameter, Justification(Justification::centred), false);
        g.setColour(Colours::white);
        g.drawText(stringVal, tx, ty, SourceDiameter, SourceDiameter, Justification(Justification::centred), false);
        
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
    FPoint mouseP(event.x, event.y);
    
    const float w = (fieldWH - SourceDiameter) /2.0f;
    
    for(int i = 0; i < this->filter->getNumSourceUsed(); ++i){

        FPoint sourceP = FPoint(*(this->filter->getListSource().at(i)->getX()), *(this->filter->getListSource().at(i)->getY()));
        NormalizeXYSourceWithScreen(sourceP, w);
        
        float dx = mouseP.x - sourceP.x;
        float dy = mouseP.y - sourceP.y;
        float distanceSquared = dx*dx + dy*dy;
        if(distanceSquared < SourceRadius*SourceRadius){
            
            this->clickedMouseP = mouseP;
            NormalizeScreenWithSpat(this->clickedMouseP,w);
            
            this->filter->getSelectItem()->selectID = i;
            this->filter->getSelectItem()->selecType = SelectedSource;
            
            this->filter->getSourceMover()->beginMouvement();
        }
    }
    
    this->editor->updateSourceParam();
}

void SpatComponent::mouseDrag(const MouseEvent &event)
{
    const int fieldWH = getWidth();
    FPoint mouseP(event.x, event.y);
    
    const float w = (fieldWH - SourceDiameter)/2.0f;
    const float x = (fieldWH - w) / 2.0f;

    
    switch(this->filter->getSelectItem()->selecType)
    {
        
        case SelectedSource:

            float dx =  (x+(w/2.0f)) - mouseP.x;
            float dy =  (x+(w/2.0f)) - mouseP.y;
            float dist = sqrt(dx*dx + dy*dy);
            
            NormalizeScreenWithSpat(mouseP, w);
            float ang = AngleInCircle(mouseP);
            dist = dist/(w/2.0f);
            if(dist > 2.0f){ dist = 2.0f; }

            this->filter->setPosXYSource(this->filter->getSelectItem()->selectID, dist*cosf(ang), dist* sinf(ang));    //(-2, 2)
            this->editor->updateSelectSource();
            break;
            
        /*case SelectedSpeaker:
            
            break;
        
        case NoSelection:
            break;*/
    }
    this->repaint();
}

void SpatComponent::mouseUp(const MouseEvent &event)
{

}

//=============================================================

void SpatComponent::drawAzimElevSource(Graphics &g, int i, const int fieldWH, const int fieldCenter){
    g.setColour(this->getColor(i));
    
    FPoint sourceP = FPoint(*(this->filter->getListSource().at(i)->getX()), *(this->filter->getListSource().at(i)->getY()));
    FPoint azimElev = GetSourceAzimElev(sourceP, true);
    
    float HRAzimSpan = 180.0f *(*this->filter->getListSource().at(i)->getAzim());  //in zirkosc, this is [0,360]
    float HRElevSpan = 180.0f *(*this->filter->getListSource().at(i)->getElev());  //in zirkosc, this is [0,90]
    
    float HRAzim = azimElev.x * 180.0f;    //in zirkosc [-180,180]
    float HRElev = azimElev.y * 180.0f;    //in zirkosc [0,89.9999]
    
    
    //calculate max and min elevation in degrees
    FPoint maxElev = {HRAzim, HRElev+HRElevSpan/2.0f};
    FPoint minElev = {HRAzim, HRElev-HRElevSpan/2.0f};
    
    if(minElev.y < 0){
        maxElev.y = (maxElev.y - minElev.y);
        minElev.y = 0.0f;
    }
    
    //convert max min elev to xy
    FPoint screenMaxElev = DegreeToXy(maxElev, fieldWH);
    FPoint screenMinElev = DegreeToXy(minElev, fieldWH);
    
    //form minmax elev, calculate minmax radius
    float maxRadius = sqrtf(screenMaxElev.x * screenMaxElev.x + screenMaxElev.y *screenMaxElev.y);
    float minRadius = sqrtf(screenMinElev.x * screenMinElev.x + screenMinElev.y *screenMinElev.y);
    
    //drawing the path for spanning
    Path myPath;
    myPath.startNewSubPath(fieldCenter+screenMaxElev.x, fieldCenter+screenMaxElev.y);
    //half first arc center
    myPath.addCentredArc(fieldCenter, fieldCenter, minRadius, minRadius, 0.0, DegreeToRadian(-HRAzim),             DegreeToRadian(-HRAzim + HRAzimSpan/2 ));
    
    if (maxElev.getY() > 90.f) { // if we are over the top of the dome we draw the adjacent angle
        myPath.addCentredArc(fieldCenter, fieldCenter, maxRadius, maxRadius, 0.0,   M_PI+DegreeToRadian(-HRAzim + HRAzimSpan/2),  M_PI+DegreeToRadian(-HRAzim - HRAzimSpan/2));
    }else {
        myPath.addCentredArc(fieldCenter, fieldCenter, maxRadius, maxRadius, 0.0, DegreeToRadian(-HRAzim + HRAzimSpan/2), DegreeToRadian(-HRAzim - HRAzimSpan/2));
    }
    myPath.addCentredArc(fieldCenter, fieldCenter, minRadius, minRadius, 0.0, DegreeToRadian(-HRAzim - HRAzimSpan/2), DegreeToRadian(-HRAzim));
    
    myPath.closeSubPath();
    
    g.setColour(this->getColor(i).withAlpha(0.2f));
    g.fillPath(myPath);
    
    g.setColour(this->getColor(i).withAlpha(0.6f));
    g.strokePath(myPath, PathStrokeType(0.5));
}

Colour SpatComponent::getColor(int i) {
    float hueSelect = (float)i / this->filter->getNumSourceUsed() + +0.577251;
    if (hueSelect > 1){
        hueSelect -= 1;
    }
    return Colour::fromHSV(hueSelect, 0.85f, 0.85f, 1);
}

