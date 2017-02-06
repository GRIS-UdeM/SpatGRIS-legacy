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

#include "../JuceLibraryCode/JuceHeader.h"
#include "FieldComponent.h"
#include "SourceMover.h"

//==============================================================================
FieldComponent::FieldComponent(SpatGrisAudioProcessor* filter, SourceMover *mover)
: mFilter(filter)
, m_pMover(mover)
, m_iCurPathLines(0)
, m_iMaxPathLines(10)
, bIsDestructed(false)
, m_bJustSelectedEndPoint(false)
{
    m_pMover->setFieldComponent(this);
}

FieldComponent::~FieldComponent()
{
    bIsDestructed = true;
}

void FieldComponent::clearTrajectoryPath(){
    m_dqAllPathPoints.clear();
}

void FieldComponent::updatePositionTrace(float p_fX, float p_fY){
    if(this->getParentComponent() != nullptr && isShowing()){
        float fAbsoluteX = p_fX * getWidth();
        float fAbsoluteY = (1-p_fY) * getHeight();
        m_dqAllPathPoints.push_back(FPoint(fAbsoluteX, fAbsoluteY));
    }
}

FPoint FieldComponent::getSourcePoint(int i)
{
	const int fieldWidth = getWidth();
	FPoint p = mFilter->getSourceXY01(i);
	return FPoint((p.x * (fieldWidth - kSourceDiameter) + kSourceRadius), fieldWidth - (p.y * (fieldWidth - kSourceDiameter) + kSourceRadius));
}
//this is NOT a duplicate of one of the convert functions in processor.h
FPoint FieldComponent::convertSourceRT(float r, float t)
{
	const int fieldWidth = getWidth();
	FPoint p(r * cosf(t), r * sinf(t));
	return FPoint((((p.x + kRadiusMax) / (kRadiusMax*2)) * (fieldWidth - kSourceDiameter) + kSourceRadius), fieldWidth - (((p.y + kRadiusMax) / (kRadiusMax*2)) * (fieldWidth - kSourceDiameter) + kSourceRadius));
}

FPoint FieldComponent::getSpeakerPoint(int i)
{
	const int fieldWidth = getWidth();
	FPoint p = mFilter->getSpeakerXY(i);
    return FPoint(((p.x + kRadiusMax) / (kRadiusMax*2)) * (fieldWidth - kSpeakerDiameter) + kSpeakerRadius, fieldWidth - (((p.y + kRadiusMax) / (kRadiusMax*2)) * (fieldWidth - kSpeakerDiameter) + kSpeakerRadius));
}

float FieldComponent::getDistance(int source, int speaker)
{
	return mFilter->getSourceXY(source).getDistanceFrom(mFilter->getSpeakerXY(speaker));
}

