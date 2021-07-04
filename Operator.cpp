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

RampControler::RampControler( double ratio )
{
    target = 0;
    current = 0;
    counter = 0;
    ratioA = 1;
    ratioB = 1;
    if ( ratio >= 1.0 ) {
	ratioA = ratio;
    } else {
	ratioB = 1.0 / ratio;
    }
}

int16_t RampControler::calc( int16_t newtarget )
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

void RampControler::reset( int16_t tar, int16_t cur )
{
    target = tar;
    current = cur;
}

Operator::Operator( Machine* mcn ) : speed(0.1),azimuth(0.5) {
    log("Operator constructor");
    machine = mcn;
    distance = 0.0;
    prevAngL = prevAngR = 0;
    logCnt = 0;	
    courseMapindex =0;
    slalomStatus = 0;
    currentMethod = &Operator::lineTrace; // 通常走行から固定走行をする。
    //currentMethod = &Operator::lineTraceDummy; // 通常走行のみ版
    //currentMethod = &Operator::slalomOnPrep; // 難所「板の前半」攻略用
    //currentMethod = &Operator::slalomOff; // 難所「板の後半」攻略用
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

int16_t calcTurn( int16_t turn, int16_t forward )
{
    int16_t turnmax;

    turn = (int16_t)(((double)turn)*((double)forward)/40.0);
    turnmax = ev3api::Motor::PWM_MAX - forward;
    if ( turn < -turnmax ) turn = -turnmax;
    if ( turn > +turnmax ) turn = +turnmax;
    if ( forward < 0 ) turn = -turn;

    return turn;
}

struct Slalom {
    int32_t distance;
    int16_t angle;
};

// 目的：山中さんの固定走行プログラムができるまでの通常走行のみ版
// 処理内容：初期状態から、通常どおりライントレースし難所直前までたどり着く
// ロボの初期位置：初期状態
void Operator::lineTraceDummy()
{
    rgb_raw_t cur_rgb;
    int32_t  distance = machine->distanceL + machine->distanceR;

    // 最初から左エッジでライントレースする。
    // 山中さんのコードができたら、削除する。
    if ( distance < 28500 ) {
	int16_t forward;

	if ( distance < 4600-500 ) forward = 80;
	else if ( distance < 6100 ) forward = 50; // 第1カーブ
	else if ( distance < 7750 ) forward = 70;
	else if ( distance < 9100 ) forward = 50; // 第2カーブ
	else if ( distance < 10200 ) forward = 70;
	else if ( distance < 11150 ) forward = 50; // 第3カーブ
	else if ( distance < 12700 ) forward = 70;
	else if ( distance < 13800 ) forward = 40; // 第4カーブ (きつい)
	else if ( distance < 14600 ) forward = 70;
	else if ( distance < 15450 ) forward = 40; // 第5カーブ (きつい)
	else if ( distance < 17100 ) forward = 70;
	else if ( distance < 17600 ) forward = 50; // 第6カーブ
	else if ( distance < 18400 ) forward = 70;
	else if ( distance < 20000+500 ) forward = 50; // 第7カーブ
	else if ( distance < 25350-500 ) forward = 80;
	else if ( distance < 27350 ) forward = 50; // 第8カーブ
	else forward = 50;

	forward = speed.calc(forward);

	machine->colorSensor->getRawColor(cur_rgb);
	int16_t turn = calcTurn(60-cur_rgb.r,forward)*EDGE;
	int8_t pwm_L = forward - turn;
	int8_t pwm_R = forward + turn;
	machine->leftMotor->setPWM(pwm_L);
	machine->rightMotor->setPWM(pwm_R);
    } else {
	slalomStatus = 0;
	currentMethod = &Operator::slalomOn;
    }
}

// 目的：難所「板の前半」のトライ＆エラーをするためのもの
// 処理内容：難所の直前から、板にぶつかるまでライントレースする。
// ロボの初期位置：次のコマンドでを難所の直前に配置する。
// curl -X POST -H "Content-Type: application/json" -d '{"initLX":"8.0","initLY":"0","initLZ":"15.5","initLROT":"90"}' http://localhost:54000
void Operator::slalomOnPrep()
{
    rgb_raw_t cur_rgb;
    int32_t  distance = machine->distanceL + machine->distanceR;

    if ( distance < 1000 ) {
	int16_t forward = 30;
	machine->colorSensor->getRawColor(cur_rgb);
	int16_t turn = calcTurn(60-cur_rgb.r,forward)*EDGE;
	int8_t pwm_L = forward - turn;
	int8_t pwm_R = forward + turn;
	machine->leftMotor->setPWM(pwm_L);
	machine->rightMotor->setPWM(pwm_R);
    } else {
	slalomStatus = 0;
	currentMethod = &Operator::slalomOn;
    }
}

// 目的：逆エッジを使った走行を試して見るためのもの
// 処理内容：初期状態から、逆エッジを使ってライントレースし難所直前までたどり着く
// ロボの初期位置：初期状態
void Operator::reverseEdge()
{
    rgb_raw_t cur_rgb;
    int32_t  distance = machine->distanceL + machine->distanceR;

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
    } else {
	slalomStatus = 0;
	currentMethod = &Operator::slalomOn;
    }
}

