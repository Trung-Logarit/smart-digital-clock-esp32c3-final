#pragma once
#include "driver/gpio.h"
#include "app_state.h"

static inline void vBuzzerInit(void) {
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << BUZZER_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io);
    gpio_set_level(BUZZER_GPIO, !BUZZER_ACTIVE_LEVEL);
}

static inline void vBuzzerOn(void)  { gpio_set_level(BUZZER_GPIO,  BUZZER_ACTIVE_LEVEL); }
static inline void vBuzzerOff(void) { gpio_set_level(BUZZER_GPIO, !BUZZER_ACTIVE_LEVEL); }