void FieldComponent::paint (Graphics& g)
{
    String stringVal;
	const int fieldWidth = getWidth();
	const int fieldHeight = getHeight();
	const int processMode = mFilter->getProcessMode();
    float hue = 0.0;
    float hueSelect = 0.0;
    float w, x;
    float radius, diameter;
    float fFieldCenter = fieldWidth/2;
    FPoint pCurLoc;
	
    g.setColour(mGrisFeel.getFieldColour());
	g.fillRect(0, 0, fieldWidth, fieldHeight);
	
    // - - - - - - - - - - - -
	// draw big background circles
	// - - - - - - - - - - - -
	g.setColour(mGrisFeel.getLightColour());
    int iCurRadius = (processMode == kOscSpatMode) ? kRadiusMax : 1;
	for (; iCurRadius <= kRadiusMax; ++iCurRadius) {
		w = (iCurRadius / kRadiusMax) * (fieldWidth - kSourceDiameter);
		x = (fieldWidth - w) / 2;
		g.drawEllipse(x, x, w, w, 1);
	}
    
    // - - - - - - - - - - - -
    // draw small, center background circles
    // - - - - - - - - - - - -
	if (processMode != kFreeVolumeMode && processMode != kOscSpatMode) {
		w = (kThetaLockRampRadius / kRadiusMax) * (fieldWidth - kSourceDiameter);
		x = (fieldWidth - w) / 2;
		g.drawEllipse(x, x, w, w, 1);
		
	}
	// - - - - - - - - - - - -
	// draw the grid
	// - - - - - - - - - - - -
	if (mFilter->getShowGridLines()) {
		g.setColour(Colour::fromFloatRGBA(0, 0, 0.3f, 1));
		const int gridCount = 8;
		for (int i = 1; i < gridCount; i++) {
			g.drawLine(fieldWidth * i / gridCount, 0, fieldHeight * i / gridCount, fieldHeight);
			g.drawLine(0, fieldHeight * i / gridCount, fieldWidth, fieldHeight * i / gridCount);
		}
	}
    
    // - - - - - - - - - - - -
    //draw cross if in kOscSpatMode
    // - - - - - - - - - - - -
    if(processMode == kOscSpatMode){
        g.setColour(Colours::white);
        g.drawLine(fFieldCenter, kSourceRadius, fFieldCenter, fieldHeight-kSourceRadius);
        g.drawLine(kSourceRadius, fieldHeight/2, fieldWidth-kSourceRadius, fieldHeight/2);
    }
    
	// - - - - - - - - - - - -
	// draw translucid circles
	// - - - - - - - - - - - -
    const float adj_factor = 1 / sqrtf(2);
    
    switch (processMode){
        case kFreeVolumeMode:
            for (int i = 0; i < mFilter->getNumberOfSources(); i++) {
                
                const float reachDist = 1 / (adj_factor * mFilter->getDenormedSourceD(i));
                
                radius = (reachDist / (kRadiusMax*2)) * (fieldWidth - kSourceDiameter);
                const float diameter = radius * 2;
                
                pCurLoc = getSourcePoint(i);
                
                hue = (float)i / mFilter->getNumberOfSources() + 0.577251;
                if (hue > 1){
                    hue -= 1;
                }
                
                g.setColour(Colour::fromHSV(hue, 1, 1, 0.1));
                g.fillEllipse(pCurLoc.x - radius, pCurLoc.y - radius, diameter, diameter);
                g.setColour(Colour::fromHSV(hue, 1, 1, 0.5));
                g.drawEllipse(pCurLoc.x - radius, pCurLoc.y - radius, diameter, diameter, 1);
            }
            break;
            
        case kPanSpanMode:
            for (int i = 0; i < mFilter->getNumberOfSources(); i++) {
                hue = (float)i / mFilter->getNumberOfSources() + 0.577251;
                if (hue > 1){
                    hue -= 1;
                }
                FPoint rt = mFilter->getSourceRT(i);
                float r = rt.x;
                float angle = mFilter->getSourceD(i) * M_PI;
                float t[2] = { rt.y + angle, rt.y - angle };
                
                float fs = fieldWidth - kSourceDiameter;
                float x = fs*0.25 + kSourceRadius;
                float y = fs*0.25 + kSourceRadius;
                float w = fs*0.5;
                float h = fs*0.5;
                float r1 = 0.5*M_PI-t[0];
                float r2 = 0.5*M_PI-t[1];
                float ir = (r >= .999) ? 2 : 0;
                
                if (r >= .999) {
                    g.setColour(Colour::fromHSV(hue, 1, 1, 0.4f));
                    Path p;
                    p.addPieSegment(x, y, w, h, r1, r2, ir);
                    g.fillPath(p);
                } else {
                    float front = r * 0.5f + 0.5f;
                    float back = 1 - front;
                    {
                        g.setColour(Colour::fromHSV(hue, 1, 1, 0.4f * front));
                        Path p;
                        p.addPieSegment(x, y, w, h, r1, r2, ir);
                        g.fillPath(p);
                    }
                    {
                        g.setColour(Colour::fromHSV(hue, 1, 1, 0.4f * back));
                        Path p;
                        p.addPieSegment(x, y, w, h, r1 + M_PI, r2 + M_PI, ir);
                        g.fillPath(p);
                    }
                }
            }
            break;
            
        case kOscSpatMode:
            for (int i = 0; i < mFilter->getNumberOfSources(); i++) {
                hue = (float)i / mFilter->getNumberOfSources() + 0.577251;
                if (hue > 1){
                    hue -= 1;
                }
                float HRAzimSpan = 360*mFilter->getSourceAzimSpan01(i);  //in zirkosc, this is [0,360]
                float HRElevSpan = 90 *mFilter->getSourceElevSpan01(i);  //in zirkosc, this is [0,90]
                
                //get current azim+elev in angles
                FPoint azimElev = mFilter->getSourceAzimElev(i, true);  //azim is [-1,1],  elevation is [0,.5]
                float HRAzim = azimElev.x * 180;   //in zirkosc [-180,180]
                float HRElev = azimElev.y * 180;   //in zirkosc [0,89.9999]
                
                //calculate max and min elevation in degrees
                Point<float> maxElev = {HRAzim, HRElev+HRElevSpan/2};
                Point<float> minElev = {HRAzim, HRElev-HRElevSpan/2};
                
                if(minElev.getY() < 0){
                    maxElev.setY(maxElev.getY() - minElev.getY());
                    minElev.setY(0);
                }
                
                //convert max min elev to xy
                Point<float> screenMaxElev = degreeToXy(maxElev, fieldWidth);
                Point<float> screenMinElev = degreeToXy(minElev, fieldWidth);
                
                //form minmax elev, calculate minmax radius
                float maxRadius = sqrtf(screenMaxElev.getX()*screenMaxElev.getX() + screenMaxElev.getY()*screenMaxElev.getY());
                float minRadius = sqrtf(screenMinElev.getX()*screenMinElev.getX() + screenMinElev.getY()*screenMinElev.getY());
                
                //drawing the path for spanning
                Path myPath;
                float x = screenMinElev.getX();
                float y = screenMinElev.getY();
                
                myPath.startNewSubPath(fFieldCenter+x,fFieldCenter+y);
                
                //half first arc center
                myPath.addCentredArc(    fFieldCenter, fFieldCenter, minRadius, minRadius, 0.0, degreeToRadian(-HRAzim),                      degreeToRadian(-HRAzim + HRAzimSpan/2 ));
                
                if (maxElev.getY() > 90.f) { // if we are over the top of the dome we draw the adjacent angle
                    myPath.addCentredArc(fFieldCenter, fFieldCenter, maxRadius, maxRadius, 0.0, M_PI+degreeToRadian(-HRAzim + HRAzimSpan/2),  M_PI+degreeToRadian(-HRAzim - HRAzimSpan/2));
                } else {
                    myPath.addCentredArc(fFieldCenter, fFieldCenter, maxRadius, maxRadius, 0.0, degreeToRadian(-HRAzim+HRAzimSpan/2),         degreeToRadian(-HRAzim-HRAzimSpan/2));
                }
                myPath.addCentredArc    (fFieldCenter, fFieldCenter, minRadius, minRadius, 0.0, degreeToRadian(-HRAzim-HRAzimSpan/2),         degreeToRadian(-HRAzim));
                myPath.closeSubPath();
                
                g.setColour(Colour::fromHSV(hue, 1, 1, 0.1));
                g.fillPath(myPath);
                g.setColour(Colour::fromHSV(hue, 1, 1, 0.5));
                PathStrokeType strokeType = PathStrokeType(2.5);
                g.strokePath(myPath, strokeType);
            }
            break;
    }
    
   
    
	// - - - - - - - - - - - -
	// draw speakers
	// - - - - - - - - - - - -
    if (processMode != kOscSpatMode){
        for (int i = 0; i < mFilter->getNumberOfSpeakers(); i++) {
            radius = kSpeakerRadius, diameter = kSpeakerDiameter;
            FPoint pCurLoc = getSpeakerPoint(i);
            g.setColour(Colour::fromHSV(2.f/3.f, 0, 0.5, 1));
            g.fillEllipse(pCurLoc.x - radius, pCurLoc.y - radius, diameter, diameter);
            
            g.setColour(Colours::white);
            g.drawEllipse(pCurLoc.x - radius, pCurLoc.y - radius, diameter, diameter, 1);
            
            stringVal.clear();
            stringVal << i+1;
            
            g.setColour(Colours::black);
            g.setFont(mGrisFeel.getFont());
            g.drawText(stringVal, pCurLoc.x - radius + 1, pCurLoc.y - radius + 1, diameter, diameter, Justification(Justification::centred), false);
            g.setColour(Colours::white);
            g.drawText(stringVal, pCurLoc.x - radius, pCurLoc.y - radius, diameter, diameter, Justification(Justification::centred), false);
        }
    }

    // - - - - - - - - - - - -
    //draw line and circle for selected source
    // - - - - - - - - - - - -
    int iSelectedSrc = mFilter->getSelectedSrc();
    hueSelect = (float)iSelectedSrc / mFilter->getNumberOfSources() + 0.577251;
    if (hueSelect > 1){
        hueSelect -= 1;
    }
    g.setColour(Colour::fromHSV(hueSelect, 1, 1, 0.8f));
    FPoint sourceXY = mFilter->getSourceXY(iSelectedSrc);

    float fRadius = (fieldWidth - kSourceDiameter)/4;
    g.drawLine   (fFieldCenter, fFieldCenter, fFieldCenter + sourceXY.x * fRadius, fFieldCenter - sourceXY.y * fRadius, 1.5);
    float radiusZenith = sqrtf(pow(2 * sourceXY.x * fRadius,2) + pow(2 * sourceXY.y * fRadius,2));
    g.drawEllipse(fFieldCenter - radiusZenith/2 , fFieldCenter - radiusZenith/2, radiusZenith, radiusZenith, 1.5);

    radius = kSourceRadius*1.22, diameter = kSourceDiameter*1.22;
    pCurLoc = getSourcePoint(iSelectedSrc);
    g.setColour(Colours::whitesmoke);
    g.drawEllipse(pCurLoc.x - radius, pCurLoc.y - radius, diameter, diameter, 2);

    
	// - - - - - - - - - - - -
	// draw sources
	// - - - - - - - - - - - -
	for (int i = 0; i < mFilter->getNumberOfSources(); i++) {
        radius = kSourceRadius, diameter = kSourceDiameter;
		pCurLoc = getSourcePoint(i);
		
		hue = (float)i / mFilter->getNumberOfSources() + 0.577251;
        if (hue > 1){
            hue -= 1;
        }
		
		g.setColour(Colour::fromHSV(hue, 1, 1, 0.5f));

        //draw the line that goes with every source
		if (processMode != kFreeVolumeMode && processMode != kOscSpatMode) {
			FPoint rt = mFilter->getSourceRT(i);
			//float r = rt.x;
			//float t = rt.y;
			//FPoint p1 = convertSourceRT(1, t);
			//FPoint p2 = convertSourceRT((r >= .999) ? 2 : -1, t);
            g.drawLine(Line<float>(convertSourceRT(1, rt.y), convertSourceRT((rt.x >= .999) ? 2 : -1, rt.y)));
		}
		
		g.setColour(Colour::fromHSV(hue, 1, 1, 1));
		g.fillEllipse(pCurLoc.x - radius, pCurLoc.y - radius, diameter, diameter);
		
		g.setColour(Colours::black);
		g.drawEllipse(pCurLoc.x - radius, pCurLoc.y - radius, diameter, diameter, 1);
		

        stringVal.clear();
        if (mFilter->getProcessMode() == kOscSpatMode){
            stringVal << mFilter->getOscSpat1stSrcId()+i;
        } else {
            stringVal << i+1;
        }
	
		g.setColour(Colours::black);
        g.setFont(mGrisFeel.getFont());
		g.drawText(stringVal, pCurLoc.x - radius + 1, pCurLoc.y - radius + 1, diameter, diameter, Justification(Justification::centred), false);
					
		g.setColour(Colours::white);
		g.drawText(stringVal, pCurLoc.x - radius, pCurLoc.y - radius, diameter, diameter, Justification(Justification::centred), false);
	}
    
    // TRAJECTORY PATH
    if (m_dqAllPathPoints.size() > 2){
        Path trajectoryPath;
        FPoint startPoint = m_dqAllPathPoints[0];
        trajectoryPath.startNewSubPath (startPoint.x, startPoint.y);
        for (int iCurPoint = 1; iCurPoint < m_dqAllPathPoints.size(); ++iCurPoint){
            trajectoryPath.lineTo (m_dqAllPathPoints[iCurPoint].x, m_dqAllPathPoints[iCurPoint].y);
        }
        g.setColour(Colour::fromHSV(hueSelect, 1, 1, 0.5f));
        g.strokePath (trajectoryPath, PathStrokeType (2.0f, PathStrokeType::JointStyle::curved));
    }
    
    while (m_dqAllPathPoints.size() > m_iMaxPathLines){
        m_dqAllPathPoints.pop_front();
    }
}

