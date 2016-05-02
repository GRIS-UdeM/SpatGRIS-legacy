/*
 ==============================================================================
<<<<<<< HEAD
 SpatGRIS: multichannel sound spatialization plug-in.
=======
 Octogris2: multichannel sound spatialization plug-in.
>>>>>>> 2588dc2f3221b0a2cc68818c05101612d949a534
 
 Copyright (C) 2015  GRIS-UdeM
 
 SourceMover.h
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


#ifndef SOURCEMOVER_H_INCLUDED
#define SOURCEMOVER_H_INCLUDED

#include "PluginProcessor.h"

class FieldComponent;

typedef enum
{
	kVacant,
	kField,
	kOsc,
	kLeap,
    kHID,
    kSourceThread,
    kTrajectory
} MoverType;

class SourceMover
{
public:
    
	SourceMover(SpatGrisAudioProcessor *filter);
    void updateNumberOfSources();
	
	void begin(int s, MoverType mt);
    void sortAngles();
    void setEqualRadius();
    void setEqualAngles();
    void setEqualRadiusAndAngles();
    void setSymmetricX();
    void setSymmetricY();
	void move(FPoint p, MoverType mt);
	void end(MoverType mt);
    
    void setFieldComponent(FieldComponent* field){
        mField = field;
    }
	
private:
<<<<<<< HEAD
	SpatGrisAudioProcessor *mFilter;
=======
	OctogrisAudioProcessor *mFilter;
>>>>>>> 2588dc2f3221b0a2cc68818c05101612d949a534
	MoverType mMoverType;
	int mSelectedSrc;
	
	Array<FPoint> mSourcesDownXY;
	Array<FPoint> mSourcesDownRT;
	Array<float> mSourcesAngularOrder;
    FieldComponent* mField;
};




#endif  // SOURCEMOVER_H_INCLUDED
