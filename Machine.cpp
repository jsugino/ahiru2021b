//
//  Machine.cpp
//
//  Copyright © 2021 Ahiruchan Koubou. All rights reserved.
//

#include "Machine.hpp"

Machine::Machine() {
	printf("Machine constructor\n");

    leftMotor   = new ev3api::Motor(PORT_C);
    rightMotor  = new ev3api::Motor(PORT_B);
    tailMotor   = new ev3api::Motor(PORT_D);
    armMotor    = new ev3api::Motor(PORT_A);

    touchSensor = new ev3api::TouchSensor(PORT_1);
    sonarSensor = new ev3api::SonarSensor(PORT_2);
    colorSensor = new ev3api::ColorSensor(PORT_3);
    gyroSensor  = new ev3api::GyroSensor(PORT_4);
}

Machine::~Machine() {
    delete gyroSensor;
    delete colorSensor;
    delete sonarSensor;
    delete touchSensor;

    delete armMotor;
    delete tailMotor;
    delete rightMotor;
    delete leftMotor;

    printf("Machine destructor\n");
}
