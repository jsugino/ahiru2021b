//
//  Operator.cpp
//
//  Copyright © 2021 Ahiruchan Koubou. All rights reserved.
//

#include "Operator.hpp"
#include "Logger.hpp"
#if 1 /* yamanaka_s */
#include "ahiru_common.hpp"
#endif /* yamanaka_s */

Operator::Operator( Machine* mcn ) {
    log("Operator constructor");
    machine = mcn;
    mode = 0;
#if 1 /* yamanaka_s */
	distance = 0.0;
	prevAngL = prevAngR = 0;
	logCnt = 0;	
#endif /* yamanaka_s */
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
	machine->counter = 0;
    }
}

void Operator::lineTrace()
{
    // Added by Sugino
    rgb_raw_t cur_rgb;
    int16_t grayScaleBlueless;
    int8_t forward;      /* 前後進命令 */
    int8_t turn;         /* 旋回命令 */
    int8_t pwm_L, pwm_R; /* 左右モータPWM出力 */

    // 計算用の一時変数定義
	int32_t curAngL = 0;
	int32_t curAngR = 0;
	double deltaDistL = 0;
	double deltaDistR = 0;
	double deltaDist = 0;

    machine->colorSensor->getRawColor(cur_rgb);
    grayScaleBlueless = (cur_rgb.r * 10 + cur_rgb.g * 217 + cur_rgb.b * 29) / 256;

#if 1 /* yamanaka_s */
    forward = 90;
    turn = (30 - grayScaleBlueless)*EDGE;
	if(100 == logCnt || 0 == logCnt) {
		printf("[Operator::lineTrace]grayScaleBlueless=%d,forward=%d,turn=%d \n",grayScaleBlueless,forward,turn);
	}
#else /* yamanaka_s */
://    forward = 30;
://    turn = (30 - grayScaleBlueless)*EDGE;
#endif /* yamanaka_s */

    pwm_L = forward - turn;
    pwm_R = forward + turn;
#if 1 /* yamanaka_s */
	if(100 == logCnt || 0 == logCnt) {
		printf("[Operator::lineTrace]pwm_L=%d,pwm_R=%d\n",pwm_L,pwm_R);
	}
#endif /* yamanaka_s */

    machine->leftMotor->setPWM(pwm_L);
    machine->rightMotor->setPWM(pwm_R);

#if 1 /* yamanaka_s */
	/* 走行距離を算出する */
	curAngL = machine->leftMotor->getCount();
	curAngR = machine->rightMotor->getCount();
	deltaDistL = M_PI * TIRE_DIAMETER * (curAngL - prevAngL) / 360.0;
	deltaDistR = M_PI * TIRE_DIAMETER * (curAngR - prevAngR) / 360.0;
	deltaDist = (deltaDistL + deltaDistR) / 2.0;

	distance += deltaDist;
	prevAngL = curAngL;
	prevAngR = curAngR;

	if(100 == logCnt) {
		printf("[Operator::lineTrace] distance = %f \n",distance);
		logCnt = 0;
	}
	logCnt++;
	
	/* 走行距離が5000に到達した場合、linTraceを終了後、ショートカット */
	if( 5000 <= distance ) {
		currentMethod = &Operator::shortCut;
	}
	
    if ( mode > 100000 ) {
#else /* yamanaka_s */
    if ( mode > 10000 ) {
#endif /* yamanaka_s */
	currentMethod = NULL;
    } else {
	++mode;
    }
}
#if 1 /* yamanaka_s */
void Operator::shortCut() {
	
}
#endif /* yamanaka_s */

Operator::~Operator() {
    log("Operator destructor");
}
