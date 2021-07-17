//
//  Operator.hpp
//
//  Copyright © 2021 Ahiruchan Koubou. All rights reserved.
//

#ifndef Operator_hpp
#define Operator_hpp

#include "Machine.hpp"
#include "ahiru_common.hpp"

class LineTraceLogic
{
public:
    virtual int calcTurn( Machine* rgb, int speed ) = 0;
protected:
    virtual ~LineTraceLogic() = 0;
};

class Operator {
private:
#if 1 /* yamanaka_s */
	double		distance;
	int32_t		prevAngL, prevAngR;
	int32_t		logCnt;
    int32_t     courseMapindex;	
    /* トライ＆エラー用のプログラム */
    void lineTraceDummy();
    void lineTraceSample();
    void slalomOnPrep();
    void reverseEdge();
    /* コースマップを使って走る */
    void blindRunner();
    /* 走行開始 */
	void startRun();
    /* 走行距離更新 */
    void updateDistance();
#endif /* yamanaka_s */
protected:
    Machine* machine;
    void (Operator::*currentMethod)();
public:
#if defined(MAKE_RIGHT)
    static const int EDGE = -1;
#else
    static const int EDGE = 1;
#endif
    Operator( Machine* machine );
    bool operate();
    //void waitForTouch();
    void lineTrace();
    ~Operator();

    // 難所攻略用定義
private:
    int sequenceNumber;
    int slalomMessagedStatus;
    int32_t slalomCounter;
    int32_t slalomDistance;
    int32_t slalomAzimuth;
    int32_t slalomPreviousDistance;
public:
    // Status : 現在の処理番号
    int getSequenceNumber() { return sequenceNumber; }

    // Counter : この処理番号の実行回数 (０オリジン)
    int getCounter() { return slalomCounter; }

    // Current Sequence : 現在の処理内容
    bool currentSequence( const char* const message, ... );

    // Goto Next Status : 次の処理番号に進む
    void nextSequence( int falgs = 0 );
    // flags には次のものが指定可能
    const int DIST = 1;    // 距離をリセットする (markCheckPoint() をする)
    const int AZIMUTH = 2; // 方角をリセットする (resetAzimuth() をする)
    // 複数のフラグを同時に設定する場合は、次のように | で指定する。
    // nextSequence(DIST|ANGLE);

    // Goto Next Method
    void nextMethod( void (Operator::*nextMethod)() );

    // Absolute Distance : 開始位置からの角度
    int32_t getAbsAzimuth() { return machine->distanceL - machine->distanceR; }

    // Relative Azimuth : リセット時からの相対方角
    int32_t getAzimuth() { return getAbsAzimuth() - slalomAzimuth; }

    // Check Azimuth : 相対角度が一定の誤差範囲内か確認する
    bool checkAzimuth( int32_t azi );

    // Reset Azimuth : 方角をリセットする
    void resetAzimuth() { slalomAzimuth = getAbsAzimuth(); }

    // Absolute Distance : 開始位置からの距離
    int32_t getAbsDistance() { return machine->distanceL + machine->distanceR; }

    // Distance from Check Point : 一つ前のチェックポイントからの距離
    int32_t getCPDistance() { return getAbsDistance() - slalomDistance; }

    // Mark Check Point : チェックポイントをマークする
    void markCheckPoint() { slalomDistance = getAbsDistance(); }

    // Relative Distance : 直前のシーケンスからの距離
    int32_t getRelDistance() { return getAbsDistance() - slalomPreviousDistance; }

    // 指定された速度と指定された角速度で進行する。返り値は台形制御後の速度
    int moveAt( int spd, int turn = 0 );

    // 指定された速度で、指定された方位に曲がる。返り値は台形制御後の角速度
    int curveTo( int spd, int azi );

    // ライントレース用メソッド
    int lineTraceAt( int spd, LineTraceLogic* logic );

    // 難所攻略用メソッド
    void slalomOn();
    void slalomOff();
    void moveToBlock();
    void moveToGarage();

    // 計測用のメソッド
    void azimuthCheck();
    void rampCheck();
};

struct courseSection {
	char	id[6];
	int32_t sectionEnd;
	double  curvature;
};
const struct courseSection courseMap[] = {
//	{"Bst00", 2000, 0.0},    // the st00 end point used to set d_cv01_midpoint below!!!
//	{"Bcv01", 2500, 0.5334},
//	{"Bcv02", 3300, 0.0}, // the cv01 end point used to set d_cv01_midpoint below!!!
//	{"Bcv03", 4200, 0.5555}, // the cv01 end point used to set d_cv01_midpoint below!!!
//	{"Lcv15",DIST_end_blind,-0.247}
    {"Bst00", 2050, 0.0},
    {"Bcv00", 2565, 0.5334},
    {"Bcv02", 3265, 0.0},
    {"Bcv03", 3980, 0.8},
    {"Bst04", 4285, 0.0},
    {"Bcv05", 4750, -0.5334},
    {"Bst06", 4910, 0.0},
    {"Bcv07", 5375, -0.5335},
    {"Bst08", 5835, 0.0},
    {"Bcv09", 6300, 0.5335},
    {"Bst10", 6608, 0.0},
    {"Bcv11", 7083, 0.5335},
    {"Bst12", 7093, 0.0},
//    {"Bst08", 5535, 0.0},
//    {"Bcv09", 6000, 0.5335},
//    {"Bst10", 6008, 0.0},
//    {"Bcv11", 6483, 0.5335},
//    {"Bst12", 6493, 0.0},
    {"Bcv13", 7133, -0.927},
    {"Bst14", 7200, 0.0},
    {"Lst15", 9000, 0.0},
    {"Lcv16",DIST_end_blind,0.247}

/*
    {"Bst00", 2050, 0.0},
    {"Bcv00", 2565, -0.5334},
    {"Bcv02", 3265, 0.0},
    {"Bcv03", 3980, -0.8},
    {"Bst04", 4285, 0.0},
    {"Bcv05", 4750, 0.5334},
    {"Bst06", 4910, 0.0},
    {"Bcv07", 5375, 0.5335},
    {"Bst08", 5535, 0.0},
    {"Bcv09", 6000, -0.5335},
    {"Bst10", 6008, 0.0},
    {"Bcv11", 6483, -0.5335},
    {"Bst12", 6493, 0.0},
    {"Bcv13", 7133, 0.927},
    {"Bst14", 7200, 0.0},
    {"Lst15", 9000, 0.0},
    {"Lcv16",DIST_end_blind,-0.247}
*/
    }; // Note: size of this array is given by sizeof(courseMap)/sizeof(*courseMap)

#endif /* Operator_hpp */
