/*
 ==============================================================================
 SpatGRIS: multichannel sound spatialization plug-in.
 
 Copyright (C) 2015  GRIS-UdeM
 
 SourceMover.cpp
 Created: 8 Aug 2014 1:04:53pm
 
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

#include "SourceMover.h"
#include "FieldComponent.h"


SourceMover::SourceMover(SpatGrisAudioProcessor *filter)
: mFilter(filter)
 ,mMoverType(kVacant)
 ,mSelectedSrc(0)
{
//    JUCE_COMPILER_WARNING("at this point, mFilter is not constructed, so we can't call updateNumnerOfSources");
////    updateNumberOfSources();
//    
    int iNbrSrc = JucePlugin_MaxNumInputChannels;
    for (int j = 0; j < iNbrSrc; j++) {
        mSourcesDownXY.add(FPoint(0,0));
        mSourcesDownRT.add(FPoint(0,0));
        mSourcesAngularOrder.add(0);
    }
}

void SourceMover::begin(int s, MoverType mt) {
	if (mMoverType != kVacant) return;
	mMoverType = mt;
	mSelectedSrc = s;
    mFilter->setSelectedSrc(s);
	
    if (mMoverType != kSourceThread){
        mFilter->setIsRecordingAutomation(true);
        mFilter->beginParameterChangeGesture(mFilter->getParamForSourceX(mSelectedSrc));
        mFilter->beginParameterChangeGesture(mFilter->getParamForSourceY(mSelectedSrc));
#if ALLOW_MVT_MODE_AUTOMATION
//        mFilter->beginParameterChangeGesture(kMovementMode);
#endif

        storeAllDownPositions();
    }
}

void SourceMover::storeAllDownPositions(){
    int iNbrSrc = mFilter->getNumberOfSources();
    for (int j = 0; j < iNbrSrc; j++) {
        mSourcesDownRT.setUnchecked(j, mFilter->getSourceRT(j));
        mSourcesDownXY.setUnchecked(j, mFilter->getSourceXY(j));
    }
}

void SourceMover::storeDownPosition(int id, FPoint pointRT){
    jassert (id < mSourcesDownRT.size());
    
    mSourcesDownRT.setUnchecked(id, pointRT);
    mSourcesDownXY.setUnchecked(id, mFilter->convertRt2Xy(pointRT));
}


//in kSourceThread, FPoint p is the current location of the selected source, as read on the automation, and we move only the non-selected sources based on location of selected source
void SourceMover::move(FPoint pointXY01, MoverType mt) {
    if (mMoverType != mt){
        return;
    }
    
    //move selected source to pointXY01 only if not kSourceThread. In kSourceThread it is already being moved by automation
    if (mMoverType != kSourceThread){
        mFilter->setSourceXY01(mSelectedSrc, pointXY01);
        if (mFieldExists){
            mField->updatePositionTrace(pointXY01.x, pointXY01.y);
        }
    }
    
    // if we're not in independent mode and have more than 1 source
    if (mFilter->getMovementMode() != 0 && mFilter->getNumberOfSources() > 1) {
        //calculate delta for selected source
        
        //in normal, non-source-thread mode, we calculate delta for selected source compared to its starting point
        FPoint oldSelSrcPosRT = mSourcesDownRT[mSelectedSrc];
        FPoint newSelSrcPosRT = mFilter->getSourceRT(mSelectedSrc); //in kSourceThread, this will be the same as mFilter->convertXy012Rt(pointXY01)
        FPoint delSelSrcPosRT = newSelSrcPosRT - oldSelSrcPosRT;
        
        if (delSelSrcPosRT.isOrigin()){
            return;     //return if delta is null
        }
        float vxo = pointXY01.x, vyo = pointXY01.y;
        if (kSourceThread){
            mFilter->setPreventSourceLocationUpdate(true);
        }
        
        for (int iCurSrc = 0; iCurSrc < mFilter->getNumberOfSources(); iCurSrc++) {
            if (iCurSrc == mSelectedSrc) {
                continue;
            }
            //all x's and y's here are actually r's and t's
            switch(mFilter->getMovementMode()) {
                //these are all the same because in fixed cases, sources were moved right when the movement mode was selected
                case Circular:
                case CircularFixedRadius:
                case CircularFixedAngle:
                case CircularFullyFixed:{
                    //calculate new position for curSrc using delta for selected source
                    FPoint oldCurSrcPosRT = mSourcesDownRT[iCurSrc];
                    
                    
                    FPoint newCurSrcPosRT = oldCurSrcPosRT + delSelSrcPosRT;
                    if (newCurSrcPosRT.x < 0){
                        newCurSrcPosRT.x = 0;
                    }
                    if (newCurSrcPosRT.x > kRadiusMax){
                        newCurSrcPosRT.x = kRadiusMax;
                    }
                    if (newCurSrcPosRT.y < 0){
                        newCurSrcPosRT.y += kThetaMax;
                    }
                    if (newCurSrcPosRT.y > kThetaMax){
                        newCurSrcPosRT.y -= kThetaMax;
                    }
                    
                    mFilter->setSourceRT(iCurSrc, newCurSrcPosRT, false);
                    break;
                }
                case DeltaLock:{
                    FPoint delSelSrcPosXY;
                    if (mMoverType == kSourceThread){
                        delSelSrcPosXY = mFilter->convertRt2Xy(newSelSrcPosRT) - mFilter->convertRt2Xy(oldSelSrcPosRT);
                    } else {
                        delSelSrcPosXY = mFilter->getSourceXY(mSelectedSrc) - mSourcesDownXY[mSelectedSrc];
                    }
                    
                    //calculate new position for curSrc using delta for selected source
                    FPoint oldCurSrcPosXY = mSourcesDownXY[iCurSrc];                    
                    FPoint newCurSrcPosXY = oldCurSrcPosXY + delSelSrcPosXY;
                    
                    mFilter->setSourceXY(iCurSrc, newCurSrcPosXY, false);
                    break;
                }
                case SymmetricX:
                    vyo = 1 - vyo;
                    mFilter->setSourceXY01(iCurSrc, FPoint(vxo, vyo));
                    break;
                case SymmetricY: 
                    vxo = 1 - vxo;
                    mFilter->setSourceXY01(iCurSrc, FPoint(vxo, vyo));
                    break;
                default:
                    jassert(0);
                    break;
            }
        }
        if (kSourceThread){
            mFilter->setPreventSourceLocationUpdate(false);
        }
    }
}

void SourceMover::end(MoverType mt, bool clearTrajectory) {
    if (mMoverType != mt){
        return;
    } else if (mMoverType != kSourceThread){
        mFilter->endParameterChangeGesture(mFilter->getParamForSourceX(mSelectedSrc));
        mFilter->endParameterChangeGesture(mFilter->getParamForSourceY(mSelectedSrc));
#if ALLOW_MVT_MODE_AUTOMATION
//        mFilter->endParameterChangeGesture(kMovementMode);
#endif
        mFilter->setIsRecordingAutomation(false);
        if(clearTrajectory){
            mField->clearTrajectoryPath();
        }
    }
    
    mMoverType = kVacant;
}

void SourceMover::updateNumberOfSources(){
    mSourcesDownXY.clear();
    mSourcesDownRT.clear();
    mSourcesAngularOrder.clear();
    
    for (int i = 0; i < mFilter->getNumberOfSources(); i++) {
        mSourcesDownXY.add(FPoint(0,0));
        mSourcesDownRT.add(FPoint(0,0));
        mSourcesAngularOrder.add(0);
    }
}

void SourceMover::sortAngles(){
    
    int iNbrSrc = mFilter->getNumberOfSources();
    
    IndexedAngle * ia = new IndexedAngle[iNbrSrc];
    
    for (int j = 0; j < iNbrSrc; j++) {
        ia[j].i = j;
        ia[j].a = mFilter->getSourceRT(j).y;
    }
    
    qsort(ia, iNbrSrc, sizeof(IndexedAngle), IndexedAngleCompare);
    
    int b;
    for (b = 0; b < iNbrSrc && ia[b].i != mSelectedSrc; b++) ;
    
    if (b == iNbrSrc) {
        printf("sort angle error!\n");
        b = 0;
    }
    
    for (int j = 1; j < iNbrSrc; j++) {
        int o = (b + j) % iNbrSrc;
        o = ia[o].i;
        mSourcesAngularOrder.set(o, (M_PI * 2. * j) / iNbrSrc);
    }
    
    delete[] ia;
}

void SourceMover::setEqualRadius(){
    FPoint selSrcRT = mFilter->getSourceRT(mSelectedSrc);
    for (int iCurSrc = 0; iCurSrc < mFilter->getNumberOfSources(); iCurSrc++) {
        if (iCurSrc == mSelectedSrc){
            continue;
        }
        FPoint curSrcRT = mFilter->getSourceRT(iCurSrc);
        curSrcRT.x = selSrcRT.x;
        if (curSrcRT.y < 0) curSrcRT.y += kThetaMax;
        if (curSrcRT.y > kThetaMax) curSrcRT.y -= kThetaMax;
        mFilter->setPreventSourceLocationUpdate(true);
        mFilter->setSourceRT(iCurSrc, curSrcRT, false);
        storeDownPosition(iCurSrc, curSrcRT);
        mFilter->setPreventSourceLocationUpdate(false);
    }
}


void SourceMover::setEqualAngles(){
    //first figure out the correct angles
    sortAngles();
    
    //then set them
    FPoint selSrcRT = mFilter->getSourceRT(mSelectedSrc);
    for (int iCurSrc = 0; iCurSrc < mFilter->getNumberOfSources(); iCurSrc++) {
        if (iCurSrc == mSelectedSrc){
            continue;
        }
        FPoint curSrcRT = mFilter->getSourceRT(iCurSrc);
        curSrcRT.y = selSrcRT.y + mSourcesAngularOrder[iCurSrc];
        if (curSrcRT.x < 0) curSrcRT.x = 0;
        if (curSrcRT.x > kRadiusMax) curSrcRT.x = kRadiusMax;
        if (curSrcRT.y < 0) curSrcRT.y += kThetaMax;
        if (curSrcRT.y > kThetaMax) curSrcRT.y -= kThetaMax;
        mFilter->setPreventSourceLocationUpdate(true);
        mFilter->setSourceRT(iCurSrc, curSrcRT, false);
        storeDownPosition(iCurSrc, curSrcRT);
        mFilter->setPreventSourceLocationUpdate(false);

    }
}

void SourceMover::setEqualRadiusAndAngles(){
    //first figure out the correct angles
    sortAngles();
    
    //then set them
    FPoint selSrcRT = mFilter->getSourceRT(mSelectedSrc);
    for (int iCurSrc = 0; iCurSrc < mFilter->getNumberOfSources(); iCurSrc++) {
        if (iCurSrc == mSelectedSrc){
            continue;
        }
        FPoint curSrcRT = mFilter->getSourceRT(iCurSrc);

        curSrcRT.x = selSrcRT.x;
        curSrcRT.y = selSrcRT.y + mSourcesAngularOrder[iCurSrc];
        if (curSrcRT.y < 0) curSrcRT.y += kThetaMax;
        if (curSrcRT.y > kThetaMax) curSrcRT.y -= kThetaMax;
        mFilter->setPreventSourceLocationUpdate(true);
        mFilter->setSourceRT(iCurSrc, curSrcRT, false);
        storeDownPosition(iCurSrc, curSrcRT);
        mFilter->setPreventSourceLocationUpdate(false);
    }

}

void SourceMover::setSymmetricX(){
    int mSlaveSrc = 1;
    if (mSelectedSrc == 1){
        mSlaveSrc = 0;
    }
    FPoint pointXY01 = mFilter->getSourceXY01(mSelectedSrc);
    float vxo = pointXY01.x, vyo = pointXY01.y;
    vyo = 1 - vyo;
    mFilter->setSourceXY01(mSlaveSrc, FPoint(vxo, vyo));
}

void SourceMover::setSymmetricY(){
    int mSlaveSrc = 1;
    if (mSelectedSrc == 1){
        mSlaveSrc = 0;
    }
    FPoint pointXY01 = mFilter->getSourceXY01(mSelectedSrc);
    float vxo = pointXY01.x, vyo = pointXY01.y;
    vxo = 1 - vxo;
    mFilter->setSourceXY01(mSlaveSrc, FPoint(vxo, vyo));
}






