/*
 ==============================================================================
 SpatGRIS: multichannel sound spatialization plug-in.
 
 Copyright (C) 2015  GRIS-UdeM
 
 HIDDelegate.cpp
 Created: 23 Jan 2014 8:09:25am
 
 Developers: Antoine Landrieux, Vincent Berthiaume
 
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


#include "HIDDelegate.h"

#if USE_JOYSTICK

#include <iostream>
#include "../JuceLibraryCode/JuceHeader.h"
#include "FieldComponent.h"

//==============================================================================


HIDDelegate::HIDDelegate(SpatGrisAudioProcessor *filter, SpatGrisAudioProcessorEditor *editor):
mFilter (filter),
mEditor (editor),
m_iNbOfJoystickButtons(0),
m_fCurX01(0),
m_fCurY01(0),
deviceSetRef(NULL),
deviceRef(NULL)
{
    
}

/** Handle_DeviceMatchingCallback is called whenever a HID Device matching the Matching Dictionnary is connected. In our case not much is done.  */
void HIDDelegate::Handle_DeviceMatchingCallback(void *inContext, IOReturn inResult, void *inSender, IOHIDDeviceRef inIOHIDDeviceRef) {
#pragma unused (  inContext, inSender )
    
//    printf("(context: %p, result: 0x%08X, sender: %p, device: %p)",
//           inContext, inResult, inSender, (void *) inIOHIDDeviceRef);
#ifdef DEBUG
    HIDDumpDeviceInfo(inIOHIDDeviceRef);
#endif // def DEBUG
    uint32_t vendorID = IOHIDDevice_GetVendorID(inIOHIDDeviceRef);
    uint32_t productID = IOHIDDevice_GetProductID(inIOHIDDeviceRef);
    if ((vendorID != 0x12BA) || (productID != 0x0030)) {
        //what to do when a joystick get plugged
    }
}

/** Handle_DeviceRemovalCallback is called whenever a HID Device matching the Matching Dictionnary is physically disconnected. In our case we call the uncheckJoystickButton method which will uncheck the joystick checkbox and reinitialize the IOHIDManager as NULL  */
void HIDDelegate::Handle_DeviceRemovalCallback(void *inContext, IOReturn inResult, void *inSender, IOHIDDeviceRef inIOHIDDeviceRef) {
#pragma unused (  inContext, inResult, inSender )
    
//    printf("(context: %p, result: 0x%08X, sender: %p, device: %p).\n", inContext, inResult, inSender, (void *) inIOHIDDeviceRef);
    SpatGrisAudioProcessorEditor * tempEditor = (SpatGrisAudioProcessorEditor*) inContext;
    tempEditor->uncheckJoystickButton();
    
    //what to do when a joystick get unplugged
}

/** joystickPositionCallback is called evereytime the connected joystick is used, the type of use and value of use are recovered from IOHIDValueRef sent by the event. First the method convert the IOHIDValueRef to a IOHIDElementRef which allow us to get the usagePage (type of control), the usage (the id of the control), the PhysicalMin and PhysicalMax which are 0 and 1 for common buttons or the max can vary from 256 to 1024 in our experience for the axis from one joystick to an other. We use the physical maximum to get a normalized value otherwise a less precise joystick would not permit mouvement accross the whole circle.
 Exemple for usagePage, usage and value, if I press the button 5 of my joystick usagePage will be 9(Id of the button type and usage will be 5 (number of the button)and value will be 1 (1 if pressed and 0 if not) */
