// rmaker_alarm.c — phiên bản FreeRTOS style

#include <string.h>
#include "esp_log.h"

#include "esp_rmaker_core.h"
#include "esp_rmaker_standard_devices.h"
#include "esp_rmaker_standard_params.h"

#include "app_state.h"       // s_alarm_*, s_force_refresh, g_dev
#include "alarm_task.h"      // vAlarmSendStopRing()
#include "max7219.h"
#include "rmaker_alarm.h"

#include "app_wifi.h"
#include "app_storage.h"
#include "wifi.h" 

static const char *TAG = "RmakerAlarm";

/* Handle RainMaker */
static esp_rmaker_node_t   *pxRmakerNode     = NULL;
static esp_rmaker_device_t *pxAlarmDevice    = NULL;
static esp_rmaker_param_t  *pxParamHour      = NULL;
static esp_rmaker_param_t  *pxParamMinute    = NULL;
static esp_rmaker_param_t  *pxParamBrightness = NULL;

/* ======================================================================
 *  CALLBACK xử lý các param nhận từ app RainMaker
 * ====================================================================== */
static esp_err_t prvAlarmWriteCallback(const esp_rmaker_device_t *pxDevice,
                                       const esp_rmaker_param_t *pxParam,
                                       const esp_rmaker_param_val_t xVal,
                                       void *pvPrivate,
                                       esp_rmaker_write_ctx_t *pxCtx)
{
    (void)pxDevice;
    (void)pvPrivate;

    const char *pcName = esp_rmaker_param_get_name(pxParam);

    if (pxCtx) {
        ESP_LOGI(TAG, "Write %s via %s",
                 pcName, esp_rmaker_device_cb_src_to_str(pxCtx->src));
    }

    /* POWER — bật/tắt báo thức */
    if (strcmp(pcName, ESP_RMAKER_DEF_POWER_NAME) == 0) {

        s_alarm_enabled = xVal.val.b;
        ESP_LOGI(TAG, "Alarm Power -> %s", s_alarm_enabled ? "ON" : "OFF");

        /* Nếu đang kêu mà tắt → gửi stop ring */
        if (!s_alarm_enabled && s_alarm_ringing) {
            s_alarm_ringing = false;
            vAlarmSendStopRing();
        }

        esp_rmaker_param_update_and_report(pxParam, xVal);
        return ESP_OK;
    }

    /* HOUR */
    if (strcmp(pcName, "AlarmHour") == 0) {

        int h = xVal.val.i;
        if (h < 0)  h = 0;
        if (h > 23) h = 23;
        s_alarm_hour = h;
        s_force_refresh = true;

        ESP_LOGI(TAG, "AlarmHour -> %02d", h);
        esp_rmaker_param_update_and_report(pxParam, esp_rmaker_int(h));
        return ESP_OK;
    }

    /* MINUTE */
    if (strcmp(pcName, "AlarmMinute") == 0) {

        int m = xVal.val.i;
        if (m < 0)  m = 0;
        if (m > 59) m = 59;
        s_alarm_min = m;
        s_force_refresh = true;

        ESP_LOGI(TAG, "AlarmMinute -> %02d", m);
        esp_rmaker_param_update_and_report(pxParam, esp_rmaker_int(m));
        return ESP_OK;
    }

    /* BRIGHTNESS (0–15) */
    if (strcmp(pcName, "Brightness") == 0) {
        int level = xVal.val.i;
        if (level < 0)  level = 0;
        if (level > 15) level = 15;

        ESP_LOGI(TAG, "Brightness -> %d", level);

        if (level == 0) {
            max7219_set_brightness(&g_dev, 0);
            max7219_clear(&g_dev);
        } else {
            max7219_set_brightness(&g_dev, (uint8_t)level);
            s_force_refresh = true;
        }

        esp_rmaker_param_update_and_report(pxParam, esp_rmaker_int(level));
        return ESP_OK;
    }

    ESP_LOGW(TAG, "Unknown param: %s", pcName);
    return ESP_OK;
}

/* ======================================================================
 *  INIT RainMaker (CHUẨN FREERTOS)
 * ====================================================================== */
void vRmakerAlarmInit(void)
{
    ESP_LOGI(TAG, "Initializing RainMaker Alarm...");

    app_storage_init();
    app_wifi_init();
    vWifiInit();

    /* Node cấu hình */
    esp_rmaker_config_t xCfg = {
        .enable_time_sync = false,   // dự án đã có time_svc riêng
    };

    pxRmakerNode = esp_rmaker_node_init(&xCfg,
                                        "Smart Clock Alarm",
                                        "esp.device.alarm");
    if (!pxRmakerNode) {
        ESP_LOGE(TAG, "Failed to init RainMaker node");
        return;
    }

    /* Tạo device Alarm dạng switch */
    pxAlarmDevice = esp_rmaker_switch_device_create(
                        "Alarm", NULL, s_alarm_enabled);
    esp_rmaker_device_add_cb(pxAlarmDevice, prvAlarmWriteCallback, NULL);

    /* PARAM — AlarmHour */
    pxParamHour = esp_rmaker_param_create(
                      "AlarmHour", NULL,
                      esp_rmaker_int(s_alarm_hour),
                      PROP_FLAG_READ | PROP_FLAG_WRITE);

    esp_rmaker_param_add_bounds(pxParamHour,
                                esp_rmaker_int(0),
                                esp_rmaker_int(23),
                                esp_rmaker_int(1));
    esp_rmaker_device_add_param(pxAlarmDevice, pxParamHour);

    /* PARAM — AlarmMinute */
    pxParamMinute = esp_rmaker_param_create(
                        "AlarmMinute", NULL,
                        esp_rmaker_int(s_alarm_min),
                        PROP_FLAG_READ | PROP_FLAG_WRITE);

    esp_rmaker_param_add_bounds(pxParamMinute,
                                esp_rmaker_int(0),
                                esp_rmaker_int(59),
                                esp_rmaker_int(1));
    esp_rmaker_device_add_param(pxAlarmDevice, pxParamMinute);

    /* PARAM — Brightness */
    pxParamBrightness = esp_rmaker_param_create(
                            "Brightness", NULL,
                            esp_rmaker_int(8),
                            PROP_FLAG_READ | PROP_FLAG_WRITE);

    esp_rmaker_param_add_bounds(pxParamBrightness,
                                esp_rmaker_int(0),
                                esp_rmaker_int(15),
                                esp_rmaker_int(1));

    esp_rmaker_param_add_ui_type(pxParamBrightness, "esp.ui.slider");
    esp_rmaker_device_add_param(pxAlarmDevice, pxParamBrightness);

    /* Add device vào node */
    esp_rmaker_node_add_device(pxRmakerNode, pxAlarmDevice);

    /* Start RainMaker */
    ESP_ERROR_CHECK(esp_rmaker_start());

    /* Wi-Fi provisioning + in QR (POP random) */
    ESP_ERROR_CHECK(app_wifi_start(POP_TYPE_RANDOM));

    ESP_LOGI(TAG, "RainMaker Alarm started.");
}
