#include <stdio.h>
#include "alarm_task.h"
#include "app_state.h"
#include "buzzer.h"
#include "led.h"
#include "time_svc.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

static const char *TAG = "alarm";

/* Queue dùng chung cho mọi request về alarm (button, v.v.) */
/* Được gán từ pvParameters trong vAlarmTask */
static QueueHandle_t s_alarm_q = NULL;

/* -------------------------------------------------------------------- */
/* Các helper nhỏ cho beep/buzzer                                       */
/* -------------------------------------------------------------------- */

/* Click beep ngắn */
static void prvClickBeep(void)
{
    vBuzzerOn();
    vTaskDelay(pdMS_TO_TICKS(40));
    vBuzzerOff();
}

/* Confirm beep: beep – pause – beep */
static void prvConfirmBeep(void)
{
    vBuzzerOn();
    vTaskDelay(pdMS_TO_TICKS(80));
    vBuzzerOff();
    vTaskDelay(pdMS_TO_TICKS(60));
    vBuzzerOn();
    vTaskDelay(pdMS_TO_TICKS(120));
    vBuzzerOff();
}

/* Mẫu reo chuông: kêu 200 ms, im 300 ms, lặp lại cho tới khi s_alarm_ringing=false */
void vRingLoop(void)
{
    ESP_LOGW(TAG, "Alarm RING start (%02d:%02d)", s_alarm_hour, s_alarm_min);

    while (s_alarm_ringing) {
        vBuzzerOn();
        vAlarmLedOn();
        vTaskDelay(pdMS_TO_TICKS(200));
        vBuzzerOff();
        vAlarmLedOff();
        vTaskDelay(pdMS_TO_TICKS(300));
    }

    /* Kết thúc chuông */
    vBuzzerOff();
    vAlarmLedOff();
    ESP_LOGI(TAG, "Alarm RING stopped.");
}

/* -------------------------------------------------------------------- */
/* Software timer callback: mỗi 1 giây check giờ hiện tại               */
/* -------------------------------------------------------------------- */

void vAlarmCheckTimerCallback(TimerHandle_t xTimer)
{
    (void)xTimer;

    /* Nếu alarm đang tắt thì thôi */
    if (!s_alarm_enabled) {
        return;
    }

    /* Nếu đang reo rồi thì không cần check trùng giờ nữa */
    if (s_alarm_ringing) {
        return;
    }

    struct tm now;
    if (!bTimeServiceGetLocalTime(&now)) {
        return;
    }

    /* Khi đến đúng giờ-phút-giây 00 thì kích hoạt ring */
    if ((now.tm_hour == s_alarm_hour) &&
        (now.tm_min  == s_alarm_min)  &&
        (now.tm_sec  == 0)) {

        AlarmCommand_t cmd = eAlarmCmdStartRing;
        if (s_alarm_q) {
            /* callback timer chạy trong context Timer Service, dùng FromISR an toàn nhất */
            BaseType_t xHigher = pdFALSE;
            xQueueSendFromISR(s_alarm_q, &cmd, &xHigher);
            if (xHigher) {
                portYIELD_FROM_ISR();
            }
        }
    }
}

/* -------------------------------------------------------------------- */
/* Task chính quản lý alarm + beep                                      */
/* -------------------------------------------------------------------- */

void vAlarmTask(void *pvParameters)
{
    /* Queue được tạo ở app_main, truyền qua pvParameters */
    s_alarm_q = (QueueHandle_t)pvParameters;
    configASSERT(s_alarm_q != NULL);

    ESP_LOGI(TAG, "Alarm task started (using external queue).");

    for (;;) {
        AlarmCommand_t cmd;
        if (xQueueReceive(s_alarm_q, &cmd, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        switch (cmd) {
        case eAlarmCmdClickBeep:
            prvClickBeep();
            break;

        case eAlarmCmdConfirmBeep:
            prvConfirmBeep();
            break;

        case eAlarmCmdStartRing:
            if (!s_alarm_ringing) {
                /* Đánh dấu trạng thái – display / button nhìn vào biến này */
                s_alarm_ringing = true;
            }
            vRingLoop();   // sẽ tự thoát khi s_alarm_ringing=false
            break;

        case eAlarmCmdStopRing:
            /* Đảm bảo tắt mọi thứ (an toàn nếu gọi nhiều lần). */
            s_alarm_ringing = false;
            vBuzzerOff();
            vAlarmLedOff();
            ESP_LOGI(TAG, "Alarm stop command received.");
            break;

        default:
            ESP_LOGW(TAG, "Unknown alarm cmd: %d", (int)cmd);
            break;
        }
    }
}

/* -------------------------------------------------------------------- */
/* API public cho các module khác                                       */
/* -------------------------------------------------------------------- */

void vAlarmSendClickBeep(void)
{
    if (!s_alarm_q) return;
    AlarmCommand_t cmd = eAlarmCmdClickBeep;
    xQueueSend(s_alarm_q, &cmd, 0);
}

void vAlarmSendConfirmBeep(void)
{
    if (!s_alarm_q) return;
    AlarmCommand_t cmd = eAlarmCmdConfirmBeep;
    xQueueSend(s_alarm_q, &cmd, 0);
}

void vAlarmSendStopRing(void)
{
    if (!s_alarm_q) return;
    AlarmCommand_t cmd = eAlarmCmdStopRing;
    xQueueSend(s_alarm_q, &cmd, 0);
}
