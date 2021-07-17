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

    currentMethod = &Operator::lineTrace; // 通常走行から固定走行をする。
    //currentMethod = &Operator::lineTraceDummy; // 通常走行のみ版
    //currentMethod = &Operator::slalomOn; // 難所「板の前半」攻略用
    //currentMethod = &Operator::slalomOff; // 難所「板の後半」攻略用
    //currentMethod = &Operator::moveToBlock; // 難所「ブロックキャッチ」攻略用
    //currentMethod = &Operator::moveToGarage; // 難所「ガレージで停止」攻略用

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

#define ERROR_AZIMUTH 5
bool Operator::checkAzimuth( int32_t azi )
{
    azi *= EDGE;
    int32_t cur = getAzimuth();
    return ((azi-ERROR_AZIMUTH) <= cur) && (cur <= (azi+ERROR_AZIMUTH));
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
    int edge;
    int threshold;
public:
    WithRed( int thre, int edge );
    virtual int calcTurn( Machine* machine, int speed );
    virtual ~WithRed();
};

WithRed::WithRed( int thre, int eg )
{
    threshold = thre;
    edge = eg;
}

WithRed::~WithRed()
{
    // Nothing to do. But need to be defined.
}

// PID制御のP制御のみを行う
int WithRed::calcTurn( Machine* machine, int forward )
{
    int turn = threshold - machine->getRawRGB()->r; // 赤色だけを使用する
    turn = (int)(((double)turn)*((double)forward)/40.0);
    if ( forward < 0 ) turn = -turn;
    return turn*edge;
}

// 赤色の30をスレッショルドとして、turn 値を計算する。
WithRed withR30(30,Operator::EDGE);

// 赤色の60をスレッショルドとして、turn 値を計算する。
WithRed withR60(60,Operator::EDGE);

// 赤色の60をスレッショルドとして、turn 値を計算する。逆エッジを使う。
WithRed withR60rev(60,-Operator::EDGE);

// 目的：山中さんの固定走行プログラムができるまでの通常走行のみ版
// 処理内容：初期状態から、通常どおりライントレースし難所直前までたどり着く
// ロボの初期位置：初期状態
void Operator::lineTraceDummy()
{
    int seqnum = 0;
    if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::lineTraceDummy] ライントレース開始");
	nextSequence(DIST);

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::lineTraceDummy] 第１直線");
	lineTraceAt(80,&withR60);
	if ( getCPDistance() > 4600-500 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::lineTraceDummy] 第１カーブ");
	lineTraceAt(50,&withR60);
	if ( getCPDistance() > 6100 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::lineTraceDummy] 第１カーブ後直線");
	lineTraceAt(70,&withR60);
	if ( getCPDistance() > 7750 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::lineTraceDummy] 第２カーブ");
	lineTraceAt(50,&withR60);
	if ( getCPDistance() > 9100 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::lineTraceDummy] 第２カーブ後直線");
	lineTraceAt(70,&withR60);
	if ( getCPDistance() > 10200 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::lineTraceDummy] 第３カーブ");
	lineTraceAt(50,&withR60);
	if ( getCPDistance() > 11150 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::lineTraceDummy] 第３カーブ後直線");
	lineTraceAt(70,&withR60);
	if ( getCPDistance() > 12700 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::lineTraceDummy] 第４カーブ(きつい)");
	lineTraceAt(40,&withR60);
	if ( getCPDistance() > 13800 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::lineTraceDummy] 第４カーブ後直線");
	lineTraceAt(70,&withR60);
	if ( getCPDistance() > 14600 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::lineTraceDummy] 第５カーブ(きつい)");
	lineTraceAt(40,&withR60);
	if ( getCPDistance() > 15450 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::lineTraceDummy] 第５カーブ後直線");
	lineTraceAt(70,&withR60);
	if ( getCPDistance() > 17100 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::lineTraceDummy] 第６カーブ");
	lineTraceAt(50,&withR60);
	if ( getCPDistance() > 17600 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::lineTraceDummy] 第６カーブ後直線");
	lineTraceAt(70,&withR60);
	if ( getCPDistance() > 18400 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::lineTraceDummy] 第７カーブ");
	lineTraceAt(50,&withR60);
	if ( getCPDistance() > 20000+500 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::lineTraceDummy] 第７カーブ後直線");
	lineTraceAt(80,&withR60);
	if ( getCPDistance() > 25350-500 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::lineTraceDummy] 第８カーブ");
	lineTraceAt(50,&withR60);
	if ( getCPDistance() > 27350 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::lineTraceDummy] 第８カーブ後直線");
	lineTraceAt(50,&withR60);
	if ( getCPDistance() > 27500 ) nextSequence();

    } else {
	currentSequence("[Operator::lineTraceDummy] ライントレース終了");
	nextMethod(&Operator::slalomOn);
    }

    /*
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
	int turn = withR60.calcTurn(machine,forward);
	machine->moveDirect(forward,turn);
    } else {
	sequenceNumber = 0;
	currentMethod = &Operator::slalomOn;
    }
    */
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
	int turn = withR60.calcTurn(machine,forward)*edge;
	machine->moveDirect(forward,turn);
    } else {
	sequenceNumber = 0;
	currentMethod = &Operator::slalomOn;
    }
}

