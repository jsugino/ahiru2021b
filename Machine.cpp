//
//  Machine.cpp
//
//  Copyright © 2021 Ahiruchan Koubou. All rights reserved.
//

#include "Machine.hpp"
#include "Logger.hpp"

Machine::Machine() {
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
    speed.reset(0);
    azimuth.reset(0);
}

bool Machine::detect() {
    distanceL = leftMotor->getCount();
    distanceR = rightMotor->getCount();

    return true;
}

void Machine::moveDirect( int forward, int turn )
{
    int pwm_L = forward - turn;
    int pwm_R = forward + turn;
    leftMotor->setPWM(pwm_L);
    rightMotor->setPWM(pwm_R);
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

RampControler::RampControler()
{
    target = 0;
    current = 0;
    counter = 0;
    ratioA = 1;
    ratioB = 1;
}

void RampControler::reset( int cur )
{
    target = cur;
    current = cur;
}

void RampControler::ratio( double ratio )
{
    ratioA = 1;
    ratioB = 1;
    if ( ratio >= 1.0 ) {
	ratioA = ratio;
    } else {
	ratioB = 1.0 / ratio;
    }
}

int RampControler::calc( int newtarget )
{
    if ( target != newtarget ) counter = 0;
    target = newtarget;
    if ( current != target ) {
	++counter;
	if ( current < target ) {
	    current += (counter % ratioB) == 0 ? ratioA : 0;
	    if ( current > target ) current = target;
	} else {
	    current -= (counter % ratioB) == 0 ? ratioA : 0;
	    if ( current < target ) current = target;
	}
    }
    return current;
}

Ramp2Controler::Ramp2Controler()
{
    maxspeed = 0;
}

void Ramp2Controler::reset( int ofs )
{
    speed.reset(0);
    offset = ofs;
}

void Ramp2Controler::resetSpeed( int spd )
{
    speed.reset(spd);
}

void Ramp2Controler::ratio( double ratio, int max )
{
    speed.ratio(ratio);
    maxspeed = max;
}

int Ramp2Controler::calc( int current, int newtarget )
{
    current -= offset;
    if ( current == newtarget ) {
	speed.reset(0);
	return 0;
    }
    int spd;
    if ( newtarget > current ) {
	if ( (newtarget-current) > speed.getCurrent()*speed.getCurrent() ) {
	    spd = speed.calc(-maxspeed);
	} else {
	    spd = speed.calc(0);
	}
    } else {
	if ( (current-newtarget) > speed.getCurrent()*speed.getCurrent() ) {
	    spd = speed.calc(maxspeed);
	} else {
	    spd = speed.calc(0);
	}
    }
    return spd;
}

bool Ramp2Controler::inTarget( int diff )
{
    diff -= offset;
    return (-ERROR <= diff) && (diff <= ERROR);
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
