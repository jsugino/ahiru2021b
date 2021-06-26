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

class Machine {
private:
protected:
public:
    ev3api::Motor*          leftMotor;
    ev3api::Motor*          rightMotor;
    AngleMotor*             armMotor;
    AngleMotor*             tailMotor;

    ev3api::TouchSensor*    touchSensor;
    ev3api::SonarSensor*    sonarSensor;
    ev3api::GyroSensor*     gyroSensor;
    ev3api::ColorSensor*    colorSensor;

    ev3api::Clock*          clock;

    Logger loggerDistL;
    Logger loggerDistR;

    int32_t distanceL, distanceR;

    Machine();
    void initialize();
    bool detect();
    ~Machine();
};

#endif /* Machine_hpp */
