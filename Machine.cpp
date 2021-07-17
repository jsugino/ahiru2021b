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
    azimuth.setSpeed(0);
}

bool Machine::detect() {
    distanceL = leftMotor->getCount();
    distanceR = rightMotor->getCount();
    colorSensor->getRawColor(cur_rgb);
    logging("rgbR",cur_rgb.r);
    logging("rgbG",cur_rgb.g);
    logging("rgbB",cur_rgb.b);
    logging("distL",distanceL);
    logging("distR",distanceR);

    return true;
}

void Machine::moveDirect( int forward, int turn )
{
    int turnmax = ev3api::Motor::PWM_MAX - (forward > 0 ? forward : -forward);
    if ( turn < -turnmax ) turn = -turnmax;
    if ( turn > +turnmax ) turn = +turnmax;

    int pwm_L = forward - turn;
    int pwm_R = forward + turn;
    logging("forward",forward);
    logging("turn",turn);
    leftMotor->setPWM(pwm_L);
    rightMotor->setPWM(pwm_R);
}

rgb_raw_t* Machine::getRawRGB()
{
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

void Ramp2Controler::ratio( double ratio, int max )
{
    speed.ratio(ratio);
    maxspeed = max;
}

// このパラメーターは発見的に見つけている。
#define RAMP2RATIO1 0.3
#define RAMP2RATIO2 1.0

int Ramp2Controler::calc( int target )
{
    int spd;
    int mode = 0;
    if ( target == 0 ) {
	speed.reset(0);
	spd = 0;
    } else {
	double cur = (double)speed.getCurrent();
	int threshold = cur * ( cur * RAMP2RATIO1 + RAMP2RATIO2 );
	if ( target > 0 ) {
	    if ( target > threshold ) {
		spd = speed.calc(-maxspeed);
		mode = 1;
	    } else {
		spd = speed.calc(0);
		mode = 2;
	    }
	} else { // target < 0
	    if ( -target > threshold ) {
		spd = speed.calc(maxspeed);
		mode = 3;
	    } else {
		spd = speed.calc(0);
		mode = 4;
	    }
	}
    }
    logging("AngMode",mode);
    logging("AngSpd",spd);
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
