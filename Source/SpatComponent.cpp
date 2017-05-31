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
    String stringVal;
    
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
    if(this->filter->getTypeProcess() == OSCZirkonium || this->filter->getTypeProcess() == OSCSpatServer){
        g.setColour(this->grisFeel->getLightColour().withBrightness(0.5));
    }
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
    // draw angles value
    // - - - - - - - - - - - -
    g.setColour(this->grisFeel->getFontColour().withBrightness(0.7));
    g.setFont(this->grisFeel->getFont());
    
    g.drawText("0", fieldWH-30 , fieldCenter, SourceDiameter, SourceDiameter, Justification(Justification::centred), false);
    g.drawText("90", fieldCenter , 10, SourceDiameter, SourceDiameter, Justification(Justification::centred), false);
    g.drawText("180", 14 , fieldCenter, SourceDiameter, SourceDiameter, Justification(Justification::centred), false);
    g.drawText("270", fieldCenter+2 , fieldWH-28, SourceDiameter, SourceDiameter, Justification(Justification::centred), false);

    
    // - - - - - - - - - - - -
    // draw Speaker
    // - - - - - - - - - - - -
    w = (fieldWH - SourceDiameter);
    if(this->filter->getTypeProcess() == FreeVolum || this->filter->getTypeProcess() == PanSpan){
        for(int i = 0; i < this->filter->getNumSpeakerUsed(); ++i){
            FPoint sourceP = this->filter->getListSpeaker()[i]->getPosXY();
            sourceP.x = (w/2.0f) + ((w/4.0f)*sourceP.x);
            sourceP.y = (w/2.0f) - ((w/4.0f)*sourceP.y);
            
            g.setColour(Colour::fromHSV(1.2,0,0.5,1));
            g.fillEllipse(sourceP.x , sourceP.y , SourceDiameter, SourceDiameter);
            
            g.setColour(Colours::white);
            g.drawText(String(i+1), sourceP.x, sourceP.y, SourceDiameter, SourceDiameter, Justification(Justification::centred), false);
        }
    }
    
    
    
    // - - - - - - - - - - - -
    // draw translucid circles (mode)
    // - - - - - - - - - - - -
    for (int i = 0; i < filter->getNumSourceUsed(); ++i) {
        switch (this->filter->getTypeProcess()){
                
            case FreeVolum :
                drawCircleSource(g, i, fieldWH, fieldCenter);
                break;
                
            case PanSpan :
                drawSpanSource(g, i, fieldWH, fieldCenter);
                break;
                
            default:
                drawAzimElevSource(g, i, fieldWH, fieldCenter);
                break;
        }
        
    }
    
    // - - - - - - - - - - - -
    // draw Source
    // - - - - - - - - - - - -
    for(int i = 0; i < this->filter->getNumSourceUsed(); ++i){
        
        FPoint sourceP = FPoint(*(this->filter->getListSource()[i]->getX()), *(this->filter->getListSource()[i]->getY()));
        sourceP.x = (w/2.0f) + ((w/4.0f)*sourceP.x);
        sourceP.y = (w/2.0f) - ((w/4.0f)*sourceP.y);
        
        stringVal.clear();
        stringVal << (int)(this->filter->getListSource()[i]->getId() + (this->filter->getOscFirstIdSource()-1));
        
        
        // - - - - - - - - - - - -
        // draw Select Source
        // - - - - - - - - - - - -
        if(this->filter->getSelectItem()->selectID == i && this->filter->getSelectItem()->selecType == SelectedSource){
            
            g.setColour(this->getColor(i));
            g.drawEllipse(fieldWH-40, 39 , SourceDiameter, SourceDiameter,2);
            float Radius = SourceDiameter/2;
            g.drawLine   (fieldCenter, fieldCenter,  sourceP.x+Radius , sourceP.y+Radius , 1);
            
            g.setColour(this->grisFeel->getLightColour());
            g.drawEllipse(sourceP.x-2 , sourceP.y-2 , SourceDiameter+4, SourceDiameter+4, 1);
            

            g.setColour(Colours::white);
            g.drawText(stringVal, fieldWH-40, 40, SourceDiameter, SourceDiameter, Justification(Justification::centred), false);
        }
        
        g.setColour(this->getColor(i));
        g.fillEllipse(sourceP.x , sourceP.y , SourceDiameter, SourceDiameter);

        g.setColour(Colours::black);
        g.drawText(stringVal, sourceP.x+1 , sourceP.y+1, SourceDiameter, SourceDiameter, Justification(Justification::centred), false);
        g.setColour(Colours::white);
        g.drawText(stringVal, sourceP.x, sourceP.y, SourceDiameter, SourceDiameter, Justification(Justification::centred), false);
        
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
    this->filter->getSelectItem()->mouseOver = false;
    
    if(this->filter->getTypeProcess() == FreeVolum || this->filter->getTypeProcess() == PanSpan){
        //Speakers
        for(int i = 0; i < this->filter->getNumSpeakerUsed(); ++i){
            
            FPoint sourceP = this->filter->getListSpeaker()[i]->getPosXY();
            NormalizeXYSourceWithScreen(sourceP, w);
            
            float dx = mouseP.x - sourceP.x;
            float dy = mouseP.y - sourceP.y;
            float distanceSquared = dx*dx + dy*dy;
            if(distanceSquared < SourceRadius*SourceRadius){
                
                this->clickedMouseP = mouseP;
                NormalizeScreenWithSpat(this->clickedMouseP,w);
                
                this->filter->getSelectItem()->selectID = i;
                this->filter->getSelectItem()->selecType = SelectedSpeaker;
                this->filter->getSelectItem()->mouseOver = true;
            }
        }
    }
    
    //Sources
    for(int i = 0; i < this->filter->getNumSourceUsed(); ++i){

        FPoint sourceP = FPoint(*(this->filter->getListSource()[i]->getX()), *(this->filter->getListSource()[i]->getY()));
        NormalizeXYSourceWithScreen(sourceP, w);
        
        float dx = mouseP.x - sourceP.x;
        float dy = mouseP.y - sourceP.y;
        float distanceSquared = dx*dx + dy*dy;
        if(distanceSquared < SourceRadius*SourceRadius){
            
            this->clickedMouseP = mouseP;
            NormalizeScreenWithSpat(this->clickedMouseP,w);
            
            this->filter->getSelectItem()->selectID = i;
            this->filter->getSelectItem()->selecType = SelectedSource;
            this->filter->getSelectItem()->mouseOver = true;
            
            this->filter->getSourceMover()->beginMouvement();
        }
    }
    
    this->editor->updateSourceParam();
}