void HIDDelegate::joystickPositionCallback(
                                                       void *          inContext,      // context from IOHIDDeviceRegisterInputValueCallback
                                                       IOReturn        inResult,       // completion result for the input value operation
                                                       void *          inSender,       // IOHIDDeviceRef of the device this element is from
                                                       IOHIDValueRef   inIOHIDValueRef) // the new element value
{
    do {
        IOHIDElementRef tIOHIDElementRef = IOHIDValueGetElement(inIOHIDValueRef);
        //We get the informations we need from inIOHIDValueRef
        uint32_t usagePage  = IOHIDElementGetUsagePage(tIOHIDElementRef);
        uint32_t usage      = IOHIDElementGetUsage(tIOHIDElementRef);
        double min          = IOHIDElementGetPhysicalMin(tIOHIDElementRef);
        double max          = IOHIDElementGetPhysicalMax(tIOHIDElementRef);
        
        double value = IOHIDValueGetScaledValue(inIOHIDValueRef, kIOHIDValueScaleTypePhysical);
        SpatGrisAudioProcessorEditor* tempEditor = (SpatGrisAudioProcessorEditor*) inContext;  //we get the editor from the context
        if(tempEditor->getHIDDel() != NULL) {
            if(usagePage==1) {   //axis
                tempEditor->getHIDDel()->JoystickUsed(usage, value,min,max);  //calling Joystick used the function that will modify the source position
            }
            if(usagePage == 9) {   //buttons
                
                double state = IOHIDValueGetScaledValue(inIOHIDValueRef, kIOHIDValueScaleTypePhysical);
                if(state == 1) {  //being pressed
                    if(usage<= tempEditor->getNbSources() ) {
                        tempEditor->getHIDDel()->setButtonPressedTab(usage,1);
                        tempEditor->getMover()->begin(usage-1, kHID);
                    }
                } else if (state == 0) {  //released
                    if(usage<= tempEditor ->getNbSources() ) {
                        tempEditor->getHIDDel()->setButtonPressedTab(usage,0);
                        tempEditor->getMover()->end(kHID);
                    }
                }
            }
            if (!tIOHIDElementRef) {
//                printf("tIOHIDElementRef == NULL\n");
                break;                                                              // (no)
            }
            // length ok?
            CFIndex length = IOHIDValueGetLength(inIOHIDValueRef);
            if (length > sizeof(double_t)) {
                break;                                                              // (no)
            }
        }
    }
    while (false);
}

/**create a matching dictionnary that allow you to only look for one type of HID devices, in our case the joysticks.
 As this method is also used as a static method in the HID_Utilities there probably is a better way to use it but this is the way it was explained to me on the website I used to create my hid communications. */
CFDictionaryRef HIDDelegate::hu_CreateMatchingDictionary(uint32_t inUsagePage, uint32_t inUsage) {
    // create a dictionary to add usage page/usages to
    CFMutableDictionaryRef refHIDMatchDictionary = CFDictionaryCreateMutable(kCFAllocatorDefault,0,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
    if (refHIDMatchDictionary) {
        if (inUsagePage) {
            // Add key for device type to refine the matching dictionary.
            CFNumberRef pageCFNumberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &inUsagePage);
            if (pageCFNumberRef) {
                CFDictionarySetValue(refHIDMatchDictionary, CFSTR(kIOHIDPrimaryUsagePageKey), pageCFNumberRef);
                CFRelease(pageCFNumberRef);
                // note: the usage is only valid if the usage page is also defined
                if (inUsage) {
                    CFNumberRef usageCFNumberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &inUsage);
                    if (usageCFNumberRef) {
                        CFDictionarySetValue(refHIDMatchDictionary, CFSTR(kIOHIDPrimaryUsageKey), usageCFNumberRef);
                        CFRelease(usageCFNumberRef);
                    } else {
                        fprintf(stderr, "%s: CFNumberCreate(usage) failed.", __PRETTY_FUNCTION__);
                    }
                }
            } else {
                fprintf(stderr, "%s: CFNumberCreate(usage page) failed.", __PRETTY_FUNCTION__);
            }
        }
    } else {
        fprintf(stderr, "%s: CFDictionaryCreateMutable failed.", __PRETTY_FUNCTION__);
    }
    
    return (refHIDMatchDictionary);
}