void Operator::slalomOn()
{
    rgb_raw_t cur_rgb;
    int32_t  distance = machine->distanceL + machine->distanceR;
    int32_t  angle = machine->distanceL - machine->distanceR;

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
#define PUSHSPEED 10
	    forward = PUSHSPEED;
	} else {
	    ++slalomStatus;
	    slalomCounter = 0;
	    speed.reset(PUSHSPEED,PUSHSPEED);
	    log("[Operator::slalomOn] 停止する");
	}
	break;
    case 2: // 停止する
	if ( ++slalomCounter < 500 ) {
	    forward = speed.calc(0);
	} else {
	    ++slalomStatus;
	    slalomDistance = distance; // 距離のリセット
	    slalomCounter = 0;
	    log("[Operator::slalomOn] バックする");
	}
	break;
    case 3: // バックする
	if ( (distance-slalomDistance) > -300 ) {
	    forward = speed.calc(-10);
	} else {
	    ++slalomStatus;
	    slalomCounter = 0;
	    log("[Operator::slalomOn] アームを上げつつつ前進する");
	}
	break;
    case 4: // アームを上げつつつ前進する
	if ( (distance-slalomDistance) < 300 ) {
	    machine->armMotor->setAngle(-20);
	    forward = speed.calc(50);
	} else {
	    ++slalomStatus;
	    slalomCounter = 0;
	    log("[Operator::slalomOn] アームをおろしつつ減速しライントレースする");
	}
	break;
    case 5: // アームをおろしつつ減速しライントレースする
#define SLALOMDIST 950
	if ( (distance-slalomDistance) < SLALOMDIST ) {
	    machine->armMotor->setAngle(-50);
	    forward = speed.calc(10);
	    machine->colorSensor->getRawColor(cur_rgb);
	    turn = calcTurn(30-cur_rgb.r,forward)*EDGE;
	} else {
	    ++slalomStatus;
	    slalomCounter = 0;
	    log("[Operator::slalomOn] 逆走しつつライントレースする");
	}
	break;
    case 6: // 逆走しつつライントレースする
#define BACKDIST 650
	if ( (distance-slalomDistance) > BACKDIST ) {
	    forward = speed.calc(-10);
	    machine->colorSensor->getRawColor(cur_rgb);
	    turn = calcTurn(30-cur_rgb.r,forward)*EDGE;
	} else {
	    ++slalomStatus;
	    slalomCounter = 0;
	    log("[Operator::slalomOn] 前進しつつライントレースする");
	}
	break;
    case 7: // 前進しつつライントレースする
	if ( (distance-slalomDistance) < SLALOMDIST ) {
	    forward = speed.calc(10);
	    machine->colorSensor->getRawColor(cur_rgb);
	    turn = calcTurn(30-cur_rgb.r,forward)*EDGE;
	} else {
	    slalomAngle = angle; // 角度のリセット
	    azimuth.reset(0,0);
	    ++slalomStatus;
	    slalomCounter = 0;
	    log("[Operator::slalomOn] スラロームする");
	}
	break;
    case 8: // スラロームする
	base = SLALOMDIST;
	if ( (distance-slalomDistance) < 2900 ) {
	    struct Slalom slalom[] = { /* ★距離と方角(260 count = 90度)を調整する */
		{ 330, 120 },  // 右に移動
		{ 240, 0 },    // 直進
		{ 520, -120 }, // 左に移動
		{ 300, 0 },    // 直進
		{ 430, 120 },  // 右に移動
		{9999, 0 },    // 直進
	    };
	    for ( int i = 0; i < (sizeof(slalom)/sizeof(Slalom)); ++i ) {
		base += slalom[i].distance;
		if ( (distance-slalomDistance) < base ) {
		    turn = azimuth.calc(slalom[i].angle-(angle-slalomAngle))*EDGE/-4;
		    break;
		}
	    }
	} else {
	    ++slalomStatus;
	    slalomCounter = 0;
	    log("[Operator::slalomOn] スラローム前半終了");
	}
	break;
    case 9: // スラローム前半終了
	slalomStatus = 0;
	currentMethod = &Operator::slalomOff;
	return;
    }
    int8_t pwm_L = forward - turn;
    int8_t pwm_R = forward + turn;
    machine->leftMotor->setPWM(pwm_L);
    machine->rightMotor->setPWM(pwm_R);
}

