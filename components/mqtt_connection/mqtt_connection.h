#ifndef MQTT_H_
#define MQTT_H_

void mqttConnectionInit(void);
xQueueHandle getMQTTConnectionMessageQueue(void);
EventGroupHandle_t getMQTTConnectionEventGroup(void);
void mqttConnectionResetNVS(void);

#define MQTT_CONNECTED_BIT	BIT0

typedef struct{
	char topic[32];
	char value[32];
} mqttConnectionMessage_t;

#endif