/** Initialize_HID has to be used once you have created your HIDDelegate in order for the program to know all there is to know about your joystick (address, number of buttons, setting the callback etc) */
OSStatus HIDDelegate::Initialize_HID(void *inContext) {
    
    // create the manager
    if (!gIOHIDManagerRef) {
        printf("%s: Could not create IOHIDManager.\n", __PRETTY_FUNCTION__);
        return -1;
    }
    
    CFMutableArrayRef matchingCFArrayRef = CFArrayCreateMutable(kCFAllocatorDefault,0, &kCFTypeArrayCallBacks);
    if(CFGetTypeID(gIOHIDManagerRef)==IOHIDManagerGetTypeID() && matchingCFArrayRef){
        CFDictionaryRef matchingCFDictJoystickRef = hu_CreateMatchingDictionary(kHIDPage_GenericDesktop,kHIDUsage_GD_Joystick); //we set the matching dictionnary only with joysticks
        CFDictionaryRef matchingCFDictGamePadRef  = hu_CreateMatchingDictionary(kHIDPage_GenericDesktop,kHIDUsage_GD_GamePad); //we set the matching dictionnary only with gamepads
        CFArrayAppendValue( matchingCFArrayRef, matchingCFDictJoystickRef );
        CFArrayAppendValue( matchingCFArrayRef, matchingCFDictGamePadRef );
        if(matchingCFArrayRef) {
            IOHIDManagerSetDeviceMatchingMultiple( gIOHIDManagerRef, matchingCFArrayRef );
            IOHIDManagerRegisterDeviceMatchingCallback(gIOHIDManagerRef, Handle_DeviceMatchingCallback, inContext);
            IOHIDManagerRegisterDeviceRemovalCallback (gIOHIDManagerRef, Handle_DeviceRemovalCallback, inContext);
            IOHIDManagerScheduleWithRunLoop(gIOHIDManagerRef, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
        } else {
            fprintf(stderr, "%s: hu_CreateDeviceMatchingDictionary failed.", __PRETTY_FUNCTION__);
            return -1;
        }
        
        //we get the list of joystick connected and we go through it to have the address
        deviceSetRef = IOHIDManagerCopyDevices(gIOHIDManagerRef);
        if(deviceSetRef != 0x0) {
            int nbJoysticks = (int)CFSetGetCount(deviceSetRef);
            CFTypeRef array[nbJoysticks];
            CFSetGetValues(deviceSetRef, array);
            
            for(int i = 0; i < nbJoysticks; ++i) {
                if(CFGetTypeID(array[i])== IOHIDDeviceGetTypeID()) {
                    deviceRef = (IOHIDDeviceRef)array[i];
                }
                IOHIDDeviceRegisterInputValueCallback(deviceRef, joystickPositionCallback, inContext);
                uint32_t page = kHIDPage_GenericDesktop;
                uint32_t joystickUsage = kHIDUsage_GD_Joystick;
                uint32_t gamepadUsage = kHIDUsage_GD_GamePad;
                
                //if device is not a joystick or a gamepad, continue
                if (! (IOHIDDeviceConformsTo(deviceRef, page, joystickUsage) || IOHIDDeviceConformsTo(deviceRef, page, gamepadUsage))) {
                    continue;
                }
                
                CFArrayRef elementRefTab = IOHIDDeviceCopyMatchingElements(deviceRef, NULL, kIOHIDOptionsTypeNone);
                
                IOHIDDeviceScheduleWithRunLoop(deviceRef, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
                
                CFIndex nbElement = CFArrayGetCount(elementRefTab);
                m_iNbOfJoystickButtons = nbElement-13;
                
                m_bJoystickButtonsCurentlyPressed = vector<bool>(m_iNbOfJoystickButtons, false);
                
                gElementCFArrayRef =  IOHIDDeviceCopyMatchingElements(deviceRef, NULL, kIOHIDOptionsTypeNone);
                
                if (!gElementCFArrayRef) {
                    continue;
                }
                for (CFIndex i = 0; i < nbElement; ++i) {
                    IOHIDElementRef tIOHIDElementRef  = (IOHIDElementRef) CFArrayGetValueAtIndex(gElementCFArrayRef,i);
                    IOHIDElementType tIOHIDElementType = IOHIDElementGetType(tIOHIDElementRef);
                    if (tIOHIDElementType > kIOHIDElementTypeInput_ScanCodes) {
                        continue;
                    }
                    
                    uint32_t reportSize = IOHIDElementGetReportSize(tIOHIDElementRef);
                    uint32_t reportCount = IOHIDElementGetReportCount(tIOHIDElementRef);
                    if ((reportSize * reportCount) > 64) {
                        continue;
                    }
                    
                    uint32_t usagePage = IOHIDElementGetUsagePage(tIOHIDElementRef);
                    uint32_t usage = IOHIDElementGetUsage(tIOHIDElementRef);
                    if (!usagePage || !usage || usage == -1) {
                        continue;
                    }
                }
            }
        }
    }
    // open it
    IOReturn tIOReturn = IOHIDManagerOpen(gIOHIDManagerRef, kIOHIDOptionsTypeNone);
    if (kIOReturnSuccess != tIOReturn) {
        printf("%s: IOHIDManagerOpen error: 0x%08u (\"\" - \"\").\n", __PRETTY_FUNCTION__, tIOReturn);
        return -1;  // THROW
    }
    
//    printf("IOHIDManager (%p) creaded and opened!", (void *) gIOHIDManagerRef);
    return 0;
}


//void HIDDelegate::readAndUseJoystickValues() {
//    int nbJoysticks = CFSetGetCount(deviceSetRef);
//    CFTypeRef array[nbJoysticks];
//    CFSetGetValues(deviceSetRef, array);
//    for(int iCurJoy = 0; iCurJoy < nbJoysticks; iCurJoy++) {
//        
//        if(gElementCFArrayRef == NULL || CFGetTypeID(array[iCurJoy]) != IOHIDDeviceGetTypeID()){
//            continue;
//        }
//        CFIndex nbElement = CFArrayGetCount(gElementCFArrayRef);
//        
//        for (CFIndex iCurElm = 0; iCurElm < nbElement; ++iCurElm) {
//            if(mEditor->getHIDDel() == NULL) {
//                continue;
//            }
//            
//            IOHIDElementRef tIOHIDElementRef= (IOHIDElementRef) CFArrayGetValueAtIndex(gElementCFArrayRef, iCurElm);
//            uint32_t usagePage              = IOHIDElementGetUsagePage(tIOHIDElementRef);
//            uint32_t usage                  = IOHIDElementGetUsage(tIOHIDElementRef);
//            double value                    = IOHIDElement_GetValue(tIOHIDElementRef,  kIOHIDValueScaleTypePhysical);
//
//            //axis
//            if(usagePage == 1 && !std::isnan(value)){
//                //calling Joystick used the function that will modify the source position
//                double min = IOHIDElementGetPhysicalMin(tIOHIDElementRef);
//                double max = IOHIDElementGetPhysicalMax(tIOHIDElementRef);
//
//                mEditor->getHIDDel()->JoystickUsed(usage, value, min, max);
//            }
//            //buttons
//            else if(usagePage == 9 && value == 1 && usage <= mEditor->getNbSources()){
//                mEditor->getHIDDel()->setButtonPressedTab(usage,1);
//                if (value == 0 && usage<= mEditor ->getNbSources()){  //released
//                    mEditor->getHIDDel()->setButtonPressedTab(usage,0);
//                }
//            }
//            
//        }
//    }
//}

/** this is called, to handle the effect of the use of the axis while pressing a button on the joystick,
 by joystickPositionCallback because as a static method it is quite limited.
 We give  the usage to know which axis is being used, the scaledValue to know how much the joystick is bent.
 MaxValue is used to know the resolution of the axis. */
void HIDDelegate::JoystickUsed(uint32_t usage, float scaledValue, double minValue, double maxValue) {
    
    for(int iCurBut = 0; iCurBut < getNbButton(); ++iCurBut) {    //Sweep accross all the joystick buttons to check which is being pressed
        if(!m_bJoystickButtonsCurentlyPressed[iCurBut]) {
            continue;
        }
        //smoothing values
        JUCE_COMPILER_WARNING("smooth should be multiplied by sampling rate")
        const float smooth = denormalize(kSmoothMin, kSmoothMax, mFilter->getSmoothing()); // milliseconds
        const float fOldValuesPortion = powf(0.01f, 1000.f / smooth);
        
        //Switch to detect what part of the device is being used
        switch (usage) {
            case 48:
                {
                    //Normalizing the coordinate from every joystick as float between 0 and 1, multiplied by the size of the panel to have the new x coordinate of the new point.
                    float fNewX01 = (scaledValue  / maxValue);
                
                    //smooting new position
                    if (m_fCurX01 > 0){
                        m_fCurX01 = fOldValuesPortion * m_fCurX01 + (1-fOldValuesPortion) * fNewX01;
                    } else {
                        m_fCurX01 = fNewX01;
                    }
                
                    if(((m_fCurX01-0.5)*(m_fCurX01-0.5))+((m_fCurY01-0.5)*(m_fCurY01-0.5)) <= 1.26) {
                        FPoint newPosition(m_fCurX01, m_fCurY01);
                        mEditor->getMover()->move(newPosition, kHID);
                    }
                }
                break;
            case 49:
                {
                    //Normalizing the coordonate from every joystick as float between -1 and 1, multiplied by the size of the panel to have the new x coordinate of the new point.
                    float fNewY01 = (1 - (scaledValue  / (maxValue)));
                    if (m_fCurY01 > 0){
                        m_fCurY01 = fOldValuesPortion * m_fCurY01 + (1-fOldValuesPortion) * fNewY01;
                    } else {
                        m_fCurY01 = fNewY01;
                    }
                
                    if(((m_fCurX01-0.5)*(m_fCurX01-0.5))+((m_fCurY01-0.5)*(m_fCurY01-0.5))<=1.26) {
                        FPoint newPoint(m_fCurX01, m_fCurY01);
                        mEditor->getMover()->move(newPoint, kHID);
                    }
                }
                break;
            case 53:
                //printf("Axe RZ !!!! \n");
                break;
            case 54:
                //printf("Slider !!!! \n");
                break;
            case 57:
                //printf("Hat Switch !!!! \n");
                break;
            default:
                break;
        }
    }
}

void HIDDelegate::setButtonPressedTab(u_int32_t usage, bool state)  //Get and Set to use the button pressed array
{
    m_bJoystickButtonsCurentlyPressed[usage-1]=state;
}
HIDDelegate::Ptr HIDDelegate::CreateHIDDelegate(SpatGrisAudioProcessor *filter, SpatGrisAudioProcessorEditor *editor)
{
    return new HIDDelegate(filter, editor);
}

#endif