// ----------------------------------------------------------------------
bool Operator::currentSequence( const char* const message, ... )
{
    ++slalomCounter;
    if ( slalomMessagedStatus != sequenceNumber ) {
	va_list ap;
	va_start(ap,message);
	vlog(message,ap);
	slalomMessagedStatus = sequenceNumber;
	return true;
    }
    return false;
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
    azi *= EDGE;
    spd = machine->speed.calc(spd);
    int turn = machine->azimuth.calc(slalomAzimuth+azi-getAbsAzimuth());
    machine->moveDirect(spd,turn);
    return turn;
}

int Operator::lineTraceAt( int spd, LineTraceLogic* logic )
{
    spd = machine->speed.calc(spd);
    int turn = logic->calcTurn(machine,spd);
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
	currentSequence("[Operator::slalomOn] スラローム前半開始");
	nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOn] 板にぶつかるまでライントレースする");
	lineTraceAt(30,&withR60);
	if ( getCounter() > 1000 ) nextSequence(); // 4ms x 1000 秒たったら次へ

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOn] 姿勢を揃える");
	moveAt(10);
	if ( getCounter() > 1000 ) nextSequence(); // 4ms x 1000 秒たったら次へ

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOn] 徐々に速度を下げる");
	int speed = moveAt(0);
	if ( speed == 0 ) nextSequence(DIST); // 速度が0になったら、板の端をチェックポイントとしてマークして次へ

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOn] バックする");
	moveAt(-10);
	if ( getCPDistance() < -300 ) nextSequence(); // チェックポイントの後方 -300 まで来たら次へ

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOn] アームを上げつつつ前進する");
	machine->armMotor->setAngle(-20);
	moveAt(50);
	if ( getCPDistance() > 300 ) nextSequence(); // チェックポイントの前方 300 まで来たら次へ

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOn] アームをおろしつつ減速しライントレースする");
	machine->armMotor->setAngle(-50);
	int speed = lineTraceAt(0,&withR30);
	if ( speed == 0 ) nextSequence(); // 停止したら次へ

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOn] 逆走する");
	moveAt(-10);
	if ( getCPDistance() < 650 ) nextSequence(); // チェックポイントから 650 までバックしたら次へ

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOn] 前進しつつライントレースする");
	lineTraceAt(10,&withR30);
	if ( getCPDistance() > 950 ) nextSequence(AZIMUTH); // チェックポイントから 950 まで来たら、角度をリセットして次へ

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOn] 右に移動");
	curveTo(10,120);
	if ( getRelDistance() > 330 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOn] 直進");
	curveTo(10,0);
	if ( getRelDistance() > 240 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOn] 左に移動");
	curveTo(10,-120);
	if ( getRelDistance() > 520 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOn] 直進");
	curveTo(10,0);
	if ( getRelDistance() > 300 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOn] 右に移動");
	curveTo(10,120);
	if ( getRelDistance() > 430 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOn] 直進");
	curveTo(10,0);
	if ( getCPDistance() > 2800 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOn] スラローム前半終了");
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
	currentSequence("[Operator::slalomOff] スラローム後半開始");
	nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOff] 左に向きつつ黒線を探す");
	curveTo(10,-90);
	if ( machine->getRGB() < 30 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOff] そのまま少し進む");
	curveTo(10,-90);
	if ( getRelDistance() > 100 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOff] 前方を向く");
	curveTo(10,0);
	if ( checkAzimuth(0) ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOff] ライントレースする");
	lineTraceAt(10,&withR30);
	if ( getRelDistance() > 300 ) nextSequence(AZIMUTH); // 方角をリセットする

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOff] ライントレースしつつ黒線が切れるところを探す");
	lineTraceAt(10,&withR30);
	if ( machine->getRGB() > 80 ) nextSequence(DIST); // 距離をリセットする

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOff] 少しバックしつつ角度調整する");
	curveTo(-10,0);
	if ( getCPDistance() < -50 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOff] 90度回転する");
	curveTo(0,260);
	if ( checkAzimuth(260) ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOff] 直進");
	curveTo(10,260);
	if ( getRelDistance() > 500 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOff] 右に移動");
	curveTo(10,260+150);
	if ( getRelDistance() > 400 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOff] 直進");
	curveTo(10,260);
	if ( getCPDistance() > 1000 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOff] 少しバックする");
	moveAt(-10);
	if ( getCPDistance() < 950 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOff] ライントレースせずに加速してアームを上げる");
	machine->armMotor->setAngle(-20);
	moveAt(50);
	if ( getCPDistance() > 1500 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOff] ライントレースせずに停止してアームを下げる");
	machine->armMotor->setAngle(-50);
	int speed = moveAt(0);
	if ( speed == 0 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOff] ブロックの方に向く");
	curveTo(0,520);
	if ( checkAzimuth(520) ) nextSequence();

    } else {
	currentSequence("[Operator::slalomOff] スラローム後半終了");
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
	currentSequence("[Operator::moveToBlock] ブロックの直前へ移動開始");
	nextSequence(AZIMUTH|DIST); // 方角と距離をリセットする

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::moveToBlock] １つめの黒線を見つけるまで進む");
	curveTo(20,30);
	if ( machine->getRGB() < 30 ) nextSequence(DIST); // 距離をリセットする

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::moveToBlock] 少しの間だけ黒線を無視して右に向く");
	curveTo(20,260);
	if ( getCPDistance() > 100 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::moveToBlock] 右に向きつつ２つめの黒線を見つけるまで進む");
	curveTo(20,260);
	if ( machine->getRGB() < 30 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::moveToBlock] 前方を向く");
	curveTo(-7,0);
	if ( checkAzimuth(0) ) nextSequence(DIST); // 距離をリセットする

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::moveToBlock] ライントレースする");
	lineTraceAt(10,&withR60);
	if ( getCPDistance() > 400 ) nextSequence(AZIMUTH); // 角度をリセットする

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::moveToBlock] ブロックをキャッチしに行く");
	curveTo(10,0);
	if ( getCPDistance() > 1000 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::moveToBlock] 左に移動");
	curveTo(10,-270);
	if ( getCPDistance() > 1800 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::moveToBlock] 戻る");
	curveTo(10,-540);
	if ( checkAzimuth(-540) ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::moveToBlock] ブロックの直前へ移動終了");
	nextMethod(&Operator::moveToGarage);
    }
}

