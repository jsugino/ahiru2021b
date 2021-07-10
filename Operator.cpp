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
    machine->speed.ratio(0.1);
    machine->azimuth.ratio(0.2,20);
    //currentMethod = &Operator::lineTrace; // 通常走行から固定走行をする。
    //currentMethod = &Operator::lineTraceDummy; // 通常走行のみ版
    currentMethod = &Operator::slalomOnPrep; // 難所「板の前半」攻略用
    //currentMethod = &Operator::slalomOff; // 難所「板の後半」攻略用
    //currentMethod = &Operator::catchBlock; // 難所「ブロックキャッチ」攻略用
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
    int forward;      /* 前後進命令 */
    int turn;         /* 旋回命令 */

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
    machine->moveDirect(forward,turn);

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
    int forward;      /* 前後進命令 */
    int turn;         /* 旋回命令 */

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

    machine->moveDirect(forward,turn);

    /* 走行距離がXXXXに到達した場合、lineTraceへ遷移 */
    if( CHANGE_LINETRACE <= distance ) {
        currentMethod = &Operator::lineTrace;
    }

}

int calcTurn( int turn, int forward )
{
    int turnmax;

    turn = (int)(((double)turn)*((double)forward)/40.0);
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
	int forward;

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

	forward = machine->speed.calc(forward);

	machine->colorSensor->getRawColor(cur_rgb);
	int turn = calcTurn(60-cur_rgb.r,forward)*EDGE;
	machine->moveDirect(forward,turn);
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
	int forward = 30;
	machine->colorSensor->getRawColor(cur_rgb);
	int turn = calcTurn(60-cur_rgb.r,forward)*EDGE;
	machine->moveDirect(forward,turn);
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
	int forward;      /* 前後進命令 */

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
	    machine->moveDirect(forward,0);
	    return;
	} else {
	    edge = EDGE; // 正エッジ
	}
	machine->colorSensor->getRawColor(cur_rgb);
	int turn = calcTurn(60-cur_rgb.r,forward)*edge;
	machine->moveDirect(forward,turn);
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

    int forward = 30; // TODO: slalomOn に入る前のスピードを設定する。
    int turn = 0;
    int32_t base;

    switch ( slalomStatus ) {
    case 0: // initial state
	slalomCounter = 0;
	++slalomStatus;
	machine->speed.reset(forward);
	log("[Operator::slalomOn] 姿勢を揃える");
	break;
    case 1: // 板にぶつかり続けて、姿勢を揃える
	if ( ++slalomCounter < 1000 ) {
	    forward = machine->speed.calc(10);
	} else {
	    ++slalomStatus;
	    slalomDistance = distance; // 距離のリセット
	    slalomCounter = 0;
	    log("[Operator::slalomOn] バックする");
	}
	break;
    case 2: // バックする
	if ( (distance-slalomDistance) > -300 ) {
	    forward = machine->speed.calc(-10);
	} else {
	    ++slalomStatus;
	    slalomCounter = 0;
	    log("[Operator::slalomOn] アームを上げつつつ前進する");
	}
	break;
    case 3: // アームを上げつつつ前進する
	if ( (distance-slalomDistance) < 300 ) {
	    machine->armMotor->setAngle(-20);
	    forward = machine->speed.calc(50);
	} else {
	    ++slalomStatus;
	    slalomCounter = 0;
	    log("[Operator::slalomOn] アームをおろしつつ減速しライントレースする");
	}
	break;
    case 4: // アームをおろしつつ減速しライントレースする
#define SLALOMDIST 950
	if ( (distance-slalomDistance) < SLALOMDIST ) {
	    machine->armMotor->setAngle(-50);
	    forward = machine->speed.calc(10);
	    machine->colorSensor->getRawColor(cur_rgb);
	    turn = calcTurn(30-cur_rgb.r,forward)*EDGE;
	} else {
	    ++slalomStatus;
	    slalomCounter = 0;
	    log("[Operator::slalomOn] 逆走しつつライントレースする");
	}
	break;
    case 5: // 逆走しつつライントレースする
#define BACKDIST 650
	if ( (distance-slalomDistance) > BACKDIST ) {
	    forward = machine->speed.calc(-10);
	    machine->colorSensor->getRawColor(cur_rgb);
	    turn = calcTurn(30-cur_rgb.r,forward)*EDGE;
	} else {
	    ++slalomStatus;
	    slalomCounter = 0;
	    log("[Operator::slalomOn] 前進しつつライントレースする");
	}
	break;
    case 6: // 前進しつつライントレースする
	if ( (distance-slalomDistance) < SLALOMDIST ) {
	    forward = machine->speed.calc(10);
	    machine->colorSensor->getRawColor(cur_rgb);
	    turn = calcTurn(30-cur_rgb.r,forward)*EDGE;
	} else {
	    machine->azimuth.reset(angle); // 角度のリセット
	    ++slalomStatus;
	    slalomCounter = 0;
	    log("[Operator::slalomOn] スラロームする");
	}
	break;
    case 7: // スラロームする
	base = SLALOMDIST;
	forward = machine->speed.calc(10);
	if ( (distance-slalomDistance) < 2800 ) {
	    struct Slalom slalom[] = { /* ★距離と方角(260 count = 90度)を調整する */
		{ 330, 120 },  // 右に移動
		{ 240, 0 },    // 直進
		{ 520, -120 }, // 左に移動
		{ 300, 0 },    // 直進
		{ 430, 120 },  // 右に移動
		{9999, 0 },    // 直進
	    };
	    for ( int i = 0; i < (int)(sizeof(slalom)/sizeof(Slalom)); ++i ) {
		base += slalom[i].distance;
		if ( (distance-slalomDistance) < base ) {
		    turn = machine->azimuth.calc(angle,slalom[i].angle)*EDGE;
		    break;
		}
	    }
	} else {
	    ++slalomStatus;
	    slalomCounter = 0;
	    log("[Operator::slalomOn] スラローム前半終了");
	}
	break;
    case 8: // スラローム前半終了
	slalomStatus = 0;
	currentMethod = &Operator::slalomOff;
	return;
    }
    machine->moveDirect(forward,turn);
}

