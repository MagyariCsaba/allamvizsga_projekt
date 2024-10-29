#include "gps.h"
#include "mqtt.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "esp_log.h"

// Föld sugara (méterben)
#define EARTH_RADIUS 6371000.0

// Kezdőpont koordinátái, amelyhez viszonyítunk (pl. egy referencia GPS pont)
#define REF_LAT 47.4979  // Példa Budapest középpontjára
#define REF_LON 19.0402

// Koordináta-struktúra definiálása
typedef struct {
    float x;
    float y;
    float z;
} Coordinates;

static const char *TAG = "GPS";

// GPS adat átalakítása X, Y, Z koordinátákra
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
    if (Serial2.available()) {
        char gps_data[100];
        int len = Serial2.readBytesUntil('\n', gps_data, sizeof(gps_data) - 1);
        gps_data[len] = '\0';

        // Parselés ($GPGGA formátum keresése)
        if (strstr(gps_data, "$GPGGA") != NULL) {
            // Példa értékek (valós GPS parsolás szükséges)
            float latitude = 47.4979;   // helyettesíts valós adatokkal
            float longitude = 19.0402;  // helyettesíts valós adatokkal
            float altitude = 130.0;     // helyettesíts valós adatokkal

            // Átalakítás X, Y, Z-re
            Coordinates coords = gps_to_xyz(latitude, longitude, altitude);

            // X, Y, Z küldése MQTT-n keresztül
            char payload[100];
            snprintf(payload, sizeof(payload), "X: %.2f, Y: %.2f, Z: %.2f", coords.x, coords.y, coords.z);
            mqtt_publish("gps/xyz", payload);

            ESP_LOGI(TAG, "GPS X: %.2f, Y: %.2f, Z: %.2f", coords.x, coords.y, coords.z);
        }
    }
}
