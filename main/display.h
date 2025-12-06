#pragma once

void vDisplayHardwareInit(void);    /* init MAX7219, SPI – không tạo task */
void vDisplayTask(void *pvParameters);
