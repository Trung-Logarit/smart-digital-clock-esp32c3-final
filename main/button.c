#include "app_state.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "freertos/queue.h"
#include "time_svc.h"
#include "alarm_task.h"
#include "button.h"

// Thêm cho RainMaker factory reset
#include "esp_err.h"
#include "esp_rmaker_utils.h"   // khai báo esp_rmaker_factory_reset()

static const char *TAGB = "button";

/* Queue handle sẽ được set trong vButtonTask từ pvParameters */
static QueueHandle_t gpio_evt_queue = NULL;

static volatile int64_t s_last_btn1_us = -1;
static volatile int64_t s_last_btn2_us = -1;
static volatile int64_t s_last_btn3_us = -1;
static volatile int64_t s_last_btn4_us = -1;

static volatile int64_t s_btn1_press_us = -1;   // thời điểm bắt đầu nhấn BTN1
static volatile int64_t s_btn3_press_us = -1;
static volatile int64_t s_btn4_press_us = -1;

static const int64_t HOLD_CONFIRM_US = 1000000;

static void prvHandleButton1Normal(void);
static void prvHandleButton2Normal(void);

static inline int prvWrap(int v, int lo, int hi);
static void prvAlarmEnterOrToggleField(void);
static void prvAlarmConfirmIfHolding(int64_t held_us);
static void prvHandleBtn1Alarm(void);
static void prvHandleBtn2Alarm(void);

static void prvCdEnterOrToggleField(void);
static void prvCdConfirmIfHolding(int64_t held_us);
static void prvHandleBtn1Cd(void);
static void prvHandleBtn2Cd(void);

static inline bool bIsActiveLowPressed(gpio_num_t gpio) {
    return gpio_get_level(gpio) == 0;
}

static void IRAM_ATTR prvButtonIsrHandler(void *arg)
{
    if (!gpio_evt_queue) return;

    uint32_t gpio_num = (uint32_t)arg;
    ButtonEvent_t e = { .ulGpioNum = gpio_num, .llTimestampUs = esp_timer_get_time() };
    BaseType_t hpw = pdFALSE;
    xQueueSendFromISR(gpio_evt_queue, &e, &hpw);
    if (hpw) portYIELD_FROM_ISR();
}

/* ---- logic cũ đổi tên một chút cho rõ ---- */

static void prvHandleButton1Normal(void)
{
    if (s_mode == MODE_ALARM_SET) return;

    if (s_mode == MODE_SW) {
        s_mode = MODE_TIME;
        s_force_refresh = true;
        return;
    }

    s_mode = (display_mode_t)((s_mode + 1) % MODE_SW);
    if (s_mode == MODE_SW) s_mode = MODE_TIME;
    s_force_refresh = true;
}

static void prvHandleButton2Normal(void)
{
    if (s_mode != MODE_SW) {
        s_mode = MODE_SW;
        s_sw_state = SW_RESET_SHOWN;
        s_sw_mm = 0; s_sw_ss = 0;
        s_force_refresh = true;
        return;
    }

    if (s_sw_state == SW_RESET_SHOWN)      s_sw_state = SW_RUNNING;
    else if (s_sw_state == SW_RUNNING)     s_sw_state = SW_PAUSED;
    else {
        s_sw_state = SW_RESET_SHOWN;
        s_sw_mm = 0; s_sw_ss = 0;
    }
    s_force_refresh = true;
}

static inline int prvWrap(int v, int lo, int hi) {
    if (v < lo) return hi;
    if (v > hi) return lo;
    return v;
}

static void prvAlarmEnterOrToggleField(void)
{
    if (s_alarm_ringing) {
        s_alarm_ringing = false;
        vAlarmSendStopRing();
        ESP_LOGW(TAGB, "Alarm stopped by BTN3.");
        return;
    }

    if (s_mode != MODE_ALARM_SET) {
        struct tm nowtm;
        bTimeServiceGetLocalTime(&nowtm);
        s_alarm_hour = nowtm.tm_hour;
        s_alarm_min  = nowtm.tm_min;

        s_mode = MODE_ALARM_SET;
        s_alarm_sel = ALARM_SEL_HOUR;
        s_blink_on = true;
        s_force_refresh = true;
        ESP_LOGI(TAGB, "Enter ALARM SET from %02d:%02d", s_alarm_hour, s_alarm_min);

    } else {
        s_alarm_sel = (s_alarm_sel == ALARM_SEL_HOUR) ? ALARM_SEL_MIN : ALARM_SEL_HOUR;
        s_force_refresh = true;
    }
}

