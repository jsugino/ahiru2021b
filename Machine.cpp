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

    clock       = new ev3api::Clock();

	counter = -1;
    prevAngL = leftMotor->getCount();
    prevAngR = rightMotor->getCount();
}

bool Machine::detect() {
    prevAngL = leftMotor->getCount();
    prevAngR = rightMotor->getCount();
	if ( counter != -1 ) {
		++counter;
		if ( (counter % 250) == 0 ) {
			printf("L = %d, R = %d, counter = %d\n",(int)prevAngL,(int)prevAngR,counter);
		}
	}
	return true;
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
