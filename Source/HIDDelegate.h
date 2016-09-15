/*
 ==============================================================================
 SpatGRIS: multichannel sound spatialization plug-in.
 
 Copyright (C) 2015  GRIS-UdeM
 
 HIDDelegate.h
 Created: 12 March 2015 1:23:01pm
 
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

#ifndef __Octogris2__HIDDelegate__
#define __Octogris2__HIDDelegate__

#include "PluginProcessor.h"

#if USE_JOYSTICK

#include <stdio.h>
#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginEditor.h"
#include "HID_Utilities_External.h"

class HIDDelegate : public ReferenceCountedObject
{
public:
    typedef ReferenceCountedObjectPtr<HIDDelegate> Ptr;
    
    //! Interface to call the builder using the ReferenceCountedObject benefits
    static HIDDelegate::Ptr CreateHIDDelegate(SpatGrisAudioProcessor *filter, SpatGrisAudioProcessorEditor *editor);
    //! Builder
    HIDDelegate(SpatGrisAudioProcessor *filter, SpatGrisAudioProcessorEditor *editor);
    //! Initializing all the member with the data from the joystick
    OSStatus Initialize_HID(void *inContext);
    
    //! Handling the data sent by the joystick
    static void joystickPositionCallback(void * inContext,IOReturn inResult,void * inSender, IOHIDValueRef   inIOHIDValueRef);
    //! Event called when there is a match of device
    static void Handle_DeviceMatchingCallback(void *inContext, IOReturn inResult, void *inSender, IOHIDDeviceRef inIOHIDDeviceRef);
    //! Event called when a matched device is disconnected
    static void Handle_DeviceRemovalCallback(void *inContext, IOReturn inResult, void *inSender, IOHIDDeviceRef inIOHIDDeviceRef);
    //! Method called to handle the axis mouvement while a button is being pushed
    void JoystickUsed(uint32_t usage, float scaledValue, double min, double max);
    //!Method creating matching Dictonnaries
    CFDictionaryRef hu_CreateMatchingDictionary(uint32_t inUsagePage, uint32_t inUsage);
    //! Set for the table memorizing button states
    void setButtonPressedTab(u_int32_t index, bool state);
    //! Return the number of button counted on the joystick
    long getNbButton(){return m_iNbOfJoystickButtons;};
    //! Return the address of the device
    IOHIDDeviceRef getDeviceRef(){return deviceRef;}
    //! Return the address of the set of devices
    CFSetRef getDeviceSetRef(){return deviceSetRef;}
    
//    void readAndUseJoystickValues();
    virtual ~HIDDelegate() {};
    
private:
    
    SpatGrisAudioProcessor *mFilter;
    SpatGrisAudioProcessorEditor *mEditor;
    //! Number of counted buttons
    long m_iNbOfJoystickButtons;
    //! Table memorizing button states
    vector<bool> m_bJoystickButtonsCurentlyPressed;
    //! Variables recording the last and present coordinates (calculated or not)
    float m_fCurX01, m_fCurY01;
    //! Set of devices address
    CFSetRef deviceSetRef;
    //! Current device address
    IOHIDDeviceRef deviceRef;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HIDDelegate)
};


#endif  
#endif  // OCTOLEAP_H_INCLUDED
