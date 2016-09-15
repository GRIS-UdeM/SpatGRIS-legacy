/*
 ==============================================================================
 SpatGRIS: multichannel sound spatialization plug-in.
 
 Copyright (C) 2015  GRIS-UdeM
 
 Areas.h
 Created: 2016-07-12
 
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



#ifndef Areas_hpp
#define Areas_hpp

#include <stdio.h>

class Area
{
public:
    Area() {}
    Area(int sp, float ix1, float iy1, float ix2, float iy2)
    :	speaker(sp),
    x1(ix1), x2(ix2) //, y1(iy1), y2(iy2)
    {
        m = (iy2-iy1) / (ix2-ix1);
        b = iy1 - m * ix1;
    }
    
    float eval(float x) const { return m * x + b; }
    
    int speaker;
    float x1, x2;
    //float y1, y2;
    float m, b;
};


static void AddArea(int speaker, float ix1, float iy1, float ix2, float iy2, vector<Area> &areas, int &areaCount, int &speakerCount)
{
    jassert(ix1 < ix2);
    
    //fprintf(stderr, "speaker: %d x1: %f x2: %f dc: %f\n", speaker, ix1, ix2, ix2 - ix1);
    //fflush(stderr);
    
    if (ix1 < 0)
    {
        jassert(ix2 >= 0 && ix2 <= kThetaMax);
        jassert(ix1 + kThetaMax > 0);
        
        float yc = (0 - ix1) / (ix2 - ix1) * (iy2 - iy1) + iy1;
        
        jassert(areaCount < speakerCount * s_iMaxAreas);
        areas[areaCount++] = Area(speaker, kThetaMax + ix1, iy1, kThetaMax, yc);
        
        jassert(areaCount < speakerCount * s_iMaxAreas);
        areas[areaCount++] = Area(speaker, 0, yc, ix2, iy2);
    }
    else if (ix2 > kThetaMax)
    {
        jassert(ix1 >= 0 && ix1 <= kThetaMax);
        jassert(ix2 - kThetaMax < kThetaMax);
        
        float yc = (kThetaMax - ix1) / (ix2 - ix1) * (iy2 - iy1) + iy1;
        
        jassert(areaCount < speakerCount * s_iMaxAreas);
        areas[areaCount++] = Area(speaker, ix1, iy1, kThetaMax, yc);
        
        jassert(areaCount < speakerCount * s_iMaxAreas);
        areas[areaCount++] = Area(speaker, 0, yc, ix2 - kThetaMax, iy2);
    }
    else
    {
        jassert(ix1 >= 0 && ix2 <= kThetaMax);
        
        jassert(areaCount < speakerCount * s_iMaxAreas);
        areas[areaCount++] = Area(speaker, ix1, iy1, ix2, iy2);
    }
}
static void Integrate(float x1, float x2, const vector<Area> &areas, int areaCount, vector<float> &outFactors, float factor)
{
    if (x1 == x2)
    {
        //fprintf(stderr, "x1 == x2 (%f == %f)\n", x1, x2);
        return;
    }
    
    jassert(x1 < x2);
    jassert(x1 >= 0);
    jassert(x2 <= kThetaMax);
    
    for (int a = 0; a < areaCount; a++)
    {
        const Area &area = areas[a];
        
        if (x2 <= area.x1 || x1 >= area.x2)
            continue; // completely outside
        
        float c1 = (x1 < area.x1) ? area.x1 : x1;
        float c2 = (x2 > area.x2) ? area.x2 : x2;
        
        float y1 = area.eval(c1);
        float y2 = area.eval(c2);
        
        float dc = c2 - c1;
        //fprintf(stderr, "x1: %f x2: %f area.x1: %f area.x2: %f c1: %f c2: %f dc: %f\n", x1, x2, area.x1, area.x2, c1, c2, dc);
        //fflush(stderr);
        jassert(dc > 0);
        
        float v = dc * (y1+y2); // * 0.5f;
        if (v <= 0) v = 1e-6;
        
        outFactors[area.speaker] += v * factor;
    }
}

#endif /* Areas_hpp */