// slalomOff() からスタートする場合は、次のコマンドを実行する。
// 左コース用：
// curl -X POST -H "Content-Type: application/json" -d '{"initLX":"22.0","initLY":"0.1","initLZ":"15.2","initLROT":"90"}' http://localhost:54000
// 右コース用：
// curl -X POST -H "Content-Type: application/json" -d '{"initRX":"-22.0","initRY":"0.1","initRZ":"15.2","initRROT":"270"}' http://localhost:54000
void Operator::slalomOff()
{
    int forward = 10;
    int turn = 0;
    int32_t base;
    rgb_raw_t cur_rgb;

    int32_t  distance = machine->distanceL + machine->distanceR;
    int32_t  angle = machine->distanceL - machine->distanceR;

    switch ( slalomStatus ) {
    case 0: // initial state
	slalomCounter = 0;
	++slalomStatus;
	log("[Operator::slalomOff] 左に向く");
	break;
    case 1: // 左に向く
	forward = machine->speed.calc(5);
	turn = machine->azimuth.calc(angle,-180)*EDGE;
	machine->colorSensor->getRawColor(cur_rgb);
	if ( machine->azimuth.inTarget(angle+180) ) {
	    ++slalomStatus;
	    slalomCounter = 0;
	    log("[Operator::slalomOff] 黒線を探す");
	} else if ( (cur_rgb.r + cur_rgb.g + cur_rgb.b) < 30 ) {
	    ++slalomStatus;
	    ++slalomStatus;
	    slalomCounter = 0;
	    log("[Operator::slalomOff] 前方を向く");
	}
	break;
    case 2: // 黒線を探す
	machine->colorSensor->getRawColor(cur_rgb);
	forward = machine->speed.calc(10);
	turn = machine->azimuth.calc(angle,-180)*EDGE;
	if ( (cur_rgb.r + cur_rgb.g + cur_rgb.b) < 30 ) {
	    ++slalomStatus;
	    slalomCounter = 0;
	    log("[Operator::slalomOff] 前方を向く");
	}
	break;
    case 3: // 前方を向く
	forward = machine->speed.calc(12);
	turn = machine->azimuth.calc(angle,0)*EDGE;
	if ( machine->azimuth.inTarget(angle) ) {
	    ++slalomStatus;
	    slalomCounter = 0;
	    log("[Operator::slalomOff] ライントレースしつつ黒線が切れるところを探す");
	}
	break;
    case 4: // ライントレースしつつ黒線が切れるところを探す
	forward = machine->speed.calc(10);
	machine->colorSensor->getRawColor(cur_rgb);
	turn = calcTurn(30-cur_rgb.r,forward)*EDGE;
	if ( turn < -10 ) {
	    ++slalomStatus;
	    slalomCounter = 0;
	    slalomDistance = distance; // 距離をリセットする
	    machine->speed.reset(forward);
	    machine->azimuth.resetSpeed(turn);
	    log("[Operator::slalomOff] 少しバックしつつ角度調整する");
	}
	break;
    case 5: // 少しバックしつつ角度調整する
	forward = machine->speed.calc(-10);
	turn = machine->azimuth.calc(angle,0)*EDGE;
	if ( (distance-slalomDistance) < -200 ) {
	    ++slalomStatus;
	    slalomCounter = 0;
	    log("[Operator::slalomOn] 90度回転する");
	}
	break;
    case 6: // 90度回転する
	forward = machine->speed.calc(0);
	turn = machine->azimuth.calc(angle,260)*EDGE; // 260 count = 90度
	if ( machine->azimuth.inTarget(angle-260) ) {
	    slalomCounter = 0;
	    ++slalomStatus;
	    log("[Operator::slalomOff] スラロームする");
	}
	break;
    case 7: // スラロームする
	base = 0;
	forward = machine->speed.calc(10);
	if ( (distance-slalomDistance) < 1000 ) {
	    struct Slalom slalom[] = { /* ★距離と方角(260 count = 90度)を調整する */
		{ 350, 260+0 },   // 直進
		{ 450, 260+150 }, // 右に移動
		{9999, 260+0 },   // 直進
	    };
	    for ( int i = 0; i < (int)(sizeof(slalom)/sizeof(Slalom)); ++i ) {
		base += slalom[i].distance;
		if ( (distance-slalomDistance) < base ) {
		    turn = machine->azimuth.calc(angle,slalom[i].angle)*EDGE;
		    break;
		}
	    }
	} else {
	    ++slalomStatus;
	    slalomCounter = 0;
	    log("[Operator::slalomOn] 少しバックする");
	}
	break;
    case 8: // 少しバックする
	forward = machine->speed.calc(-10);
	turn = 0;
	if ( (distance-slalomDistance) < 950 ) {
	    ++slalomStatus;
	    slalomCounter = 0;
	    log("[Operator::slalomOff] ライントレースせずに加速してアームを上げる");
	}
	break;
    case 9: // ライントレースせずに加速してアームを上げる
	if ( (distance-slalomDistance) < 1500 ) {
	    machine->armMotor->setAngle(-20);
	    forward = machine->speed.calc(50);
	    turn = 0;
	} else {
	    ++slalomStatus;
	    slalomCounter = 0;
	    log("[Operator::slalomOff] ライントレースせずに停止してアームを下げる");
	}
	break;
    case 10: // ライントレースせずに停止してアームを下げる
	machine->armMotor->setAngle(-50);
	forward = machine->speed.calc(0);
	turn = 0;
	if ( forward == 0 ) {
	    ++slalomStatus;
	    slalomCounter = 0;
	    machine->azimuth.resetSpeed(0);
	    log("[Operator::slalomOff] ブロックの方に向く");
	}
	break;
    case 11: // ブロックの方に向く
	forward = 0;
	turn = machine->azimuth.calc(angle,520)*EDGE;
	if ( machine->azimuth.inTarget(angle-520) ) {
	    ++slalomStatus;
	    machine->speed.reset(0);
	    slalomCounter = 0;
	    log("[Operator::slalomOff] スラローム後半終了");
	}
	break;
    case 12: // スラローム後半終了
	slalomStatus = 0;
	currentMethod = &Operator::catchBlock;
	return;
    }
    machine->moveDirect(forward,turn);
}

