//
//  Operator.cpp
//
//  Copyright © 2021 Ahiruchan Koubou. All rights reserved.
//

#include "Operator.hpp"

Operator::Operator( Machine* mcn ) {
	printf("Operator constructor\n");
	machine = mcn;
	mode = 0;
	currentMethod = &Operator::waitForTouch;
}

bool Operator::operate()
{
	if ( currentMethod == NULL ) return false;
	(this->*currentMethod)();
	if ( currentMethod == NULL ) return false;
	return true;
}

void Operator::waitForTouch()
{
	if ( machine->touchSensor->isPressed() ) {
		currentMethod = &Operator::lineTrace;
	}
}

void Operator::lineTrace()
{
    rgb_raw_t cur_rgb;
	int16_t grayScaleBlueless;
    int8_t forward;      /* 前後進命令 */
    int8_t turn;         /* 旋回命令 */
    int8_t pwm_L, pwm_R; /* 左右モータPWM出力 */

    machine->colorSensor->getRawColor(cur_rgb);
	grayScaleBlueless = (cur_rgb.r * 10 + cur_rgb.g * 217 + cur_rgb.b * 29) / 256;

	forward = 30;
	turn = (30 - grayScaleBlueless)*EDGE;

    pwm_L = forward - turn;
    pwm_R = forward + turn;

    machine->leftMotor->setPWM(pwm_L);
    machine->rightMotor->setPWM(pwm_R);

	if ( mode > 10000 ) {
		currentMethod = NULL;
	} else {
		++mode;
	}
}

Operator::~Operator() {
    printf("Operator destructor\n");
}