void SpatComponent::mouseDrag(const MouseEvent &event)
{
    if(!this->filter->getSelectItem()->mouseOver){ return; }
    const int fieldWH = getWidth();
    FPoint mouseP(event.x, event.y);
    
    const float w = (fieldWH - SourceDiameter)/2.0f;
    const float x = (fieldWH - w) / 2.0f;

    
    switch(this->filter->getSelectItem()->selecType)
    {
        
        case SelectedSource:{
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
        }
            
        case SelectedSpeaker:{
            float dx =  (x+(w/2.0f)) - mouseP.x;
            float dy =  (x+(w/2.0f)) - mouseP.y;
            float dist = sqrt(dx*dx + dy*dy);
            
            NormalizeScreenWithSpat(mouseP, w);
            float ang = AngleInCircle(mouseP);
            if(this->filter->getTypeProcess() == PanSpan){
                dist = 1.0f;
            }else{
                dist = dist/(w/2.0f);
                if(dist > 2.0f){ dist = 2.0f; }
            }
            
            this->filter->getListSpeaker()[this->filter->getSelectItem()->selectID]->setPosXY(FPoint (dist*cosf(ang), dist* sinf(ang)));
            this->editor->updateSelectSpeaker();
            break;
        }
        
        case NoSelection:
            break;
    }
    this->repaint();
}

void SpatComponent::mouseUp(const MouseEvent &event)
{

}

