#include "Trial.hpp"

Trial::Trial( Machine* machine ) : Operator(machine)
{
    printf("Trial constructor\n");
    //trialMethod = &Trial::rampTestMid;
    trialMethod = &Trial::sonarTest;
}

Trial::~Trial()
{
    printf("Trial destructor\n");
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

#define ANGLE -270*2
#define SPEED 50

class Ramp2ControlerA : public Ramp2Controler
{
protected:
public:
    Ramp2ControlerA();
    int calc( int target );
};

Ramp2ControlerA::Ramp2ControlerA()
{
    toZero = 0;
}

// パラメーターの調整方法
// 回転しすぎ → 大きくする
// 回転不足   → 小さくする
// 速度高が回転しすぎ、速度低が回転不足 → R1:大きく、R2:小さく
// 速度低が回転しすぎ、速度高が回転不足 → R1:小さく、R2:大きく : Spd=70, 30
#define RAMP2RATIO1 0.2
#define RAMP2RATIO2 1.0
#define RAMP2THRE2to1 3/2
#define RAMP2THRE1to2 2/3
#define RAMP2THRE2to3 2/3
#define RAMP2THRE3to2 3/2

int Ramp2ControlerA::calc( int target )
{
    int spd;
    double cur = (double)speed.getCurrent();
    if ( cur < 0 ) cur = -cur;
    int threshold = cur * ( cur * RAMP2RATIO1 + RAMP2RATIO2 );
    int mode = toZero < 0 ? -toZero : toZero;
    int sign = target < 0 ? -1 : 1;
    if ( toZero != 0 ) {
	if ( target*toZero <= 0 ) {
	    speed.reset(0);
	    spd = 0;
	    toZero = 0;
	} else {
	    switch ( mode ) {
	    case 1:
		if ( target*sign < threshold*RAMP2THRE1to2 ) mode = 2;
		break;
	    case 2:
		if ( target*sign > threshold*RAMP2THRE2to1 ) mode = 1;
		if ( target*sign < threshold*RAMP2THRE2to3 ) mode = 3;
		break;
	    case 3:
		if ( target*sign > threshold*RAMP2THRE3to2 ) mode = 2;
		break;
	    }
	    spd = speed.calc(0,mode-1);
	    toZero =
		( spd == 0 ) ? 0 :
		( toZero < 0 ) ? -mode : mode;
	}
    } else if ( target == 0 ) { // toZero == 0
	speed.reset(0);
	spd = 0;
    } else {
	if ( target*sign > threshold ) {
	    spd = speed.calc(-maxspeed*sign);
	    toZero = 0;
	} else {
	    spd = speed.calc(0);
	    toZero = sign*2;
	}
    }
    logging("mode",toZero);
    logging("thre",(int32_t)threshold);
    return spd;
}

Ramp2ControlerA testCont;

// 台形制御のパラメータを得る。
void Trial::rampTest()
{
    int seqnum = 0;
    int Ang = ANGLE;
    int Spd = SPEED;

    if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::rampTest] スタート");
	nextSequence(DIST); // 距離をリセットする
	logging("thre",0); logging("mode",0);

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::rampTest] 低速でライントレースする");
	lineTraceAt(10,withR60ptr);
	if ( getRelDistance() > 300 ) nextSequence(AZIMUTH);

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::rampTest] 広場に出る");
	int turn = curveTo(Spd,-270);
	if ( getRelDistance() > Spd*5 && -2 <= turn && turn <= 2 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	if ( currentSequence("[Operator::rampTest] 広場に出る(継続)") ) {
	    startLogging("distL"); startLogging("distR"); startLogging("turn");
	    startLogging("thre"); startLogging("mode");
	}
	curveTo(Spd,-270);
	if ( getRelDistance() > Spd*40 - 500 ) nextSequence(AZIMUTH);

    } else if ( seqnum++ == getSequenceNumber() ) {
	if ( currentSequence("[Operator::rampTest] カーブする") ) {
	    testCont.ratio(0.1,20);
	    testCont.setSpeed(0);
	}
	// int turn = machine->azimuth.calc(slalomAzimuth+azi-getAbsAzimuth());

	int turn = testCont.calc(Ang-getAzimuth());
	machine->moveDirect(Spd,turn);

	//curveTo(Spd,Ang);

	if ( getRelDistance() > Spd*50 ) nextSequence();

    } else {
	currentSequence("[Operator::rampTest] ストップ");
	stopLogging("distL"); stopLogging("distR"); stopLogging("turn");
	stopLogging("thre"); stopLogging("mode");
	nextMethod(NULL);
    }
}

