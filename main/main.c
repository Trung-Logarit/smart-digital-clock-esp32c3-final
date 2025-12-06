#include "app_state.h"
#include "wifi.h"
#include "time_svc.h"
#include "display.h"
#include "button.h"
#include "sensor_dht.h"
#include "buzzer.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "alarm_task.h"
#include "insight.h"
#include "rmaker_alarm.h"
#include "led.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"

void app_main(void)
{
    /* 1. NVS init */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    /* 3. Tạo RTOS primitives (tất cả ở đây) */
    g_time_mutex      = xSemaphoreCreateMutex();
    configASSERT(g_time_mutex != NULL);

    s_wifi_event_group = xEventGroupCreate();
    configASSERT(s_wifi_event_group != NULL);

    /* Queue cho nút bấm */
    QueueHandle_t xButtonQueue = xQueueCreate(10, sizeof(ButtonEvent_t));
    configASSERT(xButtonQueue != NULL);

    /* Queue cho báo thức */
    QueueHandle_t xAlarmQueue = xQueueCreate(8, sizeof(AlarmCommand_t));
    configASSERT(xAlarmQueue != NULL);

    TimerHandle_t xAlarmCheckTimer = xTimerCreate("alarm_chk",
                                                   pdMS_TO_TICKS(1000),
                                                   pdTRUE,                // auto-reload
                                                   NULL,
                                                   vAlarmCheckTimerCallback);
    configASSERT(xAlarmCheckTimer != NULL);
    xTimerStart(xAlarmCheckTimer, 0);


    /* 4. Init module không tạo RTOS object */
    
    vDisplayHardwareInit();
    vButtonGpioInit();
    vAlarmLedInit();
    vBuzzerInit();
    vTimeServiceInit();
    vRmakerAlarmInit();
    vInsightInit();
    

    /* 5. Tạo các task FreeRTOS – tất cả ở đây */
    xTaskCreate(vButtonTask,  "Button Task",  3072, xButtonQueue,10, NULL);
    xTaskCreate(vAlarmTask,   "Alarm Task",   3072, xAlarmQueue,  7, NULL);
    xTaskCreate(vTimeTask,    "Time Task",    2048, NULL,         5, NULL);
    xTaskCreate(vDisplayTask, "Display Task", 4096, NULL,         5, NULL);
    xTaskCreate(vDhtTask,     "DHT Task",     2048, NULL,         5, NULL);
    xTaskCreate(vNtpTask,     "NTP Task",     4096, NULL,         5, NULL);
}
