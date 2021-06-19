//
//  Operator.cpp
//
//  Copyright © 2021 Ahiruchan Koubou. All rights reserved.
//

#include "Operator.hpp"
#include "Logger.hpp"
#include "ahiru_common.hpp"
#define LOG 100 

Operator::Operator( Machine* mcn ) {
    log("Operator constructor");
    machine = mcn;
    mode = 0;
    distance = 0.0;
    prevAngL = prevAngR = 0;
    logCnt = 0;	
    courseMapindex =0;
//    currentMethod = &Operator::lineTrace;
    currentMethod = &Operator::startRun;
}

bool Operator::operate()
{
    if ( currentMethod == NULL ) return false;
	this->updateDistance();
    (this->*currentMethod)();
    if(LOG == logCnt){
        logCnt = 0;
    }else{
        logCnt++;
    }

    if ( currentMethod == NULL ) return false;
    return true;
}

/*
void Operator::waitForTouch()
{
    if ( machine->touchSensor->isPressed() ) {
	currentMethod = &Operator::lineTrace;
	machine->counter = 0;
    }
}
*/

void Operator::lineTrace()
{
    // Added by Sugino
    rgb_raw_t cur_rgb;
    int16_t grayScaleBlueless;
    int8_t forward;      /* 前後進命令 */
    int8_t turn;         /* 旋回命令 */
    int8_t pwm_L, pwm_R; /* 左右モータPWM出力 */

    machine->colorSensor->getRawColor(cur_rgb);
    grayScaleBlueless = (cur_rgb.r * 10 + cur_rgb.g * 217 + cur_rgb.b * 29) / 256;
    //grayScaleBlueless = cur_rgb.r;

    forward = 30;
    turn = (30 - grayScaleBlueless)*EDGE;

    /* ログ出力　*/
    if(LOG == logCnt || 0 == logCnt) {
        printf("[Operator::lineTrace]grayScaleBlueless=%d,forward=%d,turn=%d \n",grayScaleBlueless,forward,turn);
    }

    pwm_L = forward - turn;
    pwm_R = forward + turn;

    /* ログ出力　*/
    if(LOG == logCnt || 0 == logCnt) {
        printf("[Operator::lineTrace]pwm_L=%d,pwm_R=%d\n",pwm_L,pwm_R);
    }

    machine->leftMotor->setPWM(pwm_L);
    machine->rightMotor->setPWM(pwm_R);

    /* 走行距離が5000に到達した場合、linTraceを終了後、ショートカット */
    if( 5000 <= distance ) {
        currentMethod = &Operator::shortCut;
    }
	
    if ( mode > 100000 ) {
    currentMethod = NULL;
    } else {
    ++mode;
    }
}
/* コースマップを使って走る */
void Operator::useCourseMap()
{
    // Added by Sugino
    rgb_raw_t cur_rgb;
    int16_t grayScaleBlueless;
    int8_t forward;      /* 前後進命令 */
    int8_t turn;         /* 旋回命令 */
    int8_t pwm_L, pwm_R; /* 左右モータPWM出力 */

//    machine->colorSensor->getRawColor(cur_rgb);
//    grayScaleBlueless = (cur_rgb.r * 10 + cur_rgb.g * 217 + cur_rgb.b * 29) / 256;

    forward = SPEED_BLIND;
//    turn = (30 - grayScaleBlueless)*EDGE;

    if( distance < courseMap[courseMapindex].sectionEnd){
    } else{
        courseMapindex++;
    }
    turn = forward*courseMap[courseMapindex].curvature*EDGE/2;

    /* ログ出力　*/
    if(LOG == logCnt || 0 == logCnt) {
        printf("[Operator::lineTrace]grayScaleBlueless=%d,forward=%d,turn=%d \n",grayScaleBlueless,forward,turn);
    }

    pwm_L = forward - turn;
    pwm_R = forward + turn;

    /* ログ出力　*/
    if(LOG == logCnt || 0 == logCnt) {
        printf("[Operator::lineTrace]pwm_L=%d,pwm_R=%d\n",pwm_L,pwm_R);
    }

    machine->leftMotor->setPWM(pwm_L);
    machine->rightMotor->setPWM(pwm_R);

    /* 走行距離が5000に到達した場合、useCourseMapを終了後、ショートカット */
    if( 5000 <= distance ) {
        currentMethod = &Operator::shortCut;
    }

}
#if 1 /* yamanaka_s */
void Operator::shortCut() {

    // 杉野より、山中さんへ、
    // 以下には、とりあえず lineTrace と同じ動作をするプログラムを入れておきます。
    // 正しく shortCut して、難所に入る直前に slalomOn に制御を渡してください。

    // 【ここから】
    rgb_raw_t cur_rgb;
    machine->colorSensor->getRawColor(cur_rgb);
    int16_t grayScaleBlueless = cur_rgb.r;
    int8_t forward = 50;
    int8_t turn = (50 - grayScaleBlueless)*EDGE;
    int8_t pwm_L = forward - turn;
    int8_t pwm_R = forward + turn;

    machine->leftMotor->setPWM(pwm_L);
    machine->rightMotor->setPWM(pwm_R);
    if ( (machine->distanceL + machine->distanceR) > 28000 ) {
	currentMethod = &Operator::slalomOn;
    }
    // 【ここまで】
}
#endif /* yamanaka_s */

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
void Operator::startRun()
{
    rgb_raw_t cur_rgb;
    int16_t grayScaleBlueless;
    int8_t forward;      /* 前後進命令 */
    int8_t turn;         /* 旋回命令 */
    int8_t pwm_L, pwm_R; /* 左右モータPWM出力 */

    machine->colorSensor->getRawColor(cur_rgb);
    grayScaleBlueless = (cur_rgb.r * 10 + cur_rgb.g * 217 + cur_rgb.b * 29) / 256;
    //grayScaleBlueless = cur_rgb.r;

    forward = 45;
    turn = (45 - grayScaleBlueless)*EDGE;

    /* ログ出力　*/
    if(LOG == logCnt || 0 == logCnt) {
        printf("[Operator::startRun]grayScaleBlueless=%d,forward=%d,turn=%d \n",grayScaleBlueless,forward,turn);
    }

    pwm_L = forward - turn;
    pwm_R = forward + turn;

    /* ログ出力　*/
    if(LOG == logCnt || 0 == logCnt) {
        printf("[Operator::startRun]pwm_L=%d,pwm_R=%d\n",pwm_L,pwm_R);
    }

    machine->leftMotor->setPWM(pwm_L);
    machine->rightMotor->setPWM(pwm_R);

    /* 走行距離が200に到達した場合、useCourseMapへ変更 */
    if( 200 <= distance ) {
        currentMethod = &Operator::useCourseMap;
    }

}
/* 走行距離更新 */
void Operator::updateDistance()
{
    // 計算用の一時変数定義
	int32_t curAngL = 0;
	int32_t curAngR = 0;
	double deltaDistL = 0;
	double deltaDistR = 0;
	double deltaDist = 0;

	/* 走行距離を算出する */
	curAngL = machine->leftMotor->getCount();
	curAngR = machine->rightMotor->getCount();
	deltaDistL = M_PI * TIRE_DIAMETER * (curAngL - prevAngL) / 360.0;
	deltaDistR = M_PI * TIRE_DIAMETER * (curAngR - prevAngR) / 360.0;
	deltaDist = (deltaDistL + deltaDistR) / 2.0;

	distance += deltaDist;
	prevAngL = curAngL;
	prevAngR = curAngR;

	if(LOG == logCnt) {
		printf("[Operator::lineTrace] distance = %f \n",distance);
	}
}

Operator::~Operator() {
    log("Operator destructor");
}
