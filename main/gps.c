#include "gps.h"
#include "mqtt.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define UART_NUM UART_NUM_2  // A GPS UART porthoz beállítva
#define BUF_SIZE 1024
#define EARTH_RADIUS 6371000.0

#define REF_LAT 47.4979
#define REF_LON 19.0402

static const char *TAG = "GPS";

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

void gps_read_and_publish() {
    uint8_t* data = (uint8_t*) malloc(BUF_SIZE);
    int len = uart_read_bytes(UART_NUM, data, BUF_SIZE - 1, 20 / portTICK_RATE_MS);
    
    if (len > 0) {
        data[len] = '\0';
        
        // Parselés ($GPGGA formátum keresése)
        if (strstr((char*)data, "$GPGGA") != NULL) {
            float latitude = 47.4979;   // helyettesíts valós adatokkal
            float longitude = 19.0402;  // helyettesíts valós adatokkal
            float altitude = 130.0;     // helyettesíts valós adatokkal

            Coordinates coords = gps_to_xyz(latitude, longitude, altitude);

            char payload[100];
            snprintf(payload, sizeof(payload), "X: %.2f, Y: %.2f, Z: %.2f", coords.x, coords.y, coords.z);
            mqtt_publish("gps/xyz", payload);

            ESP_LOGI(TAG, "GPS X: %.2f, Y: %.2f, Z: %.2f", coords.x, coords.y, coords.z);
        }
    }

    free(data);
}
