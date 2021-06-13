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
    void lineTrace();
    void slalomOn();
    ~Operator();
};

#endif /* Operator_hpp */
