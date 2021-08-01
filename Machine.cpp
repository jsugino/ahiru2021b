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
    sonarDist = sonarSensor->getDistance();
    logging("rgbR",cur_rgb.r);
    logging("rgbG",cur_rgb.g);
    logging("rgbB",cur_rgb.b);
    logging("distL",distanceL);
    logging("distR",distanceR);
    logging("distS",sonarDist);

    return true;
}

int Machine::moveDirect( int forward, int turn )
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
    return turn;
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

    printf("Machine destructor\n");
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

int RampControler::calc( int target, int count )
{
    for ( int i = 0; i < count; ++i ) calcSingle(target);
    return current;
}

void RampControler::calcSingle( int target )
{
    ++counter;
    if ( current < target ) {
	current += (counter % ratioB) == 0 ? ratioA : 0;
	if ( current > target ) current = target;
    } else if ( current > target ) {
	current -= (counter % ratioB) == 0 ? ratioA : 0;
	if ( current < target ) current = target;
    }
}

// 階段状に変換する量の二乗に対する積分値の平均計算
int calcInteg( int x, int r )
{
    return x * (r*3+1) / 3;
}

int calcIntegRev( int x, int r )
{
    return x * 3 / (r*3+1);
}

int RampControler::calcRatio( int x )
{
    return calcInteg(x,ratioB) / ratioA;
}

Ramp2Controler::Ramp2Controler()
{
    maxspeed = 0;
    toZero = 0;
}

void Ramp2Controler::ratio( double ratio, int max )
{
    speed.ratio(ratio);
    maxspeed = max;
}

// 以下のパラメーターは発見的に見つけている。

// パラメーターの調整方法
// 回転しすぎ → 大きくする
// 回転不足   → 小さくする
// 速度高が回転しすぎ、速度低が回転不足 → RATIO1:大きく、RATIO2:小さく
// 速度低が回転しすぎ、速度高が回転不足 → RATIO1:小さく、RATIO2:大きく
// 次のどちらか(もしくは、その中間)ぐらいが良さそう
// RATIO1 = 0.25, RATIO2 = 0.0
// RATIO1 = 0.24, RATIO2 = 0.1
// RATIO1 = 0.20, RATIO2 = 1.0
#define RAMP2RATIO1 1/4
#define RAMP2RATIO2 0

// ゼロに近づいて来たときに微調整するためのパラメータ
// mode = 1 : ゆるやかな傾斜角
//        2 : 通常の傾斜角
//        3 : きつい傾斜角
// THRE2to1 : 通常の傾斜角から、ゆるやかな傾斜角にするときの比率 ( > 1 )
// THRE1to2 : ゆるやかな傾斜角から、通常の傾斜角に戻すときの比率 ( < 1 )
// THRE2to3 : 通常の傾斜角から、きつい傾斜角にするときの比率 ( < 1 )
// THRE3to2 : きつい傾斜角から、通常の傾斜角に戻すときの比率 ( > 1 )
#define RAMP2THRE2to1 12/4
#define RAMP2THRE1to2 3/4
#define RAMP2THRE2to3 3/4
#define RAMP2THRE3to2 5/4

int Ramp2Controler::calc( int target )
{
    int spd;
    int cur = speed.getCurrent();
    if ( cur < 0 ) cur = -cur;
    int threshold = cur*cur*RAMP2RATIO1 + cur*RAMP2RATIO2;
    threshold = calcIntegRev(speed.calcRatio(threshold),10); // 標準が ratio = 1/10 として計算
    int mode = toZero < 0 ? -toZero : toZero;
    int sign = target < 0 ? -1 : 1;
    if ( toZero != 0 ) {
	if ( target*toZero <= 0 ) {
	    speed.reset(0);
	    spd = 0;
	    toZero = 0;
	} else {
	    switch ( mode ) {
	    case 1:
		if ( target*sign < threshold*RAMP2THRE1to2 ) mode = 2;
		break;
	    case 2:
		if ( target*sign > threshold*RAMP2THRE2to1 ) mode = 1;
		if ( target*sign < threshold*RAMP2THRE2to3 ) mode = 3;
		break;
	    case 3:
		if ( target*sign > threshold*RAMP2THRE3to2 ) mode = 2;
		break;
	    }
	    spd = speed.calc(0,mode-1);
	    toZero =
		( spd == 0 ) ? 0 :
		( toZero < 0 ) ? -mode : mode;
	}
    } else if ( target == 0 ) { // toZero == 0
	speed.reset(0);
	spd = 0;
    } else {
	if ( target*sign > threshold ) {
	    spd = speed.calc(-maxspeed*sign);
	    toZero = 0;
	} else {
	    spd = speed.calc(0);
	    toZero = sign*2;
	}
    }
    logging("mode",toZero);
    logging("thre",(int32_t)threshold);
    return spd;
}

/*
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
		mode = 2;
	    } else {
		spd = speed.calc(0);
		mode = 1;
	    }
	} else { // target < 0
	    if ( -target > threshold ) {
		spd = speed.calc(maxspeed);
		mode = -2;
	    } else {
		spd = speed.calc(0);
		mode = -1;
	    }
	}
    }
    logging("mode",mode);
    return spd;
}
*/

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
