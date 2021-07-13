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
    sequenceNumber = 0;
    slalomMessagedStatus = -1;
    slalomCounter = 0;
    slalomDistance = 0;
    slalomAzimuth = 0;
    slalomPreviousDistance = 0;
    machine->speed.ratio(0.1);
    machine->azimuth.ratio(0.1,20); // 最大角速度は 20 に決め打ち。

    //currentMethod = &Operator::lineTrace; // 通常走行から固定走行をする。
    //currentMethod = &Operator::lineTraceDummy; // 通常走行のみ版
    //currentMethod = &Operator::slalomOn; // 難所「板の前半」攻略用
    //currentMethod = &Operator::slalomOff; // 難所「板の後半」攻略用
    //currentMethod = &Operator::moveToBlock; // 難所「ブロックキャッチ」攻略用
    currentMethod = &Operator::moveToGarage;

    //currentMethod = &Operator::azimuthCheck; // 角度の測定用
    //currentMethod = &Operator::rampCheck; // 台形制御の測定用
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

// 赤色をスレッショルドとして、turn 値を計算する。
LineTraceLogic::~LineTraceLogic()
{
    // Nothing to do. But need to be defined.
}

class WithRed : public LineTraceLogic
{
private:
    int threshold;
public:
    WithRed( int thre );
    virtual int calcTurn( rgb_raw_t* rgb, int speed );
    virtual ~WithRed();
};

WithRed::WithRed( int thre )
{
    threshold = thre;
}

WithRed::~WithRed()
{
    // Nothing to do. But need to be defined.
}

// PID制御のP制御のみを行う
int WithRed::calcTurn( rgb_raw_t* rgb, int forward )
{
    int turn = threshold - rgb->r; // 赤色だけを使用する
    int turnmax;

    turn = (int)(((double)turn)*((double)forward)/40.0);
    turnmax = ev3api::Motor::PWM_MAX - (forward > 0 ? forward : -forward);
    if ( turn < -turnmax ) turn = -turnmax;
    if ( turn > +turnmax ) turn = +turnmax;
    if ( forward < 0 ) turn = -turn;

    return turn;
}

// 赤色の30をスレッショルドとして、turn 値を計算する。
WithRed withR30(30);

// 赤色の60をスレッショルドとして、turn 値を計算する。
WithRed withR60(60);

// 目的：山中さんの固定走行プログラムができるまでの通常走行のみ版
// 処理内容：初期状態から、通常どおりライントレースし難所直前までたどり着く
// ロボの初期位置：初期状態
void Operator::lineTraceDummy()
{
    int32_t  distance = machine->distanceL + machine->distanceR;

    // 最初から左エッジでライントレースする。
    // 山中さんのコードができたら、削除する。
    if ( distance < 27500 ) {
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
	int turn = withR60.calcTurn(machine->getRawRGB(),forward);
	machine->moveDirect(forward,turn);
    } else {
	sequenceNumber = 0;
	currentMethod = &Operator::slalomOn;
    }
}

// 目的：逆エッジを使った走行を試して見るためのもの
// 処理内容：初期状態から、逆エッジを使ってライントレースし難所直前までたどり着く
// ロボの初期位置：初期状態
void Operator::reverseEdge()
{
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
	int turn = withR60.calcTurn(machine->getRawRGB(),forward)*edge;
	machine->moveDirect(forward,turn);
    } else {
	sequenceNumber = 0;
	currentMethod = &Operator::slalomOn;
    }
}

// ----------------------------------------------------------------------
void Operator::currentTask( const char* const message, ... )
{
    if ( slalomMessagedStatus != sequenceNumber ) {
	va_list ap;
	va_start(ap,message);
	vlog(message,ap);
	slalomMessagedStatus = sequenceNumber;
    }
    ++slalomCounter;
}

// Goto Next Status : 次の処理番号に進む
void Operator::nextSequence( int flags )
{
    ++sequenceNumber;
    slalomCounter = 0;
    slalomPreviousDistance = getAbsDistance();
    if ( flags & AZIMUTH ) resetAzimuth();
    if ( flags & DIST ) markCheckPoint();
}

