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
    tailMotor   = new AngleMotor(PORT_D);
    armMotor    = new AngleMotor(PORT_A);

    touchSensor = new ev3api::TouchSensor(PORT_1);
    sonarSensor = new ev3api::SonarSensor(PORT_2);
    colorSensor = new ev3api::ColorSensor(PORT_3);
    gyroSensor  = new ev3api::GyroSensor(PORT_4);

    clock       = new ev3api::Clock();
}

// 各種センサーの初期値を取得する。
void Machine::initialize()
{
    distanceL = leftMotor->getCount();
    distanceR = rightMotor->getCount();
}

bool Machine::detect() {
    distanceL = leftMotor->getCount();
    distanceR = rightMotor->getCount();
    loggerDistL.logging(distanceL);
    loggerDistR.logging(distanceR);

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

void AngleMotor::setAngle( int32_t targetAngle )
{
    int armSpeed = this->getPWM();
    int32_t armAngle = this->getCount();
    if ( armSpeed < 0 ) {
	if ( armAngle <= targetAngle ) armSpeed = 0;
    } else if ( armSpeed > 0 ) {
	if ( armAngle >= targetAngle ) armSpeed = 0;
    } else if ( (targetAngle+3) < armAngle) {
	armSpeed = -30;
    } else if ( (targetAngle-3) > armAngle ) {
	armSpeed = +30;
    }
    this->setPWM(armSpeed);
}
