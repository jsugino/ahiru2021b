//
//  app.h
//
//  Copyright © 2021 Ahiruchan Koubou. All rights reserved.
//

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Task periods in micro seconds
 * Note: It used to be in ms with HRP2 kernel)
 */
#define PERIOD_OPE_TSK  ( 4 * 1000)
#define PERIOD_LOG_TSK  ( 4 * 1000)

/*
 * Default task stack size in bytes
 */
#ifndef STACK_SIZE
#define STACK_SIZE      4096        /* タスクのスタックサイズ */
#endif /* STACK_SIZE */

/*
 *  関数のプロトタイプ宣言
 */
#ifndef TOPPERS_MACRO_ONLY

extern void main_task(intptr_t exinf);
extern void operation_task(intptr_t exinf);
#if 0 /* yamanaka_s */
://extern void observer_task(intptr_t exinf);
#endif /* yamanaka_s */
extern void log_task(intptr_t exinf);

#endif /* TOPPERS_MACRO_ONLY */

#ifdef __cplusplus
}
#endif
