#pragma once
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    eAlarmCmdClickBeep = 0,
    eAlarmCmdConfirmBeep,
    eAlarmCmdStartRing,
    eAlarmCmdStopRing,
} AlarmCommand_t;

/* Task entry – tạo task ở app_main */
void vAlarmTask(void *pvParameters);

/* API gửi lệnh beep / stop qua queue (queue được set trong vAlarmTask) */
void vAlarmSendClickBeep(void);
void vAlarmSendConfirmBeep(void);
void vAlarmSendStopRing(void);

void vRingLoop(void);

void vAlarmCheckTimerCallback(TimerHandle_t xTimer);

static inline void vAlarmCmdStop(void)
{
    vAlarmSendStopRing();
}

#ifdef __cplusplus
}
#endif
