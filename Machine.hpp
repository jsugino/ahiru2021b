//
//  Machine.hpp
//
//  Copyright © 2021 Ahiruchan Koubou. All rights reserved.
//

#ifndef Machine_hpp
#define Machine_hpp

#include "TouchSensor.h"
#include "SonarSensor.h"
#include "ColorSensor.h"
#include "GyroSensor.h"
#include "Motor.h"
#include "Clock.h"

#include "Logger.hpp"

class AngleMotor : public ev3api::Motor
{
private:
protected:
public:
    inline AngleMotor( ePortM port ) : Motor(port) {};
    void setAngle( int32_t targetAngle );
};

// １次の台形制御のクラス
class RampControler {
private:
    int current;
    int counter;
    int ratioA;
    int ratioB;
public:
    RampControler();
    void reset( int cur );
    void ratio( double ratio );
    int calc( int newtarget );
    int getCurrent() { return current; }
};

// ２次の台形制御のクラス
class Ramp2Controler {
private:
    RampControler speed;
    int maxspeed;
public:
    Ramp2Controler();
    void reset( int speed );
    void resetSpeed( int spd ) { speed.reset(spd); }
    void ratio( double ratio, int max );
    int calc( int target );
};

class Machine {
private:
    ev3api::Motor*          leftMotor;
    ev3api::Motor*          rightMotor;
    rgb_raw_t               cur_rgb;
protected:
public:
    AngleMotor*             armMotor;
    AngleMotor*             tailMotor;

    ev3api::TouchSensor*    touchSensor;
    ev3api::SonarSensor*    sonarSensor;
    ev3api::GyroSensor*     gyroSensor;
    ev3api::ColorSensor*    colorSensor;

    ev3api::Clock*          clock;

    int32_t distanceL, distanceR;

    RampControler           speed;
    Ramp2Controler          azimuth;
    void moveDirect( int forward, int turn );

    rgb_raw_t* getRawRGB();
    int getRGB( int ratioR = 1, int ratioG = 1, int ratioB = 1, bool average = false );

    Machine();
    void initialize();
    bool detect();
    ~Machine();
};

#endif /* Machine_hpp */