//=============================================================

void SpatComponent::drawCircleSource(Graphics &g, const int i, const int fieldWH, const int fieldCenter)
{
    const float w = (fieldWH - SourceDiameter);
    const float surface =  (*this->filter->getListSource()[i]->getHeigt());
    const float radius = (surface / (RadiusMax*2)) * (fieldWH - SourceDiameter);
    const float diameter = radius * 2;
    
    FPoint sourceP = FPoint(*(this->filter->getListSource()[i]->getX()), *(this->filter->getListSource()[i]->getY()));
    sourceP.x = (w/2.0f) + ((w/4.0f)*sourceP.x);
    sourceP.y = (w/2.0f) - ((w/4.0f)*sourceP.y);
    
    g.setColour(this->getColor(i).withAlpha(0.2f));
    g.fillEllipse(sourceP.x-radius , sourceP.y-radius , SourceDiameter+diameter, SourceDiameter+diameter);
    //g.setColour(this->getColor(i).withAlpha(0.5f));
    //g.drawEllipse(sourceP.x-radius , sourceP.y-radius , SourceDiameter+diameter, SourceDiameter+diameter, 1);
}

void SpatComponent::drawSpanSource(Graphics &g, const int i, const int fieldWH, const int fieldCenter)
{
    FPoint sourceP = FPoint(*(this->filter->getListSource()[i]->getX()), *(this->filter->getListSource()[i]->getY()));
    FPoint rt = GetPositionRT(sourceP);
    float angle = ((*(this->filter->getListSource()[i]->getAzim()))/2.0f) * M_PI;
    
    float fs = fieldWH - SourceDiameter;
    float x = fs*0.25f + SourceRadius;
    float y = fs*0.25f + SourceRadius;
    float w = fs*0.5f;
    float h = fs*0.5f;
    float r1 = 0.5f*M_PI-(rt.y + angle);
    float r2 = 0.5f*M_PI-(rt.y - angle );
    float ir = (rt.x >= .999f) ? 2.f : 0.f;
    
    if (rt.x >= .999f) {
        Path p;
        p.addPieSegment(x, y, w, h, r1, r2, ir);
        
        g.setColour(this->getColor(i).withAlpha(0.2f));
        g.fillPath(p);
    }
    else {
        float front = rt.x * 0.5f + 0.5f;
        float back = 1 - front;

        Path pf;
        Path pb;
        pf.addPieSegment(x, y, w, h, r1, r2, ir);
        pb.addPieSegment(x, y, w, h, r1 + M_PI, r2 + M_PI, ir);
        
        g.setColour(this->getColor(i).withAlpha(0.2f*front));
        g.fillPath(pf);

        g.setColour(this->getColor(i).withAlpha(0.2f*back));
        g.fillPath(pb);
    }
}


void SpatComponent::drawAzimElevSource(Graphics &g, int i, const int fieldWH, const int fieldCenter)
{
    //g.setColour(this->getColor(i));
    
    FPoint sourceP = FPoint(*(this->filter->getListSource()[i]->getX()), *(this->filter->getListSource()[i]->getY()));
    FPoint azimElev = GetSourceAzimElev(sourceP, true);
    
    float HRAzimSpan = 180.0f *(*this->filter->getListSource()[i]->getAzim());  //in zirkosc, this is [0,360]
    float HRElevSpan = 180.0f *(*this->filter->getListSource()[i]->getElev());  //in zirkosc, this is [0,90]
    
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
    
    if( (HRElevSpan==0 && HRAzimSpan!=0) || (HRAzimSpan==0 && HRElevSpan!=0) ){
        g.setColour(this->getColor(i).withAlpha(0.5f));
        g.strokePath(myPath, PathStrokeType(0.5));
    }
}

Colour SpatComponent::getColor(int i) {
    float hueSelect = (float)i / this->filter->getNumSourceUsed() + +0.577251;
    if (hueSelect > 1){
        hueSelect -= 1;
    }
    return Colour::fromHSV(hueSelect, 0.85f, 0.85f, 1);
}

