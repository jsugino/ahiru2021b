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


// Trial を使用しない場合は、次の #include を外す。
#include "Trial.hpp"

Operator::Operator( Machine* mcn ) {
    printf("Operator constructor\n");
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
    //currentMethod = &Operator::shortCut; // 固定走行中心に実行する。
    currentMethod = &Operator::lineTraceDummy; // 通常走行のみ版
    //currentMethod = &Operator::lineTraceSample; // 決め打ち走行のサンプル
	//currentMethod = &Operator::slalomAvoid; // 難所「スラローム回避」攻略用
    //currentMethod = &Operator::slalomOn; // 難所「板の前半」攻略用
    //currentMethod = &Operator::slalomOff; // 難所「板の後半」攻略用
    //currentMethod = &Operator::moveToBlock; // 難所「ブロックキャッチ」攻略用
    //currentMethod = &Operator::moveToGarage; // 難所「ガレージで停止」攻略用

    //currentMethod = &Operator::trial; // トライアル用
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
LineTraceLogic* withR60ptr = &withR60;

// 赤色の60をスレッショルドとして、turn 値を計算する。逆エッジを使う。
WithRed withR60rev(60,-Operator::EDGE);
LineTraceLogic* withR60revptr = &withR60rev;

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
	#if defined(MAKE_RIGHT)
    	nextMethod(&Operator::slalomAvoid);
	#else
    	nextMethod(&Operator::slalomOn);
	#endif
	
    }
}

// 決め打ち走行(モデル資料記述用)
void Operator::shortCut()
{
    int seqnum = 0;
    if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::shortCut] ライントレース開始");
	nextSequence(DIST|AZIMUTH);

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::shortCut] 第１直線前半");
	lineTraceAt(50,&withR60);
	if ( getCPDistance() > 500 ) nextSequence(AZIMUTH);

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::shortCut] 第１直線後半");
	curveTo(80,0);
	if ( getCPDistance() > 4100 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::shortCut] 第１カーブ");
	curveTo(80,-270);
	if ( getRelDistance() > 3000 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
#define CUT2SPEED 60
	if ( currentSequence("[Operator::shortCut] 第２カーブ") ) {
	    machine->azimuth.ratio(0.1,(100-CUT2SPEED)); // 早いスピードで小回りする
	}
	curveTo(CUT2SPEED,-780);
	if ( getRelDistance() > 2300 ) {
	    nextSequence();
	    machine->azimuth.ratio(0.1,20); // もとに戻す
	}
    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::shortCut] 第３カーブ");
	curveTo(80,-540);
	if ( getRelDistance() > 500 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::shortCut] ショートカット開始");
	curveTo(80,-330);
	if ( getRelDistance() > 2600 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::shortCut] ショートカット終了");
	curveTo(80,-540);
	if ( getRelDistance() > 700 ) nextSequence();
    } else if ( seqnum++ == getSequenceNumber() ) {
#define CUT6SPEED 70
	if ( currentSequence("[Operator::shortCut] 第６カーブ") ) {
	    machine->azimuth.ratio(0.1,(100-CUT6SPEED)); // 早いスピードで小回りする
	}
	curveTo(CUT6SPEED,-800);
	if ( getRelDistance() > 1400 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::shortCut] 第７カーブ前半");
	curveTo(CUT6SPEED,-290);
	if ( getAzimuth() > -400 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::shortCut] 第７カーブ後半");
	curveTo(CUT6SPEED,-290);
	if ( machine->getRGB(1,0,0) < 40  ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::shortCut] 左エッジを探す");
	curveTo(CUT6SPEED,-290);
	if ( machine->getRGB(1,0,0) > 50 ) {
	    nextSequence();
	    machine->azimuth.ratio(0.1,20); // デフォルトに戻す
	}
    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::shortCut] ライントレース開始");
	lineTraceAt(50,&withR60);
	if ( getRelDistance() > 500 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::shortCut] 加速して青線を見つける");
	lineTraceAt(80,&withR60);
	if ( machine->getRGB(-24,10,10) > 300 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::shortCut] ゴール直後の青線上");
	lineTraceAt(80,&withR60);
	if ( machine->getRGB(-24,10,10) < 300 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::shortCut] カーブ後の青線を見つける");
	lineTraceAt(50,&withR60);
	if ( machine->getRGB(-24,10,10) > 300 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::shortCut] カーブ後の青線上");
	lineTraceAt(50,&withR60);
	if ( machine->getRGB(-24,10,10) < 300 ) nextSequence();

    } else {
	currentSequence("[Operator::shortCut] ライントレース終了");
	nextMethod(&Operator::slalomOn);
    }
    logging("seqnum",getSequenceNumber());
}

