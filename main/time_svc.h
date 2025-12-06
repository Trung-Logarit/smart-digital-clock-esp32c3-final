#pragma once
#include <stdbool.h>
#include <time.h>

void vTimeServiceInit(void);          /* chỉ set timezone, không tạo mutex hay task */
void vTimeTask(void *pvParameters);   /* app_main tạo task này */
void vNtpTask(void *pvParameters);    /* app_main tạo task này */

bool bTimeServiceGetLocalTime(struct tm *out);
