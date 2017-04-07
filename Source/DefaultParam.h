//
//  DefaultParam.h
//  SpatGRIS
//
//  Created by GRIS on 2017-04-05.
//
//

#ifndef DefaultParam_h
#define DefaultParam_h

static const int    MaxSources  = 8;
static const int    MaxSpeakers = 16;
static const int    MaxBufferSize = 4096;

static const float SourceRadius = 10;
static const float SourceDiameter = SourceRadius * 2;
static const float SpeakerRadius = 10;

static const float SourceMinDistance = 2.5 * 0.5;
static const float SourceMaxDistance = 20 * 0.5;
static const float SourceDefaultDistance = SourceMinDistance;

static const float SmoothMin = 1;
static const float SmoothMax = 1000;
static const float SmoothDefault = 50;

static const float VolumeNearMin = -10;
static const float VolumeNearMax = 30;
static const float VolumeNearDefault = 6;

static const float VolumeMidMin = -30;
static const float VolumeMidMax = 10;
static const float VolumeMidDefault = 0;

static const float VolumeFarMin = -120;
static const float VolumeFarMax = 0;
static const float VolumeFarDefault = -36;

static const float MaxDistance = 2000;

static const float FilterNearMin = MaxDistance;
static const float FilterNearMax = 0;
static const float FilterNearDefault = 0;

static const float FilterMidMin = MaxDistance;
static const float FilterMidMax = 0;
static const float FilterMidDefault = MaxDistance / 10;

static const float FilterFarMin = MaxDistance;
static const float FilterFarMax = 0;
static const float FilterFarDefault = MaxDistance / 2;

static const float MaxSpanVolumeMin = 0;
static const float MaxSpanVolumeMax = 20;
static const float MaxSpanVolumeDefault = 0;

static const float RoutingVolumeMin = -120;
static const float RoutingVolumeMax = 6;
static const float RoutingVolumeDefault = 0;
/*
static const float MovementModeMin = Independent;
static const float MovementModeMax = SymmetricY;*/
static const float MovementModeDefault = 0;

static const float SpanMin = 0;
static const float SpanMax = 2;
static const float SpanDefault = 0;

static const float RadiusMax = 2;
static const float HalfCircle = M_PI;
static const float QuarterCircle = M_PI / 2;

static const float ThetaRampRadius = 0.5;
static const float ThetaLockRadius = 0.025;


static const float SourceDefaultRadius = 1.f;
static const float SpeedDefault = 1.0f;
static const float DirRandDefault = 0.5f;

static const int    Margin             = 2;
static const int    CenterColumnWidth  = 180;
static const int    DefaultFieldSize   = 500;
static const int    MinFieldSize       = 300;
static const int    RightColumnWidth   = 340;

static const int    DefaultUItWidth       = 1090;
static const int    DefaultUIHeight      = 540;

static const int    SizeWidthLevelComp = 20;
static const int    DefaultLabHeight = 18;
static const int    ParamBoxHeight = 165;
static const int    hertzRefresh = 30;

static const float  SpeedMinMax = 2.5f;

static const int    DataVersion = 3;

#endif /* DefaultParam_h */
