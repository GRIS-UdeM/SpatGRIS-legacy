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


typedef Point<float> FPoint;

struct SelectItem {
    unsigned int selectIdSource;
    unsigned int selectIdSpeaker;
    bool mouseOverSource;
    bool mouseOverSpeaker;
};

typedef enum {
    Independent = 0,
    Circular,
    CircularFixRad,
    CircularFixAng,
    CircularFullyFix,
    DeltaLock,
    SymmetricX,
    SymmetricY,
    SIZE_MM
} MouvementMode;


typedef enum {
    Circle = 0,
    Ellipse,
    Spiral,
    Pendulum,
    RandomTraj,
    RandomTarget,
    SymXTarget,
    SymYTarget,
    FreeDrawing,
    SIZE_TT
} TrajectoryType;


typedef enum {
    FreeVolum = 0,
    PanSpan,
    OSCSpatServer,
    OSCZirkonium,
    SIZE_PT
} ProcessType;

typedef enum {
    LeftAlternate = 0,
    LeftClockW,
    LeftCounterClockW,
    TopClockW,
    TopCounterClockW,
    SIZE_PSS
} PositionSourceSpeaker;


typedef struct {
    int     i;
    float   a;
} IndexedAngle;


//--------------------------------------------------
//Param
//--------------------------------------------------
static const int    MaxSources      = 8;
static const int    MaxSpeakers     = 16;
static const int    MaxBufferSize   = 4096;

static const int    OscTimerHz      = 30;
static const int    OscMinPort      = 0;
static const int    OscMaxPort      = 65535;
static const int    OscDefPort      = 18032;

static const float RadiusMax        = 2.f;
static const float AngleDegMax      = 360.f;
static const float HalfCircle       = M_PI;
static const float QuarterCircle    = M_PI / 2.f;
static const float ThetaMax         = M_PI * 2.f;

//Filter - Slider
static const float  MinVolCenter   = -10.f;
static const float  MaxVolCenter   = 30.f;
static const float  DefVolCenter   = 6.f;

static const float  MinVolSpeaker   = -30.f;
static const float  MaxVolSpeaker   = 10.f;
static const float  DefVolSpeaker   = 0.f;

static const float  MinVolFar       = -120.f;
static const float  MaxVolFar       = 0.f;
static const float  DefVolFar       = -36.f;

static const float  MinFilter       = -100.f;
static const float  MaxFilter       = 0.f;
static const float  DefFilterCenter = 0.f;
static const float  DefFilterSpeaker= -10.f;
static const float  DefFilterFar    = -50.f;

//--------------------------------------------------
//Source Param
//--------------------------------------------------
static const float  SourceRadius    = 10.f;
static const float  SourceDiameter  = SourceRadius * 2.f;
static const float  SpeakerRadius   = 10.f;

//Azim
static const float  MinAzimSource   = 0.f;
static const float  MaxAzimSource   = 2.f;
static const float  DefAzimSource   = 0.f;
//Elev
static const float  MinElevSource   = 0.f;
static const float  MaxElevSource   = 0.5f;
static const float  DefElevSource   = 0.f;
//Height - Surface
static const float  MinHeigSource   = 0.f;
static const float  MaxHeigSource   = 1.f;
static const float  DefHeigSource   = 0.f;

//--------------------------------------------------
//Trajectory Param
//--------------------------------------------------
static const float  MinSpeedTrajectory   = -3.f;
static const float  MaxSpeedTrajectory   = 3.f;
static const float  DefSpeedTrajectory   = 1.f;

static const float  MinTrajRandomSpeed   = 0.f;
static const float  MaxTrajRandomSpeed   = 1.f;
static const float  DefTrajRandomSpeed   = 0.5f;

static const float  MinTrajWidthEllipse   = 0.1f;
static const float  MaxTrajWidthEllipse   = 5.f;
static const float  DefTrajWidthEllipse   = 0.5f;

static const float  MinCyclePercent   = 0.f;
static const float  MaxCyclePercent   = 100.f;
static const float  DefCyclePercent   = 100.f;

//--------------------------------------------------
//UI Param
//--------------------------------------------------
static const float  DefaultSliderInter  = 0.00001f;
static const float  SliderInter     = 0.001f;
static const float  SliderInterInt  = 1.0f;

static const int    Margin             = 2;
static const int    CenterColumnWidth  = 180;
static const int    MinFieldSize       = 400;
static const int    RightColumnWidth   = 340;
static const int    SizeWidthLevelComp = 22;

static const int    DefaultUItWidth     = 1050;
static const int    DefaultUIHeight     = 500;

static const int    DefaultTexWidth     = 60;
static const int    DefaultLabWidth     = 120;
static const int    DefaultLabHeight    = 18;

static const int    HertzRefresh        = 30;


//--------------------------------------------------
//static funct
//--------------------------------------------------
static float GetRaySpat(float x, float y){
    return sqrtf((x*x)+ (y*y));
}
static float GetAngleSpat(float x, float y)
{
    float r;
    if(x < 0.0f){
        r = atanf(y/x)+M_PI;
    }else{
        r = atanf(y/x);
    }
    if(isnan(r)){
        return 0.0f;
    }
    return r;
}
static FPoint GetXYFromRayAng(float r, float a){
    return FPoint((r * cosf(a)),(r * sinf(a)));
}

static float AngleInCircle(FPoint p) {
    return  -atan2(( - p.y * 2.0f), (p.x * 2.0f ));
}

static float DegreeToRadian (float degree){
    return ((degree * M_PI ) / 180.0f) ;
}
static float RadianToDegree (float radian){
    float degree = ((radian * 180.0f ) / M_PI) ;
    if(degree<0.0f){
        degree += 360.0f;
    }
    return degree;
}