// slalomOff() からスタートする場合は、次のコマンドを実行する。
// 左コース用：
// curl -X POST -H "Content-Type: application/json" -d '{"initLX":"22.0","initLY":"0.1","initLZ":"15.2","initLROT":"90"}' http://localhost:54000
// 右コース用：
// curl -X POST -H "Content-Type: application/json" -d '{"initRX":"-22.0","initRY":"0.1","initRZ":"15.2","initRROT":"270"}' http://localhost:54000
void Operator::slalomOff()
{
    int16_t forward = 10;
    int16_t turn = 0;
    int32_t base;
    rgb_raw_t cur_rgb;

    int32_t  distance = machine->distanceL + machine->distanceR;
    int32_t  angle = machine->distanceL - machine->distanceR;

    switch ( slalomStatus ) {
    case 0: // initial state
	slalomCounter = 0;
	++slalomStatus;
	log("[Operator::slalomOff] 少し左回転");
	break;
    case 1: // 少し左回転
#define ABITANGLE -50
	if ( (angle-slalomAngle) > (ABITANGLE+5) ) {
	    turn = azimuth.calc(ABITANGLE-(angle-slalomAngle))*EDGE/-4;
	} else {
	    ++slalomStatus;
	    slalomCounter = 0;
	    log("[Operator::slalomOff] 黒線を探す");
	}
	break;
    case 2: // 黒線を探す
	machine->colorSensor->getRawColor(cur_rgb);
	if ( (cur_rgb.r + cur_rgb.g + cur_rgb.b) < 30 ) {
	    ++slalomStatus;
	    slalomCounter = 0;
	    log("[Operator::slalomOff] そのまま少し進む");
	}
	break;
    case 3: // そのまま少し進む
	if ( ++slalomCounter > 50 ) {
	    ++slalomStatus;
	    slalomCounter = 0;
	    log("[Operator::slalomOff] ライントレースしつつ黒線が切れるところを探す");
	}
	break;
    case 4: // ライントレースしつつ黒線が切れるところを探す
	machine->colorSensor->getRawColor(cur_rgb);
	turn = calcTurn(30-cur_rgb.r,forward)*EDGE;
	if ( turn < -10 ) {
	    ++slalomStatus;
	    slalomCounter = 0;
	    slalomDistance = distance; // 距離をリセットする
	    speed.reset(forward,forward);
	    log("[Operator::slalomOff] 少しバックしつつ角度調整する");
	}
	break;
    case 5: // 少しバックする
	if ( (distance-slalomDistance) > -100/*★*/ ) { // どのくらいバックするのか
	    forward = speed.calc(-10);
	} else {
	    ++slalomStatus;
	    slalomCounter = 0;
	    azimuth.reset(0,0);
	    log("[Operator::slalomOn] 約90度回転する");
	}
	break;
    case 6: // 約90度回転する
	if ( (angle-slalomAngle) < 260 ) { // 260 count = 90度
	    forward = speed.calc(0);
	    turn = azimuth.calc(-5)*EDGE;
	} else {
	    slalomCounter = 0;
	    ++slalomStatus;
	    log("[Operator::slalomOff] スラロームする");
	}
	break;
    case 7: // スラロームする
	base = 0;
	if ( (distance-slalomDistance) < 1000 ) {
	    struct Slalom slalom[] = { /* ★距離と方角(260 count = 90度)を調整する */
		{ 350, 260+0 },   // 直進
		{ 500, 260+150 }, // 右に移動
		{9999, 260+0 },   // 直進
	    };
	    for ( int i = 0; i < (sizeof(slalom)/sizeof(Slalom)); ++i ) {
		base += slalom[i].distance;
		if ( (distance-slalomDistance) < base ) {
		    turn = azimuth.calc(slalom[i].angle-(angle-slalomAngle))*EDGE/-4;
		    break;
		}
	    }
	} else {
	    ++slalomStatus;
	    slalomCounter = 0;
	    log("[Operator::slalomOn] ライントレースせずに加速してアームを上げる");
	}
	break;
    case 8: // ライントレースせずに加速してアームを上げる
	if ( (distance-slalomDistance) < 1500 ) {
	    machine->armMotor->setAngle(-20);
	    forward = speed.calc(30);
	} else {
	    ++slalomStatus;
	    slalomCounter = 0;
	    log("[Operator::slalomOff] ライントレースせずに停止してアームを下げる");
	}
	break;
    case 9: // ライントレースせずに停止してアームを下げる
	if ( (distance-slalomDistance) < 2000 ) {
	    machine->armMotor->setAngle(-50);
	    forward = speed.calc(0);
	    turn = azimuth.calc(260-(angle-slalomAngle))*EDGE/-4;
	} else {
	    ++slalomStatus;
	    slalomCounter = 0;
	    log("[Operator::slalomOff] スラローム後半終了");
	}
	break;
    case 10: // スラローム後半終了
	slalomStatus = 0;
	currentMethod = NULL;
	return;
    }
    int8_t pwm_L = forward - turn;
    int8_t pwm_R = forward + turn;
    machine->leftMotor->setPWM(pwm_L);
    machine->rightMotor->setPWM(pwm_R);
}

// もう不要！？
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
