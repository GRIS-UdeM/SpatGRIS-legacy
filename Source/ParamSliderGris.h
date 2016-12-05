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


#ifndef ParamSliderGris_h
#define ParamSliderGris_h

//================================================== ParamSliderGRIS ======================================================
class ParamSliderGRIS : public Slider
{
public:
    ParamSliderGRIS(int paramIndex, int paramType, ToggleButton *link, SpatGrisAudioProcessor *filter)
    :
    mParamIndex(paramIndex),
    mParamType(paramType),
    mLinkButton(link),
    mFilter(filter),
    mBeganGesture(false),
    mMouseDown(false)
    {
        jassert(mLinkButton || (mParamType != kParamSource && mParamType != kParamAzimSpan && mParamType != kParamElevSpan));
        
    }
    
    void mouseDown (const MouseEvent &e) {
        mBeganGesture = false;
        mMouseDown = true;
        
        bool resetToDefault = e.mods.isAltDown();
        //IF ALT IS PRESSED WE NEED TO RESET SLIDER TO DEFAULT VALUE
        if (resetToDefault) {
            double newVal;
            switch(mParamType) {
                case kParamSource:          newVal = normalize(kSourceMinDistance, kSourceMaxDistance, kSourceDefaultDistance); break;
                case kParamSmooth:          newVal = normalize(kSmoothMin, kSmoothMax, kSmoothDefault); break;
                case kParamVolumeFar:       newVal = normalize(kVolumeFarMin, kVolumeFarMax, kVolumeFarDefault); break;
                case kParamVolumeMid:       newVal = normalize(kVolumeMidMin, kVolumeMidMax, kVolumeMidDefault); break;
                case kParamVolumeNear:      newVal = normalize(kVolumeNearMin, kVolumeNearMax, kVolumeNearDefault); break;
                case kParamFilterFar:       newVal = normalize(kFilterFarMin, kFilterFarMax, kFilterFarDefault); break;
                case kParamFilterMid:       newVal = normalize(kFilterMidMin, kFilterMidMax, kFilterMidDefault); break;
                case kParamFilterNear:      newVal = normalize(kFilterNearMin, kFilterNearMax, kFilterNearDefault); break;
                case kParamMaxSpanVolume:   newVal = normalize(kMaxSpanVolumeMin, kMaxSpanVolumeMax, kMaxSpanVolumeDefault); break;
                case kParamRoutingVolume:   newVal = normalize(kRoutingVolumeMin, kRoutingVolumeMax, kRoutingVolumeDefault); break;
                case kParamAzimSpan:        newVal = 0; break;
                case kParamElevSpan:        newVal = 0; break;
            }
            
            if (mParamType == kParamSource && mLinkButton->getToggleState()) {
                for (int i = 0; i < mFilter->getNumberOfSources(); i++) {
                    int paramIndex = mFilter->getParamForSourceD(i);
                    if (mFilter->getParameter(paramIndex) != newVal){
                        mFilter->setParameterNotifyingHost(paramIndex, newVal);
                    }
                }
            } else if (mParamType == kParamAzimSpan && mLinkButton->getToggleState()) {
                mFilter->setPreventSourceAzimElevSpanUpdate(true);
                for (int i = 0; i < mFilter->getNumberOfSources(); i++) {
                    int paramIndex = mFilter->getParamForSourceAzimSpan(i);
                    if (mFilter->getParameter(paramIndex) != newVal){
                        mFilter->setParameterNotifyingHost(paramIndex, newVal);
//                        mFilter->setParameter(paramIndex, newVal);
                    }
                }
                mFilter->setPreventSourceAzimElevSpanUpdate(false);
            } else if (mParamType == kParamElevSpan && mLinkButton->getToggleState()) {
                mFilter->setPreventSourceAzimElevSpanUpdate(true);
                for (int i = 0; i < mFilter->getNumberOfSources(); i++) {
                    int paramIndex = mFilter->getParamForSourceElevSpan(i);
                    if (mFilter->getParameter(paramIndex) != newVal){
                        mFilter->setParameterNotifyingHost(paramIndex, newVal);
//                        mFilter->setParameter(paramIndex, newVal);
                    }
                }
                mFilter->setPreventSourceAzimElevSpanUpdate(false);
            } else if (mFilter->getParameter(mParamIndex) != newVal){
                mFilter->setParameterNotifyingHost(mParamIndex, newVal);
            }
            
        } else {
            Slider::mouseDown(e);
        }
    }
    
    void mouseUp (const MouseEvent &e)
    {
        //fprintf(stderr, "paremslider :: mouseUp\n");
        Slider::mouseUp(e);
        
        if (mBeganGesture){
            //fprintf(stderr, "paremslider :: endParameter\n");
            if (mParamType == kParamSource && mLinkButton->getToggleState()) {
                for (int i = 0; i < mFilter->getNumberOfSources(); i++){
                    int paramIndex = mFilter->getParamForSourceD(i);
                    mFilter->endParameterChangeGesture(paramIndex);
                }
            } else {
                mFilter->endParameterChangeGesture(mParamIndex);
            }
        }
        
        mMouseDown = false;
        mBeganGesture = false;
    }
    