// moveToGarage() からスタートする場合は、次のコマンドを実行する。
// 左コース用：
// curl -X POST -H "Content-Type: application/json" -d '{"initLX":"12.5","initLY":"0","initLZ":"6.1","initLROT":"90"}' http://localhost:54000
void Operator::moveToGarage()
{
    int seqnum = 0;
    if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::moveToGarage] ガレージへ移動開始");
	nextSequence(AZIMUTH|DIST); // 方角と距離をリセットする

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::moveToGarage] ゆっくりとブロックをキャッチする");
	curveTo(10,0);
	if ( getCPDistance() > 500 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
#define FORWARDANGLE -30
	currentSequence("[Operator::moveToGarage] すこし左カープする");
	curveTo(10,FORWARDANGLE);
	if ( getCPDistance() > 300 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::moveToGarage] 早くブロックを運ぶ");
	curveTo(50,FORWARDANGLE);
	if ( getCPDistance() > 1800 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::moveToGarage] ゆっくりブロックを運びつつ黒線を探す");
	curveTo(10,FORWARDANGLE);
	if ( machine->getRGB(1,0,0) < 50 ) nextSequence(DIST);

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::moveToGarage] そのまま少しバックする");
	curveTo(-30,FORWARDANGLE);
	if ( getRelDistance() < -100 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::moveToGarage] 左カープする");
	curveTo(20,-270);
	if ( getRelDistance() > 150 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::moveToGarage] 右カープする");
	curveTo(20,0);
	if ( getRelDistance() > 450 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::moveToGarage] 右カープし反転する");
	curveTo(10,270);
	if ( getRelDistance() > 400 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::moveToGarage] 少し右を向きブロックを戻す");
	curveTo(10,270+90);
	if ( getRelDistance() > 150 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::moveToGarage] そのままバックする");
	curveTo(-20,270+90);
	if ( getRelDistance() < -150 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::moveToGarage] 黒線を探す");
	curveTo(10,270+270);
	if ( machine->getRGB(1,0,0) < 50 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::moveToGarage] 左回転する");
	curveTo(0,270);
	if ( checkAzimuth(270) ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::moveToGarage] ライントレースする");
	lineTraceAt(10,&withR60);
	if ( getRelDistance() > 500 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::moveToGarage] ライントレースする(継続)");
	lineTraceAt(10,&withR60);
	if ( machine->getRGB(1,1,1) > 300 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::moveToGarage] そのまま直進する");
	moveAt(10);
	if ( getRelDistance() > 450 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::moveToGarage] 停止する");
	int spd = moveAt(0);
	if ( spd == 0 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::moveToGarage] ガレージへ移動終了");
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
// 角速度の台形制御の有無による、方角のズレがどの程度発生するのかを確かめる。
// コース上の同じところを何度回っても、方角のズレが発生しないか確かめる。
// コース上の障害物をすべて除いて、初期位置から、左コース or 右コースで走らせてみる。

