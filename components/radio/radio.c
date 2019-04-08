#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <nvs.h>
#include <string.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <crypto/crypto.h>

#include "message.h"
#include "lora.h"
#include "radio.h"

#include "elastic.h"
#include "mqtt_connection.h"

#define TAG "Radio"

static xQueueHandle radioQueue = NULL;

char loraKey[CONFIG_HTTP_NVS_MAX_STRING_LENGTH];
void * aesEncryptContext;
void * aesDecryptionContext;

xQueueHandle radioGetQueue(void){
	return radioQueue;
}


void radioCreateMessage(message_t * message, unsigned char * radioBuffer, int bufferLength) {

	unsigned char * bytes = radioBuffer;
	int length;

	length = strlen((char *) bytes);
	memcpy(message->deviceName, bytes, length);
	message->deviceName[length] = '\0';
	bytes+= length + 1;

	length = strlen((char *) bytes);
	memcpy(message->sensorName, bytes, length);
	message->sensorName[length] = '\0';
	bytes+= length + 1;

	length = sizeof(message->valueType);
	memcpy(&message->valueType, bytes, length);
	bytes+= length;

	switch (message->valueType){

		case MESSAGE_INT:
			length = sizeof(message->intValue);
			memcpy(&message->intValue, bytes, length);
			bytes+= length;
		break;

		case MESSAGE_FLOAT:
			length = sizeof(message->intValue);
			memcpy(&message->floatValue, bytes, length);
			bytes+= length;
		break;

		case MESSAGE_DOUBLE:
			length = sizeof(message->doubleValue);
			memcpy(&message->doubleValue, bytes, length);
			bytes+= length;
		break;

		case MESSAGE_STRING:
			length = strlen((char *) bytes);
			memcpy(message->stringValue, bytes, length);
			bytes+= length + 1;
		break;
	}

}
int radioCreateBuffer(unsigned char * radioBuffer, message_t * message) {

	int length = 0;
	unsigned char * bytes = radioBuffer;

	length = strlen(message->deviceName) + 1;
	memcpy(bytes, message->deviceName, length);
	bytes+= length;

	length = strlen(message->sensorName) + 1;
	memcpy(bytes, message->sensorName, length);
	bytes+= length;

	length = sizeof(message->valueType);
	memcpy(bytes, &message->valueType, length);
	bytes+= length;

	switch (message->valueType){

		case MESSAGE_INT:
			length = sizeof(message->intValue);
			memcpy(bytes, &message->intValue, length);
			bytes+= length;
		break;
		case MESSAGE_FLOAT:
			length = sizeof(message->floatValue);
			memcpy(bytes, &message->floatValue, length);
			bytes+= length;
		break;
		case MESSAGE_DOUBLE:
			length = sizeof(message->doubleValue);
			memcpy(bytes, &message->doubleValue, length);
			bytes+= length;
		break;
		case MESSAGE_STRING:
			length = strlen(message->stringValue) + 1;
			memcpy(bytes, message->stringValue, length);
			bytes+= length;
		break;
	}

	return bytes - radioBuffer;
}

void radioLoRaSendRadioMessage(message_t * message){
	int length;

	unsigned char radioBuffer[sizeof(message_t)];

	length = radioCreateBuffer(radioBuffer, message);

	printf("TX: ");
	for (int i = 0; i < length; i++) {
		printf("%c", (char) radioBuffer[i]);
	}
	printf("\n");

	lora_send_packet(radioBuffer, length);
}

void radioLoraRx(void) {
	int length;
	unsigned char radioBuffer[sizeof(message_t)];
	message_t message;

	lora_receive();    // put into receive mode
	while(lora_received()) {

		length = lora_receive_packet(radioBuffer, sizeof(radioBuffer));

		printf("RX: ");
		for (int i = 0; i < length; i++) {
			printf("%c", (char) radioBuffer[i]);
		}
		printf("\n");

		radioCreateMessage(&message, radioBuffer, length);

		ESP_LOGE(TAG, "LoRa heard |%s|%s|%d|%d", message.deviceName, message.sensorName, message.valueType, length);

		switch(message.valueType){
			case MESSAGE_INT:
				ESP_LOGE(TAG, "Value: %d", message.intValue);
			break;
			case MESSAGE_FLOAT:
				ESP_LOGE(TAG, "Value: %f", message.floatValue);
			break;
			case MESSAGE_DOUBLE:
				ESP_LOGE(TAG, "Value: %f", message.doubleValue);
			break;
			case MESSAGE_STRING:
				ESP_LOGE(TAG, "Value: %s", message.stringValue);
			break;
		}

		/* Send message to elastic */
		if (uxQueueSpacesAvailable(elasticGetQueue())) {
			xQueueSend(elasticGetQueue(), &message, 0);
		}

		/* Send message to mqtt */
		if (uxQueueSpacesAvailable(getMqttConnectionQueue())) {
			xQueueSend(getMqttConnectionQueue(), &message, 0);
		}


		lora_receive();
	}
}

void radioTask(void * arg){

	message_t message;

	while (1){

		radioLoraRx();

		if (xQueueReceive(radioQueue, &message, 2)) {
			radioLoRaSendRadioMessage(&message);
		}

	}

	vTaskDelete(NULL);
}

void radioInit(void){

	nvs_handle nvsHandle;
	ESP_ERROR_CHECK(nvs_open("BeelineNVS", NVS_READONLY, &nvsHandle));

	size_t nvsLength;
    nvsLength = sizeof(loraKey);
	nvs_get_str(nvsHandle, "loraKey", loraKey, &nvsLength);

	unsigned int loraFrequency;
	nvs_get_u32(nvsHandle, "loraFrequency", &loraFrequency);

	aesEncryptContext = aes_encrypt_init( (unsigned char *) loraKey, strlen(loraKey));
	aesDecryptionContext = aes_decrypt_init( (unsigned char *) loraKey, strlen(loraKey));

	lora_init();
	lora_set_frequency(loraFrequency);
	lora_enable_crc();

	radioQueue = xQueueCreate(4, sizeof(message_t));

	xTaskCreate(&radioTask, "radioTask", 4096, NULL, 10, NULL);
}


void radioResetNVS(void){

	nvs_handle nvsHandle;
	ESP_ERROR_CHECK(nvs_open("BeelineNVS", NVS_READWRITE, &nvsHandle));

	ESP_ERROR_CHECK(nvs_set_str(nvsHandle, "loraKey", "Encryption Key"));
	ESP_ERROR_CHECK(nvs_commit(nvsHandle));

	ESP_ERROR_CHECK(nvs_set_u32(nvsHandle, "loraFrequency", 915000000));
	ESP_ERROR_CHECK(nvs_commit(nvsHandle));

	nvs_close(nvsHandle);
}