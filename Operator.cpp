//
//  Operator.cpp
//
//  Copyright © 2021 Ahiruchan Koubou. All rights reserved.
//

#include "Operator.hpp"
#include "Logger.hpp"

Operator::Operator( Machine* mcn ) {
    log("Operator constructor");
    machine = mcn;
    currentMethod = &Operator::lineTrace;
}

bool Operator::operate()
{
    if ( currentMethod == NULL ) return false;
    (this->*currentMethod)();
    if ( currentMethod == NULL ) return false;
    return true;
}

void Operator::lineTrace()
{
    rgb_raw_t cur_rgb;
    int16_t grayScaleBlueless;
    int8_t forward;      /* 前後進命令 */
    int8_t turn;         /* 旋回命令 */
    int8_t pwm_L, pwm_R; /* 左右モータPWM出力 */

    machine->colorSensor->getRawColor(cur_rgb);
    //grayScaleBlueless = (cur_rgb.r * 10 + cur_rgb.g * 217 + cur_rgb.b * 29) / 256;
    grayScaleBlueless = cur_rgb.r;

    forward = 50;
    turn = (50 - grayScaleBlueless)*EDGE;

    pwm_L = forward - turn;
    pwm_R = forward + turn;

    machine->leftMotor->setPWM(pwm_L);
    machine->rightMotor->setPWM(pwm_R);

    if ( (machine->distanceL + machine->distanceR) > 28000 ) {
	log("change to slalomOn");
	currentMethod = &Operator::slalomOn;
    }
}

void Operator::slalomOn()
{
    machine->leftMotor->setPWM(50);
    machine->rightMotor->setPWM(50);
    if ( (machine->distanceL + machine->distanceR) < 28800 ) {
	machine->armUp();
    } else {
	machine->armDown();
    }

    if ( (machine->distanceL + machine->distanceR) > 31000 ) {
	currentMethod = NULL;
    }
}

Operator::~Operator() {
    log("Operator destructor");
}
