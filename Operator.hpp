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
	int mode;
	void (Operator::*nowfunc)();
public:
    Operator( Machine* machine );
	bool operate();
	void waitForTouch();
	void lineTrace();
    ~Operator();
};

#endif /* Operator_hpp */
