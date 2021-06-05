//
//  Operator.cpp
//
//  Copyright © 2021 Ahiruchan Koubou. All rights reserved.
//

#include "Operator.hpp"

Operator::Operator( Machine* mcn ) {
	printf("Operator constructor\n");
	machine = mcn;
	mode = 0;
	nowfunc = &Operator::waitForTouch;
}

bool Operator::operate()
{
	if ( nowfunc == NULL ) return false;
	(this->*nowfunc)();
	if ( nowfunc == NULL ) return false;
	return true;
}

void Operator::waitForTouch()
{
	if ( machine->touchSensor->isPressed() ) {
		nowfunc = &Operator::lineTrace;
	}
}

void Operator::lineTrace()
{
	if ( mode > 100 ) {
		nowfunc = NULL;
	} else {
		printf("lineTrace\n");
		++mode;
	}
}

Operator::~Operator() {
    printf("Operator destructor\n");
}