// 高速走行用のサンプルプログラム (成功率は低い)
void Operator::lineTraceSample()
{
    int seqnum = 0;
    if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::lineTraceSample] ライントレース開始");
	nextSequence(DIST|AZIMUTH);
	logging("mode",machine->azimuth.toZero);

    } else if ( seqnum++ == getSequenceNumber() ) {
	if ( currentSequence("[Operator::lineTraceSample] 第１直線") ) {
	    startLogging("distL"); startLogging("distR");
	    startLogging("rgbR"); startLogging("turn");
	    startLogging("seqnum"); startLogging("mode");
	}
	lineTraceAt(80,&withR60);
	logging("mode",machine->azimuth.toZero);
	if ( getCPDistance() > 4600-800 && machine->getRGB(1,0,0) < 45 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
#define CURV1SPEED 79
	if ( currentSequence("[Operator::lineTraceSample] 第１カーブ") ) {
	    machine->azimuth.ratio(0.3,(100-CURV1SPEED)); // 早いスピードで小回りする
	}
	curveTo(CURV1SPEED,-260);
	//lineTraceAt(50,&withR60);
	if ( getCPDistance() > 6100 && machine->getRGB(1,0,0) < 70 ) { nextSequence();
	    machine->azimuth.ratio(0.1,20); // デフォルトに戻す
	}

    } else if ( seqnum++ == getSequenceNumber() ) {
	if ( currentSequence("[Operator::lineTraceSample] 第１カーブ後直線") ) {
	}
	lineTraceAt(CURV1SPEED,&withR60);
	logging("mode",0);
	if ( getCPDistance() > 7500 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
#define CURV2SPEED 68
	if ( currentSequence("[Operator::lineTraceSample] 第２カーブ前半") ) {
	    machine->azimuth.ratio(0.3,(100-CURV2SPEED)); // 早いスピードで小回りする
	}
	curveTo(CURV2SPEED,-770);
	if ( getCPDistance() > 9100 && machine->getRGB(1,0,0) < 100 ) { nextSequence();
	}

    } else if ( seqnum++ == getSequenceNumber() ) {
	if ( currentSequence("[Operator::lineTraceSample] 第２カーブ後半") ) {
	    machine->azimuth.ratio(2,15); // 超高速で小回りする
	}
	moveAt(CURV2SPEED,15);
	if ( getCounter() > 30 ) { nextSequence();
	    machine->azimuth.ratio(0.1,20); // デフォルトに戻す
	}

    } else if ( seqnum++ == getSequenceNumber() ) {
	if ( currentSequence("[Operator::lineTraceSample] 第２カーブ後直線") ) {
	}
	lineTraceAt(70,&withR60);
	logging("mode",0);
	if ( getCPDistance() > 10200 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	if ( currentSequence("[Operator::lineTraceSample] 第３カーブ") ) {
	    stopLogging("distL"); stopLogging("distR");
	    stopLogging("rgbR"); stopLogging("turn");
	    stopLogging("seqnum"); stopLogging("mode");
	}
	lineTraceAt(50,&withR60);
	if ( getCPDistance() > 11150 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::lineTraceSample] 第３カーブ後直線");
	lineTraceAt(50,&withR60);
	if ( getCPDistance() > 12700 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::lineTraceSample] 第４カーブ(きつい)");
	lineTraceAt(40,&withR60);
	if ( getCPDistance() > 13800 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::lineTraceSample] 第４カーブ後直線");
	lineTraceAt(50,&withR60);
	if ( getCPDistance() > 14600 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::lineTraceSample] 第５カーブ(きつい)");
	lineTraceAt(40,&withR60);
	if ( getCPDistance() > 15450 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::lineTraceSample] 第５カーブ後直線");
	lineTraceAt(50,&withR60);
	if ( getCPDistance() > 17100 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::lineTraceSample] 第６カーブ");
	lineTraceAt(50,&withR60);
	if ( getCPDistance() > 17600 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::lineTraceSample] 第６カーブ後直線");
	lineTraceAt(50,&withR60);
	if ( getCPDistance() > 18400 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::lineTraceSample] 第７カーブ");
	lineTraceAt(50,&withR60);
	if ( getCPDistance() > 20000+500 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::lineTraceSample] 第７カーブ後直線");
	lineTraceAt(80,&withR60);
	if ( getCPDistance() > 25350-500 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::lineTraceSample] 第８カーブ");
	lineTraceAt(50,&withR60);
	if ( getCPDistance() > 27350 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::lineTraceSample] 第８カーブ後直線");
	lineTraceAt(50,&withR60);
	if ( getCPDistance() > 27500 ) nextSequence();

    } else {
	currentSequence("[Operator::lineTraceSample] ライントレース終了");
	nextMethod(&Operator::slalomOn);
    }
    logging("seqnum",getSequenceNumber());
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


int Operator::moveAt( int spd, int turn )
{
    spd = machine->speed.calc(spd);
    turn = machine->azimuth.calcSpeed(turn);
    machine->moveDirect(spd,turn);
    return spd;
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
    turn = machine->moveDirect(spd,turn);
    machine->azimuth.setSpeed(turn);
    return spd;
}

// slalomAvoid() からスタートする場合は、次のコマンドを実行する。
// 左コース用：
// curl -X POST -H "Content-Type: application/json" -d '{"initLX":"8.0","initLY":"0","initLZ":"15.5","initLROT":"90"}' http://localhost:54000
// 右コース用：
// curl -X POST -H "Content-Type: application/json" -d '{"initRX":"-10.5","initRY":"0","initRZ":"15.5","initRROT":"-90"}' http://localhost:54000

void Operator::slalomAvoid()
{
    int seqnum = 0;
    if ( seqnum++ == getSequenceNumber() ) {
    currentSequence("[Operator::slalomAvoid] スラローム回避開始");
    nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
    currentSequence("[Operator::slalomAvoid] 板にぶつかるまでライントレースする");
    lineTraceAt(30,&withR60);
    if ( getCounter() > 1000 ) nextSequence(); // 4ms x 1000 秒たったら次へ

    } else if ( seqnum++ == getSequenceNumber() ) {
    currentSequence("[Operator::slalomAvoid] 姿勢を揃える");
    moveAt(10);
    if ( getCounter() > 1000 ) nextSequence(); // 4ms x 1000 秒たったら次へ

    } else if ( seqnum++ == getSequenceNumber() ) {
    currentSequence("[Operator::slalomAvoid] 徐々に速度を下げる");
    int speed = moveAt(0);
    if ( speed == 0 ) nextSequence(AZIMUTH|DIST); // 速度が0になったら、板の端をチェックポイントとしてマークして次へ

    } else if ( seqnum++ == getSequenceNumber() ) {
    currentSequence("[Operator::slalomAvoid] バックする");
    moveAt(-10);
    if ( getCPDistance() < -300 ) nextSequence(); // チェックポイントの後方 -300 まで来たら次へ

    } else if ( seqnum++ == getSequenceNumber() ) {
    currentSequence("[Operator::slalomAvoid] ブロック方向を向いて走る");
    curveTo(20,250);
    if ( getCPDistance() > 1550 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
    currentSequence("[Operator::slalomAvoid] ブロックの乗っている黒線を見つけるまで進む");
    moveAt(20);
    if ( machine->getRGB() < 30 ) nextSequence(DIST);

    } else if ( seqnum++ == getSequenceNumber() ) {
    currentSequence("[Operator::slalomAvoid] ブロック方向を向く");
    curveTo(7,-15);
    if ( checkAzimuth(-17) ) nextSequence(); 

    } else if ( seqnum++ == getSequenceNumber() ) {
    currentSequence("[Operator::slalomAvoid] ブロックを取りに行く");
    moveAt(40);
    if ( getCPDistance() > 1000 ) nextSequence(); 

    } else if ( seqnum++ == getSequenceNumber() ) {
    currentSequence("[Operator::slalomAvoid] 下方向");
    curveTo(20,-100);
    if ( getCPDistance() > 2600 ) nextSequence(); 

    } else if ( seqnum++ == getSequenceNumber() ) {
    currentSequence("[Operator::slalomAvoid] ガレージ方向");
    curveTo(20,120);
    if ( getCPDistance() > 3500 ) nextSequence(); 

    } else if ( seqnum++ == getSequenceNumber() ) {
    currentSequence("[Operator::slalomAvoid] ゴール前の黒線を見つけるまで進む");
    moveAt(20);
    if ( machine->getRGB(1,0,0) < 50 ) nextSequence(AZIMUTH|DIST);
    
    }else if ( seqnum++ == getSequenceNumber() ) {
    currentSequence("[Operator::slalomAvoid] ガレージ方向を向く");
    curveTo(2,360);
    if ( checkAzimuth(360) ) nextSequence(); 

    } else if ( seqnum++ == getSequenceNumber() ) {
    currentSequence("[Operator::slalomAvoid] ライントレースしながらガレージへ");
    lineTraceAt(10,&withR60);
    if ( machine->getSonar() < 50 )nextSequence();

    }else if ( seqnum++ == getSequenceNumber() ) {
    currentSequence("[Operator::slalomAvoid] 距離50");
    lineTraceAt(10,&withR60);
	if ( machine->getSonar() < 40 )nextSequence();

    }else if ( seqnum++ == getSequenceNumber() ) {
    currentSequence("[Operator::slalomAvoid] 距離40");
    lineTraceAt(10,&withR60);
	if ( machine->getSonar() < 30 )nextSequence();

    }else if ( seqnum++ == getSequenceNumber() ) {
    currentSequence("[Operator::slalomAvoid] 距離30");
    lineTraceAt(10,&withR60);
	if ( machine->getSonar() < 25 )nextSequence();

    }else if ( seqnum++ == getSequenceNumber() ) {
    currentSequence("[Operator::slalomAvoid] 距離25 直進モード");
    moveAt(10);
	if ( machine->getSonar() < 10 )nextSequence();

    }else if ( seqnum++ == getSequenceNumber() ) {
    currentSequence("[Operator::slalomAvoid] 距離10 直進モード");
	moveAt(10);
	if ( machine->getSonar() < 4 )nextSequence();

    }else if ( seqnum++ == getSequenceNumber() ) {
    currentSequence("[Operator::slalomAvoid] 距離4 停止");
	moveAt(0);
    if ( getCounter() > 1000 ) nextSequence(); // 4ms x 1000 秒たったら次へ

    } else if ( seqnum++ == getSequenceNumber() ) {
    currentSequence("[Operator::slalomAvoid] 走行完了");
	nextMethod(NULL);
    } 

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
	if ( getCounter() > 800 ) nextSequence(); // 4ms x 800 秒たったら次へ
	 
    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOn] 姿勢を揃える");
	moveAt(10);
	if ( getCounter() > 200 ) nextSequence(); // 4ms x 200 秒たったら次へ

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOn] 徐々に速度を下げる");
	int speed = moveAt(0);
	if ( speed == 0 ) nextSequence(DIST); // 速度が0になったら、板の端をチェックポイントとしてマークして次へ

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOn] バックする");
	moveAt(-20);
	if ( getCPDistance() < -280 ) nextSequence(); // チェックポイントの後方 -280 まで来たら次へ

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOn] アームを上げつつつ前進する");
	machine->armMotor->setAngle(-20);
	moveAt(50);
	if ( getCPDistance() > 250 ) nextSequence(); // チェックポイントの前方 250 まで来たら次へ

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOn] アームをおろしつつ減速しライントレースする");
	machine->armMotor->setAngle(-50);
	int speed = lineTraceAt(10,&withR30);
	//if ( speed == 0 ) nextSequence(); // 停止したら次へ
 	if ( getCPDistance() > 500 ) nextSequence(); // チェックポイントから 650 まで来たら次へ

	//} else if ( seqnum++ == getSequenceNumber() ) {
	//currentSequence("[Operator::slalomOn] 逆走する");
	//moveAt(-10);
	//if ( getCPDistance() < 650 ) nextSequence(); // チェックポイントから 650 までバックしたら次へ

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOn] 前進しつつライントレースする");
	lineTraceAt(20,&withR30);
	if ( getCPDistance() > 870 ) nextSequence(AZIMUTH); // チェックポイントから 870 まで来たら、角度をリセットして次へ

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOn] 右に移動");
	curveTo(20,120);
	if ( getRelDistance() > 225 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOn] 直進");
	curveTo(20,0);
	if ( getRelDistance() > 225 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOn] 左に移動");
	curveTo(20,-120);
	if ( getRelDistance() > 520 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOn] 直進");
	curveTo(20,0);
	if ( getRelDistance() > 200 ) nextSequence();
	//if ( getRelDistance() > 400 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOn] 右に移動");
	curveTo(20,120);
	//if ( getRelDistance() > 200 ) nextSequence();
	if ( getRelDistance() > 400 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOn] 直進");
	curveTo(20,0);
	//if ( getRelDistance() > 50 ) nextSequence();
	if ( getRelDistance() > 430 ) nextSequence();

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
	curveTo(20,-90);
	if ( machine->getRGB() < 30 ) nextSequence();

