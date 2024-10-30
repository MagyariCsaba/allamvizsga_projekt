#include "gps.h"
#include "mqtt.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#define UART_NUM UART_NUM_2  // GPS UART porthoz
#define BUF_SIZE 1024
#define EARTH_RADIUS 6371000.0

#define REF_LAT 47.4979
#define REF_LON 19.0402

static const char *TAG = "GPS";
extern EventGroupHandle_t wifi_event_group;
extern const int WIFI_CONNECTED_BIT;
extern esp_mqtt_client_handle_t mqtt_client;

// GPS koordináták átalakítása XYZ koordinátákra
Coordinates gps_to_xyz(float latitude, float longitude, float altitude) {
    float lat_rad = latitude * M_PI / 180.0;
    float lon_rad = longitude * M_PI / 180.0;
    float ref_lat_rad = REF_LAT * M_PI / 180.0;
    float ref_lon_rad = REF_LON * M_PI / 180.0;

    float delta_lat = lat_rad - ref_lat_rad;
    float delta_lon = lon_rad - ref_lon_rad;

    Coordinates coords;
    coords.x = EARTH_RADIUS * delta_lon * cos(ref_lat_rad);
    coords.y = EARTH_RADIUS * delta_lat;
    coords.z = altitude;

    return coords;
}

// NMEA $GPGGA üzenet feldolgozása a GPS adatok kinyerésére
int parse_gpgga(char *data, float *latitude, float *longitude, float *altitude) {
    char *token;
    int field_count = 0;
    
    token = strtok(data, ",");
    while (token != NULL) {
        field_count++;
        
        // Latitude
        if (field_count == 3) {
            *latitude = atof(token) / 100.0;
        }
        // Latitude N/S
        else if (field_count == 4 && *token == 'S') {
            *latitude = -*latitude;
        }
        // Longitude
        else if (field_count == 5) {
            *longitude = atof(token) / 100.0;
        }
        // Longitude E/W
        else if (field_count == 6 && *token == 'W') {
            *longitude = -*longitude;
        }
        // Altitude
        else if (field_count == 10) {
            *altitude = atof(token);
            break;
        }
        
        token = strtok(NULL, ",");
    }
    
    return (field_count >= 10) ? 0 : -1;
}

// GPS adatok beolvasása és MQTT publikálása
void gps_read_and_publish() {
    uint8_t data[BUF_SIZE];
    int len = uart_read_bytes(UART_NUM, data, BUF_SIZE - 1, 20 / portTICK_RATE_MS);
    
    if (len > 0) {
        data[len] = '\0';
        
        // Keresés a $GPGGA formátumra
        if (strstr((char*)data, "$GPGGA") != NULL) {
            float latitude, longitude, altitude;
            if (parse_gpgga((char*)data, &latitude, &longitude, &altitude) == 0) {
                Coordinates coords = gps_to_xyz(latitude, longitude, altitude);

                char payload[100];
                snprintf(payload, sizeof(payload), "X: %.2f, Y: %.2f, Z: %.2f", coords.x, coords.y, coords.z);
                
                // Csak akkor publikálunk, ha van MQTT kapcsolat
                EventBits_t bits = xEventGroupGetBits(wifi_event_group);
                if ((bits & WIFI_CONNECTED_BIT) && mqtt_client != NULL) {
                    int msg_id = esp_mqtt_client_publish(mqtt_client, "gps/xyz", payload, 0, 1, 0);
                    ESP_LOGI(TAG, "Published GPS data, msg_id=%d", msg_id);
                } else {
                    ESP_LOGW(TAG, "MQTT client not connected, message not sent.");
                }

                ESP_LOGI(TAG, "GPS X: %.2f, Y: %.2f, Z: %.2f", coords.x, coords.y, coords.z);
            } else {
                ESP_LOGW(TAG, "Failed to parse GPGGA sentence.");
            }
        }
    } else {
        ESP_LOGW(TAG, "No data read from UART.");
    }
}
