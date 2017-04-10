//
//  UiComponent.hpp
//  SpatGRIS
//
//  Created by GRIS on 2017-04-06.
//
//

#ifndef UiComponent_h
#define UiComponent_h

#include "../JuceLibraryCode/JuceHeader.h"
#include "../../GrisCommonFiles/GrisLookAndFeel.h"


using namespace std;

class SpatGrisAudioProcessor;

//======================================= BOX ========================================
class Box : public Component
{
public:
    Box(GrisLookAndFeel *feel, String title="");
    ~Box();
    
    Component * getContent();
    void resized();
    void correctSize(unsigned int width, unsigned int height);
    void paint(Graphics &g) ;
    
private:
    Component *content;
    Viewport *viewport;
    GrisLookAndFeel *grisFeel;
    Colour bgColour;
    String title;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Box)
};


//================================== OctTabbedComponent ==============================
class OctTabbedComponent : public TabbedComponent
{
public:
    OctTabbedComponent(GrisLookAndFeel *feel, TabbedButtonBar::Orientation orientation, SpatGrisAudioProcessor *filter);
    void currentTabChanged (int newCurrentTabIndex, const String& newCurrentTabName) override;
    void initDone();
    
private:
    SpatGrisAudioProcessor *filter;
    bool mInited;
};


//================================== ProgressBarTrajec ==============================
class ProgressBarTraj : public Component
{
public:
    ProgressBarTraj();
    void paint(Graphics &g);
    void setValue(float v);
private:
    float value;
};

#endif /* UiComponent_hpp */
