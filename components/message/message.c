#include <nvs_flash.h>
#include <string.h>

#include "message.h"
#include "radio.h"
#include "mqtt_connection.h"
#include "elastic.h"

void messageIn(message_t * messagePointer, char * from){

	nvs_handle nvsHandle;
	ESP_ERROR_CHECK(nvs_open("BeelineNVS", NVS_READONLY, &nvsHandle));

	char handle[32];
	strcpy(handle, from);
	strcat(handle, "InRouting");

	unsigned char routing;
	nvs_get_u8(nvsHandle, handle, &routing);

	nvs_close(nvsHandle);

	if ((routing << LORA) && 0x01){
		radioLoRaQueueAdd(messagePointer);
	}

	if ((routing << MQTT) && 0x01){
		mqttConnectionQueueAdd(messagePointer);
	}

	if ((routing << ELASTICSEARCH) && 0x01){
		elasticQueueAdd(messagePointer);
	}

	if ((routing << INTERNAL_SENSORS) && 0x01){
		// Internal sensors has no outgoing path
	}

}

void messageNVSReset(char * from){

	nvs_handle nvsHandle;
	ESP_ERROR_CHECK(nvs_open("BeelineNVS", NVS_READWRITE, &nvsHandle));

	char handle[32];
	strcpy(handle, from);
	strcat(handle, "InRouting");

	ESP_ERROR_CHECK(nvs_set_u8(nvsHandle, handle, 0x00));

	ESP_ERROR_CHECK(nvs_commit(nvsHandle));

	nvs_close(nvsHandle);
}