//   } else if ( seqnum++ == getSequenceNumber() ) {
//	currentSequence("[Operator::slalomOff] そのまま少し進む");
//	curveTo(20,-90);
//	if ( getRelDistance() > 100 ) nextSequence();

//    } else if ( seqnum++ == getSequenceNumber() ) {
//	currentSequence("[Operator::slalomOff] 前方を向く");
//	curveTo(20,0);
//	if ( checkAzimuth(0) ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOff] ライントレースする");
	lineTraceAt(20,&withR30);
	if ( getRelDistance() > 250 ) nextSequence(AZIMUTH); // 方角をリセットする

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOff] ライントレースしつつ黒線が切れるところを探す");
	lineTraceAt(20,&withR30);
	if ( machine->getRGB() > 120 ) nextSequence(DIST); // 距離をリセットする

    //} else if ( seqnum++ == getSequenceNumber() ) {
	//currentSequence("[Operator::slalomOff] 少しバックしつつ角度調整する");
	//curveTo(-20,0);
	//if ( getCPDistance() < -50 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOff] バックしながら90度以上回転する");
	curveTo(-10,330);
	if ( checkAzimuth(320) ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOff] 直進");
	curveTo(20,330);
	if ( getRelDistance() > 650 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOff] さらに右に移動");
	curveTo(20,370);
	if ( getRelDistance() > 260 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	machine->armMotor->setAngle(-20);
	currentSequence("[Operator::slalomOff] アーム上げて直進");
	curveTo(50,330);
	if ( getRelDistance() > 800 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOff] アーム下げて直進");
	machine->armMotor->setAngle(-50);
	curveTo(30,310);
	if ( getCPDistance() > 1500 ) nextSequence();

    //} else if ( seqnum++ == getSequenceNumber() ) {
	//currentSequence("[Operator::slalomOff] 少しバックする");
	//moveAt(-20);
	//if ( getCPDistance() < 950 ) nextSequence();

    //} else if ( seqnum++ == getSequenceNumber() ) {
	//currentSequence("[Operator::slalomOff] ライントレースせずに加速してアームを上げる");
	//machine->armMotor->setAngle(-20);
	//moveAt(50);
	//if ( getCPDistance() > 1500 ) nextSequence();

    //} else if ( seqnum++ == getSequenceNumber() ) {
	//currentSequence("[Operator::slalomOff] ライントレースせずに停止してアームを下げる");
	//machine->armMotor->setAngle(-50);
	//int speed = moveAt(0);
	//if ( speed == 0 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::slalomOff] ブロックの方に向く");
	curveTo(0,580);
	if ( checkAzimuth(580)) nextSequence();

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
//ログ
//        startLogging("distL");
//        startLogging("distR");
//        startLogging("rgbR");
//        startLogging("rgbG");
//       startLogging("rgbB");
//        startLogging("seqnum");
	nextSequence(AZIMUTH|DIST); // 方角と距離をリセットする

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::moveToBlock] １つめの黒線を見つけるまで進む");
	curveTo(30,0);
	if ( machine->getRGB() < 200 ) nextSequence(DIST); // 距離をリセットする

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::moveToBlock] 少しの間だけ黒線を無視して右に向く");
	curveTo(10,270);
	if ( getCPDistance() > 100 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::moveToBlock] 右に向きつつ２つめの黒線を見つけるまで進む");
	curveTo(30,280);
	if ( machine->getRGB() < 30 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::moveToBlock] 前方を向く");
	curveTo(0,-270);
	if ( checkAzimuth(0) ) nextSequence(DIST); // 距離をリセットする

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::moveToBlock] 直進する");
	//lineTraceAt(20,&withR60);
	moveAt(20);
	if ( getCPDistance() > 620 ) nextSequence(AZIMUTH); // 角度をリセットする
