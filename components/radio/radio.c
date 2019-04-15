#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <nvs.h>
#include <string.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <crypto/crypto.h>
#include <math.h>

#include "message.h"
#include "lora.h"
#include "radio.h"

#define TAG "Radio"
#define ENC_TEST_CHAR 0xAB

static xQueueHandle radioQueue = NULL;

char loraKey[CONFIG_HTTP_NVS_MAX_STRING_LENGTH] = {0};
static void * aesEncryptContext;
static void * aesDecryptContext;

static unsigned int loraTXSyncWord;
static unsigned int loraRXSyncWord;

void radioPrintBuffer(const char * tag, unsigned char * buffer, int length){

	printf("%s: %d: ", tag, length);
	for (int i = 0; i < length; i++) {
		printf("%c|", (char) buffer[i]);
	}
	printf("\n");
}

void radioShift(unsigned char * buffer, int amount, int * length){


	if (amount > 0){
		for (int i = * length + amount; i > 0; i--){
			buffer[i] = buffer[i - amount];
		}
	}
	else if (amount < 0) {

		for (int i = 0; i < * length; i++){
			buffer[i] = buffer[i + -amount];
		}
	}

	* length = * length + amount;
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

#define BLOCK_LENGTH 16

void radioEncrypt(unsigned char * buffer, int * length){

	if (!aesEncryptContext) {
		return;
	}

	radioShift(buffer, 1, length);
	buffer[0] = ENC_TEST_CHAR;

	int blocks = ceil( (float) * length / BLOCK_LENGTH);

	for (int i = 0; i < blocks; i++) {

		int offset = i * BLOCK_LENGTH;
		aes_encrypt(aesEncryptContext, buffer + offset, buffer + offset);
	}

	*length = (blocks * BLOCK_LENGTH);
}

int radioDecrypt(unsigned char * buffer, int * length){

	if (!aesDecryptContext) {
		return 0;
	}

	int blocks = ceil( (float) * length / BLOCK_LENGTH);

	for (int i = 0; i < blocks; i++) {

		int offset = i * BLOCK_LENGTH;
		aes_decrypt(aesDecryptContext, buffer + offset, buffer + offset);
	}

	if (buffer[0] != ENC_TEST_CHAR){
		return 0;
	}

	radioShift(buffer, -1, length);

	return 1;
}

void radioLoRaSendRadioMessage(message_t * message){
	int length;

	unsigned char buffer[sizeof(message_t)] = {0};
	memset(buffer, 0, sizeof(message_t));

	length = radioCreateBuffer(buffer, message);

	radioEncrypt(buffer, &length);

	lora_set_sync_word( (unsigned char) loraTXSyncWord);

	lora_send_packet(buffer, length);
}

void radioLoraSetRx(void) {
	lora_set_sync_word( (unsigned char) loraRXSyncWord);
	lora_receive();    // put into receive mode
}

void radioLoraRx(void) {
	int length;
	unsigned char buffer[sizeof(message_t)];
	message_t message;

	radioLoraSetRx();
	while(lora_received()) {

		length = sizeof(message_t);
		length = lora_receive_packet(buffer, length);

		int success = radioDecrypt(buffer, &length);

		if (!success) {
			ESP_LOGE(TAG, "Failed to decrypt packet.");
			continue;
		}

		radioCreateMessage(&message, buffer, length);

		ESP_LOGI(TAG, "LoRa heard |%s|%s|%d|%d", message.deviceName, message.sensorName, message.valueType, length);

		switch(message.valueType){
			case MESSAGE_INT:
				ESP_LOGI(TAG, "Value: %d", message.intValue);
			break;
			case MESSAGE_FLOAT:
				ESP_LOGI(TAG, "Value: %f", message.floatValue);
			break;
			case MESSAGE_DOUBLE:
				ESP_LOGI(TAG, "Value: %f", message.doubleValue);
			break;
			case MESSAGE_STRING:
				ESP_LOGI(TAG, "Value: %s", message.stringValue);
			break;
		}

		messageIn(&message, "lora");

		radioLoraSetRx();
	}
}

void radioLoRaQueueAdd(message_t * message){
	if (uxQueueSpacesAvailable(radioQueue)) {
		xQueueSend(radioQueue, &message, 0);
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

	nvs_get_u32(nvsHandle, "loraTXSyncWord", &loraTXSyncWord);
	nvs_get_u32(nvsHandle, "loraRXSyncWord", &loraRXSyncWord);

	ESP_LOGI(TAG, "Setting key %s", loraKey);
	aesEncryptContext = aes_encrypt_init( (unsigned char *) loraKey, strlen(loraKey));

	if (!aesEncryptContext){
		ESP_LOGE(TAG, "aes_encrypt_init failed");
	}

	aesDecryptContext = aes_decrypt_init( (unsigned char *) loraKey, strlen(loraKey));

	if (!aesDecryptContext){
		ESP_LOGE(TAG, "aes_encrypt_init failed");
	}

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
	ESP_ERROR_CHECK(nvs_set_u32(nvsHandle, "loraFrequency", 915000000));
	ESP_ERROR_CHECK(nvs_set_u32(nvsHandle, "loraTXSyncWord", 190));
	ESP_ERROR_CHECK(nvs_set_u32(nvsHandle, "loraRXSyncWord", 191));
	ESP_ERROR_CHECK(nvs_set_u8(nvsHandle, "loraToMQTT", 0));
	ESP_ERROR_CHECK(nvs_set_u8(nvsHandle, "loraToElastic", 0));
	ESP_ERROR_CHECK(nvs_set_u8(nvsHandle, "loraToLoRa", 0));

	ESP_ERROR_CHECK(nvs_commit(nvsHandle));

	nvs_close(nvsHandle);

	messageNVSReset("lora");
}

/*

25: work1 staticTest       A
31: wwrr1 staticTest       A


*/