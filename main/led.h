#pragma once
#include "driver/gpio.h"
#include "app_state.h"

static inline void vAlarmLedInit(void) {
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << ALARM_LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io);
    gpio_set_level(ALARM_LED_GPIO, !ALARM_LED_ACTIVE_LEVEL);
}

static inline void vAlarmLedOn(void)  { gpio_set_level(ALARM_LED_GPIO,  ALARM_LED_ACTIVE_LEVEL); }
static inline void vAlarmLedOff(void) { gpio_set_level(ALARM_LED_GPIO, !ALARM_LED_ACTIVE_LEVEL); }
