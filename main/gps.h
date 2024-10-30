#ifndef GPS_H
#define GPS_H

#include <stdint.h>

typedef struct {
    float x;
    float y;
    float z;
} Coordinates;

// Koordináták számítása XYZ rendszerbe
Coordinates gps_to_xyz(float latitude, float longitude, float altitude);

// GPS adat olvasása és MQTT publikálása
void gps_read_and_publish(void);

// NMEA $GPGGA üzenet feldolgozása
int parse_gpgga(char *data, float *latitude, float *longitude, float *altitude);

#endif // GPS_H
