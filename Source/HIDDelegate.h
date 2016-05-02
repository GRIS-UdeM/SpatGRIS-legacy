/*!
 * ==============================================================================
 *
 *  HIDDelegate.h
 *  Created: 12 March 2015 1:23:01pm
 *  Author:  Antoine L.
 *  Description :
 *  HIDDelegate allows you to create a object handling the HIDManager through the help of Reference
 *  Counted Object which deletes it in a safe and proper way.
 *  First create the IOHIDManager then the HIDDelegate that you initialize with the Initialize_HID method.
 *  HIDDelegate constructor needs two arguments which are the addresses of
 *  the main components of the plugin the Audio Processor and the Audio Processor
 *  Editor.
 * ==============================================================================
 */

#ifndef __Octogris2__HIDDelegate__
#define __Octogris2__HIDDelegate__

#include "PluginProcessor.h"

#if USE_JOYSTICK

#include <stdio.h>
#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginEditor.h"
<<<<<<< HEAD

=======
>>>>>>> 2588dc2f3221b0a2cc68818c05101612d949a534
#include "HID_Utilities_External.h"

class HIDDelegate : public ReferenceCountedObject
{
public:
    typedef ReferenceCountedObjectPtr<HIDDelegate> Ptr;
    
    //! Interface to call the builder using the ReferenceCountedObject benefits
<<<<<<< HEAD
    static HIDDelegate::Ptr CreateHIDDelegate(SpatGrisAudioProcessor *filter, SpatGrisAudioProcessorEditor *editor);
    //! Builder
    HIDDelegate(SpatGrisAudioProcessor *filter, SpatGrisAudioProcessorEditor *editor);
=======
    static HIDDelegate::Ptr CreateHIDDelegate(OctogrisAudioProcessor *filter, OctogrisAudioProcessorEditor *editor);
    //! Builder
    HIDDelegate(OctogrisAudioProcessor *filter, OctogrisAudioProcessorEditor *editor);
>>>>>>> 2588dc2f3221b0a2cc68818c05101612d949a534
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
<<<<<<< HEAD
    //! Return the number of button counted on the joystick
    int getNbButton(){return m_iNbOfJoystickButtons;};
=======
    //! Get for the table memorizing button states
    bool getButtonPressedTab(u_int32_t index);
    //! Return the number of button counted on the joystick
    int getNbButton(){return nbButton;};
>>>>>>> 2588dc2f3221b0a2cc68818c05101612d949a534
    //! Return the address of the device
    IOHIDDeviceRef getDeviceRef(){return deviceRef;}
    //! Return the address of the set of devices
    CFSetRef getDeviceSetRef(){return deviceSetRef;}
    
//    void readAndUseJoystickValues();
<<<<<<< HEAD

=======
    
    //! Destroyer
>>>>>>> 2588dc2f3221b0a2cc68818c05101612d949a534
    virtual ~HIDDelegate() {};
    
private:
    
<<<<<<< HEAD
    SpatGrisAudioProcessor *mFilter;
    SpatGrisAudioProcessorEditor *mEditor;
    //! Number of counted buttons
    int m_iNbOfJoystickButtons;
    //! Table memorizing button states
    vector<bool> m_bJoystickButtonsCurentlyPressed;
=======
    OctogrisAudioProcessor *mFilter;
    OctogrisAudioProcessorEditor *mEditor;
    //! Number of counted buttons
    int nbButton;
    //! Table memorizing button states
    bool* buttonPressedTab;
>>>>>>> 2588dc2f3221b0a2cc68818c05101612d949a534
    //! Variables recording the last and present coordinates (calculated or not)
    float vx, vy;
    //! Set of devices address
    CFSetRef deviceSetRef;
    //! Current device address
    IOHIDDeviceRef deviceRef;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HIDDelegate)
};


#endif  
#endif  // OCTOLEAP_H_INCLUDED
