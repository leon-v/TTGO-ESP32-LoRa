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

#define LORA_IRQ 26

static xQueueHandle radioQueue = NULL;

static char loraKey[CONFIG_HTTP_NVS_MAX_STRING_LENGTH] = {0};
static void * aesEncryptContext;
static void * aesDecryptContext;

static unsigned int loraTXSyncWord;
static unsigned int loraRXSyncWord;

static void radioPrintBuffer(const char * tag, unsigned char * buffer, int length){

	printf("%s: %d: ", tag, length);
	for (int i = 0; i < length; i++) {
		printf("%c|", (char) buffer[i]);
	}
	printf("\n");
}

static void radioShift(unsigned char * buffer, int amount, int * length){


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

static void radioCreateMessage(message_t * message, unsigned char * radioBuffer, int bufferLength) {

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
static int radioCreateBuffer(unsigned char * radioBuffer, message_t * message) {

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

static void radioEncrypt(unsigned char * buffer, int * length){

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

static int radioDecrypt(unsigned char * buffer, int * length){

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

static void radioLoRaSendRadioMessage(message_t * message){
	int length;

	unsigned char buffer[sizeof(message_t)] = {0};
	memset(buffer, 0, sizeof(message_t));

	length = radioCreateBuffer(buffer, message);

	radioEncrypt(buffer, &length);

	lora_set_sync_word( (unsigned char) loraTXSyncWord);

	lora_send_packet(buffer, length);
}

static void radioLoraSetRx(void) {
	lora_set_sync_word( (unsigned char) loraRXSyncWord);
	lora_receive();    // put into receive mode
}

static void radioLoraRx(void) {

	while(lora_received()) {

		int length;
		unsigned char buffer[sizeof(message_t)];
		message_t message;

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
	}
}

static void radioTask(void * arg){

	message_t message;

	while (1){

		if (xQueueReceive(radioQueue, &message, 4000 / portTICK_RATE_MS)) {

			if (message.valueType != MESSAGE_INTERRUPT){
				radioLoRaSendRadioMessage(&message);
			}

			radioLoraRx();
		}

		radioLoraSetRx();
	}

	vTaskDelete(NULL);
}

void radioLoRaQueueAdd(message_t * message){

	if (uxQueueSpacesAvailable(radioQueue)) {
		xQueueSend(radioQueue, message, 0);
	}
}

static void radioLoraISR(void * arg){

	message_t message;

	message.valueType = MESSAGE_INTERRUPT;
	message.intValue = (uint32_t) arg;

	// if (uxQueueSpacesAvailable(radioQueue)) {
		xQueueSendFromISR(radioQueue, &message, 0);
	// }
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

	radioQueue = xQueueCreate(4, sizeof(message_t));

	gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_ANYEDGE;
    //set as output mode
    io_conf.mode = GPIO_MODE_INPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = (0x01 << LORA_IRQ);
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_LOWMED);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(LORA_IRQ, radioLoraISR, (void*) LORA_IRQ);


	lora_init();
	lora_set_frequency(loraFrequency);
	lora_enable_crc();


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

	messageNVSReset("lora", 0x00);
}

/*

25: work1 staticTest       A
31: wwrr1 staticTest       A


*/