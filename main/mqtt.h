#ifndef MQTT_H
#define MQTT_H

#include "mqtt_client.h"
#include "freertos/event_groups.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

extern esp_mqtt_client_handle_t mqtt_client;
extern EventGroupHandle_t s_wifi_event_group;

void wifi_init_sta(void);
void mqtt_app_start(void);
void send_100_messages(esp_mqtt_client_handle_t client);

#endif // MQTT_H
