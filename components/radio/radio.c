#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs.h>
#include <string.h>
#include <driver/gpio.h>
#include <esp_log.h>

#include "lora.h"

#define TAG "Radio"

typedef union{
	int intData;
	float floatData;
	double doubleData;
	char stringData[32];
} radioPacketData_t;

typedef enum {
	DATA_INT,
    DATA_FLOAT,
    DATA_DOUBLE,
    DATA_STRING
} radioPacketType_t;

typedef struct{
	radioPacketType_t type;
	radioPacketData_t data;
} radioPacket_t;

typedef union{
	radioPacket_t packet;
	unsigned char buffer[32];
} radioPacketBuffer_t;

int radioGetPacketLength(radioPacket_t * radioPacket){

	int length = 0;
	length+= sizeof(radioPacket->type);

	switch (radioPacket->type) {

		case DATA_INT:
			length+= sizeof(radioPacket->data.intData);
		break;

		case DATA_FLOAT:
			length+= sizeof(radioPacket->data.floatData);
		break;

		case DATA_DOUBLE:
			length+= sizeof(radioPacket->data.doubleData);
		break;

		case DATA_STRING:
			length+= strlen(radioPacket->data.stringData);
		break;
	}

	if (length > sizeof(radioPacket_t)) {
		length = sizeof(radioPacket_t);
	}

	return length;
}

void radioLoRaTx(void * arg){

	radioPacketBuffer_t radioPacketBuffer;

	while(1){

		radioPacketBuffer.packet.type = DATA_INT;
		radioPacketBuffer.packet.data.intData = 1234;
		lora_send_packet(radioPacketBuffer.buffer, radioGetPacketLength(&radioPacketBuffer.packet));
		ESP_LOGE(TAG, "packet sent: int L:%d", radioGetPacketLength(&radioPacketBuffer.packet));

		vTaskDelay(pdMS_TO_TICKS(5000));

		radioPacketBuffer.packet.type = DATA_FLOAT;
		radioPacketBuffer.packet.data.floatData = 123.4567;
		lora_send_packet(radioPacketBuffer.buffer, radioGetPacketLength(&radioPacketBuffer.packet));
		ESP_LOGE(TAG, "packet sent: float L:%d", radioGetPacketLength(&radioPacketBuffer.packet));

		vTaskDelay(pdMS_TO_TICKS(5000));

		radioPacketBuffer.packet.type = DATA_DOUBLE;
		radioPacketBuffer.packet.data.doubleData = 9876.54321;
		lora_send_packet(radioPacketBuffer.buffer, radioGetPacketLength(&radioPacketBuffer.packet));
		ESP_LOGE(TAG, "packet sent: double L:%d", radioGetPacketLength(&radioPacketBuffer.packet));

		vTaskDelay(pdMS_TO_TICKS(5000));

		radioPacketBuffer.packet.type = DATA_STRING;
		strcpy(radioPacketBuffer.packet.data.stringData, "Hello from other device");
		lora_send_packet(radioPacketBuffer.buffer, radioGetPacketLength(&radioPacketBuffer.packet));
		ESP_LOGE(TAG, "packet sent: string L:%d", radioGetPacketLength(&radioPacketBuffer.packet));

		vTaskDelay(pdMS_TO_TICKS(5000));
	}
}


void radioLoraRx(void *p) {
	int length;
	while(1) {
		lora_receive();    // put into receive mode
		while(lora_received()) {

			radioPacketBuffer_t radioPacketBuffer;

			length = lora_receive_packet(radioPacketBuffer.buffer, sizeof(radioPacketBuffer.buffer));

			switch (radioPacketBuffer.packet.type) {

				case DATA_INT:
					ESP_LOGE(TAG, "LoRa heard INT: %d", radioPacketBuffer.packet.data.intData);
				break;

				case DATA_FLOAT:
					ESP_LOGE(TAG, "LoRa heard FLOAT: %f", radioPacketBuffer.packet.data.floatData);
				break;

				case DATA_DOUBLE:
					ESP_LOGE(TAG, "LoRa heard DOUBLE: %f", radioPacketBuffer.packet.data.doubleData);
				break;

				case DATA_STRING:
					ESP_LOGE(TAG, "LoRa heard STRING: %s", radioPacketBuffer.packet.data.stringData);
				break;

				default:
					ESP_LOGE(TAG, "LoRa heard ????? Length: %d", length);
				break;
			}

			lora_receive();
		}
		vTaskDelay(10 / portTICK_RATE_MS);
	}
}

void radioInit(void){

	lora_init();
	lora_set_frequency(915e6);
	lora_enable_crc();
	// xTaskCreate(&radioLoRaTx, "radioLoRaTx", 2048, NULL, 5, NULL);
	xTaskCreate(&radioLoraRx, "radioLoraRx", 2048, NULL, 5, NULL);
}