void FieldComponent::mouseDown(const MouseEvent &event)
{
    
    if (mFilter->getTrState() == kTrWriting) {
        return;
    }
    
	int fieldWidth = getWidth();
	int fieldHeight = getHeight();
	mSelectionType = kNoSelection;

    //check if point inside circle
	Point<int> ml(event.x, event.y);
	if (ml.x < 0 || ml.x >= fieldWidth || ml.y < 0 || ml.y >= fieldHeight) return;
    
    //if assigning end location
    if (mFilter->isSettingEndPoint()) {
        //get point of current event
        float fCenteredX01 = (float)event.x/fieldWidth;//-_ZirkOSC_Center_X;
        float fCenteredY01 = (float)event.y/fieldHeight;//-_ZirkOSC_Center_Y;
        mFilter->setEndLocationXY01(FPoint(fCenteredX01, fCenteredY01));
        mFilter->setIsSettingEndPoint(false);
        setJustSelectedEndPoint(true);
    }

	
	for (int i = mFilter->getNumberOfSources() - 1; i >= 0; i--){
		FPoint p = getSourcePoint(i);
		float dx = ml.x - p.x;
		float dy = ml.y - p.y;
		float distanceSquared = dx*dx + dy*dy;
		if (distanceSquared < kSourceRadius*kSourceRadius) {
			mSelectionType = kSelectedSource;
			mSelectedItem = i;

			if (event.mods.isShiftDown())
				mSavedValue = mFilter->getSourceRT(mSelectedItem).y;
			else if (event.mods.isAltDown())
				mSavedValue = mFilter->getSourceRT(mSelectedItem).x;
			mLastKeys = event.mods;
			
			m_pMover->begin(i, kField);
			return;
		}
	}
	
	for (int i = mFilter->getNumberOfSpeakers() - 1; i >= 0; i--){
		FPoint p = getSpeakerPoint(i);
		float dx = ml.x - p.x;
		float dy = ml.y - p.y;
		float distanceSquared = dx*dx + dy*dy;
		if (distanceSquared < kSpeakerRadius*kSpeakerRadius){
			mSelectionType = kSelectedSpeaker;
			mSelectedItem = i;
			return;
		}
	}
}

