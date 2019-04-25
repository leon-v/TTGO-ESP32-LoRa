#ifndef WIFI_H_
#define WIFI_H_

#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_event_loop.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

void wifiInit(void);
EventGroupHandle_t wifiGetEventGroup(void);
void wifiUsed(void);

#define WIFI_CONNECTED_BIT	BIT0

#endif /* WIFI_H_ */