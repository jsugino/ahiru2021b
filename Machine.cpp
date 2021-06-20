//
//  Machine.cpp
//
//  Copyright © 2021 Ahiruchan Koubou. All rights reserved.
//

#include "Machine.hpp"
#include "Logger.hpp"

Machine::Machine() : loggerDistL("distL"), loggerDistR("distR") {
    log("Machine constructor");

    leftMotor   = new ev3api::Motor(PORT_C);
    rightMotor  = new ev3api::Motor(PORT_B);
    tailMotor   = new ev3api::Motor(PORT_D);
    armMotor    = new ev3api::Motor(PORT_A);

    touchSensor = new ev3api::TouchSensor(PORT_1);
    sonarSensor = new ev3api::SonarSensor(PORT_2);
    colorSensor = new ev3api::ColorSensor(PORT_3);
    gyroSensor  = new ev3api::GyroSensor(PORT_4);

    clock       = new ev3api::Clock();
}

// 各種センサーの初期値を取得する。
void Machine::initialize()
{
    counter = 0;

    distanceL = leftMotor->getCount();
    distanceR = rightMotor->getCount();

    armAngle = armMotor->getCount();
    armDownAngle = targetArmAngle = -50;
    armUpAngle = armDownAngle + 30;
    armSpeed = 0;
}

// Arm related operations
void Machine::armUp()
{
    if ( targetArmAngle != armUpAngle ) {
	log("armUp");
	targetArmAngle = armUpAngle;
    }
}

void Machine::armDown()
{
    if ( targetArmAngle != armDownAngle ) {
	log("armDown");
	targetArmAngle = armDownAngle;
    }
}

bool Machine::detect() {
    distanceL = leftMotor->getCount();
    distanceR = rightMotor->getCount();
    loggerDistL.logging(distanceL);
    loggerDistR.logging(distanceR);

    if ( (counter % 250) == 0 ) {
	log("L = %d, R = %d, arm = %d, target = %d",
	    (int)distanceL,(int)distanceR,(int)armAngle,(int)targetArmAngle);
    }
    ++counter;

    if ( armSpeed < 0 ) {
	if ( armAngle <= targetArmAngle ) armSpeed = 0;
    } else if ( armSpeed > 0 ) {
	if ( armAngle >= targetArmAngle ) armSpeed = 0;
    } else if ( (targetArmAngle+3) < armAngle) {
	armSpeed = -30;
    } else if ( (targetArmAngle-3) > armAngle ) {
	armSpeed = +30;
    }
    armMotor->setPWM(armSpeed);

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

    log("Machine destructor");
}