// Goto Next Method
void Operator::nextMethod( void (Operator::*next)() )
{
    sequenceNumber = 0;
    slalomCounter = 0;
    currentMethod = next;
}


int Operator::moveAt( int spd )
{
    int speed = machine->speed.calc(spd);
    machine->moveDirect(speed,0);
    machine->azimuth.resetSpeed(0);
    return speed;
}

int Operator::curveTo( int spd, int azi )
{
    spd = machine->speed.calc(spd);
    int turn = machine->azimuth.calc(slalomAzimuth+azi-getAbsAzimuth());
    machine->moveDirect(spd,turn);
    return turn;
}

int Operator::lineTraceAt( int spd, LineTraceLogic* logic )
{
    spd = machine->speed.calc(spd);
    int turn = logic->calcTurn(machine->getRawRGB(),spd)*EDGE;
    machine->azimuth.resetSpeed(turn);
    machine->moveDirect(spd,turn);
    return spd;
}

// slalomOn() からスタートする場合は、次のコマンドを実行する。
// 左コース用：
// curl -X POST -H "Content-Type: application/json" -d '{"initLX":"8.0","initLY":"0","initLZ":"15.5","initLROT":"90"}' http://localhost:54000
void Operator::slalomOn()
{
    int seqnum = 0;
    if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::slalomOn] スラローム前半開始");
	nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::slalomOn] 板にぶつかるまでライントレースする");
	lineTraceAt(30,&withR60);
	if ( getCounter() > 1000 ) nextSequence(); // 4ms x 1000 秒たったら次へ

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::slalomOn] 姿勢を揃える");
	moveAt(10);
	if ( getCounter() > 1000 ) nextSequence(); // 4ms x 1000 秒たったら次へ

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::slalomOn] 徐々に速度を下げる");
	int speed = moveAt(0);
	if ( speed == 0 ) nextSequence(DIST); // 速度が0になったら、板の端をチェックポイントとしてマークして次へ

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::slalomOn] バックする");
	moveAt(-10);
	if ( getCPDistance() < -300 ) nextSequence(); // チェックポイントの後方 -300 まで来たら次へ

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::slalomOn] アームを上げつつつ前進する");
	machine->armMotor->setAngle(-20);
	moveAt(50);
	if ( getCPDistance() > 300 ) nextSequence(); // チェックポイントの前方 300 まで来たら次へ

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::slalomOn] アームをおろしつつ減速しライントレースする");
	machine->armMotor->setAngle(-50);
	int speed = lineTraceAt(0,&withR30);
	if ( speed == 0 ) nextSequence(); // 停止したら次へ

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::slalomOn] 逆走する");
	moveAt(-10);
	if ( getCPDistance() < 650 ) nextSequence(); // チェックポイントから 650 までバックしたら次へ

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::slalomOn] 前進しつつライントレースする");
	lineTraceAt(10,&withR30);
	if ( getCPDistance() > 950 ) nextSequence(AZIMUTH); // チェックポイントから 950 まで来たら、角度をリセットして次へ

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::slalomOn] 右に移動");
	curveTo(10,120);
	if ( getRelDistance() > 330 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::slalomOn] 直進");
	curveTo(10,0);
	if ( getRelDistance() > 240 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::slalomOn] 左に移動");
	curveTo(10,-120);
	if ( getRelDistance() > 520 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::slalomOn] 直進");
	curveTo(10,0);
	if ( getRelDistance() > 300 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::slalomOn] 右に移動");
	curveTo(10,120);
	if ( getRelDistance() > 430 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::slalomOn] 直進");
	curveTo(10,0);
	if ( getCPDistance() > 2800 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::slalomOn] スラローム前半終了");
	nextMethod(&Operator::slalomOff);
    }
}

