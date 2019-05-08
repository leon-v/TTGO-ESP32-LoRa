#include <nvs_flash.h>
#include <string.h>
#include <esp_log.h>

#include "message.h"
#include "radio.h"
#include "mqtt_connection.h"
#include "elastic.h"
#include "display.h"

#define TAG "message"

void messageIn(message_t * messagePointer, const char * from){

	char valueString[sizeof(messagePointer->stringValue)] = {0};

	switch (messagePointer->valueType){

		case MESSAGE_INT:
			sprintf(valueString, "%d", messagePointer->intValue);
		break;

		case MESSAGE_FLOAT:
			sprintf(valueString, "%.4f", messagePointer->floatValue);
		break;

		case MESSAGE_DOUBLE:
			sprintf(valueString, "%.8f", messagePointer->doubleValue);
		break;

		case MESSAGE_STRING:
			sprintf(valueString, "%s", messagePointer->stringValue);
		break;
	}


	nvs_handle nvsHandle;
	ESP_ERROR_CHECK(nvs_open("BeelineNVS", NVS_READONLY, &nvsHandle));

	char handle[32];
	char log[64] = "\0";
	strcpy(handle, from);
	strcat(handle, "InRt");

	unsigned char routing;
	nvs_get_u8(nvsHandle, handle, &routing);

	nvs_close(nvsHandle);

	if ((routing >> LORA) & 0x01){
		strcat(log, " lora");
		radioLoRaQueueAdd(messagePointer);
	}

	if ((routing >> MQTT) & 0x01){
		strcat(log, " mqtt");
		mqttConnectionQueueAdd(messagePointer);
	}

	if ((routing >> ELASTICSEARCH) & 0x01){
		strcat(log, " elastic");
		elasticQueueAdd(messagePointer);
	}

	if ((routing >> DISPLAY) & 0x01){
		strcat(log, " display");
		displayQueueAdd(messagePointer);
	}

	ESP_LOGI(TAG, "%s: %s/%s/%d = %s --> %s",
		from,
		messagePointer->deviceName,
		messagePointer->sensorName,
		messagePointer->valueType,
		valueString,
		log
	);
}

void messageNVSReset(char * from, unsigned char defaults){

	nvs_handle nvsHandle;
	ESP_ERROR_CHECK(nvs_open("BeelineNVS", NVS_READWRITE, &nvsHandle));

	char handle[32];
	strcpy(handle, from);
	strcat(handle, "InRt");

	ESP_ERROR_CHECK(nvs_set_u8(nvsHandle, handle, defaults));

	ESP_ERROR_CHECK(nvs_commit(nvsHandle));

	nvs_close(nvsHandle);
}