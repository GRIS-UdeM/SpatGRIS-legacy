//
//  DefaultParam.h
//  SpatGRIS
//
//  Created by GRIS on 2017-04-05.
//
//

#ifndef DefaultParam_h
#define DefaultParam_h

#if JUCE_MSVC
#define M_PI 3.14159265358979323846264338327950288
#endif

static const int    MaxSources      = 8;
static const int    MaxSpeakers     = 16;
static const int    MaxBufferSize   = 4096;


static const float RadiusMax        = 2.f;
static const float HalfCircle       = M_PI;
static const float QuarterCircle    = M_PI / 2.f;


//--------------------------------------------------
//Source Param
//--------------------------------------------------
static const float  SourceRadius    = 10.f;
static const float  SourceDiameter  = SourceRadius * 2.f;
static const float  SpeakerRadius   = 10.f;
//Surface
static const float  MinSurfSource   = 0.f;
static const float  MaxSurfSource   = 1.f;
static const float  DefSurfSource   = 0.f;
//Azim
static const float  MinAzimSource   = 0.f;
static const float  MaxAzimSource   = 2.f;
static const float  DefAzimSource   = 0.f;
//Elev
static const float  MinElevSource   = 0.f;
static const float  MaxElevSource   = 0.5f;
static const float  DefElevSource   = 0.f;


//--------------------------------------------------
//Trajectory Param
//--------------------------------------------------
static const float  MinSpeedTrajectory   = -3.f;
static const float  MaxSpeedTrajectory   = 3.f;
static const float  DefSpeedTrajectory   = 1.f;

static const float  MinTrajRandomSpeed   = 0.f;
static const float  MaxTrajRandomSpeed   = 1.f;
static const float  DefTrajRandomSpeed   = 0.5f;

//--------------------------------------------------
//UI Param
//--------------------------------------------------
static const float  DefaultSliderInter   = 0.00001f;

static const int    Margin             = 2;
static const int    CenterColumnWidth  = 180;
static const int    MinFieldSize       = 300;
static const int    RightColumnWidth   = 340;
static const int    SizeWidthLevelComp = 22;

static const int    DefaultUItWidth     = 1090;
static const int    DefaultUIHeight     = 540;

static const int    DefaultTexWidth     = 60;
static const int    DefaultLabWidth     = 120;
static const int    DefaultLabHeight    = 18;

static const int    HertzRefresh        = 30;


#endif /* DefaultParam_h */