// slalomOff() からスタートする場合は、次のコマンドを実行する。
// 左コース用：
// curl -X POST -H "Content-Type: application/json" -d '{"initLX":"22.0","initLY":"0.1","initLZ":"15.2","initLROT":"90"}' http://localhost:54000
// 右コース用：
// curl -X POST -H "Content-Type: application/json" -d '{"initRX":"-22.0","initRY":"0.1","initRZ":"15.2","initRROT":"270"}' http://localhost:54000
void Operator::slalomOff()
{
    int seqnum = 0;

    if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::slalomOff] スラローム後半開始");
	nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::slalomOff] 左に向きつつ黒線を探す");
	curveTo(10,-90);
	if ( machine->getRGB() < 30 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::slalomOff] そのまま少し進む");
	curveTo(10,-90);
	if ( getRelDistance() > 100 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::slalomOff] 前方を向く");
	curveTo(10,0);
	if ( getAzimuth() > -5 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::slalomOff] ライントレースする");
	lineTraceAt(10,&withR30);
	if ( getRelDistance() > 300 ) nextSequence(AZIMUTH); // 方角をリセットする

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::slalomOff] ライントレースしつつ黒線が切れるところを探す");
	lineTraceAt(10,&withR30);
	if ( machine->getRGB() > 80 ) nextSequence(DIST); // 距離をリセットする

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::slalomOff] 少しバックしつつ角度調整する");
	curveTo(-10,0);
	if ( getCPDistance() < -50 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::slalomOff] 90度回転する");
	curveTo(0,260);
	if ( getAzimuth() > 260-5 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::slalomOff] 直進");
	curveTo(10,260);
	if ( getRelDistance() > 500 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::slalomOff] 右に移動");
	curveTo(10,260+150);
	if ( getRelDistance() > 400 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::slalomOff] 直進");
	curveTo(10,260);
	if ( getCPDistance() > 1000 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::slalomOff] 少しバックする");
	moveAt(-10);
	if ( getCPDistance() < 950 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::slalomOff] ライントレースせずに加速してアームを上げる");
	machine->armMotor->setAngle(-20);
	moveAt(50);
	if ( getCPDistance() > 1500 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::slalomOff] ライントレースせずに停止してアームを下げる");
	machine->armMotor->setAngle(-50);
	int speed = moveAt(0);
	if ( speed == 0 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::slalomOff] ブロックの方に向く");
	curveTo(0,520);
	if ( getAzimuth() > 520-5 ) nextSequence();

    } else {
	currentTask("[Operator::slalomOff] スラローム後半終了");
	nextMethod(&Operator::moveToBlock);
    }
}

// moveToBlock() からスタートする場合は、次のコマンドを実行する。
// 左コース用：
// curl -X POST -H "Content-Type: application/json" -d '{"initLX":"21.5","initLY":"0.0","initLZ":"7.2","initLROT":"270"}' http://localhost:54000
void Operator::moveToBlock()
{
    int seqnum = 0;
    if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::moveToBlock] ブロックの直前へ移動開始");
	nextSequence(AZIMUTH|DIST); // 方角と距離をリセットする

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::moveToBlock] １つめの黒線を見つけるまで進む");
	curveTo(20,30);
	if ( machine->getRGB() < 30 ) nextSequence(DIST); // 距離をリセットする

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::moveToBlock] 少しの間だけ黒線を無視して右に向く");
	curveTo(20,260);
	if ( getCPDistance() > 100 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::moveToBlock] 右に向きつつ２つめの黒線を見つけるまで進む");
	curveTo(20,260);
	if ( machine->getRGB() < 30 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::moveToBlock] 前方を向く");
	curveTo(-7,0);
	if ( getAzimuth() < 5 ) nextSequence(DIST); // 距離をリセットする

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::moveToBlock] ライントレースする");
	lineTraceAt(10,&withR60);
	if ( getCPDistance() > 500 ) nextSequence(AZIMUTH); // 角度をリセットする

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::moveToBlock] ブロックをキャッチしに行く");
	curveTo(10,0);
	if ( getCPDistance() > 1000 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::moveToBlock] 左に移動");
	curveTo(10,-270);
	if ( getCPDistance() > 1800 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::moveToBlock] 戻る");
	curveTo(10,-540);
	if ( getAzimuth() < -540+5 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::moveToBlock] ブロックの直前へ移動終了");
	nextMethod(&Operator::moveToGarage);
    }
}

