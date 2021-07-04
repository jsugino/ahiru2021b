//
//  Operator.hpp
//
//  Copyright © 2021 Ahiruchan Koubou. All rights reserved.
//

#ifndef Operator_hpp
#define Operator_hpp

#include "Machine.hpp"
#include "ahiru_common.hpp"

// 台形制御用のクラス
class RampControler {
private:
    int16_t target;
    int16_t current;
    int16_t counter;
    int16_t ratioA;
    int16_t ratioB;
public:
    RampControler( double ratio );
    int16_t calc( int16_t target );
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
    const int EDGE = -1;
#else
    const int EDGE = 1;
#endif
    Operator( Machine* machine );
    bool operate();
    //void waitForTouch();
    void lineTrace();
    ~Operator();

    // スラローム用定義
    RampControler speed;
    int slalomStatus;
    int32_t slalomCounter;
    int32_t slalomDistance;
    void slalomOn();
    void slalomOff();
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
