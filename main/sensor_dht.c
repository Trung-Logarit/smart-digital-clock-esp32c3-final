#include "app_state.h"
#include "esp_log.h"
#include "dht.h"
#include "esp_diagnostics_metrics.h"   // thêm dòng này

static const char *TAGS = "dht";

void vDhtTask(void *pvParameters)
{
    (void)pvParameters;
    while (1) {
        float temperature, humidity;
        if (dht_read_float_data(SENSOR_TYPE, DHT_GPIO_PIN,
                                &humidity, &temperature) == ESP_OK) {
            s_temperature = temperature;
            s_humidity = humidity;
            ESP_LOGI(TAGS, "Humidity: %.1f%% Temp: %.1fC",
                     s_humidity, s_temperature);

            /* Gửi dữ liệu thật lên Insights (đơn vị C, %) */
            esp_diag_metrics_add_float("dht_temperature", s_temperature);
            esp_diag_metrics_add_float("dht_humidity",    s_humidity);

        } else {
            ESP_LOGW(TAGS, "Could not read data from sensor");
        }
        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}
