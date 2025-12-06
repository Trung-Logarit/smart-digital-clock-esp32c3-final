/*
   Public Domain / CC0
*/
#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Types of Proof of Possession */
typedef enum {
    /** Use MAC address to generate PoP */
    POP_TYPE_MAC = 0,
    /** Use random bytes stored in factory partition (RainMaker claiming) as PoP */
    POP_TYPE_RANDOM = 1,
} app_wifi_pop_type_t;

/** Init netif + event loop + Wi-Fi driver and register handlers */
void app_wifi_init(void);

/** Start provisioning (BLE/SoftAP theo Kconfig), chờ Wi-Fi có IP rồi trả ESP_OK */
esp_err_t app_wifi_start(app_wifi_pop_type_t pop_type);

#ifdef __cplusplus
}
#endif
