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

class Machine {
private:
protected:
public:
	ev3api::Motor*          leftMotor;
    ev3api::Motor*          rightMotor;
    ev3api::Motor*          armMotor;
    ev3api::Motor*          tailMotor;

    ev3api::TouchSensor*    touchSensor;
    ev3api::SonarSensor*    sonarSensor;
    ev3api::GyroSensor*     gyroSensor;
    ev3api::ColorSensor*    colorSensor;
    
    Machine();
    ~Machine();
};

#endif /* Machine_hpp */
