#include "Trial.hpp"

Trial::Trial( Machine* machine ) : Operator(machine)
{
    printf("Trial constructor\n");
    trialMethod = &Trial::rampTest;
}

Trial::~Trial()
{
    printf("Operator destructor\n");
}

void Trial::trial()
{
    (this->*trialMethod)();
}

extern LineTraceLogic* withR60ptr;
extern LineTraceLogic* withR60revptr;

// ----------------------------------------------------------------------
// 角速度の台形制御の有無による、方角のズレがどの程度発生するのかを確かめる。
// コース上の同じところを何度回っても、方角のズレが発生しないか確かめる。
// コース上の障害物をすべて除いて、初期位置から、左コース or 右コースで走らせてみる。

// 角速度の上限とステッフアップ度は、通常のものより上げることで大きく差が出ることを期待する。

RampControler controler;

// 角速度の上昇と下降時の両方に台形制御を用いる。
void Trial::azimuthCheck()
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
	lineTraceAt(forward,withR60revptr);
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
void Trial::rampCheck()
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
	lineTraceAt(forward,withR60revptr);
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

// 台形制御の
void Trial::rampTest()
{
    int seqnum = 0;
    if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::rampTest] スタート");
	nextSequence(DIST); // 距離をリセットする
	startLogging("distL"); startLogging("distR"); startLogging("turn");

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::rampTest] ライントレースする");
#define RAMPSPEED 30
	int spd = lineTraceAt(RAMPSPEED,withR60ptr);
	if ( getRelDistance() > 500 && spd == RAMPSPEED ) nextSequence(AZIMUTH);
    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::rampTest] カーブする");
	curveTo(RAMPSPEED,-270);
	if ( getRelDistance() > 1000 ) nextSequence();

    } else {
	currentSequence("[Operator::rampTest] ストップ");
	startLogging("distL"); startLogging("distR"); startLogging("turn");
	nextMethod(NULL);
    }
}
