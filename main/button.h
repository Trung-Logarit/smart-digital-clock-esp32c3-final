#pragma once
#include <stdint.h>

/* Sự kiện queue của nút bấm – dùng cho xQueueCreate ở app_main */
typedef struct {
    uint32_t ulGpioNum;
    int64_t  llTimestampUs;
} ButtonEvent_t;

/* Config GPIO + ISR, KHÔNG tạo task hay queue */
void vButtonGpioInit(void);

/* Task xử lý nút, app_main tạo task và truyền QueueHandle_t qua pvParameters */
void vButtonTask(void *pvParameters);