// catchBlock() からスタートする場合は、次のコマンドを実行する。
// 左コース用：
// curl -X POST -H "Content-Type: application/json" -d '{"initLX":"21.5","initLY":"0.0","initLZ":"7.2","initLROT":"270"}' http://localhost:54000
void Operator::catchBlock()
{
    int forward = 10;
    int turn = 0;
    int32_t base;
    rgb_raw_t cur_rgb;

    int32_t  distance = machine->distanceL + machine->distanceR;
    int32_t  angle = machine->distanceL - machine->distanceR;
    switch ( slalomStatus ) {
    case 0: // initial state
	slalomCounter = 0;
	++slalomStatus;
	machine->speed.reset(0);
	machine->azimuth.reset(angle);
	machine->azimuth.resetSpeed(0);
	log("[Operator::catchBlock] １つめの黒線を見つけるまで進む");
	break;
    case 1: // １つめの黒線を見つけるまで進む
	forward = machine->speed.calc(20);
	turn = machine->azimuth.calc(angle,30)*EDGE;
	machine->colorSensor->getRawColor(cur_rgb);
	if ( (cur_rgb.r + cur_rgb.g + cur_rgb.b) < 30 ) {
	    slalomCounter = 0;
	    slalomDistance = distance; // 距離をリセットする
	    machine->azimuth.resetSpeed(0);
	    ++slalomStatus;
	    log("[Operator::catchBlock] 右に向く");
	}
	break;
    case 2: // 右に向く
	forward = machine->speed.calc(20);
	turn = machine->azimuth.calc(angle,260)*EDGE;
	if ( (distance-slalomDistance) > 100 ) {
	    slalomCounter = 0;
	    ++slalomStatus;
	    log("[Operator::catchBlock] 右に向きつつ２つめの黒線を見つけるまで進む");
	}
	break;
    case 3: // 右に向きつつ２つめの黒線を見つけるまで進む
	forward = machine->speed.calc(20);
	turn = machine->azimuth.calc(angle,260)*EDGE;
	machine->colorSensor->getRawColor(cur_rgb);
	if ( (cur_rgb.r + cur_rgb.g + cur_rgb.b) < 30 ) {
	    slalomCounter = 0;
	    ++slalomStatus;
	    log("[Operator::catchBlock] 前方を向く");
	}
	break;
    case 4: // 前方を向く
	forward = machine->speed.calc(-7);
	turn = machine->azimuth.calc(angle,0)*EDGE;
	if ( machine->azimuth.inTarget(angle) ) {
	    slalomCounter = 0;
	    ++slalomStatus;
	    slalomDistance = distance; // 距離をリセットする
	    log("[Operator::catchBlock] ライントレースする");
	}
	break;
    case 5: // ライントレースする
	forward = machine->speed.calc(10);
	machine->colorSensor->getRawColor(cur_rgb);
	turn = calcTurn(60-cur_rgb.r,forward)*EDGE;
	if ( (distance-slalomDistance) > 500 ) {
	    slalomCounter = 0;
	    machine->azimuth.reset(angle); // 角度をリセットする
	    machine->azimuth.resetSpeed(0);
	    ++slalomStatus;
	    log("[Operator::catchBlock] ブロックをキャッチしに行く");
	}
	break;
    case 6: // ブロックをキャッチしに行く
	forward = machine->speed.calc(10);
	{
	    struct Slalom slalom[] = { /* ★距離と方角(260 count = 90度)を調整する */
		{1000,    0 },   // 直進
		{1800, -275 }, // 左に移動
		{4500, -550 }, // 戻る
	    };
	    int i;
	    int n = (int)(sizeof(slalom)/sizeof(Slalom));
	    for ( i = 0; i < n; ++i ) {
		if ( (distance-slalomDistance) < slalom[i].distance ) {
		    turn = machine->azimuth.calc(angle,slalom[i].angle)*EDGE;
		    break;
		}
	    }
	    if ( i == n ) {
		++slalomStatus;
		slalomCounter = 0;
		log("[Operator::slalomOn] 停止する");
	    }
	}
	break;
    case 7: // 停止する
	forward = machine->speed.calc(0);
	turn = 0;
	if ( forward == 0 ) {
	    slalomCounter = 0;
	    ++slalomStatus;
	    log("[Operator::catchBlock] キャッチブロック終了");
	}
	break;
    case 8: // キャッチブロック終了
	slalomStatus = 0;
	currentMethod = NULL;
	return;
    }
    machine->moveDirect(forward,turn);
}