// moveToGarage() からスタートする場合は、次のコマンドを実行する。
// 左コース用：
// curl -X POST -H "Content-Type: application/json" -d '{"initLX":"12.5","initLY":"0","initLZ":"6.1","initLROT":"90"}' http://localhost:54000
void Operator::moveToGarage()
{
    if ( getSequenceNumber() == 0 ) {
    } else if ( getSequenceNumber() == 2 ) {
    }
    int seqnum = 0;
    if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::moveToGarage] ガレージへ移動開始");
	nextSequence(AZIMUTH|DIST); // 方角と距離をリセットする
	startLogging("distL"); startLogging("distR");

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::moveToGarage] ゆっくりとブロックを運ぶ");
	curveTo(10,0);
	if ( getCPDistance() > 500 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::moveToGarage] 早くブロックを運ぶ");
	curveTo(50,0);
	if ( getCPDistance() > 2000 ) { nextSequence();
	    startLogging("rgbR"); startLogging("rgbG"); startLogging("rgbB"); }

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::moveToGarage] ゆっくりブロックを運びつつ黒線を探す");
	curveTo(10,0);
	if ( machine->getRGB() < 150 ) { nextSequence();
	    stopLogging("distL"); stopLogging("distR"); }

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::moveToGarage] ガレージへ移動終了");
	nextMethod(NULL);
	return;
    }
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

// ----------------------------------------------------------------------
// Azimuth のカウント値と角度の比を、実際に走らせて算出してみるためのプログラム。
// コース上の同じところを何度回っても、方角のズレが発生しないか確かめる。
// コース上の障害物をすべて除いて、初期位置から、左コース or 右コースで走らせてみる。
void Operator::azimuthCheck()
{
    // 下記のデータは、7/12 に杉野が試してみた結果
    // Spd と Ang を次のように設定すると、ちょうどよい感じにもとのラインに戻る。
    // Left Course
    // Spd 70, Ang 267
    // Spd 60, Ang 269
    // Spd 50, Ang 267
    // Spd 40, Ang 269
    // Spd 30, Ang 271
    // Spd 20, Ang 271
    // Spd 10, Ang 269

    // Right Course
    // Spd 60, Ang 268
    // Spd 30, Ang 270
    // Spd 10, Ang 273

    int Ang = 271;
    int Spd = 30;
    int Sta = 600; // 助走距離
    int Len = Spd*40+200; // 縦の距離
    int Wid = Spd*40+200; // 横の距離
    int tSpd = (Spd < 30) ? Spd : 30; // ライントレースの速度

    int num = ((getSequenceNumber()-3)%4)+3; // 処理対象のシーケンス番号
    int aziOld = Ang*(getSequenceNumber()-3)*(-EDGE); // 一つ前のシーケンスでの方角
    int azi = Ang*(getSequenceNumber()-2)*(-EDGE); // そのシーケンスでの方角

    int seqnum = 0;

    if ( getSequenceNumber() == 3 ) {
	startLogging("distL");
	startLogging("distR");
	startLogging("AngMode");
	startLogging("AngSpd");
    } else if ( getSequenceNumber() == 8 ) {
	stopLogging("distL");
	stopLogging("distR");
	stopLogging("AngMode");
	stopLogging("AngSpd");
    }

    if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::azimuthCheck] スタート");
	nextSequence(DIST); // 距離をリセットする

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::azimuthCheck] ライントレースする");
	lineTraceAt(tSpd,&withR60);
	if ( getRelDistance() > (Len+Sta)/2 ) nextSequence(AZIMUTH); // 角度をリセットする

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::azimuthCheck] 直進する");
	moveAt(Spd);
	if ( getRelDistance() > (Len+Sta)/2 ) nextSequence();

	// 以下の４シーケンスを何度も繰り返す
    } else if ( seqnum++ == num ) {
	currentTask("[Operator::azimuthCheck] 左に向く diff = %d",getAzimuth()-aziOld);
	curveTo(Spd,azi);
	if ( getRelDistance() > Wid ) nextSequence();

    } else if ( seqnum++ == num ) {
	currentTask("[Operator::azimuthCheck] 逆走する diff = %d",getAzimuth()-aziOld);
	curveTo(Spd,azi);
	if ( getRelDistance() > Len ) nextSequence();

    } else if ( seqnum++ == num ) {
	currentTask("[Operator::azimuthCheck] スタート直線に向かう diff = %d",getAzimuth()-aziOld);
	curveTo(Spd,azi);
	if ( getRelDistance() > Wid ) nextSequence();

    } else if ( seqnum++ == num ) {
	currentTask("[Operator::azimuthCheck] スタート直線に乗る diff = %d",getAzimuth()-aziOld);
	curveTo(Spd,azi);
	if ( getRelDistance() > Len ) nextSequence();

	// 実際には下のシーケンスは処理されない
    } else {
	currentTask("[Operator::azimuthCheck] 終了");
	nextMethod(NULL);
    }
}

