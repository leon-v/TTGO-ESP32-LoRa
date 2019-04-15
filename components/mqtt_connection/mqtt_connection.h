#ifndef MQTT_H_
#define MQTT_H_

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/event_groups.h>

#include "message.h"

void mqttConnectionInit(void);
xQueueHandle getMQTTConnectionMessageQueue(void);
EventGroupHandle_t getMQTTConnectionEventGroup(void);
void mqttConnectionResetNVS(void);
void mqttConnectionQueueAdd(message_t * message);

#define MQTT_CONNECTED_BIT	BIT0

#endif