// もう不要！？
void Operator::startRun()
{
    rgb_raw_t cur_rgb;
    int16_t grayScaleBlueless;
    int forward;      /* 前後進命令 */
    int turn;         /* 旋回命令 */

    machine->colorSensor->getRawColor(cur_rgb);
    grayScaleBlueless = (cur_rgb.r * 10 + cur_rgb.g * 217 + cur_rgb.b * 29) / 256;
    //grayScaleBlueless = cur_rgb.r;

    forward = 35;
    turn = (35 - grayScaleBlueless)*EDGE;

    /* ログ出力　*/
    if(LOG == logCnt || 0 == logCnt) {
        log("[Operator::startRun]grayScaleBlueless=%d,forward=%d,turn=%d",grayScaleBlueless,forward,turn);
    }

    machine->moveDirect(forward,turn);

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
	curAngL = machine->distanceL;
	curAngR = machine->distanceR;
	deltaDistL = M_PI * TIRE_DIAMETER * (curAngL - prevAngL) / 360.0;
	deltaDistR = M_PI * TIRE_DIAMETER * (curAngR - prevAngR) / 360.0;
	deltaDist = (deltaDistL + deltaDistR) / 2.0;

	distance += deltaDist;
	prevAngL = curAngL;
	prevAngR = curAngR;

	if(LOG == logCnt) {
	    //log("[Operator::updateDistance] distance = %f",distance);
	}
}

Operator::~Operator() {
    log("Operator destructor");
}
