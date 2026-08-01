#ifndef APPLICATION_TASK_DUAL_LOOP_DIAG_H_
#define APPLICATION_TASK_DUAL_LOOP_DIAG_H_

#include <stdint.h>

typedef enum
{
    DUAL_LOOP_DIAG_AB_STARTUP = 0,
    DUAL_LOOP_DIAG_ONE_LAP = 1
} DualLoopDiagCase;

void TaskDualLoopDiag_Run(void);

#endif
