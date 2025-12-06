#include <string.h>
#include "app_state.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "wifi.h"

static const char *TAGW = "wifi";

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        if (s_wifi_event_group) {
            xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
            ESP_LOGI(TAGW, "Wi-Fi got IP, WIFI_CONNECTED_BIT set.");
        }
    }
}

void vWifiInit(void)
{
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
                        IP_EVENT,
                        IP_EVENT_STA_GOT_IP,
                        &event_handler,
                        NULL,
                        NULL));
}