// 角速度の上限とステッフアップ度は、通常のものより上げることで大きく差が出ることを期待する。

RampControler controler;

// 角速度の上昇と下降時の両方に台形制御を用いる。
void Operator::azimuthCheck()
{
    int Ang = 528;
    int Spd = 50;
    int Len = Spd*40+200; // 縦の距離
    int err1 = 3; // 角度の誤差範囲
    int err2 = 210; // 台形制御して減速するときの角度差

    int num = ((getSequenceNumber()-2)%2)+2; // 処理対象のシーケンス番号
    int aziOld = Ang*(getSequenceNumber()-2)*(-EDGE); // 一つ前のシーケンスでの方角
    int azi = Ang*(getSequenceNumber()-1)*(-EDGE); // そのシーケンスでの方角

    int seqnum = 0;
    int turn = 40*EDGE;

    if ( getSequenceNumber() == 2 ) { startLogging("distL"); startLogging("distR"); startLogging("turn"); }
    if ( getSequenceNumber() == 2+2*2 ) { stopLogging("distL"); stopLogging("distR"); stopLogging("turn"); }

    if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::azimuthCheck] スタート");
	nextSequence(DIST); // 距離をリセットする
	controler.ratio(0.2);

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::azimuthCheck] ライントレースする");
	int dist = getCPDistance();
	int forward =
	    ( dist < 4600-500 ) ? 80 :
	    ( dist < 6100 ) ? 50 : // 第1カーブ
	    ( dist < 7750 ) ? 70 : 50; // 第2カーブ
	lineTraceAt(forward,&withR60rev);
	if ( dist > 12000 ) nextSequence(AZIMUTH); // 角度をリセットする

	// 以下の２シーケンスを何度も繰り返す
    } else if ( seqnum++ == num ) {
	currentSequence("[Operator::azimuthCheck] 逆走する diff = %d",getAzimuth()-aziOld);
	if ( getAzimuth() < azi+err1 ) {
	    if ( controler.getCurrent() > 0 ) log("remain = %d",controler.getCurrent());
	    turn = 0;
	    controler.reset(0);
	} else if ( getAzimuth() < azi+err2 ) {
	    turn = controler.calc(0);
	} else {
	    turn = controler.calc(turn);
	}
	machine->moveDirect(Spd,turn);
	if ( getRelDistance() > Len ) nextSequence();

    } else if ( seqnum++ == num ) {
	currentSequence("[Operator::azimuthCheck] スタート直線に乗る diff = %d",getAzimuth()-aziOld);
	if ( getAzimuth() < azi+err1 ) {
	    if ( controler.getCurrent() > 0 ) log("remain = %d",controler.getCurrent());
	    turn = 0;
	    controler.reset(0);
	} else if ( getAzimuth() < azi+err2 ) {
	    turn = controler.calc(0);
	} else {
	    turn = controler.calc(turn);
	}
	machine->moveDirect(Spd,turn);
	if ( getRelDistance() > Len ) nextSequence();

	// 実際には下のシーケンスは処理されない
    } else {
	currentSequence("[Operator::azimuthCheck] 終了");
	nextMethod(NULL);
    }
}