static void prvAlarmConfirmIfHolding(int64_t held_us)
{
    if (held_us >= HOLD_CONFIRM_US && s_mode == MODE_ALARM_SET) {
        s_alarm_enabled = true;
        s_force_refresh = true;

        vAlarmSendConfirmBeep();

        s_mode = MODE_TIME;
        s_force_refresh = true;
    }
}

static void prvHandleBtn1Alarm(void) {
    if (s_alarm_sel == ALARM_SEL_HOUR) s_alarm_hour = prvWrap(s_alarm_hour + 1, 0, 23);
    else                                s_alarm_min  = prvWrap(s_alarm_min  + 1, 0, 59);
    s_force_refresh = true;
}

static void prvHandleBtn2Alarm(void) {
    if (s_alarm_sel == ALARM_SEL_HOUR) s_alarm_hour = prvWrap(s_alarm_hour - 1, 0, 23);
    else                                s_alarm_min  = prvWrap(s_alarm_min  - 1, 0, 59);
    s_force_refresh = true;
}

static void prvCdEnterOrToggleField(void)
{
    if (s_alarm_ringing) {
        s_alarm_ringing = false;
        vAlarmSendStopRing();
        ESP_LOGW(TAGB, "Ring stopped by BTN4.");

        if (s_mode == MODE_COUNTDOWN_RUN && !s_cd_running) {
            s_mode = MODE_TIME;
            s_force_refresh = true;
        }
        return;
    }

    if (s_mode != MODE_COUNTDOWN_SET && s_mode != MODE_COUNTDOWN_RUN) {
        s_cd_min = 1; s_cd_sec = 0;
        s_cd_running = false;
        s_cd_sel = CD_SEL_MIN;
        s_mode = MODE_COUNTDOWN_SET;
        s_blink_on = true;
        s_force_refresh = true;

    } else if (s_mode == MODE_COUNTDOWN_SET) {
        s_cd_sel = (s_cd_sel == CD_SEL_MIN) ? CD_SEL_SEC : CD_SEL_MIN;
        s_force_refresh = true;
    }
}

static void prvCdConfirmIfHolding(int64_t held_us)
{
    if (held_us >= HOLD_CONFIRM_US && s_mode == MODE_COUNTDOWN_SET) {
        s_cd_running = true;
        s_mode = MODE_COUNTDOWN_RUN;
        s_force_refresh = true;
        vAlarmSendConfirmBeep();
    }
}

static void prvHandleBtn1Cd(void) {
    if (s_cd_sel == CD_SEL_MIN) s_cd_min = prvWrap(s_cd_min + 1, 0, 99);
    else                        s_cd_sec = prvWrap(s_cd_sec + 1, 0, 59);
    s_force_refresh = true;
}

static void prvHandleBtn2Cd(void) {
    if (s_cd_sel == CD_SEL_MIN) s_cd_min = prvWrap(s_cd_min - 1, 0, 99);
    else                        s_cd_sec = prvWrap(s_cd_sec - 1, 0, 59);
    s_force_refresh = true;
}