static void NormalizeXYSourceWithScreen(FPoint &p, float w)
{
    p.x = ((w) + ((w/2.0f)*p.x))+SourceRadius;
    p.y = ((w) - ((w/2.0f)*p.y))+SourceRadius;
}
static void NormalizeScreenWithSpat(FPoint &p, float w)
{
    p.x = (p.x - SourceRadius - w) / (w/2.0f);
    p.y = -(p.y - SourceRadius - w) / (w/2.0f);
}

static void NormalizeSourceMoverRayAng(FPoint &newCurSrcPosRT)
{
    if (newCurSrcPosRT.x < 0.0f){
        newCurSrcPosRT.x = 0.0f;
    }
    if (newCurSrcPosRT.x > RadiusMax){
        newCurSrcPosRT.x = RadiusMax;
    }
    if (newCurSrcPosRT.y < 0.0f){
        newCurSrcPosRT.y += ThetaMax;
    }
    if (newCurSrcPosRT.y > ThetaMax){
        newCurSrcPosRT.y -= ThetaMax;
    }
}

static void NormalizeSourceMoverXY(FPoint &newCurSrcPosRT)
{
    FPoint correctedRA =  FPoint(GetRaySpat(newCurSrcPosRT.x,newCurSrcPosRT.y), GetAngleSpat(newCurSrcPosRT.x,newCurSrcPosRT.y));
    NormalizeSourceMoverRayAng(correctedRA);
    correctedRA = GetXYFromRayAng(correctedRA.x, correctedRA.y);
    newCurSrcPosRT.x = correctedRA.getX();
    newCurSrcPosRT.y = correctedRA.getY();
}


static FPoint DegreeToXy (FPoint p, int FieldWidth)
{
    float x,y;
    x = -((FieldWidth - SourceDiameter)/2) * sinf(DegreeToRadian(p.x)) * cosf(DegreeToRadian(p.y));
    y = -((FieldWidth - SourceDiameter)/2) * cosf(DegreeToRadian(p.x)) * cosf(DegreeToRadian(p.y));
    return FPoint(x, y);
}

static FPoint GetSourceAzimElev(FPoint pXY, bool bUseCosElev = false)
{
    //calculate azim in range [0,1], and negate it because zirkonium wants -1 on right side
    float fAzim = -atan2f(pXY.x, pXY.y)/M_PI;
    
    //calculate xy distance from origin, and clamp it to 2 (ie ignore outside of circle)
    float hypo = hypotf(pXY.x, pXY.y);
    if (hypo > RadiusMax){
        hypo = RadiusMax;
    }
    
    float fElev;
    if (bUseCosElev){
        fElev = acosf(hypo/RadiusMax);   //fElev is elevation in radian, [0,pi/2)
        fElev /= (M_PI/2.f);                      //making range [0,1]
        fElev /= 2.f;                            //making range [0,.5] because that's what the zirkonium wants
    } else {
        fElev = (RadiusMax-hypo)/4.0f;
    }
    
    return FPoint(fAzim, fElev);
}

static FPoint GetPositionRT(FPoint pXY) {
    float r = hypotf(pXY.x, pXY.y);
    float t = atan2f(pXY.y, pXY.x);
    if (t < 0) t += ThetaMax;
    return FPoint(r, t);
}


static int IndexedAngleCompare(const void *a, const void *b)
{
    IndexedAngle *ia = (IndexedAngle*)a;
    IndexedAngle *ib = (IndexedAngle*)b;
    return (ia->a < ib->a) ? -1 : ((ia->a > ib->a) ? 1 : 0);
}

static float GetValueInRange(float value, float min, float max)
{
    if(value < min){ return min; }
    if(value > max){ return max; }
    return value;
}

//Get Name===============================================================

static String GetMouvementModeName(MouvementMode i)
{
    switch(i) {
        case Independent:       return "Independent";
        case Circular:          return "Circular";
        case CircularFixRad:    return "Circular Fixed Radius";
        case CircularFixAng:    return "Circular Fixed Angle";
        case CircularFullyFix:  return "Circular Fully Fixed";
        case DeltaLock:         return "Delta Lock";
        case SymmetricX:        return "Symmetric X";
        case SymmetricY:        return "Symmetric Y";
            
        default:
            jassertfalse;
            return "";
    }
}

static String GetTrajectoryName(TrajectoryType i)
{
    switch(i) {
        case Circle:        return "Circle";
        case Ellipse:       return "Ellipse";
        case Spiral:        return "Spiral";
        case Pendulum:      return "Pendulum";
        case RandomTraj:    return "Random";
        case RandomTarget:  return "Random Target";
        case SymXTarget:    return "Sym X Target";
        case SymYTarget:    return "Sym Y Target";
        case FreeDrawing:   return "Free Drawing";
        default:
            jassertfalse;
            return "";
    }
}


static String GetProcessTypeName(ProcessType i)
{
    switch(i) {
        case FreeVolum:         return "Free Volume";
        case PanSpan:           return "Pan Span";
        case OSCSpatServer:     return "OSC SpatGrisServer";
        case OSCZirkonium:      return "OSC Zirkonium";
            
        default:
            jassertfalse;
            return "";
    }
}

static String GetPositionSourceSpeakerName(PositionSourceSpeaker i)
{
    switch(i) {
        case LeftAlternate:     return "Left Alternate";
        case LeftClockW:        return "Left Clockwise";
        case LeftCounterClockW: return "Left Counter Clockwise";
        case TopClockW:         return "Top Clockwise";
        case TopCounterClockW:  return "Top Counter Clockwise";
            
        default:
            jassertfalse;
            return "";
    }
}

#endif /* DefaultParam_h */
