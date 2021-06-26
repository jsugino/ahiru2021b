//
//  Operator.cpp
//
//  Copyright © 2021 Ahiruchan Koubou. All rights reserved.
//

#include "Operator.hpp"
#include "Logger.hpp"
#include "ahiru_common.hpp"
#define LOG 100 
#define CHANGE_BLINDRUNNER 5000
#define CHANGE_LINETRACE 7150

Operator::Operator( Machine* mcn ) {
    log("Operator constructor");
    machine = mcn;
    distance = 0.0;
    prevAngL = prevAngR = 0;
    logCnt = 0;	
    courseMapindex =0;
    slalomStatus = 0;
    currentMethod = &Operator::lineTrace;
//    currentMethod = &Operator::slalomOn;
//    currentMethod = &Operator::startRun;
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

    int32_t  dist = machine->distanceL + machine->distanceR;
    int8_t speed;
    if ( dist < 800 ) speed = 40;
    else if ( dist < 4600 ) speed = 70;
    else if ( dist < 6200 ) speed = 50;
    else if ( dist < 8000 ) speed = 70;
    else if ( dist < 9300 ) speed = 40;
    else speed = 70;

    forward = speed;
    turn = (speed - grayScaleBlueless)*EDGE;

    /* ログ出力　*/
    if(LOG == logCnt || 0 == logCnt) {
        log("[Operator::lineTrace]grayScaleBlueless=%d,forward=%d,turn=%d",grayScaleBlueless,forward,turn);
    }

    if ( forward+turn >= 100 ) turn = 100-forward;
    if ( forward-turn >= 100 ) turn = forward-100;
    pwm_L = forward - turn;
    pwm_R = forward + turn;

    /* ログ出力　*/
    if(LOG == logCnt || 0 == logCnt) {
        log("[Operator::lineTrace]pwm_L=%d,pwm_R=%d",pwm_L,pwm_R);
    }

    machine->leftMotor->setPWM(pwm_L);
    machine->rightMotor->setPWM(pwm_R);
	
    /* 走行距離が5000に到達した場合、blindRunnerへ遷移 */
    if( CHANGE_LINETRACE > distance ) {
        if( CHANGE_BLINDRUNNER <= distance ) {
            currentMethod = &Operator::blindRunner;
        }
    }
}
/* コースマップを使って走る */
void Operator::blindRunner()
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
        log("[Operator::blindRunner]grayScaleBlueless=%d,forward=%d,turn=%d",grayScaleBlueless,forward,turn);
    }

    pwm_L = forward - turn;
    pwm_R = forward + turn;

    /* ログ出力　*/
    if(LOG == logCnt || 0 == logCnt) {
        log("[Operator::blindRunner]pwm_L=%d,pwm_R=%d",pwm_L,pwm_R);
    }

    machine->leftMotor->setPWM(pwm_L);
    machine->rightMotor->setPWM(pwm_R);

    /* 走行距離がXXXXに到達した場合、lineTraceへ遷移 */
    if( CHANGE_LINETRACE <= distance ) {
        currentMethod = &Operator::lineTrace;
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
    int8_t forward = 60;
    int8_t turn = (60 - grayScaleBlueless)*EDGE;
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

int16_t calcTurn( int16_t turn, int16_t forward )
{
    int16_t turnmax;

    turn = (int16_t)(((double)turn)*((double)forward)/40.0);
    turnmax = ev3api::Motor::PWM_MAX - forward;
    if ( turn < -turnmax ) turn = -turnmax;
    if ( turn > +turnmax ) turn = +turnmax;

    return turn;
}

struct Slalom {
    int32_t distance;
    int16_t turn;
};

void Operator::slalomOn()
{
    rgb_raw_t cur_rgb;
    int32_t  distance = machine->distanceL + machine->distanceR;

#if 1
    // 以下のコードは難所のトライ＆エラーをするためのもの
    // 次のコマンドを実行し、少しだけ lineTrace する。
    // curl -X POST -H "Content-Type: application/json" -d '{"initLX":"8.0","initLY":"0","initLZ":"15.5","initLROT":"90"}' http://localhost:54000
    if ( distance < 1000 ) {
	int16_t forward = 30;
	machine->colorSensor->getRawColor(cur_rgb);
	int16_t turn = calcTurn(60-cur_rgb.r,forward)*EDGE;
	int8_t pwm_L = forward - turn;
	int8_t pwm_R = forward + turn;
	machine->leftMotor->setPWM(pwm_L);
	machine->rightMotor->setPWM(pwm_R);
	return;
    }
#endif

#if 0
    // 初期状態から lineTrace と同等の実装をして、難所までたどり着く
    // 山中さんのコードができたら、以下は削除する。
    if ( distance < 20000 ) {
	int16_t forward;      /* 前後進命令 */

	if ( distance < 800 ) forward = 30;
	else if ( distance < 4600 ) forward = 70;
	else if ( distance < 6200 ) forward = 50;
	else if ( distance < 8000 ) forward = 70;
	else if ( distance < 9300 ) forward = 40;
	else if ( distance < 13700 ) forward = 75;
	else if ( distance < 14400+1000 ) forward = 50;
	else if ( distance < 17300 ) forward = 75;
	else if ( distance < 19100 ) forward = 40;
	else if ( distance < 20000 ) forward = 30;
	else forward = 10;

	int edge;
	if ( distance < 18200 ) {
	    edge = -EDGE; // 逆エッジ
	} else if ( distance < 18480 ) {
	    // この期間だけ直進
	    machine->leftMotor->setPWM(forward);
	    machine->rightMotor->setPWM(forward);
	    return;
	} else {
	    edge = EDGE; // 正エッジ
	}
	machine->colorSensor->getRawColor(cur_rgb);
	int16_t turn = calcTurn(60-cur_rgb.r,forward)*edge;
	int8_t pwm_L = forward - turn;
	int8_t pwm_R = forward + turn;
	machine->leftMotor->setPWM(pwm_L);
	machine->rightMotor->setPWM(pwm_R);
	return;
    }
    // ここまで。
#endif

    int16_t forward = 10;
    int16_t turn = 0;
    int32_t base;

    switch ( slalomStatus ) {
    case 0: // initial state
	slalomCounter = 0;
	++slalomStatus;
	log("[Operator::slalomOn] 姿勢を揃える");
	break;
    case 1: // 板にぶつかり続けて、姿勢を揃える
	if ( ++slalomCounter < 1000 ) {
	    // Do Nothing
	} else {
	    ++slalomStatus;
	    slalomCounter = 0;
	    log("[Operator::slalomOn] ゆっくり停止する");
	}
	break;
    case 2: // ゆっくり停止する
	if ( ++slalomCounter < 500 ) {
	    forward -= (slalomCounter/10);
	    if ( forward < 0 ) forward = 0;
	} else {
	    ++slalomStatus;
	    slalomDistance = distance;
	    slalomCounter = 0;
	    log("[Operator::slalomOn] ゆっくりバックする");
	}
	break;
    case 3: // ゆっくりバックする
	if ( (distance-slalomDistance) > -300 ) {
	    ++slalomCounter;
	    forward = -(slalomCounter/10);
	    if ( forward < -10 ) forward = -10;
	} else {
	    ++slalomStatus;
	    slalomCounter = 0;
	    log("[Operator::slalomOn] アームを上げつつつ前進する");
	}
	break;
    case 4: // アームを上げつつつ前進する
	if ( (distance-slalomDistance) < 300 ) {
	    machine->armMotor->setAngle(-20);
	    ++slalomCounter;
	    forward = (slalomCounter/10);
#define ARMUPSPEED 50
	    if ( forward > ARMUPSPEED ) forward = ARMUPSPEED;
	} else {
	    ++slalomStatus;
	    slalomCounter = 0;
	    log("[Operator::slalomOn] アームをおろしつつ減速しライントレースする");
	}
	break;
    case 5: // アームをおろしつつ減速しライントレースする
#define ARMDOWNDIST 900
	if ( (distance-slalomDistance) < ARMDOWNDIST ) {
	    machine->armMotor->setAngle(-50);
	    ++slalomCounter;
	    forward = ARMUPSPEED - (slalomCounter/10);
	    if ( forward < 10 ) forward = 10;
	    machine->colorSensor->getRawColor(cur_rgb);
	    turn = calcTurn(30-cur_rgb.r,forward)*EDGE;
	} else {
	    ++slalomStatus;
	    slalomCounter = 0;
	    log("[Operator::slalomOn] スラロームする");
	}
	break;
    case 6: // スラロームする
	base = ARMDOWNDIST;
	if ( (distance-slalomDistance) < 3000 ) {
	    struct Slalom slalom[] = {
		{ 100, -15 },
		{ 150, 0 },
		{ 100, 15 },
		{ 400, 0 },
		{ 100, 15 },
		{ 350, 0 },
		{ 100, -15 },
		{ 200, 0 },
		{ 100, -15 },
		{ 300, 0 },
		{ 100, 15 },
	    };
	    for ( int i = 0; i < (sizeof(slalom)/sizeof(Slalom)); ++i ) {
		base += slalom[i].distance;
		if ( (distance-slalomDistance) < base ) {
		    turn = slalom[i].turn;
		    break;
		}
	    }
	} else {
	    ++slalomStatus;
	    slalomCounter = 0;
	    log("[Operator::slalomOn] スラローム前半終了");
	}
	break;
    case 7: // スラローム前半終了
	slalomStatus = 0;
	currentMethod = &Operator::slalomOff;
	return;
    }
    int8_t pwm_L = forward - turn;
    int8_t pwm_R = forward + turn;
    machine->leftMotor->setPWM(pwm_L);
    machine->rightMotor->setPWM(pwm_R);
}

void Operator::slalomOff()
{
    int16_t forward = 10;
    int16_t turn = 0;
    int32_t base;
    rgb_raw_t cur_rgb;

    int32_t  distance = machine->distanceL + machine->distanceR;

    switch ( slalomStatus ) {
    case 0: // initial state
	slalomCounter = 0;
	++slalomStatus;
	log("[Operator::slalomOff] 黒線を探す");
	break;
    case 1: // 黒線を探す
	machine->colorSensor->getRawColor(cur_rgb);
	if ( (cur_rgb.r + cur_rgb.g + cur_rgb.b) < 30 ) {
	    ++slalomStatus;
	    log("[Operator::slalomOff] 90度回転する");
	    slalomCounter = machine->distanceL;
	}
	break;
    case 2: // 90度回転する
	if ( (machine->distanceL - slalomCounter) < 15 ) {
	    forward = 0;
	    turn = -5;
	} else {
	    slalomCounter = 0;
	    ++slalomStatus;
	    log("[Operator::slalomOff] ライントレースする");
	}
	break;
    case 3: // ライントレースする
	if ( (distance-slalomDistance) < 5000 ) {
	    machine->colorSensor->getRawColor(cur_rgb);
	    turn = calcTurn(30-cur_rgb.r,forward)*EDGE;
	} else {
	    ++slalomStatus;
	    slalomCounter = 0;
	    log("[Operator::slalomOn] スラローム後半終了");
	}
	break;
    case 4: // スラローム後半終了
	slalomStatus = 0;
	currentMethod = NULL;
	return;
    }
    int8_t pwm_L = forward - turn;
    int8_t pwm_R = forward + turn;
    machine->leftMotor->setPWM(pwm_L);
    machine->rightMotor->setPWM(pwm_R);
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

    forward = 35;
    turn = (35 - grayScaleBlueless)*EDGE;

    /* ログ出力　*/
    if(LOG == logCnt || 0 == logCnt) {
        log("[Operator::startRun]grayScaleBlueless=%d,forward=%d,turn=%d",grayScaleBlueless,forward,turn);
    }

    pwm_L = forward - turn;
    pwm_R = forward + turn;

    /* ログ出力　*/
    if(LOG == logCnt || 0 == logCnt) {
        log("[Operator::startRun]pwm_L=%d,pwm_R=%d",pwm_L,pwm_R);
    }

    machine->leftMotor->setPWM(pwm_L);
    machine->rightMotor->setPWM(pwm_R);

    /* 走行距離が200に到達した場合、blindRunnerへ変更 */
    if( 200 <= distance ) {
        currentMethod = &Operator::blindRunner;
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
	    log("[Operator::updateDistance] distance = %f",distance);
	}
}

Operator::~Operator() {
    log("Operator destructor");
}
