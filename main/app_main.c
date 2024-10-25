#include "nvs_flash.h"
#include "mqtt.h"
#include "szenzor.h"
#include "esp_log.h"

void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI("Main", "ESP_WIFI_MODE_STA");
    wifi_init_sta();  // Initialize WiFi connection

    mqtt_app_start();  // Initialize and start MQTT
}
