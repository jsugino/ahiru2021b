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
    /* コースマップを使って走る */
    void blindRunner();
    /* 走行開始 */
	void startRun();
    /* 走行距離更新 */
    void updateDistance();
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
	{"Bcv01", 2500, 0.5334},
	{"Bcv02", 3300, 0.0}, // the cv01 end point used to set d_cv01_midpoint below!!!
	{"Bcv03", 4200, 0.5334}, // the cv01 end point used to set d_cv01_midpoint below!!!
	{"Lcv15",DIST_end_blind,-0.247}
}; // Note: size of this array is given by sizeof(courseMap)/sizeof(*courseMap)

#endif /* Operator_hpp */
