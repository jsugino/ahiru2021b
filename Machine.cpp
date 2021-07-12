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
    isGetRGB = false;

    return true;
}

void Machine::moveDirect( int forward, int turn )
{
    int pwm_L = forward - turn;
    int pwm_R = forward + turn;
    leftMotor->setPWM(pwm_L);
    rightMotor->setPWM(pwm_R);
}

rgb_raw_t* Machine::getRawRGB()
{
    if ( !isGetRGB ) {
	colorSensor->getRawColor(cur_rgb);
	isGetRGB = true;
    }
    return &cur_rgb;
}

int Machine::getRGB( int ratioR, int ratioG, int ratioB, bool average )
{
    getRawRGB();
    int total = cur_rgb.r * ratioR + cur_rgb.g * ratioG + cur_rgb.b * ratioB;
    if ( average ) total /= ratioR + ratioG + ratioB;
    return total;
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
    current = 0;
    counter = 0;
    ratioA = 1;
    ratioB = 1;
}

void RampControler::reset( int cur )
{
    current = cur;
}

void RampControler::ratio( double ratio )
{
    if ( ratio >= 1.0 ) {
	ratioA = (int)ratio;
	ratioB = 1;
    } else {
	ratioA = 1;
	ratioB = (int)(1.0 / ratio);
    }
}

int RampControler::calc( int target )
{
    ++counter;
    if ( current < target ) {
	current += (counter % ratioB) == 0 ? ratioA : 0;
	if ( current > target ) current = target;
    } else if ( current > target ) {
	current -= (counter % ratioB) == 0 ? ratioA : 0;
	if ( current < target ) current = target;
    }
    return current;
}

Ramp2Controler::Ramp2Controler()
{
    maxspeed = 0;
}

void Ramp2Controler::reset( int spd )
{
    speed.reset(spd);
}

void Ramp2Controler::ratio( double ratio, int max )
{
    speed.ratio(ratio);
    maxspeed = max;
}

int Ramp2Controler::calc( int target )
{
    int spd;
    if ( target == 0 ) {
	speed.reset(0);
	spd = 0;
    } else if ( target > 0 ) {
	if ( target /*(newtarget-current)*/ > speed.getCurrent()*speed.getCurrent() ) {
	    spd = speed.calc(-maxspeed);
	} else {
	    spd = speed.calc(0);
	}
    } else { // target < 0
	if ( -target /*(current-newtarget)*/ > speed.getCurrent()*speed.getCurrent() ) {
	    spd = speed.calc(maxspeed);
	} else {
	    spd = speed.calc(0);
	}
    }
    return spd;
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