// azimuthCheck() と同様に同じところを回るが、台形制御しない。
void Operator::rampCheck()
{
    // Left Course
    // # Spd 70, Ang 267
    // # Spd 60, Ang 269
    // Spd 50, Ang 268
    // # Spd 40, Ang 269
    // # Spd 30, Ang 271
    // # Spd 20, Ang 271
    // Spd 10, Ang 271, err 10

    int Ang = 268;
    int Spd = 50;
    int Sta = 600; // 助走距離
    int Len = Spd*40+200; // 縦の距離
    int Wid = Spd*40+200; // 横の距離
    int tSpd = (Spd < 30) ? Spd : 30; // ライントレースの速度
    int err = 10; // 角度の誤差範囲

    int num = ((getSequenceNumber()-3)%4)+3; // 処理対象のシーケンス番号
    int aziOld = Ang*(getSequenceNumber()-3)*(-EDGE); // 一つ前のシーケンスでの方角
    int azi = Ang*(getSequenceNumber()-2)*(-EDGE); // そのシーケンスでの方角

    int seqnum = 0;
    int turn = 20*EDGE;

    if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::azimuthCheck] スタート");
	nextSequence(DIST); // 距離をリセットする

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::azimuthCheck] ライントレースする");
	lineTraceAt(tSpd,&withR60);
	if ( getRelDistance() > (Len+Sta)/2 ) nextSequence(AZIMUTH); // 角度をリセットする

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentTask("[Operator::azimuthCheck] 直進する");
	machine->moveDirect(Spd,0);
	if ( getRelDistance() > (Len+Sta)/2 ) nextSequence();

	// 以下の４シーケンスを何度も繰り返す
    } else if ( seqnum++ == num ) {
	currentTask("[Operator::azimuthCheck] 左に向く diff = %d",getAzimuth()-aziOld);
	if ( getAzimuth() < azi+err ) turn = 0;
	machine->moveDirect(Spd,turn);
	if ( getRelDistance() > Wid ) nextSequence();

    } else if ( seqnum++ == num ) {
	currentTask("[Operator::azimuthCheck] 逆走する diff = %d",getAzimuth()-aziOld);
	if ( getAzimuth() < azi+err ) turn = 0;
	machine->moveDirect(Spd,turn);
	if ( getRelDistance() > Len ) { nextSequence(); startLogging("distL"); startLogging("distR"); }

    } else if ( seqnum++ == num ) {
	currentTask("[Operator::azimuthCheck] スタート直線に向かう diff = %d",getAzimuth()-aziOld);
	if ( getAzimuth() < azi+err ) turn = 0;
	machine->moveDirect(Spd,turn);
	if ( getRelDistance() > Wid ) { nextSequence(); stopLogging("distL"); stopLogging("distR"); }

    } else if ( seqnum++ == num ) {
	currentTask("[Operator::azimuthCheck] スタート直線に乗る diff = %d",getAzimuth()-aziOld);
	if ( getAzimuth() < azi+err ) turn = 0;
	machine->moveDirect(Spd,turn);
	if ( getRelDistance() > Len ) nextSequence();

	// 実際には下のシーケンスは処理されない
    } else {
	currentTask("[Operator::azimuthCheck] 終了");
	nextMethod(NULL);
    }
}