    void valueChanged() {
        //begin the automation if it wasn't already begun
        if (mMouseDown && !mBeganGesture) {
            //fprintf(stderr, "paremslider :: beginParameter\n");
            if (mParamType == kParamSource && mLinkButton->getToggleState()) {
                for (int i = 0; i < mFilter->getNumberOfSources(); i++) {
                    int paramIndex = mFilter->getParamForSourceD(i);
                    mFilter->beginParameterChangeGesture(paramIndex);
                }
            } else {
                mFilter->beginParameterChangeGesture(mParamIndex);
            }
            
            mBeganGesture = true;
        }
        
        if (mParamType == kParamSource) {
            const float newVal = 1.f - (float)getValue();
            if (mLinkButton->getToggleState()) {
                for (int i = 0; i < mFilter->getNumberOfSources(); i++) {
                    int paramIndex = mFilter->getParamForSourceD(i);
                    if (mFilter->getParameter(paramIndex) != newVal)
                        mFilter->setParameterNotifyingHost(paramIndex, newVal);
                }
            } else {
                if (mFilter->getParameter(mParamIndex) != newVal){
                    mFilter->setParameterNotifyingHost(mParamIndex, newVal);
                }
            }
        } else if (mParamType == kParamAzimSpan) {
            const float newVal = (float)getValue();
            if (mLinkButton->getToggleState()) {
                for (int i = 0; i < mFilter->getNumberOfSources(); i++) {
                    int paramIndex = mFilter->getParamForSourceAzimSpan(i);
                    if (mFilter->getParameter(paramIndex) != newVal)
                        mFilter->setParameterNotifyingHost(paramIndex, newVal);
                }
            } else {
                if (mFilter->getParameter(mParamIndex) != newVal){
                    mFilter->setParameterNotifyingHost(mParamIndex, newVal);
                }
            }
        } else if (mParamType == kParamElevSpan) {
            const float newVal = (float)getValue();
            if (mLinkButton->getToggleState()) {
                for (int i = 0; i < mFilter->getNumberOfSources(); i++) {
                    int paramIndex = mFilter->getParamForSourceElevSpan(i);
                    if (mFilter->getParameter(paramIndex) != newVal)
                        mFilter->setParameterNotifyingHost(paramIndex, newVal);
                }
            } else {
                if (mFilter->getParameter(mParamIndex) != newVal){
                    mFilter->setParameterNotifyingHost(mParamIndex, newVal);
                }
            }
        } else {
            const float newVal = (float)getValue();
            if (mFilter->getParameter(mParamIndex) != newVal)
                mFilter->setParameterNotifyingHost(mParamIndex, newVal);
        }
    }
    
    void setParamIndex(int p_iParamIndex){
    
        mParamIndex = p_iParamIndex;
    }
    
    String getTextFromValue (double value)
    {
        switch(mParamType)
        {
            case kParamSource: value = denormalize(kSourceMinDistance, kSourceMaxDistance, value); break;
            case kParamSmooth: value = denormalize(kSmoothMin, kSmoothMax, value); break;
            case kParamVolumeFar: value = denormalize(kVolumeFarMin, kVolumeFarMax, value); break;
            case kParamVolumeMid: value = denormalize(kVolumeMidMin, kVolumeMidMax, value); break;
            case kParamVolumeNear: value = denormalize(kVolumeNearMin, kVolumeNearMax, value); break;
            case kParamFilterFar: value = denormalize(-100, 0, value); break;
            case kParamFilterMid: value = denormalize(-100, 0, value); break;
            case kParamFilterNear: value = denormalize(-100, 0, value); break;
            case kParamMaxSpanVolume: value = denormalize(kMaxSpanVolumeMin, kMaxSpanVolumeMax, value); break;
            case kParamRoutingVolume: value = denormalize(kRoutingVolumeMin, kRoutingVolumeMax, value); break;
        }
        
        if (mParamType >= kParamSmooth || mParamType <= kParamRoutingVolume) return String(roundToInt(value));
        return String(value, 1);
    }
    
    double getValueFromText (const String& text)
    {
        double value = Slider::getValueFromText(text);
        switch(mParamType)
        {
            case kParamSource: value = normalize(kSourceMinDistance, kSourceMaxDistance, value); break;
            case kParamSmooth: value = normalize(kSmoothMin, kSmoothMax, value); break;
            case kParamVolumeFar: value = normalize(kVolumeFarMin, kVolumeFarMax, value); break;
            case kParamVolumeMid: value = normalize(kVolumeMidMin, kVolumeMidMax, value); break;
            case kParamVolumeNear: value = normalize(kVolumeNearMin, kVolumeNearMax, value); break;
            case kParamFilterFar: value = normalize(-100, 0, value); break;
            case kParamFilterMid: value = normalize(-100, 0, value); break;
            case kParamFilterNear: value = normalize(-100, 0, value); break;
            case kParamMaxSpanVolume: value = normalize(kMaxSpanVolumeMin, kMaxSpanVolumeMax, value); break;
            case kParamRoutingVolume: value = normalize(kRoutingVolumeMin, kRoutingVolumeMax, value); break;
        }
        return value;
    }
    
    
    
private:
    //examples the paramIndex is the number used for processor->getparameter() and the type is something like source or speaker
    int mParamIndex, mParamType;
    ToggleButton *mLinkButton;
    SpatGrisAudioProcessor *mFilter;
    bool mBeganGesture, mMouseDown;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParamSliderGRIS)
};


#endif /* ParamSliderGris_h */
