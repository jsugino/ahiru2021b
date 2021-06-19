//
//  Operator.hpp
//
//  Copyright © 2021 Ahiruchan Koubou. All rights reserved.
//

#ifndef Operator_hpp
#define Operator_hpp

#include "Machine.hpp"
#include "ahiru_common.hpp"

class Operator {
private:
#if 1 /* yamanaka_s */
	double		distance;
	int32_t		prevAngL, prevAngR;
	int32_t		logCnt;
    int32_t     courseMapindex;	
    /* ショートカット */
    void shortCut();
    /* 走行距離更新 */
    void updateDistance();
    /* コースマップを使って走る */
    void useCourseMap();
#endif /* yamanaka_s */
protected:
    Machine* machine;
    int mode;
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
    void slalomOn();
};
struct courseSection {
	char	id[6];
	int32_t sectionEnd;
	double  curvature;
};
const struct courseSection courseMap[] = {
	{"Bst00", 2000, 0.0},    // the st00 end point used to set d_cv01_midpoint below!!!
	{"Bcv01", 2700, 0.5334},
	{"Bcv01", 1821, 0.5333}, // the cv01 end point used to set d_cv01_midpoint below!!!
	{"Bst02", 2175, 0.0},
	{"Bcv03", 3361,-0.4793},
	{"Bst04", 3902, 0.0},
  	{"Bcv05", 4527,-0.45},
	{"Bst06", 5050, 0.0},
	{"Bcv07", 5676, 0.45},
	{"Bst08", 5951, 0.0},
	{"Bcv09", 6567, 0.45},
	{"Bst10", 6875, 0.0},  //  6905
	{"Bcv11", 7645, 0.3},  //  7675
	{"Bst12", 9040, 0.0},  //  9070
	{"Bcv13", 9715,-0.33}, //  9745
	{"Rst14",10733, 0.0},  // 10763
	{"Lst14",10733, 0.0},  // 10763
	{"Lcv15",DIST_end_blind,-0.247}
}; // Note: size of this array is given by sizeof(courseMap)/sizeof(*courseMap)

#endif /* Operator_hpp */
