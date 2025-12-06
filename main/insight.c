// main/insight.c

#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_insights.h"
#include "esp_diagnostics_metrics.h"

#include "insight.h"

static const char *TAG = "insight";

/* ------------------------------------------------------------------
 *  TOKEN INSIGHTS
 *  >>> Bạn copy lại INSIGHT_AUTH_KEY từ file insight_wifi.c cũ vào đây <<<
 * ------------------------------------------------------------------ */
#define INSIGHT_AUTH_KEY  "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1c2VyIjoiR2l0SHViX05STHpyV2tOazd2dWVOZm50M0syUFkiLCJpc3MiOiJlMzIyYjU5Yy02M2NjLTRlNDAtOGVhMi00ZTc3NjY1NDVjY2EiLCJzdWIiOiIzMmZlYmUxYy0xZDZhLTQ2NzEtYjkxMi1lOGIzNDUyNWI3NzQiLCJleHAiOjIwNzk2ODczNjMsImlhdCI6MTc2NDMyNzM2M30.I8HYkvyQ1-XYouWwypT3DsFb-iZfgcYBodi8-52gkTDYT-X7QO9L6wqW9KKqZdNkoy3SV1ekol2bRXcbY3K9_eFaR_cNL2TIjYKWAFtZ7MCvpQhEgjOnS2WNW6eyZLgWoqQDG22nUFF99IbK0xzEvI4vDntKVi4z5FfiyGZzHf17VE0kECbjlZRUNlA0kOrLVhPGZZ_Q4HvZCQz94lLtv7p3rrhZHNFjN7Iteh9NRuQd5QicjlIlUxaJGCysjIbFBourU4Y2IVv6tVGqBqV0h86P9wj3Qb8wiEsZM99KLsgKnid32AA_jyRa8CGgQEIY12N1vSHYWyHs6jTdRIeybA"

/* Đếm số lần mất WiFi */
static uint32_t ulWifiDisconnectCount = 0;

/* =======================  WiFi EVENT HANDLER  ===================== */

static void prvWifiInsightEventHandler(void *arg,
                                       esp_event_base_t base,
                                       int32_t id,
                                       void *data)
{
    (void)arg;
    (void)data;

    /* MẤT WIFI */
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        ulWifiDisconnectCount++;
        esp_diag_metrics_add_int("wifi_disconnects",
                                 (int32_t)ulWifiDisconnectCount);

        ESP_LOGW(TAG, "WiFi disconnected (%" PRIu32 ")",
                 ulWifiDisconnectCount);
    }

    /* KHI NHẬN IP */
    if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {

        /* Connected = true */
        esp_diag_metrics_add_bool("wifi_connected", true);

        /* RSSI hiện tại */
        wifi_ap_record_t info;
        if (esp_wifi_sta_get_ap_info(&info) == ESP_OK) {
            esp_diag_metrics_add_int("wifi_rssi", (int32_t)info.rssi);
            ESP_LOGI(TAG, "WiFi RSSI: %d dBm", info.rssi);
        }
    }
}

/* =======================  INIT ESP INSIGHTS  ====================== */

void vInsightInit(void)
{
    /* 1) Init ESP Insights */
    esp_insights_config_t cfg = {
        .log_type = ESP_DIAG_LOG_TYPE_ERROR |
                    ESP_DIAG_LOG_TYPE_WARNING |
                    ESP_DIAG_LOG_TYPE_EVENT,
        .auth_key = INSIGHT_AUTH_KEY,
    };

    ESP_ERROR_CHECK(esp_insights_init(&cfg));
    ESP_LOGI(TAG, "ESP Insights init OK");

    /* DHT metrics – sẽ được update từ vDhtTask() */
    esp_diag_metrics_register("dht", "dht_temperature",
                              "DHT Temperature (C)", "sensor.dht",
                              ESP_DIAG_DATA_TYPE_FLOAT);

    esp_diag_metrics_register("dht", "dht_humidity",
                              "DHT Humidity (%)", "sensor.dht",
                              ESP_DIAG_DATA_TYPE_FLOAT);

    /* 3) Register WiFi event handlers */
    ESP_ERROR_CHECK(esp_event_handler_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        prvWifiInsightEventHandler,
        NULL));

    ESP_ERROR_CHECK(esp_event_handler_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        prvWifiInsightEventHandler,
        NULL));

    ESP_LOGI(TAG, "WiFi & DHT Insights metrics registered.");
}
