#ifndef MQTT_H_
#define MQTT_H_

#include <freertos/event_groups.h>

void mqttConnectionInit(void);
xQueueHandle getMQTTConnectionMessageQueue(void);
EventGroupHandle_t getMQTTConnectionEventGroup(void);
void mqttConnectionResetNVS(void);
xQueueHandle getMqttConnectionQueue(void);

#define MQTT_CONNECTED_BIT	BIT0

#endif