// 角速度を上げるときには台形制御を行うが、目標方角に達するときの台形制御はしない。
void Operator::rampCheck()
{
    int Ang = 521;
    int Spd = 50;
    int Len = Spd*40+200; // 縦の距離
    int err = 23; // 角度の誤差範囲

    int num = ((getSequenceNumber()-2)%2)+2; // 処理対象のシーケンス番号
    int aziOld = Ang*(getSequenceNumber()-2)*(-EDGE); // 一つ前のシーケンスでの方角
    int azi = Ang*(getSequenceNumber()-1)*(-EDGE); // そのシーケンスでの方角

    int seqnum = 0;
    int turn = 40*EDGE;

    if ( getSequenceNumber() == 2 ) { startLogging("distL"); startLogging("distR"); startLogging("turn"); }
    if ( getSequenceNumber() == 2+2*2 ) { stopLogging("distL"); stopLogging("distR"); stopLogging("turn"); }

    if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::azimuthCheck] スタート");
	nextSequence(DIST); // 距離をリセットする
	controler.ratio(0.2);

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::azimuthCheck] ライントレースする");
	int dist = getCPDistance();
	int forward =
	    ( dist < 4600-500 ) ? 80 :
	    ( dist < 6100 ) ? 50 : // 第1カーブ
	    ( dist < 7750 ) ? 70 : 50; // 第2カーブ
	lineTraceAt(forward,&withR60rev);
	if ( dist > 12000 ) nextSequence(AZIMUTH); // 角度をリセットする

	// 以下の２シーケンスを何度も繰り返す
    } else if ( seqnum++ == num ) {
	currentSequence("[Operator::azimuthCheck] 逆走する diff = %d",getAzimuth()-aziOld);
	if ( getAzimuth() < azi+err ) {
	    turn = 0;
	    controler.reset(0);
	} else {
	    turn = controler.calc(turn);
	}
	machine->moveDirect(Spd,turn);
	if ( getRelDistance() > Len ) nextSequence();

    } else if ( seqnum++ == num ) {
	currentSequence("[Operator::azimuthCheck] スタート直線に乗る diff = %d",getAzimuth()-aziOld);
	if ( getAzimuth() < azi+err ) {
	    turn = 0;
	    controler.reset(0);
	} else {
	    turn = controler.calc(turn);
	}
	machine->moveDirect(Spd,turn);
	if ( getRelDistance() > Len ) nextSequence();

	// 実際には下のシーケンスは処理されない
    } else {
	currentSequence("[Operator::azimuthCheck] 終了");
	nextMethod(NULL);
    }
}
