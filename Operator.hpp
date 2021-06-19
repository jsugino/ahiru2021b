//
//  Operator.hpp
//
//  Copyright © 2021 Ahiruchan Koubou. All rights reserved.
//

#ifndef Operator_hpp
#define Operator_hpp

#include "Machine.hpp"

class Operator {
private:
#if 1 /* yamanaka_s */
	double		distance;
	int32_t		prevAngL, prevAngR;
	int32_t		logCnt;
    /* ショートカット */
    void shortCut();
    /* 走行距離更新 */
    void updatedistance();
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

#endif /* Operator_hpp */
