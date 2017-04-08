/*
 ==============================================================================
 SpatGRIS: multichannel sound spatialization plug-in.
 
 Copyright (C) 2015  GRIS-UdeM
 
 PluginEditor.h
 
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

#ifndef PLUGINEDITOR_H_INCLUDED
#define PLUGINEDITOR_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"
#include "../../GrisCommonFiles/GrisLookAndFeel.h"
#include "DefaultParam.h"

#include "SpatComponent.h"
#include "UiComponent.h"
#include "LevelComponent.h"

using namespace std;

class SpatGrisAudioProcessor;

class SpatGrisAudioProcessorEditor :    public AudioProcessorEditor,
                                        public Button::Listener,
                                        public TextEditor::Listener,
                                        public Slider::Listener,
                                        private Timer
{
    
public :
    //==============================================================================
    SpatGrisAudioProcessorEditor(SpatGrisAudioProcessor * filter = nullptr);
    ~SpatGrisAudioProcessorEditor();
    //==============================================================================

    
    //==============================================================================
    void buttonClicked (Button *button) override;
    void sliderValueChanged (Slider *slider) override;
    void textEditorFocusLost (TextEditor &textEditor) override;
    void textEditorReturnKeyPressed (TextEditor &textEditor) override;
    
    void timerCallback() override;
    void paint (Graphics& g) override;
    void resized() override;
    //==============================================================================
    
private :
    //==============================================================================
    Label*          addLabel(const String &s, const String &stooltip, int x, int y, int w, int h, Component *into);
    TextButton*     addButton(const String &s, const String &stooltip, int x, int y, int w, int h, Component *into);
    ToggleButton*   addToggleButton(const String &s, const String &stooltip, int x, int y, int w, int h, Component *into, bool toggle = false);
    TextEditor*     addTextEditor(const String &s, const String &emptyS, const String &stooltip, int x, int y, int w, int h, Component *into, int wLab = 80);
    Slider*         addSlider(const String &s, const String &stooltip, int x, int y, int w, int h, Component *into, juce::Slider::TextEntryBoxPosition tebp = juce::Slider::TextEntryBoxPosition::NoTextBox);
    //==============================================================================
    
    SpatGrisAudioProcessor  * filter;
    
    GrisLookAndFeel grisFeel;
    TooltipWindow tooltipWindow;
    //Window Resizer---------------------------------
    ComponentBoundsConstrainer resizeWindow;
    ScopedPointer<ResizableCornerComponent> resizer;
    
    //
    SpatComponent   * spatFieldComp;
    
    Box             * boxSourceParam;
    Box             * boxOutputParam;
    Box             * boxTrajectory;
    
    OctTabbedComponent  * octTab;
    
    //Component------------------------------------
    //For Source param
    Label           * labSurfaceOrPan;
    ToggleButton    * togLinkSurfaceOrPan;
    Slider          * sliSurfaceOrPan;
    
    Label           * labAzimSpan;
    ToggleButton    * togLinkAzimSpan;
    Slider          * sliAzimSpan;
    
    Label           * labElevSpan;
    ToggleButton    * togLinkElevSpan;
    Slider          * sliAElevSpann;
    
    //For Outputs param
    vector<LevelComponent *> vecLevelOut;
};

#endif  // PLUGINEDITOR_H_INCLUDED