void vButtonTask(void *pvParameters)
{
    gpio_evt_queue = (QueueHandle_t)pvParameters;
    ButtonEvent_t e;
    const int64_t debounce_us = (int64_t)DEBOUNCE_MS * 1000LL;

    for (;;) {
        if (xQueueReceive(gpio_evt_queue, &e, portMAX_DELAY)) {
            uint32_t gpio = e.ulGpioNum;
            int64_t  now  = e.llTimestampUs;
            bool is_press = (gpio_get_level(gpio) == 0);

            /* BTN1 */
            if (gpio == BUTTON_GPIO) {
                if (is_press) {
                    /* debounce chỉ cho cạnh nhấn xuống */
                    if (s_last_btn1_us < 0 || (now - s_last_btn1_us) > debounce_us) {
                        s_last_btn1_us   = now;
                        s_btn1_press_us  = now;   // lưu thời điểm bắt đầu nhấn
                        vAlarmSendClickBeep();
                    }
                } else {
                    /* cạnh nhả ra: tính thời gian giữ và quyết định reset hay click bình thường */
                    int64_t held = -1;
                    if (s_btn1_press_us > 0) {
                        held = now - s_btn1_press_us;
                    }
                    s_btn1_press_us = -1;

                    if (held >= FACTORY_RESET_HOLD_US && s_mode == MODE_TIME) {
                        // Nhấn giữ đủ lâu trong chế độ xem giờ -> Factory Reset RainMaker
                        ESP_LOGW(TAGB,
                                 "BTN1 held %lld ms -> RainMaker factory reset",
                                 (long long)(held / 1000));

                        esp_err_t err = esp_rmaker_factory_reset(2, 2);
                        if (err != ESP_OK) {
                            ESP_LOGE(TAGB,
                                     "esp_rmaker_factory_reset failed: 0x%x",
                                     err);
                        }
                        // RainMaker sẽ tự reboot sau reboot_seconds

                    } else if (held >= 0) {
                        // Nhấn ngắn (hoặc giữ < 5s) -> hành vi cũ của BTN1
                        if      (s_mode == MODE_ALARM_SET)         prvHandleBtn1Alarm();
                        else if (s_mode == MODE_COUNTDOWN_SET)     prvHandleBtn1Cd();
                        else                                       prvHandleButton1Normal();
                    }
                }
            }

            /* BTN2 */
            else if (gpio == BUTTON2_GPIO) {
                if (s_last_btn2_us < 0 || (now - s_last_btn2_us) > debounce_us) {
                    s_last_btn2_us = now;
                    if (is_press) {
                        vAlarmSendClickBeep();

                        if      (s_mode == MODE_ALARM_SET)         prvHandleBtn2Alarm();
                        else if (s_mode == MODE_COUNTDOWN_SET)     prvHandleBtn2Cd();
                        else                                       prvHandleButton2Normal();
                    }
                }
            }

            /* BTN3 */
            else if (gpio == BUTTON3_GPIO) {
                if (is_press) {
                    if (s_last_btn3_us < 0 || (now - s_last_btn3_us) > debounce_us) {
                        s_last_btn3_us = now;
                        s_btn3_press_us = now;
                        vAlarmSendClickBeep();
                        prvAlarmEnterOrToggleField();
                    }
                } else {
                    if (s_btn3_press_us > 0) {
                        int64_t held = now - s_btn3_press_us;
                        s_btn3_press_us = -1;
                        prvAlarmConfirmIfHolding(held);
                    }
                }
            }

            /* BTN4 */
            else if (gpio == BUTTON4_GPIO) {
                if (is_press) {
                    if (s_last_btn4_us < 0 || (now - s_last_btn4_us) > debounce_us) {
                        s_last_btn4_us = now;
                        s_btn4_press_us = now;
                        vAlarmSendClickBeep();
                        prvCdEnterOrToggleField();
                    }
                } else {
                    if (s_btn4_press_us > 0) {
                        int64_t held = now - s_btn4_press_us;
                        s_btn4_press_us = -1;
                        prvCdConfirmIfHolding(held);
                    }
                }
            }
        }
    }
}

void vButtonGpioInit(void)
{
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO) |
                        (1ULL << BUTTON2_GPIO) |
                        (1ULL << BUTTON3_GPIO) |
                        (1ULL << BUTTON4_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE
    };
    gpio_config(&io);

    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(BUTTON_GPIO,  prvButtonIsrHandler, (void *)(uint32_t)BUTTON_GPIO));
    ESP_ERROR_CHECK(gpio_isr_handler_add(BUTTON2_GPIO, prvButtonIsrHandler, (void *)(uint32_t)BUTTON2_GPIO));
    ESP_ERROR_CHECK(gpio_isr_handler_add(BUTTON3_GPIO, prvButtonIsrHandler, (void *)(uint32_t)BUTTON3_GPIO));
    ESP_ERROR_CHECK(gpio_isr_handler_add(BUTTON4_GPIO, prvButtonIsrHandler, (void *)(uint32_t)BUTTON4_GPIO));
}
