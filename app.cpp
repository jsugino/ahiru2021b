//
//  app.cpp
//
//  Copyright © 2021 Ahiruchan Koubou. All rights reserved.
//

#include "ev3api.h"
#include "app.h"
#include "Machine.hpp"
#include "Operator.hpp"

Machine* machine;
Operator* operater;

/* メインタスク */
void main_task(intptr_t unused)
{
	machine = new Machine();
	operater = new Operator(machine);
    sta_cyc(CYC_OPE_TSK);

	printf((operater->EDGE > 0)?"Left course\n":"Right course\n");
    printf("Hit SPACE bar to start\n");

    // sleep until being waken up
    ER ercd = slp_tsk();
    assert(ercd == E_OK);

    stp_cyc(CYC_OPE_TSK);

	delete operater;
	delete machine;
    ext_tsk();
}

/* 動作タスク */
void operation_task(intptr_t unused)
{
	if ( !operater->operate() ) {
		wup_tsk(MAIN_TASK);
	}
}
