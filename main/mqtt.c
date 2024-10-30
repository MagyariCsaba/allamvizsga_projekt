#include <string.h>
#include "mqtt.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "mqtt_client.h"

#define TAG "wifi station"
#define MQTT_TAG "MQTT Client"
#define EXAMPLE_ESP_WIFI_SSID "Alma"         // Wi-Fi SSID
#define EXAMPLE_ESP_WIFI_PASS "12345678"     // Wi-Fi password

EventGroupHandle_t s_wifi_event_group;
esp_mqtt_client_handle_t mqtt_client = NULL;
static int s_retry_num = 0;

// Event handler for WiFi and IP events
static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < 20) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// Sends 100 messages with a 5-second interval
void send_100_messages(esp_mqtt_client_handle_t client) {
    for (int i = 0; i < 100; i++) {
        char message[50];
        snprintf(message, sizeof(message), "Message number %d from ESP32", i + 1);
        int msg_id = esp_mqtt_client_publish(client, "eesTopic", message, 0, 1, 0);
        ESP_LOGI(MQTT_TAG, "Sent message %d, msg_id=%d", i + 1, msg_id);
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

// MQTT event handler
static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(MQTT_TAG, "MQTT_EVENT_CONNECTED");
            send_100_messages(client);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(MQTT_TAG, "MQTT_EVENT_DISCONNECTED");
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(MQTT_TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(MQTT_TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(MQTT_TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}

static void mqtt_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    mqtt_event_handler_cb(event_data);
}

// Initialize WiFi in STA mode
void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}

// Initialize and start MQTT client
void mqtt_app_start(void)
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://192.168.227.241:1883"
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    ESP_ERROR_CHECK(esp_mqtt_client_start(mqtt_client));
}

void gps_read_and_publish()
{
    uint8_t* data = (uint8_t*) malloc(BUF_SIZE);
    int len = uart_read_bytes(UART_NUM, data, BUF_SIZE - 1, 20 / portTICK_RATE_MS);
    
    if (len > 0) {
        data[len] = '\0';

        if (strstr((char*)data, "$GPGGA") != NULL) {
            float latitude = 47.4979;
            float longitude = 19.0402;
            float altitude = 130.0;

            Coordinates coords = gps_to_xyz(latitude, longitude, altitude);

            char payload[100];
            snprintf(payload, sizeof(payload), "X: %.2f, Y: %.2f, Z: %.2f", coords.x, coords.y, coords.z);

            int msg_id = esp_mqtt_client_publish(mqtt_client, "gps/xyz", payload, 0, 1, 0);
            if (msg_id >= 0) {
                ESP_LOGI(TAG, "MQTT Publish successful, msg_id=%d", msg_id);
            } else {
                ESP_LOGE(TAG, "MQTT Publish failed");
            }
            ESP_LOGI(TAG, "GPS X: %.2f, Y: %.2f, Z: %.2f", coords.x, coords.y, coords.z);
        }
    }

    free(data);
}
