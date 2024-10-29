// gps.c
#include <stdio.h>
#include "driver/uart.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "gps.h"
#include "mqtt.h" // Az MQTT kliens miatt

#define RXD2 (16)
#define TXD2 (17)

static const char *TAG = "GPS";

// A GPS UART inicializálása
void gps_init(void) {
    const uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_2, &uart_config);
    uart_set_pin(UART_NUM_2, TXD2, RXD2, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM_2, 1024 * 2, 0, 0, NULL, 0);
}

// Funkció, amely a GPS adatokat beolvassa és publikálja az MQTT-n
void gps_read_and_publish(void) {
    uint8_t data[128];
    int len = uart_read_bytes(UART_NUM_2, data, sizeof(data) - 1, 20 / portTICK_RATE_MS);
    if (len > 0) {
        data[len] = '\0'; // Null-terminálás
        ESP_LOGI(TAG, "GPS Data: %s", data);

        // Ellenőrzi, hogy $GPGGA üzenetet kapott-e, és publikálja, ha igen
        if (strstr((char*)data, "$GPGGA")) {
            esp_mqtt_client_publish(mqtt_client, "gps/data", (const char*)data, 0, 1, 0);
        }
    }
}