void FieldComponent::mouseDrag(const MouseEvent &event)
{
	Point<int> mouseLocation(event.x, event.y);
	
     //printf("x : %d  y : %d \n",ml.x,ml.y );
	int fieldWidth = getWidth();
	const float padSize = fieldWidth;
	
	switch(mSelectionType)
	{
		case kNoSelection:
			return;
			
		case kSelectedSource:
		{
			float vx = (mouseLocation.x - kSourceRadius) / (padSize - kSourceDiameter);
			float vy = 1 - (mouseLocation.y - kSourceRadius) / (padSize - kSourceDiameter);
			if (vx < 0) vx = 0; else if (vx > 1) vx = 1;
			if (vy < 0) vy = 0; else if (vy > 1) vy = 1;

			if (event.mods.isShiftDown())
			{
				if (!mLastKeys.isShiftDown())
					mSavedValue = mFilter->getSourceRT(mSelectedItem).y;

				FPoint point(vx - 0.5f, vy - 0.5f);
				FPoint line(cosf(mSavedValue), sinf(mSavedValue));
				
				float c = (line.x * point.x + line.y * point.y) / (line.x * line.x + line.y * line.y);
				vx = c * line.x;
				vy = c * line.y;
				
				vx += 0.5f; vy += 0.5f;
				if (vx < 0) vx = 0; else if (vx > 1) vx = 1;
				if (vy < 0) vy = 0; else if (vy > 1) vy = 1;
			}
			else if (event.mods.isAltDown())
			{
				if (!mLastKeys.isAltDown())
					mSavedValue = mFilter->getSourceRT(mSelectedItem).x;
			
				// force fixed radius
				FPoint p = mFilter->convertXy012Rt01(FPoint(vx, vy));
				p = mFilter->convertRt2Xy01(mSavedValue, p.y * kThetaMax);
				vx = p.x; vy = p.y;
			}
			mLastKeys = event.mods;
           
			m_pMover->move(FPoint(vx, vy), kField);
			break;
		}
			
		case kSelectedSpeaker:
		{
			float vx = (mouseLocation.x - kSpeakerRadius) / (padSize - kSpeakerDiameter);
			float vy = 1 - (mouseLocation.y - kSpeakerRadius) / (padSize - kSpeakerDiameter);
			mFilter->setSpeakerXY01(mSelectedItem, FPoint(vx, vy));
			break;
		}
	}
	
	repaint();
}

void FieldComponent::mouseUp(const MouseEvent &event) {
    clearTrajectoryPath();
    repaint();
	switch(mSelectionType) {
		case kNoSelection:
			return;
		case kSelectedSource:
			m_pMover->end(kField);
			break;
		case kSelectedSpeaker:
			break;
	}
	mSelectionType = kNoSelection;
}
