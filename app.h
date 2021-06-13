//
//  app.h
//
//  Copyright © 2021 Ahiruchan Koubou. All rights reserved.
//

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  各タスクの優先度の定義
 */

#define PRIORITY_LOG_TSK    (TMIN_APP_TPRI + 0)
#if 0 /* yamanaka_s */
://#define PRIORITY_OBS_TSK    (TMIN_APP_TPRI + 1)
://#define PRIORITY_OPE_TSK    (TMIN_APP_TPRI + 2)
://#define PRIORITY_MAIN_TASK  (TMIN_APP_TPRI + 3)
#else /* yamanaka_s */
#define PRIORITY_OPE_TSK    (TMIN_APP_TPRI + 1)
#define PRIORITY_MAIN_TASK  (TMIN_APP_TPRI + 2)
#endif /* yamanaka_s */

/**
 * Task periods in micro seconds
 * Note: It used to be in ms with HRP2 kernel)
 */
#if 1 /* yamanaka_s */
#define PERIOD_OPE_TSK  ( 500 )
#else /* yamanaka_s */
://#define PERIOD_OPE_TSK  ( 4 * 1000)
#endif /* yamanaka_s */
#if 0 /* yamanaka_s */
://#define PERIOD_OBS_TSK  ( 4 * 1000)
#endif /* yamanaka_s */
#define PERIOD_LOG_TSK  ( 400 * 1000)

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
