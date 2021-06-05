//
//  app.cpp
//
//  Copyright © 2021 Ahiruchan Koubou. All rights reserved.
//

#include "ev3api.h"
#include "app.h"
#include "Machine.hpp"
#include "Operator.hpp"
#include "Logger.hpp"

Machine* machine;
Operator* operater;

/* メインタスク */
void main_task(intptr_t unused)
{
    initlog();
    machine = new Machine();
    operater = new Operator(machine);

    sta_cyc(CYC_LOG_TSK);
    sta_cyc(CYC_OPE_TSK);

    log("%s cource",(operater->EDGE > 0)?"Left":"Right");
    log("Hit SPACE bar to start");

    // sleep until being waken up
    ER ercd = slp_tsk();
    assert(ercd == E_OK);

    stp_cyc(CYC_OPE_TSK);
    stp_cyc(CYC_LOG_TSK);

    delete operater;
    delete machine;
    logput();
    ext_tsk();
}

/* 動作タスク */
void operation_task(intptr_t unused)
{
    if ( !machine->detect() ) {
	wup_tsk(MAIN_TASK);
    } else if ( !operater->operate() ) {
	wup_tsk(MAIN_TASK);
    }
}

/* ログ出力タスク */
void log_task(intptr_t unused)
{
    logput();
}