// rampTestMid() からスタートする場合は、次のコマンドを実行する。
// curl -X POST -H "Content-Type: application/json" -d '{"initLX":"2","initLY":"0","initLZ":"-9","initLROT":"0"}' http://localhost:54000
void Trial::rampTestMid()
{
    int seqnum = 0;
    int Spd = 40;
    int Trn = 20;

    logging("seqnum",getSequenceNumber());

    if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::rampTest] スタート");
	nextSequence(DIST); // 距離をリセットする
	logging("mode",0);

    } else if ( seqnum++ == getSequenceNumber() ) {
	if ( currentSequence("[Operator::rampTestMid] ライントレースする") ) {
	    startLogging("distL"); startLogging("distR"); startLogging("rgbR"); startLogging("turn");
	    startLogging("mode"); startLogging("seqnum");
	}
	lineTraceAt(30,withR60ptr);
	logging("mode",0);
	if ( getRelDistance() > Spd*20 ) nextSequence(AZIMUTH);

    } else if ( seqnum++ == getSequenceNumber() ) {
	if ( currentSequence("[Operator::rampTestMid] 広場でカーブする") ) {
	}
	//curveTo(Spd,-270*2);
	moveAt(Spd,Trn); logging("mode",0);
	if ( getAzimuth() < -270*2 ) { nextSequence();
	}

    } else if ( seqnum++ == getSequenceNumber() ) {
	if ( currentSequence("[Operator::rampTestMid] カーブする") ) {
	    machine->azimuth.ratio(0.1,Trn);
	}
	curveTo(Spd,-270*4);

	if ( getRelDistance() > (Spd*50 - Trn*100 + 4000) ) nextSequence();

    } else {
	currentSequence("[Operator::rampTestMid] ストップ");
	stopLogging("distL"); stopLogging("distR"); stopLogging("rgbR"); stopLogging("turn");
	stopLogging("mode"); stopLogging("seqnum");
	nextMethod(NULL);
    }
}

// sonarTest() からスタートする場合は、次のコマンドを実行する。
// curl -X POST -H "Content-Type: application/json" -d '{"initLX":"24.2","initLY":"0.0","initLZ":"8.0","initLROT":"180"}' http://localhost:54000
void Trial::sonarTest()
{
    logging("seqnum",getSequenceNumber());

    int seqnum = 0;

    logging("seqnum",getSequenceNumber());

    if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Trial::sonarTest] スタート");
	nextSequence(DIST); // 距離をリセットする
	startLogging("distL"); startLogging("distR"); startLogging("distS");

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Trial::sonarTest] ライントレースする");
	lineTraceAt(30,withR60ptr);
	if ( getRelDistance() > 500 ) nextSequence(AZIMUTH);

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Trial::sonarTest] そのまま直進する");
	curveTo(30,0); // 角度0で直進
	if ( machine->sonarDist < 20 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Trial::sonarTest] ゆっくり直進する");
	curveTo(10,0); // 角度0で直進
	if ( machine->sonarDist < 10 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Trial::sonarTest] ソナーは使わずに直進する");
	curveTo(10,0); // 角度0で直進
	if ( getRelDistance() > 130 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Trial::sonarTest] 停止する");
	int spd = moveAt(0);
	if ( spd == 0 ) nextSequence();

    } else {
	stopLogging("distL"); stopLogging("distR"); stopLogging("distS");
	currentSequence("[Trial::sonarTest] 終了");
	nextMethod(NULL);
    }
}
