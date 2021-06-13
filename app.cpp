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
#if 0 /* yamanaka_s */
://#include "Observer.hpp"
#endif /* yamanaka_s */

Machine* machine;
Operator* operater;
#if 0 /* yamanaka_s */
://Observer* observer;
#endif /* yamanaka_s */

/* メインタスク */
void main_task(intptr_t unused)
{
#if 0 /* yamanaka_s */
://    Motor*          leftMotor;
://    Motor*          rightMotor;
#endif /* yamanaka_s */
    initlog();
    machine = new Machine();
    operater = new Operator(machine);
#if 0 /* yamanaka_s */
://    leftMotor   = new Motor(PORT_C);
://    rightMotor  = new Motor(PORT_B);
://    observer = new Observer(leftMotor, rightMotor);
#endif /* yamanaka_s */

    sta_cyc(CYC_LOG_TSK);
    sta_cyc(CYC_OPE_TSK);
#if 0 /* yamanaka_s */
:// 	sta_cyc(CYC_OBS_TSK);
#endif /* yamanaka_s */

    log("%s cource",(operater->EDGE > 0)?"Left":"Right");
    log("Hit SPACE bar to start");

    // sleep until being waken up
    ER ercd = slp_tsk();
    assert(ercd == E_OK);

    stp_cyc(CYC_OPE_TSK);
    stp_cyc(CYC_LOG_TSK);
#if 0 /* yamanaka_s */
://	stp_cyc(CYC_OBS_TSK);
#endif /* yamanaka_s */

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

#if 0 /* yamanaka_s */
:// Observer's periodic task
://void observer_task(intptr_t unused) {
://    if (observer != NULL) {
://		observer->operate();
://	}
://}
#endif /* yamanaka_s */

/* ログ出力タスク */
void log_task(intptr_t unused)
{
    logput();
}
