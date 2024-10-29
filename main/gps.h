#ifndef GPS_H
#define GPS_H

typedef struct {
    float x;
    float y;
    float z;
} Coordinates;

Coordinates gps_to_xyz(float latitude, float longitude, float altitude);
void gps_read_and_publish();

#endif // GPS_H
