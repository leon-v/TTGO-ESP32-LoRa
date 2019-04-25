#include <nvs_flash.h>
#include <string.h>
#include <esp_log.h>

#include "message.h"
#include "radio.h"
#include "mqtt_connection.h"
#include "elastic.h"
#include "display.h"


#include "ssd1306.h"

#define TAG "message"

unsigned char disaplyLine = 0;

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

	ESP_LOGI(TAG, "Message From %s: deviceName=%s, sensorName=%s, valueType=%d, value=%s",
		from,
		messagePointer->deviceName,
		messagePointer->sensorName,
		messagePointer->valueType,
		valueString
	);

	disaplyLine++;
	if (disaplyLine > 8){
		disaplyLine = 0;
	}

	ssd1306Text_t ssd1306Text;
	ssd1306Text.line = disaplyLine;

	strcpy(ssd1306Text.text, messagePointer->deviceName);
	strcat(ssd1306Text.text, " / ");
	strcat(ssd1306Text.text, messagePointer->sensorName);
	strcat(ssd1306Text.text, " = ");
	strcat(ssd1306Text.text, valueString);

	ssd1306QueueText(&ssd1306Text);


	nvs_handle nvsHandle;
	ESP_ERROR_CHECK(nvs_open("BeelineNVS", NVS_READONLY, &nvsHandle));

	char handle[32];
	strcpy(handle, from);
	strcat(handle, "InRt");

	unsigned char routing;
	nvs_get_u8(nvsHandle, handle, &routing);

	nvs_close(nvsHandle);

	if ((routing >> LORA) & 0x01){
		ESP_LOGI(TAG, "Forwarding to LoRa");
		radioLoRaQueueAdd(messagePointer);
	}

	if ((routing >> MQTT) & 0x01){
		ESP_LOGI(TAG, "Forwarding to MQTT");
		mqttConnectionQueueAdd(messagePointer);
	}

	if ((routing >> ELASTICSEARCH) & 0x01){
		ESP_LOGI(TAG, "Forwarding to Elasticsearch");
		elasticQueueAdd(messagePointer);
	}

	if ((routing >> DISPLAY) & 0x01){
		ESP_LOGI(TAG, "Forwarding to Display");
		displayQueueAdd(messagePointer);
	}

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