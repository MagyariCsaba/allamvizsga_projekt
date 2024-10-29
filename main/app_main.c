// app_main.c
#include "nvs_flash.h"
#include "mqtt.h"
#include "szenzor.h"
#include "gps.h"
#include "esp_log.h"

void app_main(void)
{
    // NVS inicializálása
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI("Main", "ESP_WIFI_MODE_STA");
    wifi_init_sta();  // WiFi kapcsolat inicializálása

    mqtt_app_start();  // MQTT inicializálása és indítása
    
    gps_init();  // GPS inicializálása

    while (1) {
        gps_read_and_publish();  // GPS adat beolvasása és publikálása
        vTaskDelay(pdMS_TO_TICKS(1000));  // 1 másodperces késleltetés
    }
}