/*
    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::moveToBlock] ブロックをキャッチしに行く");
	curveTo(20,0);
	if ( getCPDistance() > 600 ) nextSequence();
*/
    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::moveToBlock] 左に移動");
	curveTo(20,-200);
	if ( getCPDistance() > 1350 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::moveToBlock] 戻る");
	curveTo(15,-540);
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
#define FORWARDANGLE -30
	currentSequence("[Operator::moveToGarage] ゆっくりとブロックをキャッチする");
	curveTo(0,FORWARDANGLE);
	nextSequence();
  
    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::moveToGarage] 左カープする");
	curveTo(50,-20);
	if ( getCPDistance() > 300 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::moveToGarage] 早くブロックを運ぶ");
	curveTo(50,FORWARDANGLE);
	if ( getCPDistance() > 1700 ) nextSequence();

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
	curveTo(10,0);
	if ( getRelDistance() > 500 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::moveToGarage] 右カープし反転する");
	curveTo(10,360);
	if ( getRelDistance() > 420 ) nextSequence();

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
	curveTo(0,300);
	if ( checkAzimuth(300) ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::moveToGarage] ライントレースする");
	lineTraceAt(10,&withR60);
	if ( getRelDistance() > 500 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::moveToGarage] ライントレースする(継続)");
	lineTraceAt(20,&withR60);
	if ( machine->getRGB(1,1,1) > 200 ) nextSequence();

    } else if ( seqnum++ == getSequenceNumber() ) {
	currentSequence("[Operator::moveToGarage] そのまま直進する");
	moveAt(15);
	if ( getRelDistance() > 550 ) nextSequence();

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
    printf("Operator destructor\n");
}

void Operator::trial()
{
    nextMethod(NULL);
}
