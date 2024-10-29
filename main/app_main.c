#include "nvs_flash.h"
#include "mqtt.h"
#include "gps.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define UART_NUM UART_NUM_2
#define TX_PIN 17  // UART TX pin
#define RX_PIN 16  // UART RX pin

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
    wifi_init_sta();  // WiFi inicializálás

    mqtt_app_start();  // MQTT inicializálás

    // UART konfigurálása GPS-hez
    const uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM, &uart_config);
    uart_set_pin(UART_NUM, TX_PIN, RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM, 1024 * 2, 0, 0, NULL, 0);

    // GPS adat olvasása és publikálása
    while (1) {
        gps_read_and_publish();
        vTaskDelay(pdMS_TO_TICKS(1000));  // 1 másodperc várakozás
    }
}
